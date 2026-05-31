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
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include "d3dx12.h"

// LeiaSR includes
#include "sr/utility/exception.h"
#include "sr/sense/core/inputstream.h"
#include "sr/sense/system/systemsense.h"
#include "sr/sense/eyetracker/eyetracker.h"
#include "sr/world/display/display.h"
#include "sr/weaver/dx12weaver.h"
#include "leia_math.h"

// This app's includes.
#include "resource.h"

// Use STB for image loading.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Library for SetProcessDpiAwareness().
#pragma comment(lib, "shcore.lib")

// DX libraries.
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

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
const char*               g_windowTitle               = "LeiaSR Weaving Example (D3D12)";  // Title of the window
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
SR::PredictingDX12Weaver* g_srWeaver                  = nullptr;                           // Legacy weaver
SREyePairListener*        g_srEyePairListener         = nullptr;                           // SR eyepair listener, to get the eye positions
#else
SR::IDX12Weaver1*          g_srWeaver                  = nullptr;                           // Current weaver
#endif
bool                      g_sceneReady                = false;                             // True when Render() can be called

// User-changeable options.
int            g_windowX       = 0;                            // Initial window left position when not in fullscreen mode
int            g_windowY       = 0;                            // Initial window top position when not in fullscreen mode
int            g_windowWidth   = 1280;                         // Initial window width when not in fullscreen mode
int            g_windowHeight  = 720;                          // Initial window height when not in fullscreen mode
bool           g_fullscreen    = true;                         // Whether to start in fullscreen (use F11 to toggle)
eDemoMode      g_demoMode      = eDemoMode::StereoImage;       // Demo mode (image or 3D rendering)
eColorSpace    g_colorSpace    = eColorSpace::sRGBHardware;    // Default to using sRGB with hardware conversions
eTargetDisplay g_targetDisplay = eTargetDisplay::Auto;         // Start on SR display

// Global DX Variables.
const int                     g_frameCount                            = 2;                              // Number of swapchain buffers
ID3D12Device*                 g_device                                = nullptr;                        // D3D12 device
ID3D12CommandQueue*           g_commandQueue                          = nullptr;                        // Command queue
ID3D12CommandAllocator*       g_commandAllocator[g_frameCount]        = {};                             // Command allocator 
IDXGISwapChain3*              g_swapChain                             = nullptr;                        // Swapchain
ID3D12Resource*               g_SwapChainBuffers[g_frameCount]        = {};                             // Swapchain buffers
CD3DX12_CPU_DESCRIPTOR_HANDLE g_SwapChainBufferViews[g_frameCount]    = {};                             // Swapchain buffer views
int                           g_frameIndex                            = 0;                              // Current frame index
DXGI_FORMAT                   g_swapChainFormat                       = DXGI_FORMAT_R8G8B8A8_UNORM;     // Swapchain format (view format is dynamic, depending on sRGB)
ID3D12DescriptorHeap*         g_srvHeap                               = nullptr;                        // Shader resource view heap
UINT                          g_srvHeapUsed                           = 0;                              // Shader resource view heap used
UINT                          g_srvHeapDescriptorSize                 = 0;                              // Shader resource view heap descriptor size
ID3D12DescriptorHeap*         g_rtvHeap                               = nullptr;                        // Render-target view heap
UINT                          g_rtvHeapUsed                           = 0;                              // Render-target view heap used
UINT                          g_rtvHeapDescriptorSize                 = 0;                              // Render-target view heap descriptor size
ID3D12DescriptorHeap*         g_dsvHeap                               = nullptr;                        // Depth-stencil view heap
UINT                          g_dsvHeapUsed                           = 0;                              // Depth-stencil view heap used
UINT                          g_dsvHeapDescriptorSize                 = 0;                              // Depth-stencil view heap descriptor size
ID3D12Fence*                  g_fence                                 = nullptr;                        // GPU fence
UINT64                        g_fenceValues[g_frameCount]             = {};                             // Fence values (for each swapchain buffer)
HANDLE                        g_fenceEvent                            = NULL;                           // Event used with the fence
ID3D12Resource*               g_viewTexture                           = nullptr;                        // View texture
CD3DX12_CPU_DESCRIPTOR_HANDLE g_viewTextureSRV                        = {};                             // View texture read view
CD3DX12_CPU_DESCRIPTOR_HANDLE g_viewTextureRTV                        = {};                             // View texture write view
ID3D12Resource*               g_viewDepthTexture                      = nullptr;                        // View depth texture
CD3DX12_CPU_DESCRIPTOR_HANDLE g_viewDepthTextureDSV                   = {};                             // View depth texture write view
const FLOAT                   g_viewTextureDefaultColor[4]            = { 0.05f, 0.05f, 0.25f, 1.0f };  // Default color of view texture.
int                           g_viewTextureWidth                      = 0;                              // Width of view texture
int                           g_viewTextureHeight                     = 0;                              // Height of view texture
DXGI_FORMAT                   g_viewDepthTextureFormat                = DXGI_FORMAT_D32_FLOAT;          // View depth texture format
ID3D12GraphicsCommandList*    g_textureUploadCommandList              = nullptr;                        // Command list used to upload stereo image texture
const FLOAT                   g_backbufferColor[4]                    = { 0.0f, 0.25f, 0.0f, 1.0f };    // Default color of swapchain buffers.
ID3D12Resource*               g_vertexBuffer                          = nullptr;                        // Vertex buffer for cube geometry
ID3D12Resource*               g_indexBuffer                           = nullptr;                        // Index buffer for cube geometry
D3D12_VERTEX_BUFFER_VIEW      g_vertexBufferView                      = {};                             // Vertex buffer view for cube geometry
D3D12_INDEX_BUFFER_VIEW       g_indexBufferView                       = {};                             // Index buffer view for cube geometry
D3D12_INPUT_LAYOUT_DESC       g_inputLayoutDesc                       = {};                             // Input layout of vertex buffer
ID3D12Resource*               g_constantBuffer[g_frameCount]          = {};                             // Constant buffer for cube geometry (for each swapchain buffer)
void*                         g_constantBufferDataBegin[g_frameCount] = {};                             // Pointer to start of constant buffer memory (for each swapchain buffer)
ID3D12GraphicsCommandList*    g_commandList                           = nullptr;                        // D3D12 Command list
ID3D12RootSignature*          g_rootSignature                         = nullptr;                        // D3D12 root signature
ID3D12PipelineState*          g_pipelineState                         = nullptr;                        // D3D12 pipeline state
ID3DBlob*                     g_compiledVertexShaderBlob              = nullptr;                        // Compiled vertex shader for rendering the cube
ID3DBlob*                     g_compiledPixelShaderBlob               = nullptr;                        // Compiled pixel shader for rendering the cube

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

void TransitionResourceState(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES fromState, D3D12_RESOURCE_STATES toState)
{
    assert(resource != nullptr);
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, fromState, toState);
    commandList->ResourceBarrier(1, &barrier);
}

bool CreateSwapChainBuffers()
{
    // Create a RTV for each frame.
    for (int n = 0; n < g_frameCount; n++)
    {
        // Get buffer.
        {
            HRESULT hr = g_swapChain->GetBuffer(n, _uuidof(ID3D12Resource), (void**)&g_SwapChainBuffers[n]);
            if (FAILED(hr))
            {
                OnError("Failed to get swapchain buffer");
                return false;
            }
        }

        // Create view.
        {
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Format               = GetTextureFormat();
            rtvDesc.Texture2D.MipSlice   = 0;
            rtvDesc.Texture2D.PlaneSlice = 0;

            g_SwapChainBufferViews[n] = CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());
            g_SwapChainBufferViews[n].Offset(g_rtvHeapUsed);

            g_device->CreateRenderTargetView(g_SwapChainBuffers[n], &rtvDesc, g_SwapChainBufferViews[n]);

            g_rtvHeapUsed += g_rtvHeapDescriptorSize;
        }
    }

    return true;
}

// Wait for pending GPU work to complete.
void WaitForGpu()
{
    UINT64 fenceVal = g_fenceValues[g_frameIndex];
    // Schedule a Signal command in the queue.
    g_commandQueue->Signal(g_fence, fenceVal);

    // Wait until the fence has been processed.
    g_fence->SetEventOnCompletion(fenceVal, g_fenceEvent);
    WaitForSingleObjectEx(g_fenceEvent, INFINITE, FALSE);

    // Increment the fence value for the current frame.
    //g_fenceValues[g_frameIndex]++;
    for (int i=0; i<g_frameCount; i++)
        g_fenceValues[i] = fenceVal + 1;
}

bool ResizeBuffers(int width, int height)
{
    if ((g_device != nullptr) && (g_swapChain != nullptr))
    {
        // Wait for queue to be empty.
        WaitForGpu();

        // Release swapchain buffers.
        for (int i = 0; i < g_frameCount; i++)
            SAFE_RELEASE(g_SwapChainBuffers[i]);

        // Clear swapchain buffer views.
        memset(g_SwapChainBufferViews, 0, sizeof(g_SwapChainBufferViews));

        // Reset rtv heap used so we re-create the view at the same offset.
        g_rtvHeapUsed = 0;

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
            OnError("Failed to resize swapchain.");
            return false;
        }

        // Get current swapchain buffer index.
        g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();

        // Create swapchain buffers.
        CreateSwapChainBuffers();
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

            const bool isAMD       = adapterDesc.VendorId == 0x1002;
            const bool isIntel     = adapterDesc.VendorId == 0x8086;
            const bool isNVIDIA    = adapterDesc.VendorId == 0x10DE;
            const bool isMicrosoft = adapterDesc.VendorId == 0x1414;
            const bool isSoftware  = adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;

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
            if (SUCCEEDED(D3D12CreateDevice(currentAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                ret = currentAdapter;
                ret->AddRef();
            }
        }

        currentAdapter->Release();
    }

    return ret;
}

// Prepare to render the next frame.
void MoveToNextFrame()
{
    // Schedule a Signal command in the queue.
    const UINT64 currentFenceValue = g_fenceValues[g_frameIndex];
    g_commandQueue->Signal(g_fence, currentFenceValue);

    // Get current swapchain buffer index.
    g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (g_fence->GetCompletedValue() < g_fenceValues[g_frameIndex])
    {
        g_fence->SetEventOnCompletion(g_fenceValues[g_frameIndex], g_fenceEvent);
        WaitForSingleObjectEx(g_fenceEvent, INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    g_fenceValues[g_frameIndex] = currentFenceValue + 1;
}

HRESULT InitializeD3D12()
{
    HRESULT hr = S_OK;

    // Get window size.
    RECT rc;
    GetClientRect(g_hWnd, &rc);
    const UINT width = rc.right   - rc.left;
    const UINT height = rc.bottom - rc.top;

    // Create factory.
    IDXGIFactory4* factory = nullptr;
    {
        // Get flags and enable the debug layer (requires the Graphics Tools "optional feature").
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        UINT flags = 0;
#if defined(_DEBUG)
        {
            ID3D12Debug* debugController = nullptr;
            if (SUCCEEDED(D3D12GetDebugInterface(_uuidof(ID3D12Debug), (void**) &debugController)))
            {
                debugController->EnableDebugLayer();

                // Enable additional debug layers.
                flags |= DXGI_CREATE_FACTORY_DEBUG;
            }
        }
#endif
    
        // Create factory.
        hr = CreateDXGIFactory2(flags, _uuidof(IDXGIFactory4), (void**) &factory);
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
    hr = D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), (void**) &g_device);
    if (FAILED(hr))
    {
        OnError("Failed to create D3D12 device");
        return hr;
    }

    // Release adapter.
    hardwareAdapter->Release();
    
    // Create the command queue.
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        hr = g_device->CreateCommandQueue(&queueDesc, _uuidof(ID3D12CommandQueue), (void**)&g_commandQueue);
        if (FAILED(hr))
        {
            OnError("Failed to create command queue");
            return hr;
        }
    }

    // Create the swap chain.
    {
        // Create swapchain description.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount      = g_frameCount;
        swapChainDesc.Width            = width;
        swapChainDesc.Height           = height;
        swapChainDesc.Format           = g_swapChainFormat;
        swapChainDesc.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        // Create swapchain.
        IDXGISwapChain1* swapChain = nullptr;
        hr = factory->CreateSwapChainForHwnd(g_commandQueue, g_hWnd, &swapChainDesc, nullptr, nullptr, &swapChain);
        if (FAILED(hr))
        {
            OnError("Failed to create swapchain");
            return hr;
        }

        // Get swapchain interface.
        swapChain->QueryInterface(_uuidof(IDXGISwapChain3), (void**) &g_swapChain);
        if (g_swapChain == nullptr)
        {
            OnError("Failed to create query interface for IDXGISwapChain3");
            return hr;
        }

        // Release swapchain now that we have a newer interface.
        swapChain->Release();
    }

    // Associate window and release factory.
    {
        // Prevent Alt-Enter from going to fullscreen mode (has higher latency)
        factory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_WINDOW_CHANGES);
        factory->Release();
    }

    // Get current swapchain buffer index.
    g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();

    // Create render target view (RTV) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 64;
    rtvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = g_device->CreateDescriptorHeap(&rtvHeapDesc, _uuidof(ID3D12DescriptorHeap), (void**) &g_rtvHeap);
    if (FAILED(hr))
    {
        OnError("Failed to create RTV descriptor heap");
        return hr;
    }
    g_rtvHeapDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create depth stencil view (DSV) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 64;
    dsvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = g_device->CreateDescriptorHeap(&dsvHeapDesc, _uuidof(ID3D12DescriptorHeap), (void**)&g_dsvHeap);
    if (FAILED(hr))
    {
        OnError("Failed to create DSV descriptor heap");
        return hr;
    }
    g_dsvHeapDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // Ceate shader resource view (SRV) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 64;
    srvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = g_device->CreateDescriptorHeap(&srvHeapDesc, _uuidof(ID3D12DescriptorHeap), (void**)&g_srvHeap);
    if (FAILED(hr))
    {
        OnError("Failed to create SRV descriptor heap");
        return hr;
    }
    g_srvHeapDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    // Create command allocators.
    for (int i = 0; i < g_frameCount; i++)
    {
        hr = g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, _uuidof(ID3D12CommandAllocator), (void**)&g_commandAllocator[i]);
        if (FAILED(hr))
        {
            OnError("Failed to create command allocator.");
            return hr;
        }
    }

    // Create commandlist for rendering.
    {
        hr = g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator[g_frameIndex], NULL, _uuidof(ID3D12GraphicsCommandList), (void**)&g_commandList);
        if (FAILED(hr))
        {
            OnError("Failed to create command list.");
            return hr;
        }

        hr = g_commandList->Close();
        if (FAILED(hr))
        {
            OnError("Failed to close command list.");
            return hr;
        }
    }

    {
        // Create command list for uploading a texture.
        hr = g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator[g_frameIndex], nullptr, _uuidof(ID3D12GraphicsCommandList), (void**)&g_textureUploadCommandList);
        if (FAILED(hr))
        {
            OnError("Failed to create texture upload command list.");
            assert(false);
        }

        hr = g_textureUploadCommandList->Close();
        if (FAILED(hr))
        {
            OnError("Failed to close texture upload command list.");
            assert(false);
        }
    }

    // Create swapchain buffers.
    CreateSwapChainBuffers();

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        hr = g_device->CreateFence(g_fenceValues[g_frameIndex], D3D12_FENCE_FLAG_NONE, _uuidof(ID3D12Fence), (void**)&g_fence);
        if (FAILED(hr))
            return hr;
        g_fenceValues[g_frameIndex]++;

        // Create an event handle to use for frame synchronization.
        g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (g_fenceEvent == nullptr)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (FAILED(hr))
                return hr;
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        WaitForGpu();
    }

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
    g_srWeaver = new SR::PredictingDX12Weaver(*g_srContext, g_device, g_commandAllocator[0], g_commandQueue, nullptr, nullptr, g_hWnd);
    g_srWeaver->setInputFrameBuffer(g_viewTexture);
    g_srWeaver->setOutputFrameBuffer(g_SwapChainBuffers[0]);
    g_srWeaver->setCommandList(g_commandList);
#else
    WeaverErrorCode createWeaverResult = SR::CreateDX12Weaver(g_srContext, g_device, g_hWnd, &g_srWeaver);
    if (createWeaverResult != WeaverErrorCode::WeaverSuccess)
    {
        OnError("Failed to create weaver");
        return false;
    }
    g_srWeaver->setInputViewTexture(g_viewTexture, g_viewTextureWidth, g_viewTextureHeight, GetTextureFormat());
    g_srWeaver->setOutputFormat(GetTextureFormat());
#endif

    // Set in-shader sRGB conversion if necessary.
    if (g_colorSpace == eColorSpace::sRGBShader)
        g_srWeaver->setShaderSRGBConversion(true, true);

    // Initialize context after creating weaver.
    g_srContext->initialize();

    return true;
}

bool CreateViewTexture()
{
    SAFE_RELEASE(g_viewTexture);
    SAFE_RELEASE(g_viewDepthTexture);
   
    // Describe and create a Texture2D.
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels          = 1;
    textureDesc.Format             = GetTextureFormat();
    textureDesc.Width              = g_viewTextureWidth * 2;
    textureDesc.Height             = g_viewTextureHeight;
    textureDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    textureDesc.DepthOrArraySize   = 1;
    textureDesc.SampleDesc.Count   = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    // Create resource.
    {
        D3D12_CLEAR_VALUE optimizedClearValue = {};
        optimizedClearValue.Color[0] = GetSRGB(g_viewTextureDefaultColor[0]);
        optimizedClearValue.Color[1] = GetSRGB(g_viewTextureDefaultColor[1]);
        optimizedClearValue.Color[2] = GetSRGB(g_viewTextureDefaultColor[2]);
        optimizedClearValue.Color[3] = GetSRGB(g_viewTextureDefaultColor[3]);
        optimizedClearValue.Format   = textureDesc.Format;
        
        CD3DX12_HEAP_PROPERTIES heapPropertiesDefault(D3D12_HEAP_TYPE_DEFAULT);

        HRESULT hr = g_device->CreateCommittedResource(
            &heapPropertiesDefault,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COMMON,
            &optimizedClearValue,
            _uuidof(ID3D12Resource),
            (void**) &g_viewTexture);
        
        if (FAILED(hr))
        {
            OnError("Failed to create offscreen frame buffer texture.");
            return false;
        }
    }

    // Create shader view.
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format                        = textureDesc.Format;
        srvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip     = 0;
        srvDesc.Texture2D.MipLevels           = 1;
        srvDesc.Texture2D.PlaneSlice          = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        g_viewTextureSRV = CD3DX12_CPU_DESCRIPTOR_HANDLE(g_srvHeap->GetCPUDescriptorHandleForHeapStart());
        g_viewTextureSRV.Offset(g_srvHeapUsed);

        g_device->CreateShaderResourceView(g_viewTexture, &srvDesc, g_viewTextureSRV);

        g_srvHeapUsed += g_srvHeapDescriptorSize;
    }

    // Create render-target view.
    {
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Format               = textureDesc.Format;
        rtvDesc.Texture2D.MipSlice   = 0;
        rtvDesc.Texture2D.PlaneSlice = 0;

        g_viewTextureRTV = CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());
        g_viewTextureRTV.Offset(g_rtvHeapUsed);

        g_device->CreateRenderTargetView(g_viewTexture, &rtvDesc, g_viewTextureRTV);

        g_rtvHeapUsed += g_rtvHeapDescriptorSize;
    }

    // Modify description for a depth texture.
    textureDesc.Format = g_viewDepthTextureFormat;
    textureDesc.Flags  = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // Create depth texture.
    {
        D3D12_CLEAR_VALUE optimizedClearValue = {};
        optimizedClearValue.DepthStencil.Depth = 1.0f;
        optimizedClearValue.Format             = textureDesc.Format;

        CD3DX12_HEAP_PROPERTIES heapPropertiesDefault(D3D12_HEAP_TYPE_DEFAULT);

        HRESULT hr = g_device->CreateCommittedResource(
            &heapPropertiesDefault,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &optimizedClearValue,
            _uuidof(ID3D12Resource),
            (void**)&g_viewDepthTexture);

        if (FAILED(hr))
        {
            OnError("Failed to create depth texture");
            return false;
        }
    }

    // Create depth-stencil view.
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Format             = textureDesc.Format;
        dsvDesc.Flags              = D3D12_DSV_FLAG_NONE;
        dsvDesc.Texture2D.MipSlice = 0;

        g_viewDepthTextureDSV = CD3DX12_CPU_DESCRIPTOR_HANDLE(g_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        g_viewDepthTextureDSV.Offset(g_dsvHeapUsed);

        g_device->CreateDepthStencilView(g_viewDepthTexture, &dsvDesc, g_viewDepthTextureDSV);

        g_dsvHeapUsed += g_dsvHeapDescriptorSize;
    }

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
    // Upload heap used for textures, to be released only after upload completes.
    ID3D12Resource* textureUploadHeap = nullptr;

    if (g_demoMode == eDemoMode::Spinning3DCube)
    {
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

            CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
            CD3DX12_RESOURCE_DESC   resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

            // Note: using upload heaps to transfer static data like vert buffers is not 
            // recommended. Every time the GPU needs it, the upload heap will be marshalled 
            // over. Please read up on Default Heap usage. An upload heap is used here for 
            // code simplicity and because there are very few verts to actually transfer.
            HRESULT hr = g_device->CreateCommittedResource(
                &uploadHeap,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                _uuidof(ID3D12Resource),
                (void**) &g_vertexBuffer);

            if (FAILED(hr))
            {
                OnError("Failed to create vertex buffer.");
                return false;
            }

            // Copy the triangle data to the vertex buffer.
            void* pVertexDataBegin = nullptr;
            CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
            hr = g_vertexBuffer->Map(0, &readRange, &pVertexDataBegin);
            if (FAILED(hr))
            {
                OnError("Failed to map vertex buffer.");
                return false;
            }

            memcpy(pVertexDataBegin, vertices.data(), vertexBufferSize);
            g_vertexBuffer->Unmap(0, nullptr);

            // Initialize the vertex buffer view.
            g_vertexBufferView.BufferLocation = g_vertexBuffer->GetGPUVirtualAddress();
            g_vertexBufferView.StrideInBytes  = vertexSize;
            g_vertexBufferView.SizeInBytes    = vertexBufferSize;
        }

        // Create index buffer.
        {
            // Format = int
            const int indexSize = sizeof(unsigned int);
            const int indexBufferSize = indexCount * indexSize;

            CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
            CD3DX12_RESOURCE_DESC   resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

            // Note: using upload heaps to transfer static data like vert buffers is not 
            // recommended. Every time the GPU needs it, the upload heap will be marshalled 
            // over. Please read up on Default Heap usage. An upload heap is used here for 
            // code simplicity and because there are very few verts to actually transfer.
            HRESULT hr = g_device->CreateCommittedResource(
                &uploadHeap,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                _uuidof(ID3D12Resource),
                (void**) &g_indexBuffer);

            if (FAILED(hr))
            {
                OnError("Failed to create index buffer.");
                return false;
            }

            // Copy the triangle data to the index buffer.
            void* pIndexDataBegin = nullptr;
            CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
            hr = g_indexBuffer->Map(0, &readRange, &pIndexDataBegin);
            if (FAILED(hr))
            {
                OnError("Failed to map index buffer.");
                return false;
            }

            memcpy(pIndexDataBegin, indices.data(), indexBufferSize);
            g_indexBuffer->Unmap(0, nullptr);

            // Initialize the index buffer view.
            g_indexBufferView.BufferLocation = g_indexBuffer->GetGPUVirtualAddress();
            g_indexBufferView.SizeInBytes = indexBufferSize;
            g_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
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

        UINT compileFlags = 0;
#ifdef _DEBUG
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        // Compile the vertex shader
        ID3DBlob* pVSErrors = nullptr;
        HRESULT hr = D3DCompile(vertexShaderText, strlen(vertexShaderText), NULL, NULL, NULL, "VSMain", "vs_5_0", compileFlags, 0, &g_compiledVertexShaderBlob, &pVSErrors);
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

        // Compile the pixel shader        
        ID3DBlob* pPSErrors = nullptr;
        hr = D3DCompile(pixelShaderText, strlen(pixelShaderText), NULL, NULL, NULL, "PSMain", "ps_5_0", compileFlags, 0, &g_compiledPixelShaderBlob, &pPSErrors);
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

        // Create vertex input layout.
        {
            static D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };

            g_inputLayoutDesc.NumElements = _countof(inputElementDescs);
            g_inputLayoutDesc.pInputElementDescs = inputElementDescs;
        }

        {
            const int shaderUniformBufferSize        = sizeof(CONSTANTBUFFER);
            const int shaderUniformBufferSizeRounded = (shaderUniformBufferSize + 255) & ~255; // round to 256 bytes

            CD3DX12_HEAP_PROPERTIES heapPropsUpload(D3D12_HEAP_TYPE_UPLOAD);
            CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(shaderUniformBufferSizeRounded);

            for (int i = 0; i < g_frameCount; i++)
            {
                HRESULT hr = g_device->CreateCommittedResource(
                    &heapPropsUpload,
                    D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    _uuidof(ID3D12Resource),
                    (void**)&g_constantBuffer[i]);

                if (FAILED(hr))
                {
                    OnError("Failed to create constant buffer.");
                    return false;
                }

                // Map and initialize the constant buffer. We don't unmap this until the
                // app closes. Keeping things mapped for the lifetime of the resource is okay.
                CD3DX12_RANGE readRange(0, 0);
                hr = g_constantBuffer[i]->Map(0, &readRange, reinterpret_cast<void**>(&g_constantBufferDataBegin[i]));
                if (FAILED(hr))
                {
                    OnError("Failed to map constant buffer.");
                    return false;
                }
            }
        }

        // Create root signature.
        {
            D3D12_ROOT_DESCRIPTOR rootCBVDescriptor = {};
            rootCBVDescriptor.RegisterSpace  = 0;
            rootCBVDescriptor.ShaderRegister = 0;

            D3D12_ROOT_PARAMETER rootParameters = {};
            rootParameters.ParameterType    = D3D12_ROOT_PARAMETER_TYPE_CBV;
            rootParameters.Descriptor       = rootCBVDescriptor;
            rootParameters.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

            const D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            // Create root signature.
            CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
            rootSignatureDesc.Init(1, &rootParameters, 0, nullptr, rootSignatureFlags);

            // Serialize signature.
            ID3DBlob* signature = nullptr;
            ID3DBlob* pErrorBlob = nullptr;
            HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &pErrorBlob);
            if (FAILED(hr))
            {
                char msg[2048] = {};
                if (pErrorBlob != nullptr)
                {
                    strncpy(msg, (const char*)pErrorBlob->GetBufferPointer(), pErrorBlob->GetBufferSize());
                    pErrorBlob->Release();
                }

                OnError("Failed to serialize root signature.");
                return false;
            }

            // Create signature.
            hr = g_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_rootSignature));
            if (FAILED(hr))
            {
                OnError("Failed to create root signature.");
                return false;
            }
        }

        // Create pipeline state object to render into the view texture.
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.InputLayout              = g_inputLayoutDesc;
            psoDesc.pRootSignature           = g_rootSignature;
            psoDesc.VS                       = CD3DX12_SHADER_BYTECODE(g_compiledVertexShaderBlob);
            psoDesc.PS                       = CD3DX12_SHADER_BYTECODE(g_compiledPixelShaderBlob);
            psoDesc.RasterizerState          = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            psoDesc.BlendState               = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                    
            psoDesc.DepthStencilState        = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
            psoDesc.SampleDesc.Count         = 1;
            psoDesc.SampleDesc.Quality       = 0;
            psoDesc.SampleMask               = UINT_MAX;
            psoDesc.PrimitiveTopologyType    = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets         = 1;
            psoDesc.RTVFormats[0]            = GetTextureFormat();
            psoDesc.DSVFormat                = g_viewDepthTextureFormat;
            psoDesc.SampleDesc.Count         = 1;

            HRESULT hr = g_device->CreateGraphicsPipelineState(&psoDesc, _uuidof(ID3D12PipelineState), (void**)&g_pipelineState);
            if (FAILED(hr))
            {
                OnError("Failed to create pipeline state");
                return false;
            }
        }

        // Create view texture.
        CreateViewTexture();
    }
    else if (g_demoMode == eDemoMode::StereoImage)
    {
        // Load stereo image (always expand channels to 4).
        int imageWidth = 0;
        int imageHeight = 0;
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

        // Update view texture size to match image size.
        g_viewTextureWidth  = imageWidth / 2;
        g_viewTextureHeight = imageHeight;

        // Create view texture.
        CreateViewTexture();

        // Upload stereo image into view texture.
        {
            const UINT64 uploadBufferSize = GetRequiredIntermediateSize(g_viewTexture, 0, 1);

            CD3DX12_HEAP_PROPERTIES heapPropertiesUpload(D3D12_HEAP_TYPE_UPLOAD);

            CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

            // Create the GPU upload buffer.
            HRESULT hr = g_device->CreateCommittedResource(
                &heapPropertiesUpload,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                _uuidof(ID3D12Resource),
                (void**)&textureUploadHeap);

            if (FAILED(hr))
            {
                OnError("Failed to create texture upload buffer");
                return false;
            }

            // Prepare command-list.
            {
                // Reset list.
                hr = g_textureUploadCommandList->Reset(g_commandAllocator[g_frameIndex], NULL);
                if (FAILED(hr))
                {
                    OnError("Failed to reset texture upload command-list");
                    return false;
                }
            }

            // Fill command-list.
            {
                D3D12_SUBRESOURCE_DATA textureData = {};
                textureData.pData      = imageData;
                textureData.RowPitch   = imageWidth * 4;
                textureData.SlicePitch = imageHeight * textureData.RowPitch;

                UpdateSubresources(g_textureUploadCommandList, g_viewTexture, textureUploadHeap, 0, 0, 1, &textureData);

                TransitionResourceState(g_textureUploadCommandList, g_viewTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
            }

            // Close command-list.
            hr = g_textureUploadCommandList->Close();
            if (FAILED(hr))
            {
                OnError("Failed to close texture upload command-list");
                return false;
            }

            // Execute command list.
            ID3D12CommandList* ppCommandLists[] = { g_textureUploadCommandList };
            g_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
        }
    }

    // Wait for loading to complete.
    {
        // Create fence.
        ID3D12Fence* loadSceneFence = nullptr;
        HRESULT hr = g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, _uuidof(ID3D12Fence), (void**)&loadSceneFence);
        if (FAILED(hr))
        {
            OnError("Failed to create load scene fence");
            return false;
        }

        // Create an event handle to use for frame synchronization.
        HANDLE loadSceneFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (loadSceneFenceEvent == nullptr)
        {
            OnError("Failed to create load scene fence event");
            return false;
        }

        // Signal wait for fence event.
        g_commandQueue->Signal(loadSceneFence, 1);
        loadSceneFence->SetEventOnCompletion(1, loadSceneFenceEvent);
        WaitForSingleObject(loadSceneFenceEvent, INFINITE);

        // Release.
        loadSceneFence->Release();
        CloseHandle(loadSceneFenceEvent);
        SAFE_RELEASE(textureUploadHeap);
    }

#ifdef USE_DEPRECATED_WEAVER
    g_srWeaver->setInputFrameBuffer(g_viewTexture);
    g_srWeaver->setOutputFrameBuffer(g_SwapChainBuffers[0]);
#else
    g_srWeaver->setInputViewTexture(g_viewTexture, g_viewTextureWidth, g_viewTextureHeight, GetTextureFormat());
    g_srWeaver->setOutputFormat(GetTextureFormat());
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

        assert((g_ScreenWidthInMM != 0.0f) && (g_ScreenHeightInMM != 0.0f));

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

void Render(bool presentAsFastAsPossible=false)
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

    // Reset allocator.
    g_commandAllocator[g_frameIndex]->Reset();

    // Reset command-list.
    g_commandList->Reset(g_commandAllocator[g_frameIndex], g_pipelineState);
    if (g_rootSignature != nullptr)
        g_commandList->SetGraphicsRootSignature(g_rootSignature);

    // Transition swapchain render-target (final output) from common/present to render-target.
    TransitionResourceState(g_commandList, g_SwapChainBuffers[g_frameIndex], D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Clear back-buffer.
    const FLOAT color[4] = { GetSRGB(g_backbufferColor[0]), GetSRGB(g_backbufferColor[1]), GetSRGB(g_backbufferColor[2]), GetSRGB(g_backbufferColor[3]) };
    g_commandList->ClearRenderTargetView(g_SwapChainBufferViews[g_frameIndex], color, 0, NULL);

    if (g_demoMode == eDemoMode::StereoImage)
    {
        // Nothing to do.
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

        // Render cube into stereo view texture.

        // Transition offscreen render-target (intermediate stereo texture with views) from shader-input to render-target.
        TransitionResourceState(g_commandList, g_viewTexture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

        //! [Drawing Scene]

        // Clear view texture to blue
        const FLOAT offscreenColor[4] = { GetSRGB(g_viewTextureDefaultColor[0]), GetSRGB(g_viewTextureDefaultColor[1]), GetSRGB(g_viewTextureDefaultColor[2]), GetSRGB(g_viewTextureDefaultColor[3]) };
        g_commandList->ClearRenderTargetView(g_viewTextureRTV, offscreenColor, 0, NULL);
        g_commandList->ClearDepthStencilView(g_viewDepthTextureDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);

        // Render stereo views.
        for (int i = 0; i < 2; i++)
        {
            // Set viewport to render to left, then right.
            CD3DX12_VIEWPORT viewport = {};
            viewport.TopLeftX = (FLOAT)(i * g_viewTextureWidth);
            viewport.TopLeftY = 0.0f;
            viewport.Width    = (FLOAT)g_viewTextureWidth;
            viewport.Height   = (FLOAT)g_viewTextureHeight;
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            g_commandList->RSSetViewports(1, &viewport);

            // Set scissor.
            D3D12_RECT scissorRect;
            scissorRect.left   = i * g_viewTextureWidth;
            scissorRect.top    = 0;
            scissorRect.right  = scissorRect.left + g_viewTextureWidth;
            scissorRect.bottom = scissorRect.top + g_viewTextureHeight;
            g_commandList->RSSetScissorRects(1, &scissorRect);

            g_commandList->OMSetRenderTargets(1, &g_viewTextureRTV, FALSE, &g_viewDepthTextureDSV);
            g_commandList->IASetIndexBuffer(&g_indexBufferView);
            g_commandList->IASetVertexBuffers(0, 1, &g_vertexBufferView);
            g_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                       
            g_commandList->SetGraphicsRootConstantBufferView(0, g_constantBuffer[i]->GetGPUVirtualAddress());
            mat4f mvp = CalculateModelViewProjectionMatrix((i == 0) ? leftEye : rightEye, (float)elapsedTime);
            memcpy(g_constantBufferDataBegin[i], &mvp, sizeof(mvp));

            g_commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
        }

        //! [Drawing Scene]

        // Transition offscreen texture to a shader input.
        TransitionResourceState(g_commandList, g_viewTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
    }
     
    // Setup to render to backbuffer.
    g_commandList->OMSetRenderTargets(1, &g_SwapChainBufferViews[g_frameIndex], FALSE, NULL);

    // Set viewport to cover entire window.
    CD3DX12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = (FLOAT)g_windowWidth;
    viewport.Height = (FLOAT)g_windowHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    g_commandList->RSSetViewports(1, &viewport);

    // Set scissor.
    D3D12_RECT scissorRect = {};
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = g_windowWidth;
    scissorRect.bottom = g_windowHeight;
    g_commandList->RSSetScissorRects(1, &scissorRect);

    // Perform weaving.
#if !defined(USE_DEPRECATED_WEAVER)
    // Set the command list, viewport and scissorRect for the weaver.
    g_srWeaver->setCommandList(g_commandList);
    g_srWeaver->setViewport(viewport);
    g_srWeaver->setScissorRect(scissorRect);
    g_srWeaver->weave();
#else
    // NOTE: runtimes <1.34 do not support non-fullscreen windows, nor non-fullscreen viewports!

    // NOTE: deprecated weaver interface for DX12 does not have support for fractional viewports
    assert(viewport.TopLeftX == (int)viewport.TopLeftX && "Viewport top-left must be integer for deprecated weaver");
    assert(viewport.TopLeftY == (int)viewport.TopLeftY && "Viewport top-left must be integer for deprecated weaver");
    assert(viewport.Width    == (int)viewport.Width    && "Viewport size must be integer for deprecated weaver");
    assert(viewport.Height   == (int)viewport.Height   && "Viewport size must be integer for deprecated weaver");
    
    g_srWeaver->weave((int)viewport.Width, (int)viewport.Height, (int)viewport.TopLeftX, (int)viewport.TopLeftY);
#endif

    TransitionResourceState(g_commandList, g_SwapChainBuffers[g_frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    // Execute the command list.
    {
        g_commandList->Close();

        ID3D12CommandList* ppCommandLists[] = { g_commandList };
        g_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    }

    if (presentAsFastAsPossible)
        g_swapChain->Present(0, 0); // No Vsync.
    else
        g_swapChain->Present(1, 0);

    MoveToNextFrame();

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
            // Render to the new backbuffer as fast as possible, so that the compositor does not have to stretch the previous frame.
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

    // Show warning if we're using sRGB color space.
#ifdef USE_DEPRECATED_WEAVER
    if (g_colorSpace == eColorSpace::sRGBHardware)
    {
        const char* msg = "Warning: To use the deprecated weaver with hardware sRGB, set the render-target view format in the weaver constructor instead of calling setOutputFrameBuffer().\n";
        std::cout << msg << std::endl;
        OutputDebugString(msg);
    }
#endif

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
    g_ScreenWidthInMM  = systemResolutionWidth  * 25.4f / dpiX;
    g_ScreenHeightInMM = systemResolutionHeight * 25.4f / dpiY;

    // Switch to fullscreen if requested.
    if (g_fullscreen)
        SetFullscreen(g_hWnd, true);

    // Initialize graphics.
    HRESULT hr = InitializeD3D12();
    if (FAILED(hr))
    {
        OnError("Failed to initialize D3D11");
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

                    // Wait for any rendering to finish.
                    WaitForGpu();

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

    // Wait for any rendering to finish.
    WaitForGpu();

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
    {
        CloseHandle(g_fenceEvent);

        for (int i = 0; i < g_frameCount; i++)
        {
            SAFE_RELEASE(g_constantBuffer[i]);
            SAFE_RELEASE(g_SwapChainBuffers[i]);
            SAFE_RELEASE(g_commandAllocator[i]);
        }

        SAFE_RELEASE(g_compiledPixelShaderBlob);
        SAFE_RELEASE(g_compiledVertexShaderBlob);
        SAFE_RELEASE(g_pipelineState);
        SAFE_RELEASE(g_rootSignature);
        SAFE_RELEASE(g_commandList);    
        SAFE_RELEASE(g_indexBuffer);
        SAFE_RELEASE(g_vertexBuffer);
        SAFE_RELEASE(g_viewDepthTexture);
        SAFE_RELEASE(g_viewTexture);
        SAFE_RELEASE(g_fence);
        SAFE_RELEASE(g_dsvHeap);    
        SAFE_RELEASE(g_rtvHeap);    
        SAFE_RELEASE(g_textureUploadCommandList);
        SAFE_RELEASE(g_srvHeap);
        SAFE_RELEASE(g_swapChain);    
        SAFE_RELEASE(g_commandQueue);
        SAFE_RELEASE(g_device);
    }

    return 0;
}