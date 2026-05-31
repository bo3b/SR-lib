/*!
 * Copyright (C) 2025 Leia, Inc.
 */

// Windows includes.
#define WIN32_LEAN_AND_MEAN
#include "targetver.h"
#include <windows.h>
#include <assert.h>
#include <shellscalingapi.h>

// GL includes.
#define SOGL_IMPLEMENTATION_WIN32
#define SOGL_MAJOR_VERSION 4
#define SOGL_MINOR_VERSION 3
#include "simple-opengl-loader.h"
#include "gl/GL.h"

static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;
#define myGLLoadFunc(f,p) f = (p)wglGetProcAddress(#f)


// LeiaSR includes
#include "sr/utility/exception.h"
#include "sr/sense/core/inputstream.h"
#include "sr/sense/system/systemsense.h"
#include "sr/sense/eyetracker/eyetracker.h"
#include "sr/world/display/display.h"
#include "sr/weaver/glweaver.h"
#include "leia_math.h"

// This app's includes.
#include "resource.h"

// Use STB for image loading.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Library for SetProcessDpiAwareness().
#pragma comment(lib, "shcore.lib")

// GL libraries.
#pragma comment(lib, "opengl32.lib")

// Enable this line to use deprecated weaver.
//#define USE_DEPRECATED_WEAVER

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) if(x != nullptr) { x->Release(); x = nullptr; }
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(x) if(x != nullptr) { delete x; x = nullptr; }
#endif

#ifndef SAFE_DESTROY
#define SAFE_DESTROY(x) if(x != nullptr) { x->destroy(); x = nullptr; }
#endif

// This is the mode this app can be in.
enum class eDemoMode { Spinning3DCube, StereoImage };

// This is the display you want to run on. Auto will select the first SR display.
enum eTargetDisplay { Primary, Secondary, Auto };

// This is the color space used.
enum eColorSpace { Default, sRGBHardware, sRGBShader };

// This class allows receiving SR system events.
class SRSystemEventListener : public SR::SystemEventListener
{
public:
    SR::InputStream<SR::SystemEventStream> stream;
    virtual void accept(const SR::SystemEvent& frame) override;
};

#ifdef USE_DEPRECATED_WEAVER
// This class will allow the application to get the tracked user eye positions.
class SREyePairListener : public SR::EyePairListener
{
public:
    SR::InputStream<SR::EyePairStream> stream;
    vec3f left = vec3f(-30.0f, 0.0f, 600.0f);
    vec3f right = vec3f(30.0f, 0.0f, 600.0f);

    SREyePairListener(SR::EyeTracker* tracker)
    {
        stream.set(tracker->openEyePairStream(this));
    }

    virtual void accept(const SR_eyePair& eyePair) override
    {
        left = vec3f((float)eyePair.left.x, (float)eyePair.left.y, (float)eyePair.left.z);
        right = vec3f((float)eyePair.right.x, (float)eyePair.right.y, (float)eyePair.right.z);
    }
};
#endif

// Global Variables.
const char*               g_windowTitle               = "LeiaSR Weaving Example (OpenGL)";  // Title of the window
const char*               g_windowClass               = g_windowTitle;                      // Class name for the window
SR::SRContext*            g_srContext                 = nullptr;                            // SR context
SRSystemEventListener*    g_srSystemEventListener     = nullptr;                            // SR event listener, to receive SR status updates
float                     g_ScreenWidthInMM           = 0;                                  // Width of SR display
float                     g_ScreenHeightInMM          = 0;                                  // Height of SR display
HWND                      g_hWnd                      = NULL;                               // Window handle
char                      g_stereoImageFile[MAX_PATH] = "StereoImage.jpg";                  // Path to default stereo image to load
bool                      g_reCreateContext           = false;                              // When the context is unexpectedly lost, this is set to true
std::recursive_mutex      g_Mutex;                                                          // Mutex for synchronization
#ifdef USE_DEPRECATED_WEAVER
SR::PredictingGLWeaver*   g_srWeaver                  = nullptr;                            // Legacy weaver
SREyePairListener*        g_srEyePairListener         = nullptr;                            // SR eyepair listener, to get the eye positions
#else
SR::IGLWeaver1*            g_srWeaver                  = nullptr;                            // Current weaver
#endif
bool                      g_sceneReady                = false;                              // True when Render() can be called

// User-changeable options.
int            g_windowX                  = 0;                             // Initial window left position when not in fullscreen mode
int            g_windowY                  = 0;                             // Initial window top position when not in fullscreen mode
int            g_windowWidth              = 1280;                          // Initial window width when not in fullscreen mode
int            g_windowHeight             = 720;                           // Initial window height when not in fullscreen mode
bool           g_fullscreen               = true;                          // Whether to start in fullscreen (use F11 to toggle)
eDemoMode      g_demoMode                 = eDemoMode::StereoImage;        // Demo mode (image or 3D rendering)
eColorSpace    g_colorSpace               = eColorSpace::sRGBHardware;     // Default to using sRGB with hardware conversions
eTargetDisplay g_targetDisplay            = eTargetDisplay::Auto;          // Start on SR display
bool           g_useDeprecatedFramebuffer = false;                         // Use the weaver-provided view texture with the deprecated weaver

// Global GL Variables.
int    g_viewTextureWidth            = 0;       // Width of view texture
int    g_viewTextureHeight           = 0;       // Height of view texture
GLuint g_viewTexture                 = 0;       // View texture attached to view framebuffer
GLuint g_viewDepthTexture            = 0;       // View depth texture attached to view framebuffer
GLuint g_viewFramebuffer             = 0;       // View framebuffer to render cube into
GLuint g_vao                         = 0;       // Vertex array object to draw cube
GLuint g_vaoVertexXYZAttributeIndex  = 0;       // Index of the position attribute
GLuint g_vaoVertexRGBAttributeIndex  = 1;       // Index of the color attribute
GLuint g_vaoTriangleCount            = 12;      // Number of triangles per cube
GLuint g_vaoIndexCount               = 12 * 3;  // Number of indices per cube
GLuint g_shaderProgram               = 0;       // Shader program
GLint  g_uniformTransform            = 0;       // Transform matrix uniform id

#pragma pack(push, 1)

// Vertex buffer data layout.
struct VERTEX
{
    float x, y, z;
    float r, g, b;

    VERTEX() = default;
    VERTEX(const float* p, const float* c) : x(p[0]), y(p[1]), z(p[2]), r(c[0]), g(c[1]), b(c[2]) {}
};

#pragma pack(pop)

// Function to get the format used for the view texture and swapchain.
GLenum GetTextureFormat()
{
    return (g_colorSpace == eColorSpace::sRGBHardware) ? GL_SRGB8_ALPHA8 : GL_RGBA8;
}

void SRSystemEventListener::accept(const SR::SystemEvent& frame)
{
    switch (frame.eventType)
    {
    case SR_eventType::Info: std::cout << "System Event: Info" << std::endl; break;
    case SR_eventType::ContextInvalid:
    {
        std::cout << "System Event: ContextInvalid" << std::endl;
        std::lock_guard<std::recursive_mutex> lock(g_Mutex);
        g_reCreateContext = true;
        break;
    }
    case SR_eventType::SRUnavailable:               std::cout << "System Event: SRUnavailable" << std::endl; break;
    case SR_eventType::SRRestored:                  std::cout << "System Event: SRRestored" << std::endl; break;
    case SR_eventType::USBNotConnected:             std::cout << "System Event: USBNotConnected" << std::endl; break;
    case SR_eventType::USBNotConnectedResolved:     std::cout << "System Event: USBNotConnectedResolved" << std::endl; break;
    case SR_eventType::DisplayNotConnected:         std::cout << "System Event: DisplayNotConnected" << std::endl; break;
    case SR_eventType::DisplayNotConnectedResolved: std::cout << "System Event: DisplayNotConnectedResolved" << std::endl; break;
    case SR_eventType::Duplicated:                  std::cout << "System Event: Duplicated" << std::endl; break;
    case SR_eventType::DuplicatedResolved:          std::cout << "System Event: DuplicatedResolved" << std::endl; break;
    case SR_eventType::NonNativeResolution:         std::cout << "System Event: NonNativeResolution" << std::endl; break;
    case SR_eventType::NonNativeResolutionResolved: std::cout << "System Event: NonNativeResolutionResolved" << std::endl; break;
    case SR_eventType::DeviceConnectedAndReady:     std::cout << "System Event: DeviceConnectedAndReady" << std::endl; break;
    case SR_eventType::DeviceDisconnected:          std::cout << "System Event: DeviceDisconnected" << std::endl; break;
    case SR_eventType::LensOn:                      std::cout << "System Event: LensOn" << std::endl; break;
    case SR_eventType::LensOff:                     std::cout << "System Event: LensOff" << std::endl; break;
    case SR_eventType::UserFound:                   std::cout << "System Event: UserFound" << std::endl; break;
    case SR_eventType::UserLost:                    std::cout << "System Event: UserLost" << std::endl; break;
    default:                                        std::cout << "System Event: Unknown" << std::endl; break;
    }
}

void OnError(const char* msg)
{
    // Show dialog with error.
    MessageBox(NULL, msg, g_windowTitle, MB_ICONERROR | MB_OK);

    // Write to std out and debug output.
#ifdef _DEBUG
    std::cout << msg << std::endl;
    OutputDebugString(msg);
    assert(false);
#endif
}

float GetSRGB(float value)
{
    // If already in sRGB, no change.
    if (g_colorSpace != eColorSpace::Default)
        return value;

    // Convert linear->sRGB.
    if (value <= 0.0f)
        return 0.0f;
    else if (value >= 1.0f)
        return 1.0f;
    else if (value <= 0.0031308f)
        return value * 12.92f;
    else
        return 1.055f * pow(value, 1.0f / 2.4f) - 0.055f;
}

BOOL CALLBACK GetDefaultWindowStartPos_MonitorEnumProc(__in  HMONITOR hMonitor, __in  HDC hdcMonitor, __in  LPRECT lprcMonitor, __in  LPARAM dwData)
{
    std::vector<MONITORINFOEX>& infoArray = *reinterpret_cast<std::vector<MONITORINFOEX>*>(dwData);
    MONITORINFOEX info;
    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(info);
    GetMonitorInfo(hMonitor, &info);
    infoArray.push_back(info);
    return TRUE;
}

bool GetNonPrimaryDisplayTopLeftCoordinate(int& x, int& y)
{
    // Get connected monitor info.
    std::vector<MONITORINFOEX> mInfo;
    mInfo.reserve(::GetSystemMetrics(SM_CMONITORS));
    EnumDisplayMonitors(NULL, NULL, GetDefaultWindowStartPos_MonitorEnumProc, reinterpret_cast<LPARAM>(&mInfo));

    // If we have multiple monitors, select the first non-primary one.
    if (mInfo.size() > 1)
    {
        for (int i = 0; i < mInfo.size(); i++)
        {
            const MONITORINFOEX& mi = mInfo[i];

            if (0 == (mi.dwFlags & MONITORINFOF_PRIMARY))
            {
                x = mi.rcMonitor.left;
                y = mi.rcMonitor.top;
                return true;
            }
        }
    }

    // Didn't find a non-primary, there is only one display connected.
    x = 0;
    y = 0;
    return false;
}

HWND CreateGraphicsWindow(HINSTANCE hInstance, int& monitorTopLeftX, int& monitorTopLeftY)
{
    // Create window.
    HWND hWnd = NULL;
    {
        int displayTopLeftX = 0;
        int displayTopLeftY = 0;
        
        switch (g_targetDisplay)
        {
        case eTargetDisplay::Primary:
            break;
        case eTargetDisplay::Secondary:
            GetNonPrimaryDisplayTopLeftCoordinate(displayTopLeftX, displayTopLeftY);
            break;
        case eTargetDisplay::Auto:
            if (g_srContext != nullptr)
            {
                SR::Display* display = SR::Display::create(*g_srContext);
                SR_recti displayLocation = display->getLocation();
                displayTopLeftX = (int)displayLocation.left;
                displayTopLeftY = (int)displayLocation.top;
            }
            break;
        }

        monitorTopLeftX = displayTopLeftX;
        monitorTopLeftY = displayTopLeftY;

        DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;             // Window Extended Style
        DWORD dwStyle   = WS_OVERLAPPEDWINDOW;                            // Windows Style

        RECT WindowRect;
        WindowRect.left   = (long)displayTopLeftX + g_windowX;
        WindowRect.right  = (long)(WindowRect.left + g_windowWidth);
        WindowRect.top    = (long)displayTopLeftY + g_windowY;
        WindowRect.bottom = (long)(WindowRect.top + g_windowHeight);

        hWnd = CreateWindowEx
        (
            dwExStyle,
            g_windowClass,                         // Class Name
            g_windowTitle,                         // Window Title
            dwStyle |                              // Defined Window Style
            WS_CLIPSIBLINGS |                      // Required Window Style
            WS_CLIPCHILDREN,                       // Required Window Style
            WindowRect.left,                       // Window left
            WindowRect.top,                        // Window top
            WindowRect.right - WindowRect.left,    // Calculate Window Width
            WindowRect.bottom - WindowRect.top,    // Calculate Window Height
            NULL,                                  // No Parent Window
            NULL,                                  // No Menu
            hInstance,                             // Instance
            NULL                                   // Dont Pass Anything To WM_CREATE
        );

        if (hWnd == NULL)
            OnError("Failed to create window.");
    }

    return hWnd;
}

void SetFullscreen(HWND hWnd, bool fullscreen)
{
    static int windowPrevX = 0;
    static int windowPrevY = 0;
    static int windowPrevWidth = 0;
    static int windowPrevHeight = 0;

    DWORD style = GetWindowLong(hWnd, GWL_STYLE);
    if (fullscreen)
    {
        RECT rect = {};
        MONITORINFO mi = { sizeof(mi) };
        GetWindowRect(hWnd, &rect);

        windowPrevX = rect.left;
        windowPrevY = rect.top;
        windowPrevWidth = rect.right - rect.left;
        windowPrevHeight = rect.bottom - rect.top;

        GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), &mi);
        SetWindowLong(hWnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(hWnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    else
    {
        MONITORINFO mi = { sizeof(mi) };
        UINT flags = SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW;
        GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), &mi);
        SetWindowLong(hWnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPos(hWnd, HWND_NOTOPMOST, windowPrevX, windowPrevY, windowPrevWidth, windowPrevHeight, flags);
    }
}

HGLRC InitializeGL(HWND hWnd, HDC hDC)
{
    HGLRC context = NULL;

    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARBFunc = nullptr;
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARBFunc = nullptr;
    {
        // First create a context for the purpose of getting access to wglChoosePixelFormatARB / wglCreateContextAttribsARB.
        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize      = sizeof(pfd);
        pfd.nVersion   = 1;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.dwFlags    = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 16;
        int pf = ChoosePixelFormat(hDC, &pfd);
        if (pf == 0)
        {
            OnError("Failed to choose pixel format.");
            return NULL;
        }

        if (!SetPixelFormat(hDC, pf, &pfd))
        {
            OnError("Failed to set pixel format.");
            return NULL;
        }

        HGLRC context = wglCreateContext(hDC);
        if (context == 0)
        {
            OnError("wglCreateContextfailed failed.");
            return NULL;
        }

        if (!wglMakeCurrent(hDC, context))
        {
            OnError("wglMakeCurrent failed.");
            return NULL;
        }

        wglChoosePixelFormatARBFunc = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
        wglCreateContextAttribsARBFunc = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

        wglDeleteContext(context);

        if (wglChoosePixelFormatARBFunc == nullptr || wglCreateContextAttribsARBFunc == nullptr)
        {
            OnError("wglChoosePixelFormatARB and/or wglCreateContextAttribsARB missing.");
            return NULL;
        }
    }

    // Now create the real context that we will be using.
    const int iAttributes[] =
    {
        WGL_SUPPORT_OPENGL_ARB,           GL_TRUE,
        WGL_COLOR_BITS_ARB,               32,
        WGL_DEPTH_BITS_ARB,               16,
        WGL_DOUBLE_BUFFER_ARB,            GL_TRUE,
        WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
        0, 0
    };

    const float fAttributes[] = { 0, 0 };
    UINT  numFormats = 0;
    int   pf = 0;
    if (!wglChoosePixelFormatARBFunc(hDC, iAttributes, fAttributes, 1, &pf, &numFormats))
    {
        OnError("wglChoosePixelFormatARBFunc failed.");
        return NULL;
    }

    PIXELFORMATDESCRIPTOR pfd = {};
    if (!SetPixelFormat(hDC, pf, &pfd))
    {
        OnError("SetPixelFormat failed.");
        return NULL;
    }

#ifdef _DEBUG
    bool UseDebugContext = true;
#else
    bool UseDebugContext = false;
#endif

    // Crete context attributes.
    GLint attribs[16] = {};
    {
        int attribCount = 0;
        if (UseDebugContext)
        {
            attribs[attribCount++] = WGL_CONTEXT_FLAGS_ARB;
            attribs[attribCount++] = WGL_CONTEXT_DEBUG_BIT_ARB;
        }

        attribs[attribCount++] = WGL_CONTEXT_MAJOR_VERSION_ARB;
        attribs[attribCount++] = 3;

        attribs[attribCount++] = WGL_CONTEXT_MINOR_VERSION_ARB;
        attribs[attribCount++] = 0;

        attribs[attribCount++] = GL_CONTEXT_PROFILE_MASK;
        attribs[attribCount++] = GL_CONTEXT_CORE_PROFILE_BIT;

        attribs[attribCount] = 0;
    }

    context = wglCreateContextAttribsARBFunc(hDC, 0, attribs);
    if (!wglMakeCurrent(hDC, context))
    {
        OnError("wglMakeCurrent failed.");
        return NULL;
    }

    //
    if (!sogl_loadOpenGL()) 
    {
        OnError("Failed to load OpenGL.");
        return NULL;
    }
    myGLLoadFunc(wglSwapIntervalEXT, PFNWGLSWAPINTERVALEXTPROC);
    if (wglSwapIntervalEXT == nullptr)
    {
        OnError("wglSwapIntervalEXT not available.");
        return NULL;
    }

    // Enable sRGB in the framebuffer if needed.
    if (g_colorSpace == eColorSpace::sRGBHardware)
        glEnable(GL_FRAMEBUFFER_SRGB);

    // Basic OpenGL setup.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    
    return context;
}

bool CreateSRContext(double maxTime)
{
    //! [SR Initialization]

    const double startTime = (double)GetTickCount64() / 1000.0;

    // Create SR context.
    while (g_srContext == nullptr)
    {
        try
        {
            g_srContext = SR::SRContext::create();
            break;
        }
        catch (SR::ServerNotAvailableException e)
        {
            // Ignore errors because SR may be starting-up.
        }

        std::cout << "Waiting for context" << std::endl;

        // Wait a bit.
        Sleep(100);

        // Abort if we exceed the maximum allowed time.
        double curTime = (double)GetTickCount64() / 1000.0;
        if ((curTime - startTime) > maxTime)
            break;
    }

    // Wait for display to be ready.
    bool displayReady = false;
    while (g_srContext && !displayReady)
    {
        // Attempt to get display.
        SR::Display* display = SR::Display::create(*g_srContext);
        if (display != nullptr)
        {
            // Get the display location, and when it is valid, the device is ready.
            SR_recti displayLocation = display->getLocation();
            int64_t width = displayLocation.right - displayLocation.left;
            int64_t height = displayLocation.bottom - displayLocation.top;
            if ((width != 0) && (height != 0))
            {
                displayReady = true;
                break;
            }
        }

        std::cout << "Waiting for display" << std::endl;

        // Wait a bit.
        Sleep(100);

        // Abort if we exceed the maximum allowed time.
        double curTime = (double)GetTickCount64() / 1000.0;
        if ((curTime - startTime) > maxTime)
            break;
    }

    //! [SR Initialization]

    // Return if we have a valid context and device is ready.
    return (g_srContext != nullptr) && displayReady;
}

bool InitializeLeiaSR(double maxTime)
{
    std::lock_guard<std::recursive_mutex> lock(g_Mutex);

    // weaver needs to be deleted before deleting SRContext as weaver is using the context, but this function does not reconstruct weaver. Construct weaver wherever is needed
#if !defined(USE_DEPRECATED_WEAVER)
    SAFE_DESTROY(g_srWeaver);
#else
    SAFE_DELETE(g_srWeaver);
    SAFE_DELETE(g_srEyePairListener);
#endif
    SAFE_DELETE(g_srSystemEventListener);

    // Create SR context.
    if (!CreateSRContext(maxTime))
    {
        OnError("Failed to create SR context");
        return false;
    }

#if defined(USE_DEPRECATED_WEAVER)
    // Create EyePairListener
    g_srEyePairListener = new SREyePairListener(SR::EyeTracker::create(*g_srContext));
#endif

    // set systemEvent listener to the newly constructed systemsense
    SR::SystemSense* systemSense = SR::SystemSense::create(*g_srContext);
    g_srSystemEventListener = new SRSystemEventListener();
    g_srSystemEventListener->stream.set(systemSense->openSystemEventStream(g_srSystemEventListener));

    // Get recommendede view texture size.
    SR::Display* display = SR::Display::create(*g_srContext);
    g_viewTextureWidth  = display->getRecommendedViewsTextureWidth();
    g_viewTextureHeight = display->getRecommendedViewsTextureHeight();

    //! [Construct weaver]

    // Create weaver.
#ifdef USE_DEPRECATED_WEAVER
    g_srWeaver = new SR::PredictingGLWeaver(*g_srContext, g_viewTextureWidth * 2, g_viewTextureHeight, GetTextureFormat(), g_hWnd);
    if (!g_useDeprecatedFramebuffer || (g_demoMode == eDemoMode::StereoImage))
        g_srWeaver->setInputFrameBuffer(g_viewFramebuffer, g_viewTexture);
#else
    WeaverErrorCode createWeaverResult = SR::CreateGLWeaver(*g_srContext, g_hWnd, &g_srWeaver);
    if (createWeaverResult != WeaverErrorCode::WeaverSuccess)
    {
        OnError("Failed to create weaver");
        return false;
    }
    g_srWeaver->setInputViewTexture(g_viewTexture, g_viewTextureWidth, g_viewTextureHeight, GetTextureFormat());
#endif

    //! [Construct weaver]

    // Set in-shader sRGB conversion if necessary.
    if (g_colorSpace == eColorSpace::sRGBShader)
        g_srWeaver->setShaderSRGBConversion(true, true);

    // Initialize context after creating weaver.
    g_srContext->initialize();

    return true;
}

void CompileShader(const char* source, GLuint shader)
{
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status == 0)
    {
        char shaderInfoLog[4096] = {};
        glGetShaderInfoLog(shader, 4096, NULL, shaderInfoLog);

        char msg[4096] = {};
        sprintf(msg, "Error compiling shader: %s\nThe shader was:\n%s", shaderInfoLog, source);
        OnError(msg);
    }
}

void LinkShader(unsigned int program)
{
    glLinkProgram(program);

    int status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (!status)
    {
        char programInfoLog[4096] = {};
        glGetProgramInfoLog(program, 4096, NULL, programInfoLog);

        char msg[4096] = {};
        sprintf(msg, "Error linking shader program: %s", programInfoLog);
        OnError(msg);
    }
}

bool GetFilePathAdjacentToExecutable(const char* inputFilename, char* outputFilename)
{
    assert(inputFilename != outputFilename);

    // Get executable filename.
    char exePath[MAX_PATH] = {};
    if (0 == GetModuleFileNameA(nullptr, exePath, sizeof(exePath)))
        return false;

    // Get executable folder.
    char* lastBackslash = strrchr(exePath, '\\');
    if (lastBackslash == nullptr)
        return false;
    *lastBackslash = '\0';

    //
    sprintf(outputFilename, "%s\\%s", exePath, inputFilename);
    return true;
}

bool Load()
{
    if (g_demoMode == eDemoMode::Spinning3DCube)
    {
        // Define cube.
        const float cubeWidth  = 1.0f;
        const float cubeHeight = 1.0f;
        const float cubeDepth  = 1.0f;

        const float l = -cubeWidth / 2.0f;
        const float r = l + cubeWidth;
        const float b = -cubeHeight / 2.0f;
        const float t = b + cubeHeight;
        const float n = -cubeDepth / 2.0f;
        const float f = n + cubeDepth;

        const int vertexCount = 8;
        const int indexCount  = 36;

        // Define cube 3D vertices.
        const float cubeVerts[vertexCount][3] =
        {
            {l, n, b}, // Left Near Bottom
            {l, f, b}, // Left Far Bottom
            {r, f, b}, // Right Far Bottom
            {r, n, b}, // Right Near Bottom
            {l, n, t}, // Left Near Top
            {l, f, t}, // Left Far Top
            {r, f, t}, // Right Far Top
            {r, n, t}  // Right Near Top
        };

        // Define cube face indices.
        static const int faces[6][4] =
        {
            {0,1,2,3}, // bottom
            {1,0,4,5}, // left
            {0,3,7,4}, // front
            {3,2,6,7}, // right
            {2,1,5,6}, // back
            {4,7,6,5}  // top
        };

        // Define cube face colors.
        static const float c = GetSRGB(0.6f);
        static const float g = GetSRGB(0.05f);
        static const float faceColors[6][3] =
        {
            {c,g,g},
            {g,c,g},
            {g,g,c},
            {c,c,g},
            {g,c,c},
            {c,g,c}
        };

        // Create vertex and index buffer data.
        std::vector<VERTEX> vertices;
        std::vector<int> indices;
        for (int i = 0; i < 6; i++)
        {
            const int i0 = faces[i][0];
            const int i1 = faces[i][1];
            const int i2 = faces[i][2];
            const int i3 = faces[i][3];

            // Add indices.
            const int startIndex = (int)vertices.size();
            indices.emplace_back(startIndex + 0);
            indices.emplace_back(startIndex + 1);
            indices.emplace_back(startIndex + 2);
            indices.emplace_back(startIndex + 0);
            indices.emplace_back(startIndex + 2);
            indices.emplace_back(startIndex + 3);

            // Add vertices.
            vertices.emplace_back(VERTEX(cubeVerts[i0], faceColors[i]));
            vertices.emplace_back(VERTEX(cubeVerts[i1], faceColors[i]));
            vertices.emplace_back(VERTEX(cubeVerts[i2], faceColors[i]));
            vertices.emplace_back(VERTEX(cubeVerts[i3], faceColors[i]));
        }
        
        glGenVertexArrays(1, &g_vao);
        glBindVertexArray(g_vao);

        // Create index buffer.
        GLuint indexBuffer = 0;
        glGenBuffers(1, &indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

        // Create vertex buffer.
        GLuint vertexBuffer = 0;
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VERTEX), vertices.data(), GL_STATIC_DRAW);

        // Set vertex buffer position attributes.
        glEnableVertexAttribArray(g_vaoVertexXYZAttributeIndex);
        glVertexAttribPointer(g_vaoVertexXYZAttributeIndex, 3, GL_FLOAT, GL_FALSE, sizeof(VERTEX), NULL);

        // Set vertex buffer colors attributes.
        glEnableVertexAttribArray(g_vaoVertexRGBAttributeIndex);
        glVertexAttribPointer(g_vaoVertexRGBAttributeIndex, 3, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (const GLvoid*)offsetof(VERTEX, r));

        // Unbind
        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        const char* szVertexShader =
            "#version 330 core\n"
            "layout(location = 0) in vec3 inPos; \n"
            "layout(location = 1) in vec3 inColor; \n"
            "out vec3 color; \n"
            "uniform mat4 transform; \n"
            "void main() {\n"
            "  gl_Position = transform * vec4(inPos, 1.0); \n"
            "  color = inColor; \n"
            "}\n";

        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        CompileShader(szVertexShader, vertex_shader);

        const char* szFragmentShader =
            "#version 330 core\n"
            "in vec3 color;\n"
            "out vec4 frag_color;\n"
            "void main() {\n"
            "    frag_color = vec4(color, 1.0);\n"
            "}\n";

        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        CompileShader(szFragmentShader, fragment_shader);

        g_shaderProgram = glCreateProgram();
        glAttachShader(g_shaderProgram, vertex_shader);
        glAttachShader(g_shaderProgram, fragment_shader);
        LinkShader(g_shaderProgram);

        g_uniformTransform = glGetUniformLocation(g_shaderProgram, "transform");

        // Create view texture
        {
            // Generate and bind new texture.
            glGenTextures(1, &g_viewTexture);
            glBindTexture(GL_TEXTURE_2D, g_viewTexture);

            // Set texture properties.
            glTexImage2D(GL_TEXTURE_2D, 0, GetTextureFormat(), g_viewTextureWidth * 2, g_viewTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

            // Set wrap mode.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Set filter mode.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        // Create view depth texture.
        {
            // Generate and bind new depthbuffer
            glGenRenderbuffers(1, &g_viewDepthTexture);
            glBindRenderbuffer(GL_RENDERBUFFER, g_viewDepthTexture);

            // Set depthbuffer properties.
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, g_viewTextureWidth * 2, g_viewTextureHeight);
        }

        // Create frame buffer
        {
            glGenFramebuffers(1, &g_viewFramebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, g_viewFramebuffer);
        
            // Set "newTexture" as our colour attachement #0
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, g_viewTexture, 0);

            // Set the list of draw buffers.
            GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

            // Bind depthbuffer to framebuffer.
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_viewDepthTexture);
        
            // Check for errors.
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                OnError("Failed to create offscreen frame buffer.");
                return false;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }
    else if (g_demoMode == eDemoMode::StereoImage)
    {
        // Load stereo image (always expand channels to 4).
        int imageWidth    = 0;
        int imageHeight   = 0;
        int imageChannels = 0;
        stbi_set_flip_vertically_on_load(1);
        unsigned char* imageData = stbi_load(g_stereoImageFile, &imageWidth, &imageHeight, &imageChannels, 4);

        // If the image wasn't loaded, try the same filename but adjacent to the executable.
        if (imageData == nullptr)
        {
            char localFilename[MAX_PATH] = {};
            if (GetFilePathAdjacentToExecutable(g_stereoImageFile, localFilename))
                imageData = stbi_load(localFilename, &imageWidth, &imageHeight, &imageChannels, 4);
        }

        // Handle error if we couldn't load the file.
        if (imageData == nullptr)
        {
            OnError("Failed to read image.");
            return false;
        }

        // Generate and bind new texture.
        glGenTextures(1, &g_viewTexture);
        glBindTexture(GL_TEXTURE_2D, g_viewTexture);

        // Set texture properties and image data.
        glTexImage2D(GL_TEXTURE_2D, 0, GetTextureFormat(), imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

        // Set wrap mode.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Set filter mode.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Remember texture settings.
        g_viewTextureWidth  = imageWidth / 2;
        g_viewTextureHeight = imageHeight;
    }

#ifdef USE_DEPRECATED_WEAVER
    if (!g_useDeprecatedFramebuffer || (g_demoMode == eDemoMode::StereoImage))
        g_srWeaver->setInputFrameBuffer(g_viewFramebuffer, g_viewTexture);
#else
    g_srWeaver->setInputViewTexture(g_viewTexture, g_viewTextureWidth, g_viewTextureHeight, GetTextureFormat());
#endif

    // Set in-shader sRGB conversion if necessary.
    if (g_colorSpace == eColorSpace::sRGBShader)
        g_srWeaver->setShaderSRGBConversion(true, true);

    return true;
}

mat4f CalculateModelViewProjectionMatrix(vec3f eye, float elapsedTime)
{
    // Compute model matrix.
    mat4f model = {};
    {
        const float size = 60.0;                            // Size of the object in mm
        float angle      = elapsedTime * 0.75f;             // Rotation amount

        const mat4f translation = mat4f::translation(0.0f, 0.0f, 0.0f);
        const mat4f scaling     = mat4f::scaling(size, size, size);
        const mat4f rotation    = mat4f::rotationY(angle);

        model = scaling * translation * rotation;
    }

    // Compute view matrix.
    mat4f view = mat4f::identity();

    // Compute projection matrix.
    mat4f projection = {};
    {
        // Implementation of: Kooima, Robert. "Generalized perspective projection." J. Sch. Electron. Eng. Comput. Sci (2009).
        const float znear = 0.1f;
        const float zfar  = 10000.0f;

        assert((g_ScreenWidthInMM != 0) && (g_ScreenHeightInMM != 0));

        vec3f pa = vec3f(-g_ScreenWidthInMM / 2.0f,  g_ScreenHeightInMM / 2.0f, 0.0f);
        vec3f pb = vec3f( g_ScreenWidthInMM / 2.0f,  g_ScreenHeightInMM / 2.0f, 0.0f);
        vec3f pc = vec3f(-g_ScreenWidthInMM / 2.0f, -g_ScreenHeightInMM / 2.0f, 0.0f);

        vec3f vr = vec3f(1.0f, 0.0f, 0.0f);
        vec3f vu = vec3f(0.0f, 1.0f, 0.0f);
        vec3f vn = vec3f(0.0f, 0.0f, 1.0f);

        // Compute the screen corner vectors.
        vec3f va = pa - eye;
        vec3f vb = pb - eye;
        vec3f vc = pc - eye;

        // Find the distance from the eye to screen plane.
        float distance = -vec3f::dot(va, vn);

        // Find the extent of the perpendicular projection.
        float l = vec3f::dot(vr, va) * znear / distance;
        float r = vec3f::dot(vr, vb) * znear / distance;
        float b = vec3f::dot(vu, vc) * znear / distance;
        float t = vec3f::dot(vu, va) * znear / distance;

        // Load the perpendicular projection
        mat4f frustum = mat4f::perspective(l, r, b, t, znear, zfar);

        // Move the apex of the frustum to the origin.
        mat4f translate = mat4f::translation(-eye);

        // Combine
        projection = frustum * translate;
    }

    // Compute combined matrix.
    mat4f mvp = projection * view * model;

    return mvp;
}

void UpdateWindowTitle(HWND hWnd, double curTime)
{
    static double prevTime = 0;
    static int frameCount = 0;

    frameCount++;

    if (curTime - prevTime > 0.25)
    {
        const double fps = frameCount / (curTime - prevTime);

        char newWindowTitle[128];
        sprintf(newWindowTitle, "%s (%.1f FPS)", g_windowTitle, fps);
        SetWindowText(hWnd, newWindowTitle);

        prevTime = curTime;
        frameCount = 0;
    }
}

void Render(bool presentAsFastAsPossible = false)
{
    // When the window is minimized, this render function would cause maxing out the CPU (vsync not working?). Just sleep instead.

    // In addition: When late latching is enabled, this would cause too many frames in flight and late latching will be disabled.
    if (IsIconic(g_hWnd))
    {
        Sleep(1);
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(g_Mutex);

    // Get timing.
    const ULONGLONG  curTick = GetTickCount64();
    static ULONGLONG prevTick = curTick;
    const ULONGLONG  elapsedTicks = curTick - prevTick;
    const double     elapsedTime = (double)elapsedTicks / 1000.0;
    const double     curTime = (double)curTick / 1000.0;

    // Clear back-buffer (not necessary but helps driver performance to fully clear RT each frame).
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(GetSRGB(0.0f), GetSRGB(0.25f), GetSRGB(0.0f), GetSRGB(1.0f));
    glClear(GL_COLOR_BUFFER_BIT);

    if (g_demoMode == eDemoMode::StereoImage)
    {
        // View texture already contains image. Nothing to do.
    }
    else if (g_demoMode == eDemoMode::Spinning3DCube)
    {
        // Get eye positions.
        vec3f leftEye, rightEye;
#if !defined(USE_DEPRECATED_WEAVER)
        g_srWeaver->getPredictedEyePositions(&leftEye.x, &rightEye.x);

#else
        leftEye = g_srEyePairListener->left;
        rightEye = g_srEyePairListener->right;
#endif

        //! [Drawing Scene]

        // Render cube into stereo view texture.

        //! [Bind framebuffer for weaving]

        // Set view texture as target.
#if defined(USE_DEPRECATED_WEAVER)
        if (!g_useDeprecatedFramebuffer || (g_demoMode == eDemoMode::StereoImage))
        {
            // Use the app-provided view texture.
            glBindFramebuffer(GL_FRAMEBUFFER, g_viewFramebuffer);
        }
        else
        {
            // Use the deprecated weaver-provided view texture.
            GLuint View = g_srWeaver->getFrameBuffer();
            glBindFramebuffer(GL_FRAMEBUFFER, View);
        }
#else
        glBindFramebuffer(GL_FRAMEBUFFER, g_viewFramebuffer);
#endif

        // Clear view texture to blue
        glClearColor(GetSRGB(0.05f), GetSRGB(0.05f), GetSRGB(0.25f), GetSRGB(1.0f));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //! [Bind framebuffer for weaving]

        // Render stereo views.
        for (int i = 0; i < 2; i++)
        {
            // Set viewport to render to left, then right.
            glViewport(i * g_viewTextureWidth, 0, g_viewTextureWidth, g_viewTextureHeight);

            // Get transform matrix.
            mat4f mvp = CalculateModelViewProjectionMatrix((i == 0) ? leftEye : rightEye, (float)elapsedTime);

            // Draw.
            glUseProgram(g_shaderProgram);
            glUniformMatrix4fv(g_uniformTransform, 1, GL_FALSE, (const GLfloat*) &mvp);
            glBindVertexArray(g_vao);
            glEnableVertexAttribArray(g_vaoVertexXYZAttributeIndex);
            glEnableVertexAttribArray(g_vaoVertexRGBAttributeIndex);
            glDrawElements(GL_TRIANGLES, g_vaoIndexCount, GL_UNSIGNED_INT, NULL);
            glDisableVertexAttribArray(g_vaoVertexXYZAttributeIndex);
            glDisableVertexAttribArray(g_vaoVertexRGBAttributeIndex);
        }

        //! [Drawing Scene]
    }

    //! [Render woven image]

    // Set viewport to cover entire window.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_windowWidth, g_windowHeight);

    // Perform weaving.
#if !defined(USE_DEPRECATED_WEAVER)
    g_srWeaver->weave();
#else
    // NOTE: runtimes <1.34 do not support non-fullscreen windows, nor non-fullscreen viewports!
    // NOTE: runtimes <1.34 need the fullscreen size as input to the weave function, >=1.34 will ignore it

    g_srWeaver->weave(/* ignored in runtime >=1.34 */ g_windowWidth, /* ignored in runtime >=1.34 */g_windowHeight);
#endif

    //! [Render woven image]

    // Display.
    glFlush();

    // Get DC.
    HDC hDC = GetDC(g_hWnd);
    if (presentAsFastAsPossible) wglSwapIntervalEXT(0);
    SwapBuffers(hDC);
    if (presentAsFastAsPossible) wglSwapIntervalEXT(1);
    ReleaseDC(g_hWnd, hDC);

    // Update window title with FPS.
    UpdateWindowTitle(g_hWnd, curTime);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool isMoving = false;
    switch (message)
    {

    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0; // Message handled

    case WM_DESTROY:
        PostQuitMessage(0); // Causes GetMessage() to return 0 and end message loop
        return 0; // Message handled

    // Handle keypresses.
    case WM_KEYDOWN:
        switch (wParam)
        {
            // Quit.
            case VK_ESCAPE:
                PostMessage(hWnd, WM_CLOSE, 0, 0); // Gracefully initiate close
                return 0; // Message handled

            // Toggle windowed/fullscreen.
            case VK_F11:
                g_fullscreen = !g_fullscreen;
                SetFullscreen(hWnd, g_fullscreen);
                return 0; // Message handled

            // Simulate destruction of context.
            case 'c':
            case 'C':
            {
                SR::SystemEvent newSystemEvent{
                    (uint64_t)std::chrono::system_clock::now().time_since_epoch().count(),
                    SR_eventType::ContextInvalid,
                    "ContextInvalidEvent",
                };
                g_srSystemEventListener->accept(newSystemEvent);
                return 0; // Message handled
            }
        }
        break;

    // Limit minimum size of window to not cause crash when width or height becomes 0
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);

        // Set the minimum tracking size (in pixels)
        mmi->ptMinTrackSize.x = 100; // Min width
        mmi->ptMinTrackSize.y = 100; // Min height

        return 0; // Message handled
    }

    // Keep track of window size. Assumption: rendering is not in  a separate thread (needs mutex)
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED &&
            (
                g_windowWidth != LOWORD(lParam) ||
                g_windowHeight != HIWORD(lParam) 
            ) ) 
        {
            g_windowWidth = LOWORD(lParam);
            g_windowHeight = HIWORD(lParam);
            if (g_sceneReady) Render(true);
        }
        break;

    case WM_ENTERSIZEMOVE:
        // Assumption: rendering is not in a separate thread
        // When user holds an edge/titlebar, but not moves, WM_PAINT will never be called. Invalidate the rect here, and in WM_PAINT invalididate the window.
        // We need to rerender in order to keep 3D directed to the user's position.
        isMoving = true;
        InvalidateRect(hWnd, nullptr, false);
        break;

    case WM_EXITSIZEMOVE:
        isMoving = false;
        break;

    case WM_PAINT:
        // Don't waste cycles on the default paint handler.
        if (isMoving)
        {
            // Assumption: rendering is not in a separate thread
            // Keep on rendering while windows is holding WndProc hostage when user drags titlebar or resizes the window.
            Render();
            // Do not validate the window here, so WM_PAINT will be sent again
        }
        else
        { 
            // Validate the window, so no more paint messages are sent (rendering will be done in the main loop)
            PAINTSTRUCT ps;
            if (BeginPaint(hWnd, &ps))
                EndPaint(hWnd, &ps);
        }
        return 0; // return value seems not to matter. Microsoft uses 0 in the examples.

    case WM_ERASEBKGND:
        // We fully draw the entire surface already, no need to erase the background. It also prevents showing a white backround when we dont render fast enough.
        return 1; // Message handled
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

void CreateConsole()
{
    if (!AllocConsole())
    {
        OnError("Failed to create console");
        return;
    }

    // std::cout, std::clog, std::cerr, std::cin
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();

    // std::wcout, std::wclog, std::wcerr, std::wcin
    HANDLE hConOut = CreateFile("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hConIn = CreateFile("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
    SetStdHandle(STD_ERROR_HANDLE, hConOut);
    SetStdHandle(STD_INPUT_HANDLE, hConIn);
    std::wcout.clear();
    std::wclog.clear();
    std::wcerr.clear();
    std::wcin.clear();
}

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    // Ensure the application receives unscaled display metrics
    SetProcessDpiAwareness(PROCESS_DPI_AWARENESS::PROCESS_PER_MONITOR_DPI_AWARE);

    // Get path for stereo image file from command-line if available.
    bool needToCreateConsoleWindow = false;
    if (lpCmdLine != nullptr)
    {
        const char* consoleToken = "-console";

        if (strlen(lpCmdLine) > 0)
        {
            // Get lowercase command-line.
            _strlwr(lpCmdLine);
            std::string cmdLine = lpCmdLine;

            // Find and strip console token if it's present.                        
            size_t consoleTokenOffset = cmdLine.find(consoleToken);
            if (consoleTokenOffset != std::string::npos)
            {
                cmdLine = cmdLine.erase(consoleTokenOffset, strlen(consoleToken));
                needToCreateConsoleWindow = true;
            }

            // Trim whitespaces.
            cmdLine.erase(cmdLine.begin(), std::find_if_not(cmdLine.begin(), cmdLine.end(), [](char c) { return std::isspace(c); }));
            cmdLine.erase(std::find_if_not(cmdLine.rbegin(), cmdLine.rend(), [](char c) { return std::isspace(c); }).base(), cmdLine.end());

            // If there's anything else on the cmdline, it's the stereo image filename.
            if (cmdLine.length() > 0)
                strcpy(g_stereoImageFile, cmdLine.c_str());
        }
    }

    // Always create console window for debug builds.
#ifdef _DEBUG
    needToCreateConsoleWindow = true;
#endif

    // Create console window.
    if (needToCreateConsoleWindow)
        CreateConsole();

    // Create SR context.
    if (!CreateSRContext(10.0))
    {
        OnError("Failed to create SR context");
        return 1;
    }

    // Register window class.
    WNDCLASSEX wcex = {};
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LEIA_LARGE));
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName  = NULL;
    wcex.lpszClassName = g_windowClass;
    wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_LEIA_SMALL));
    if (0 == RegisterClassEx(&wcex))
    {
        OnError("Failed to register window class");
        return 2;
    }

    // Create window.
    int monitorTopLeftX = 0;
    int monitorTopLeftY = 0;
    g_hWnd = CreateGraphicsWindow(hInstance, monitorTopLeftX, monitorTopLeftY);
    if (g_hWnd == NULL)
    {
        OnError("Failed to create window");
        return 3;
    }

    // Get DC.
    HDC hDC = GetDC(g_hWnd);

    // Get monitor we are using.
    const HMONITOR monitorHandle = MonitorFromPoint({ monitorTopLeftX,monitorTopLeftY }, MONITOR_DEFAULTTOPRIMARY);
        
    // Get DPI for monitor.
    UINT dpiX = 0;
    UINT dpiY = 0;
    if (FAILED(GetDpiForMonitor(monitorHandle, MDT_RAW_DPI, &dpiX, &dpiY)))
    {
        OnError("Failed to create window");
        return 4;
    }
                
    // Compute screen size in millimeters, which we need to compute the projection matrices for 3D mode.
    const size_t systemResolutionWidth  = GetSystemMetrics(SM_CXSCREEN);
    const size_t systemResolutionHeight = GetSystemMetrics(SM_CYSCREEN);
    g_ScreenWidthInMM  = systemResolutionWidth * 25.4f / dpiX;
    g_ScreenHeightInMM = systemResolutionHeight * 25.4f / dpiY;

    // Switch to fullscreen at startup if requested.
    if (g_fullscreen)
        SetFullscreen(g_hWnd, true);

    // Initialize graphics.
    HGLRC hglrc = InitializeGL(g_hWnd, hDC);
    if (hglrc == nullptr)
    {
        OnError("Failed to initialize OpenGL");
        return 5;
    }
        
    // Initialize LeiaSR.
    if (!InitializeLeiaSR(10.0))
    {
        OnError("Failed to initialize LeiaSR");
        return 6;
    }

    // Prepare everything to draw.
    if (!Load())
    {
        OnError("Failed to load");
        return 7;
    }

    // Show window.
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
    
    // Main loop.
    bool finished = false;
    g_sceneReady = true;
    while (!finished)
    {
        // Empty all messages from queue.
        MSG msg = {};
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (WM_QUIT == msg.message)
            {
                finished = true;
                break;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        // Perform app logic.
        if (!finished)
        {
            // If the context was destroyed, re-create it.
            {
                std::lock_guard<std::recursive_mutex> lock(g_Mutex);

                if (g_reCreateContext)
                {
                    g_reCreateContext = false;

                    // Delete SR objects.
#if !defined(USE_DEPRECATED_WEAVER)
                    SAFE_DESTROY(g_srWeaver);
#else
                    SAFE_DELETE(g_srWeaver);
                    SAFE_DELETE(g_srEyePairListener);
#endif
                    SAFE_DELETE(g_srSystemEventListener);
                    SR::SRContext::deleteSRContext(g_srContext);
                    g_srContext = nullptr;

                    // Re-ininitialize but allow a longer time for it to succeed than at startup.
                    if (!InitializeLeiaSR(30.0))
                    {
                        OnError("Failed to re-initialize LeiaSR after context destruction");
                        return 8;
                    }
                }
            }

            // Render.
            Render();
        }
    }
    
    // Shutdown SR.
#if !defined(USE_DEPRECATED_WEAVER)
    SAFE_DESTROY(g_srWeaver);
#else
    SAFE_DELETE(g_srWeaver);
    SAFE_DELETE(g_srEyePairListener);
#endif
    SAFE_DELETE(g_srSystemEventListener);
    SR::SRContext::deleteSRContext(g_srContext);
    
    // Shutdown GL.
    if (g_viewTexture != 0)
        glDeleteTextures(1, &g_viewTexture);
    if (g_viewDepthTexture != 0)
        glDeleteTextures(1, &g_viewDepthTexture);
    if (g_viewFramebuffer != 0)
        glDeleteFramebuffers(1, &g_viewFramebuffer);
    if (g_vao != 0)
        glDeleteVertexArrays(1, &g_vao);
    if (g_shaderProgram != 0)
        glDeleteProgram(g_shaderProgram);

    return 0;
}