/*!
 * Copyright (C) 2025 Leia, Inc.
 */

// Windows includes.
#define WIN32_LEAN_AND_MEAN
#include "targetver.h"
#include <windows.h>
#include <assert.h>
#include <shellscalingapi.h>

// DX includes.
#include <d3d10_1.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>

// LeiaSR includes
#include "sr/utility/exception.h"
#include "sr/sense/core/inputstream.h"
#include "sr/sense/system/systemsense.h"
#include "sr/sense/eyetracker/eyetracker.h"
#include "sr/world/display/display.h"
#include "sr/weaver/dx10weaver.h"
#include "leia_math.h"

// This app's includes.
#include "resource.h"

// Use STB for image loading.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Library for SetProcessDpiAwareness().
#pragma comment(lib, "shcore.lib")

// DX libraries.
#pragma comment(lib, "d3d10.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

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
const char*               g_windowTitle               = "LeiaSR Weaving Example (D3D10)"; // Title of the window
const char*               g_windowClass               = g_windowTitle;                     // Class name for the window
SR::SRContext*            g_srContext                 = nullptr;                           // SR context
SRSystemEventListener*    g_srSystemEventListener     = nullptr;                           // SR event listener, to receive SR status updates
float                     g_ScreenWidthInMM           = 0;                                 // Width of SR display
float                     g_ScreenHeightInMM          = 0;                                 // Height of SR display
HWND                      g_hWnd                      = NULL;                              // Window handle
char                      g_stereoImageFile[MAX_PATH] = "StereoImage.jpg";                 // Path to default stereo image to load
bool                      g_reCreateContext           = false;                             // When the context is unexpectedly lost, this is set to true
std::recursive_mutex      g_Mutex;                                                         // Mutex for synchronization
#ifdef USE_DEPRECATED_WEAVER
SR::PredictingDX10Weaver* g_srWeaver                  = nullptr;                           // Legacy weaver
SREyePairListener*        g_srEyePairListener         = nullptr;                           // SR eyepair listener, to get the eye positions
#else
SR::IDX10Weaver1*          g_srWeaver                  = nullptr;                           // Current weaver
#endif

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

// Global DX Variables.
ID3D10Device*             g_device               = nullptr;     // D3D11 device
IDXGISwapChain*           g_swapChain            = nullptr;     // Swapchain
ID3D10RenderTargetView*   g_renderTargetView     = nullptr;     // Swapchain buffer view
ID3D10Texture2D*          g_viewTexture          = nullptr;     // View texture
ID3D10ShaderResourceView* g_viewTextureSRV       = nullptr;     // View texture read view
ID3D10RenderTargetView*   g_viewTextureRTV       = nullptr;     // View texture write view
ID3D10Texture2D*          g_viewDepthTexture     = nullptr;     // View depth texture
ID3D10DepthStencilView*   g_viewDepthTextureDSV  = nullptr;     // View depth texture write view
int                       g_viewTextureWidth     = 0;           // Width of view texture
int                       g_viewTextureHeight    = 0;           // Height of view texture
ID3D10Buffer*             g_vertexBuffer         = nullptr;     // Vertex buffer for cube geometry
ID3D10Buffer*             g_indexBuffer          = nullptr;     // Index buffer for cube geometry
ID3D10Buffer*             g_shaderConstantBuffer = nullptr;     // Constant buffer for cube geometry
ID3D10VertexShader*       g_vertexShader         = nullptr;     // Vertex shader for cube geometry
ID3D10InputLayout*        g_inputLayout          = nullptr;     // Input layout of vertex buffer
ID3D10PixelShader*        g_pixelShader          = nullptr;     // Pixel shader for cube geometry

#pragma pack(push, 1)

// Shader unforms.
struct CONSTANTBUFFER
{
    float transform[16];
};

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
DXGI_FORMAT GetTextureFormat()
{
    return (g_colorSpace == eColorSpace::sRGBHardware) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
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
        RECT rect;
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

bool ResizeBuffers(int width, int height)
{
    if ((g_device != nullptr) && (g_swapChain != nullptr))
    {
        g_device->OMSetRenderTargets(0, nullptr, nullptr);  // Unbind any render targets

        if (g_renderTargetView != nullptr)
        {
            SAFE_RELEASE(g_renderTargetView);
        }

        // Resize swapchain.
        HRESULT hr = g_swapChain->ResizeBuffers(
            0,              // Buffer count: 0 = keep current count
            width,
            height,
            DXGI_FORMAT_UNKNOWN, // Keep existing format
            0
        );
        if (FAILED(hr))
        {
            // Handle error: device lost, etc.
            OnError("Error while resizing swapchain.");
            return false;
        }
        
        // Get swapchain buffer.
        ID3D10Texture2D* pBackBuffer = nullptr;
        hr = g_swapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), reinterpret_cast<void**>(&pBackBuffer));
        if (FAILED(hr))
        {
            OnError("Error while getting swapchain buffer.");
            return false;
        }

        // Create a render target view.
        hr = g_device->CreateRenderTargetView(pBackBuffer, nullptr, &g_renderTargetView);
        pBackBuffer->Release();
        if (FAILED(hr))
        {
            OnError("Error while creating rendertarget view.");
            return false;
        }
    }

    return true;
}

// Helper function for finding the best available hardware adapter.
IDXGIAdapter1* GetHardwareAdapter(IDXGIFactory1* pFactory)
{
    IDXGIAdapter1* ret = nullptr;

    // Find best GPU
    std::vector<IDXGIAdapter1*> availableAdapterList;
    {
        IDXGIAdapter1* currentAdapter = nullptr;
        SIZE_T highestDedicatedGpuMemory = 0;

        for (UINT AdapterIndex = 0; pFactory->EnumAdapters1(AdapterIndex, &currentAdapter) != DXGI_ERROR_NOT_FOUND; AdapterIndex++)
        {
            DXGI_ADAPTER_DESC1 adapterDesc;
            currentAdapter->GetDesc1(&adapterDesc);

            const bool isAMD = adapterDesc.VendorId == 0x1002;
            const bool isIntel = adapterDesc.VendorId == 0x8086;
            const bool isNVIDIA = adapterDesc.VendorId == 0x10DE;
            const bool isMicrosoft = adapterDesc.VendorId == 0x1414;
            const bool isSoftware = adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;

            // Decide whether this adapter should be considered for usage
            if (!isMicrosoft && !isSoftware)
            {
                //Note the highest amount of dedicated GPU memory:
                if (adapterDesc.DedicatedVideoMemory > highestDedicatedGpuMemory)
                {
                    highestDedicatedGpuMemory = adapterDesc.DedicatedVideoMemory;

                    // Put adapter at the start of the list.
                    availableAdapterList.insert(availableAdapterList.begin(), (IDXGIAdapter1*)currentAdapter);
                }
                else
                {
                    // Put the adapter at the end of the list.
                    availableAdapterList.push_back((IDXGIAdapter1*)currentAdapter);
                }
            }
            else
            {
                currentAdapter->Release();
            }
        }
    }

    // Select an adapter, always take the first compatible adapter in the list since it will have the most dedicated memory.
    for (IDXGIAdapter1* currentAdapter : availableAdapterList)
    {
        if (ret == nullptr)
        {
            //if (SUCCEEDED(D3D10CreateDevice(currentAdapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, NULL)))
            {
                ret = currentAdapter;
                ret->AddRef();
            }
        }

        currentAdapter->Release();
    }

    return ret;
}

HRESULT InitializeD3D10()
{
    HRESULT hr = E_FAIL;

    // Get window size.
    RECT rc;
    GetClientRect(g_hWnd, &rc);
    const UINT width  = rc.right  - rc.left;
    const UINT height = rc.bottom - rc.top;

    // Create DXGI factory.
    IDXGIFactory4* factory = nullptr;
    {
        // Get creation flags.
        UINT flags = 0;
#ifdef _DEBUG
        flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

        hr = CreateDXGIFactory2(flags, _uuidof(IDXGIFactory4), (void**)&factory);
        if (FAILED(hr))
        {
            OnError("Failed to create DXGI factory");
            return hr;
        }
    }

    // Get hardware adapter.
    IDXGIAdapter1* hardwareAdapter = GetHardwareAdapter(factory);
    if (hardwareAdapter == nullptr)
    {
        OnError("Failed to get hardware adapter");
        return hr;
    }

    // Create device.
    {
        UINT flags = 0;
#ifdef _DEBUG
        flags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

        const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_10_1 };
        const int featureLevelCount = ARRAYSIZE(featureLevels);

        // Create device.
        hr = D3D10CreateDevice
        (
            hardwareAdapter,
            D3D10_DRIVER_TYPE_HARDWARE,
            nullptr,
            flags,
            D3D10_SDK_VERSION,
            &g_device
        );

        if (FAILED(hr))
        {
            OnError("Could not find a Direct3D10 device.");
            return hr;
        }
    }

    // Release adapter.
    hardwareAdapter->Release();

    // Create swap chain
    {
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount            = 1;
        sd.BufferDesc.Width       = width;
        sd.BufferDesc.Height      = height;
        sd.BufferDesc.Format      = GetTextureFormat();
        sd.BufferDesc.RefreshRate = {0, 0};
        sd.BufferUsage            = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow           = g_hWnd;
        sd.SampleDesc.Count       = 1;
        sd.SampleDesc.Quality     = 0;
        sd.Windowed               = TRUE;
        sd.SwapEffect             = DXGI_SWAP_EFFECT_DISCARD;

        hr = factory->CreateSwapChain(g_device, &sd, &g_swapChain);
        if (FAILED(hr))
            OnError("Error creating IDXGISwapChain.");
        
        // Don't continue if we failed.
        if (FAILED(hr))
            return hr;
    }

    // Associate window and release factory.
    {
        // Prevent Alt-Enter from going to fullscreen mode (has higher latency)
        factory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_WINDOW_CHANGES);
        factory->Release();
    }

    // Update render-target view.
    if (!ResizeBuffers(width, height))
        return E_FAIL;

    return S_OK;
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

    // Handle error if context wasn't created (likely because SRService.exe isn't running).
    if (g_srContext == nullptr)
        return false;

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

    // Create weaver.
#ifdef USE_DEPRECATED_WEAVER
    g_srWeaver = new SR::PredictingDX10Weaver(*g_srContext, g_device, g_viewTextureWidth * 2, g_viewTextureHeight, g_hWnd);
    if (!g_useDeprecatedFramebuffer || (g_demoMode == eDemoMode::StereoImage))
        g_srWeaver->setInputFrameBuffer(g_viewTextureSRV);
#else
    WeaverErrorCode createWeaverResult = SR::CreateDX10Weaver(g_srContext, g_device, g_hWnd, &g_srWeaver);
    if (createWeaverResult != WeaverErrorCode::WeaverSuccess)
    {
        OnError("Failed to create weaver");
        return false;
    }
    g_srWeaver->setInputViewTexture(g_viewTextureSRV, g_viewTextureWidth, g_viewTextureHeight, GetTextureFormat());
#endif

    // Set in-shader sRGB conversion if necessary.
    if (g_colorSpace == eColorSpace::sRGBShader)
        g_srWeaver->setShaderSRGBConversion(true, true);

    // Initialize context after creating weaver.
    g_srContext->initialize();

    return true;
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
            indices.emplace_back(startIndex + 2);
            indices.emplace_back(startIndex + 1);
            indices.emplace_back(startIndex + 0);
            indices.emplace_back(startIndex + 3);
            indices.emplace_back(startIndex + 2);

            // Add vertices.
            vertices.emplace_back(VERTEX(cubeVerts[i0], faceColors[i]));
            vertices.emplace_back(VERTEX(cubeVerts[i1], faceColors[i]));
            vertices.emplace_back(VERTEX(cubeVerts[i2], faceColors[i]));
            vertices.emplace_back(VERTEX(cubeVerts[i3], faceColors[i]));
        }        

        // Create vertex buffer.
        {
            // Format = XYZ|RGB
            const int vertexSize       = sizeof(VERTEX);
            const int vertexBufferSize = (int)vertices.size() * vertexSize;

            D3D10_BUFFER_DESC bd = {};
            bd.Usage          = D3D10_USAGE_DEFAULT;
            bd.ByteWidth      = vertexBufferSize;
            bd.BindFlags      = D3D10_BIND_VERTEX_BUFFER;
            bd.CPUAccessFlags = 0;

            D3D10_SUBRESOURCE_DATA initData = {};
            initData.pSysMem = vertices.data();

            HRESULT hr = g_device->CreateBuffer(&bd, &initData, &g_vertexBuffer);
            if (FAILED(hr))
            {
                OnError("Error creating vertex buffer");
                return false;
            }
        }

        // Create index buffer.
        {
            // Format = int
            const int ibsize = indexCount * sizeof(int);

            D3D10_BUFFER_DESC bd = {};
            bd.Usage          = D3D10_USAGE_DEFAULT;
            bd.ByteWidth      = ibsize;
            bd.BindFlags      = D3D10_BIND_INDEX_BUFFER;
            bd.CPUAccessFlags = 0;

            D3D10_SUBRESOURCE_DATA initData = {};
            initData.pSysMem = indices.data();

            HRESULT hr = g_device->CreateBuffer(&bd, &initData, &g_indexBuffer);
            if (FAILED(hr))
            {
                OnError("Error creating index buffer");
                return false;
            }
        }        

        {
            // Round up to 16 bytes.
            int shaderUniformBufferSizeRounded = (sizeof(CONSTANTBUFFER) + 15) & ~15;

            D3D10_BUFFER_DESC bd = {};
            bd.ByteWidth           = shaderUniformBufferSizeRounded;
            bd.Usage               = D3D10_USAGE_DEFAULT;
            bd.BindFlags           = D3D10_BIND_CONSTANT_BUFFER;
            bd.CPUAccessFlags      = 0;
            bd.MiscFlags           = 0;

            // Create the buffer.
            HRESULT hr = g_device->CreateBuffer(&bd, NULL, &g_shaderConstantBuffer);
            if (FAILED(hr))
            {
                OnError("Failed to create constant buffer");
                return false;
            }
        }

        const char* vertexShaderText = 
            "struct VSInput\n"
            "{\n"
            "    float3 Pos : POSITION;\n"
            "    float3 Col : COLOR;\n"
            "};\n"
            "struct PSInput\n"
            "{\n"
            "    float4 Pos : SV_POSITION;\n"
            "    float3 Col : COLOR;\n"
            "};\n"
            "cbuffer ConstantBufferData : register(b0)\n"
            "{\n"
            "    float4x4 transform;\n"
            "};\n"
            "PSInput VSMain(VSInput input)\n"
            "{\n"
            "    PSInput output = (PSInput)0;\n"
            "    output.Pos = mul(transform, float4(input.Pos, 1.0f));\n"
            "    output.Col = input.Col;\n"
            "    return output;\n"
            "}\n";

	    const char* pixelShaderText =
            "struct PSInput\n"
            "{\n"
            "    float4 Pos : SV_POSITION;\n"
            "    float3 Col : COLOR;\n"
            "};\n"
            "float4 PSMain(PSInput input) : SV_Target0\n"
            "{\n"
            "    return float4(input.Col, 1);\n"
            "};\n";

        // Compile the vertex shader
        ID3DBlob* pVSBlob = nullptr;
        ID3DBlob* pVSErrors = nullptr;
        HRESULT hr = D3DCompile(vertexShaderText, strlen(vertexShaderText), NULL, NULL, NULL, "VSMain", "vs_4_0", 0, 0, &pVSBlob, &pVSErrors);
        if (FAILED(hr))
        {
            // Get compile error.
            std::string errorMsg;
            if (pVSErrors != nullptr)
            {
                errorMsg.append((char*)pVSErrors->GetBufferPointer(), pVSErrors->GetBufferSize());
                pVSErrors->Release();
            }
            OnError("Failed to compile vertex shader");
            return false;
        }

        // Create the vertex shader
        hr = g_device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_vertexShader);
        if (FAILED(hr))
        {
            SAFE_RELEASE(pVSBlob);
            OnError("Failed to create vertex shader");
            return false;
        }

        // Define the input layout
        const D3D10_INPUT_ELEMENT_DESC layoutElements[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D10_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        };
        const int layoutElementCount = ARRAYSIZE(layoutElements);

        // Create the input layout        
        hr = g_device->CreateInputLayout(layoutElements, layoutElementCount, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_inputLayout);
        SAFE_RELEASE(pVSBlob);
        if (FAILED(hr))
        {
            OnError("Failed to create vertex layout");
            return false;
        }

        // Compile the pixel shader
        ID3DBlob* pPSBlob = nullptr;
        ID3DBlob* pPSErrors = nullptr;
        hr = D3DCompile(pixelShaderText, strlen(pixelShaderText), NULL, NULL, NULL, "PSMain", "ps_4_0", 0, 0, &pPSBlob, &pPSErrors);
        if (FAILED(hr))
        {
            // Get compile error.
            std::string errorMsg;
            if (pPSErrors != nullptr)
            {
                errorMsg.append((char*)pPSErrors->GetBufferPointer(), pPSErrors->GetBufferSize());
                pPSErrors->Release();
            }

            OnError("Failed to compile pixel shader");
            return false;
        }

        // Create the pixel shader
        hr = g_device->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), &g_pixelShader);
        SAFE_RELEASE(pPSBlob);
        if (FAILED(hr))
        {
            OnError("Failed to create pixel shader");
            return false;
        }

        // Create view texture.
        D3D10_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width            = g_viewTextureWidth * 2;
        textureDesc.Height           = g_viewTextureHeight;
        textureDesc.Format           = GetTextureFormat();
        textureDesc.MipLevels        = 1;
        textureDesc.ArraySize        = 1;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage            = D3D10_USAGE_DEFAULT;
        textureDesc.BindFlags        = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET;
        hr = g_device->CreateTexture2D(&textureDesc, nullptr, &g_viewTexture);
        if (FAILED(hr))
        {
            OnError("Failed to create offscreen texture");
            return false;
        }

        // Create view texture SRV.        
        {
            D3D10_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
            SRVDesc.Format                    = textureDesc.Format;
            SRVDesc.ViewDimension             = D3D10_SRV_DIMENSION_TEXTURE2D;
            SRVDesc.Texture2D.MostDetailedMip = 0;
            SRVDesc.Texture2D.MipLevels       = 1;

            hr = g_device->CreateShaderResourceView(g_viewTexture, &SRVDesc, &g_viewTextureSRV);
            if (FAILED(hr))
            {
                OnError("Failed to create shader resource view");
                return false;
            }
        }

        // Create view texture RTV.        
        {
            D3D10_RENDER_TARGET_VIEW_DESC RTVDesc = {};
            RTVDesc.Format             = textureDesc.Format;
            RTVDesc.ViewDimension      = D3D10_RTV_DIMENSION_TEXTURE2D;
            RTVDesc.Texture2D.MipSlice = 0;
            hr                         = g_device->CreateRenderTargetView(g_viewTexture, &RTVDesc, &g_viewTextureRTV);
            if (FAILED(hr))
            {
                OnError("Failed to create render-target view");
                return false;
            }
        }

        // Create view depth texture.
        textureDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
        textureDesc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
        hr = g_device->CreateTexture2D(&textureDesc, nullptr, &g_viewDepthTexture);
        if (FAILED(hr))
        {
            OnError("Failed to create depth texture");
            return false;
        }

        // Create view depth texture DSV.
        {
            D3D10_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
            DSVDesc.Format             = textureDesc.Format;
            DSVDesc.ViewDimension      = D3D10_DSV_DIMENSION_TEXTURE2D;
            DSVDesc.Texture2D.MipSlice = 0;
            hr = g_device->CreateDepthStencilView(g_viewDepthTexture, &DSVDesc, &g_viewDepthTextureDSV);
            if (FAILED(hr))
            {
                OnError("Failed to create depth-stencil view");
                return false;
            }
        }        
    }
    else if (g_demoMode == eDemoMode::StereoImage)
    {
        // Load stereo image (always expand channels to 4).
        int imageWidth    = 0;
        int imageHeight   = 0;
        int imageChannels = 0;
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

        // Setup subresource data to load image data at texture creation.
        D3D10_SUBRESOURCE_DATA initData = {};
        initData.pSysMem          = imageData;
        initData.SysMemPitch      = imageWidth * 4;
        initData.SysMemSlicePitch = imageHeight * initData.SysMemPitch;

        // Create texture.
        D3D10_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width            = imageWidth;
        textureDesc.Height           = imageHeight;
        textureDesc.Format           = GetTextureFormat();
        textureDesc.MipLevels        = 1;
        textureDesc.ArraySize        = 1;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage            = D3D10_USAGE_DEFAULT;
        textureDesc.BindFlags        = D3D10_BIND_SHADER_RESOURCE;
        HRESULT hr = g_device->CreateTexture2D(&textureDesc, &initData, &g_viewTexture);
        if (FAILED(hr))
        {
            OnError("Failed to create stereo image texture");
            return false;
        }

        // Free image data.
        delete [] imageData;

        // Setup view texture SRV.
        D3D10_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
        SRVDesc.Format                    = textureDesc.Format;
        SRVDesc.ViewDimension             = D3D10_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MostDetailedMip = 0;
        SRVDesc.Texture2D.MipLevels       = 1;

        // Create view texture SRV.
        hr = g_device->CreateShaderResourceView(g_viewTexture, &SRVDesc, &g_viewTextureSRV);
        if (FAILED(hr))
        {
            OnError("Failed to create stereo image shader resource view");
            return false;
        }

        // Remember texture settings.
        g_viewTextureWidth  = textureDesc.Width / 2;
        g_viewTextureHeight = textureDesc.Height;
    }

#ifdef USE_DEPRECATED_WEAVER
    if (!g_useDeprecatedFramebuffer || (g_demoMode == eDemoMode::StereoImage))
        g_srWeaver->setInputFrameBuffer(g_viewTextureSRV);
#else
    g_srWeaver->setInputViewTexture(g_viewTextureSRV, g_viewTextureWidth, g_viewTextureHeight, GetTextureFormat());
#endif

    // Set in-shader sRGB conversion if necessary.
    if (g_colorSpace == eColorSpace::sRGBShader)
        g_srWeaver->setShaderSRGBConversion(true, true);

    return true;
}

mat4f CalculateModelViewProjectionMatrix(const vec3f& eye, float elapsedTime)
{
    // Compute model matrix.
    mat4f model = {};
    {
        const float size  = 60.0;                           // Size of the object in mm
        const float angle = elapsedTime * 0.75f;            // Rotation amount

        const mat4f translation = mat4f::translation(0.0f, 0.0f, 0.0f);
        const mat4f scaling     = mat4f::scaling(size, size, size);
        const mat4f rotation    = mat4f::rotationY(angle);

        model = rotation * scaling * translation;
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

void Render()
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
    const FLOAT backBufferColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f};
    g_device->ClearRenderTargetView(g_renderTargetView, backBufferColor);

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

        // Clear view texture to blue
        const FLOAT offscreenColor[4] = { GetSRGB(0.05f), GetSRGB(0.05f), GetSRGB(0.25f), 1.0f };
        g_device->ClearRenderTargetView(g_viewTextureRTV, offscreenColor);
        g_device->ClearDepthStencilView(g_viewDepthTextureDSV, D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 1.0f, 0);

        // Set view texture as target.
#if defined(USE_DEPRECATED_WEAVER)
        if (!g_useDeprecatedFramebuffer || (g_demoMode == eDemoMode::StereoImage))
        {
            // Use the app-provided view texture.
            g_device->OMSetRenderTargets(1, &g_viewTextureRTV, g_viewDepthTextureDSV);
        }
        else
        {
            // Use the deprecated weaver-provided view texture.
            ID3D10RenderTargetView* View = g_srWeaver->getFrameBuffer();
            g_device->ClearRenderTargetView(View, offscreenColor);
            g_device->OMSetRenderTargets(1, &View, g_viewDepthTextureDSV);
        }
#else
        g_device->OMSetRenderTargets(1, &g_viewTextureRTV, g_viewDepthTextureDSV);
#endif

        // Set shaders and vertex input layout.
        g_device->VSSetShader(g_vertexShader);
        g_device->PSSetShader(g_pixelShader);
        g_device->IASetInputLayout(g_inputLayout);

        // Set vertex buffer (XYZ|RGB)
        UINT stride = 6 * sizeof(float);
        UINT offset = 0;
        g_device->IASetVertexBuffers(0, 1, &g_vertexBuffer, &stride, &offset);

        // Set index buffer.
        g_device->IASetIndexBuffer(g_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

        // Set primitive topology
        g_device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Render stereo views.
        for (int i = 0; i < 2; i++)
        {
            // Set viewport to cover left or right of view texture size.
            D3D10_VIEWPORT viewport = {};
            viewport.TopLeftX = i * g_viewTextureWidth;
            viewport.TopLeftY = 0;
            viewport.Width    = g_viewTextureWidth;
            viewport.Height   = g_viewTextureHeight;
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            g_device->RSSetViewports(1, &viewport);

            // Update constant buffer.
            mat4f mvp = CalculateModelViewProjectionMatrix((i == 0) ? leftEye : rightEye, (float)elapsedTime);
            g_device->UpdateSubresource(g_shaderConstantBuffer, 0, NULL, &mvp, 0, 0);
            g_device->VSSetConstantBuffers(0, 1, &g_shaderConstantBuffer);
            g_device->PSSetConstantBuffers(0, 1, &g_shaderConstantBuffer);

            // Render.
            g_device->DrawIndexed(36, 0, 0);
        }

        //! [Drawing Scene]
    }

    // Set viewport to cover entire window.
    D3D10_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width    = g_windowWidth;
    viewport.Height   = g_windowHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    g_device->RSSetViewports(1, &viewport);

    // Set render-target to swapchain buffer view.
    g_device->OMSetRenderTargets(1, &g_renderTargetView, nullptr);

    // Perform weaving.
#if !defined(USE_DEPRECATED_WEAVER)
    g_srWeaver->weave();
#else
    // NOTE: runtimes <1.34 do not support non-fullscreen windows, nor non-fullscreen viewports!
    // NOTE: runtimes <1.34 need the fullscreen size as input to the weave function, >=1.34 will ignore it

    g_srWeaver->weave(/* ignored in runtime >=1.34 */ g_windowWidth, /* ignored in runtime >=1.34 */g_windowHeight);
#endif

    // Display.
    g_swapChain->Present(1, 0);
    
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
            if (!ResizeBuffers(g_windowWidth, g_windowHeight))
            {
                OnError("Failed resize buffers");
                exit(9);
            }
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

    // Ensure the application receives unscaled display metrics
    SetProcessDpiAwareness(PROCESS_DPI_AWARENESS::PROCESS_PER_MONITOR_DPI_AWARE);

    // Create SR context.
    if (!CreateSRContext(10.0))
    {
        OnError("Failed to create SR context");
        return 1;
    }

    // Register window class.
    WNDCLASSEX wcex;
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
    HRESULT hr = InitializeD3D10();
    if (FAILED(hr))
    {
        OnError("Failed to initialize D3D10");
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
    
    // Shutdown D3D.
    SAFE_RELEASE(g_pixelShader);
    SAFE_RELEASE(g_inputLayout);
    SAFE_RELEASE(g_vertexShader);
    SAFE_RELEASE(g_shaderConstantBuffer);
    SAFE_RELEASE(g_indexBuffer);
    SAFE_RELEASE(g_vertexBuffer);
    SAFE_RELEASE(g_viewDepthTextureDSV);
    SAFE_RELEASE(g_viewDepthTexture);
    SAFE_RELEASE(g_viewTextureRTV);
    SAFE_RELEASE(g_viewTextureSRV);
    SAFE_RELEASE(g_viewTexture);
    SAFE_RELEASE(g_renderTargetView);
    SAFE_RELEASE(g_swapChain);
    SAFE_RELEASE(g_device);

    return 0;
}