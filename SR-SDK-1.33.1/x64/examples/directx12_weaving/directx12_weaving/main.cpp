/*!
 * Copyright (C) 2025 Leia, Inc.
 */

// External dependencies
#include <thread>
#include <chrono>
#include <DirectXMath.h>
#include <shellscalingapi.h>

// Internal dependencies
#include "pyramid.h"
#include "shader.h"
#include "renderer.h"
#include "window.h"

// Simulated Reality includes
#include "d3dx12.h"
#include "sr/types.h"
#include "sr/sense/core/inputstream.h"
#include "sr/sense/handtracker/handtracker.h"
#include "sr/sense/eyetracker/eyetracker.h"
#include "sr/sense/system/systemsense.h"
#include "sr/sense/system/systemevent.h"
#include "sr/world/display/display.h"
#include "sr/weaver/dx12weaver.h"
#include "sr/utility/exception.h"

using namespace std::chrono_literals;

// Flag to indicate that context is no longer valid
bool contextValid = false;

bool initializeSrObjects(); // forward declaration

// User implementation of the SystemEventListener interface
class SystemEventMonitor : public SR::SystemEventListener {
public:
    // Ensures SystemEventStream is cleaned up when Listener object is out of scope
    SR::InputStream<SR::SystemEventStream> stream;

    // The accept function can process the system event data as soon as it becomes available
    virtual void accept(const SR::SystemEvent& frame) override {
        switch (frame.eventType) {
        case SR_eventType::ContextInvalid:
        {
            std::cout
                << "ContextInvalid event received: "
                << frame.time << " "
                << frame.message << "\n";

                contextValid = false;

                std::thread([]() {
                    initializeSrObjects();
                    }).detach();

                return;
        }
            break;
        default:
            std::cout << "Unknown event type" << std::endl;
            break;
        }
    }
};

// My SR::EyePairListener that stores the last tracked eye positions
class MyEyes : public SR::EyePairListener {
private:
    SR::InputStream<SR::EyePairStream> stream;
public:
    DirectX::XMFLOAT3 left, right;
    MyEyes(SR::EyeTracker* tracker) : left(-30, 0, 600), right(30, 0, 600) {
        // Open a stream between tracker and this class
        stream.set(tracker->openEyePairStream(this));
    }
    // Called by the tracker for each tracked eye pair
    virtual void accept(const SR_eyePair& eyePair) override
    {
        // Remember the eye positions
        left = DirectX::XMFLOAT3(eyePair.left.x, eyePair.left.y, eyePair.left.z);
        right = DirectX::XMFLOAT3(eyePair.right.x, eyePair.right.y, eyePair.right.z);
    }
};

HWND WindowHandle;
// Create Window object
Window window("DirectXWindow", "Simulated Reality - Cube demo");
// Create SRContext
SR::SRContext* context = nullptr;
SR::PredictingDX12Weaver* weaver = nullptr;
Renderer* renderer = nullptr;
std::mutex constructNewContextMutex;
MyEyes* eyes = nullptr;
SystemEventMonitor* listener = nullptr;

bool createSrContext() {
    if (context != nullptr) {
        delete context;
        context = nullptr;
    }
    while (context == nullptr && contextValid == false) {
        try {
            context = new SR::SRContext();
            return true;
        }
        catch (SR::ServerNotAvailableException e) {
            std::cout << "Server not available, trying again in 0.5 second" << std::endl;
            std::this_thread::sleep_for(500ms);
        }
    }
    return false;
}

bool initializeSrObjects() {
    std::lock_guard<std::mutex> lock(constructNewContextMutex);
    // weaver needs to be deleted before deleting SRContext as weaver is using the context, but this function does not reconstruct weaver. Construct weaver wherever is needed
    if (weaver != nullptr) {
        delete weaver;
        weaver = nullptr;
    }
    if (eyes != nullptr) {
        delete eyes;
        eyes = nullptr;
    }
    if (listener != nullptr) {
        delete listener;
        listener = nullptr;
    }

    // constructing context
    if (createSrContext() == false) {
        return false;
    }
    // constructing EyePairListener
    eyes = new MyEyes(SR::EyeTracker::create(*context));

    SR::SystemSense* systemSense = SR::SystemSense::create(*context);
    listener = new SystemEventMonitor();
    // set systemEvent listener to the newly constructed systemsense
    listener->stream.set(systemSense->openSystemEventStream(listener));

    context->initialize();
    contextValid = true;
    return true;
}

DirectX::XMMATRIX CalculateModel()
{
    const float size = 20.0; // Size of the object in mm
    using namespace std::chrono;
    auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    const int ms_per_revolution = 5000;
    float angle = float((2 * 3.1415 / ms_per_revolution) * (now % ms_per_revolution));
    
    DirectX::XMMATRIX Model = DirectX::XMMatrixIdentity();
    Model *= DirectX::XMMatrixTranslation(0, 1, 0);     //translate bottom face to origin
    Model *= DirectX::XMMatrixScaling(size, size, size); //scale by size
    Model *= DirectX::XMMatrixRotationY(angle);  //rotate by angle

    return Model;
}

float screenWidth_mm = 697.0;
float screenHeight_mm = 394.0;

DirectX::XMMATRIX CalculateProjection(DirectX::XMVECTOR eye)
{
    // Implementation of: Kooima, Robert. "Generalized perspective projection." J. Sch. Electron. Eng. Comput. Sci (2009).
    const float fnear = 0.1;
    const float ffar = 10000.0;

    DirectX::XMVECTOR pa = { -screenWidth_mm / 2, screenHeight_mm / 2, 0 };
    DirectX::XMVECTOR pb = { screenWidth_mm / 2, screenHeight_mm / 2, 0 };
    DirectX::XMVECTOR pc = { -screenWidth_mm / 2, -screenHeight_mm / 2, 0 };

    DirectX::XMVECTOR vr = { 1, 0, 0 };
    DirectX::XMVECTOR vu = { 0, 1, 0 };
    DirectX::XMVECTOR vn = { 0, 0, 1 };

    // Compute the screen corner vectors.
    DirectX::XMVECTOR va = DirectX::XMVectorSubtract(pa, eye);
    DirectX::XMVECTOR vb = DirectX::XMVectorSubtract(pb, eye);
    DirectX::XMVECTOR vc = DirectX::XMVectorSubtract(pc, eye);

    // Find the distance from the eye to screen plane.
    float distance = -1 * DirectX::XMVector3Dot(va, vn).m128_f32[0];

    // Find the extent of the perpendicular projection.
    float l = DirectX::XMVector3Dot(vr, va).m128_f32[0] * fnear / distance;
    float r = DirectX::XMVector3Dot(vr, vb).m128_f32[0] * fnear / distance;
    float b = DirectX::XMVector3Dot(vu, vc).m128_f32[0] * fnear / distance;
    float t = DirectX::XMVector3Dot(vu, va).m128_f32[0] * fnear / distance;

    float M00 = (2 * fnear) / (r - l);
    float M11 = (2 * fnear) / (t - b);
    float M20 = (r + l) / (r - l);
    float M21 = (t + b) / (t - b);
    float M22 = -(ffar + fnear) / (ffar - fnear);
    float M23 = -1;
    float M32 = -2 * (ffar * fnear) / (ffar - fnear);

    DirectX::XMMATRIX Frustum = DirectX::XMMATRIX(M00, 0, 0, 0,
                                                    0, M11, 0, 0,
                                                    M20, M21, M22, M23,
                                                    0, 0, M32, 0);

    DirectX::XMMATRIX Translate = DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorScale(eye, -1));

    DirectX::XMMATRIX Result = DirectX::XMMatrixMultiply(Translate, Frustum);

    return Result;
}

void RenderScene(Renderer& renderer, const size_t renderWidth, const size_t renderHeight, DirectX::XMFLOAT3 leftEye, DirectX::XMFLOAT3 rightEye, Pyramid& pyramid, ColorShader& shader)
{
    //! [Drawing Scene]
    // Update Model to put object on the finger
    DirectX::XMMATRIX View = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX Model = CalculateModel();

    // Set projection for left rendering
    DirectX::XMMATRIX Projection = CalculateProjection(DirectX::XMLoadFloat3(&leftEye));
    DirectX::XMMATRIX MVP = Model * View * Projection;
    shader.TransformationData[0].Transformation = MVP;
    shader.Bind(renderer.GetCommandList(), 0);

    // Set viewport to the left half
    D3D12_VIEWPORT Viewport;
    Viewport.TopLeftX = 0.0f;
    Viewport.TopLeftY = 0.0f;
    Viewport.Width = renderWidth;
    Viewport.Height = renderHeight;
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    renderer.GetCommandList()->RSSetViewports(1, &Viewport);

    D3D12_RECT ScissorRect;
    ScissorRect.left = 0;
    ScissorRect.right = renderWidth;
    ScissorRect.top = 0;
    ScissorRect.bottom = Viewport.Height;
    renderer.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

    // Render the scene for the left eye
    pyramid.draw(renderer.GetCommandList());

    // Set projection for right rendering
    Projection = CalculateProjection(DirectX::XMLoadFloat3(&rightEye));
    MVP = Model * View * Projection;
    shader.TransformationData[1].Transformation = MVP;
    shader.Bind(renderer.GetCommandList(), 1);

    // Set viewport to the right half
    Viewport.TopLeftX = renderWidth;
    renderer.GetCommandList()->RSSetViewports(1, &Viewport);

    ScissorRect.left = renderWidth;
    ScissorRect.right = renderWidth * 2;
    renderer.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

    // Render the scene for the right eye
    pyramid.draw(renderer.GetCommandList());
    //! [Drawing Scene]
}

int main(void)
{
    // Ensure the application receives unscaled display metrics
    SetProcessDpiAwareness(PROCESS_DPI_AWARENESS::PROCESS_PER_MONITOR_DPI_AWARE);

    //! [SR Initialization]
    initializeSrObjects();
    SR::Display* display = SR::Display::create(*context);
    //! [SR Initialization]

    // Get default size for our views texture.
    int renderWidth = display->getRecommendedViewsTextureWidth();
    int renderHeight = display->getRecommendedViewsTextureHeight();

    // Create renderer object
    Renderer* renderer = nullptr;

    //Can throw a runtime error if a DX12 compatible render adapter could not be found.
    try {
        renderer = new Renderer(window, { renderWidth * 2, renderHeight });
    }
    catch (const std::exception& e) {
        std::cout << "Exception thrown: " << e.what();
        std::cout << "Press enter to exit.\n";
        std::cin.get();
        return 0;
    }

    ColorShader shader(*renderer);
    Pyramid pyramid(*renderer);

    // Wait for all assets to load
    renderer->WaitForFence();

    std::string DisplayDevice = renderer->GetOutputDeviceName(window.GetHandle());

    DEVMODEA deviceMode;
    memset(&deviceMode, 0, sizeof(deviceMode));
    deviceMode.dmSize = sizeof(deviceMode);
    EnumDisplaySettingsA(DisplayDevice.c_str(), ENUM_CURRENT_SETTINGS, &deviceMode);

    if (deviceMode.dmPelsWidth != display->getResolutionWidth() || deviceMode.dmPelsHeight != display->getResolutionHeight())
    {
        deviceMode.dmPelsWidth = display->getResolutionWidth();
        deviceMode.dmPelsHeight = display->getResolutionHeight();

        long Result = ChangeDisplaySettingsExA(DisplayDevice.c_str(), &deviceMode, 0, CDS_FULLSCREEN, 0);
    }

    // The resolution of the DS-1 as recorded on the test desktop PC in "C:/ProgramData/Simulated Reality/Server/Resources/screen.ini"
    // is 7680x4320, while the resolution according to Windows is 3840x2160. Using GetSystemMetrics(SM_C*SCREEN), we get the resolution
    // according to windows of the primary monitor.
    size_t systemResolutionWidth = GetSystemMetrics(SM_CXSCREEN);
    size_t systemResolutionHeight = GetSystemMetrics(SM_CYSCREEN);
    window.SetPositionOnTop(0, 0, systemResolutionWidth, systemResolutionHeight);

    // Get dpi (dots/pixels per inch) from primary monitor and calculate the screen size in millimeters
    HMONITOR PrimMonitorHandle = MonitorFromPoint({ 0,0 }, MONITOR_DEFAULTTOPRIMARY);
    uint dpiX, dpiY;
    GetDpiForMonitor(PrimMonitorHandle, MDT_RAW_DPI, &dpiX, &dpiY);
    // (25.4/dpi is the number of millimeters per pixel: 25.4 = mm/inch, dpi = pixels/ich)
    screenWidth_mm = systemResolutionWidth * 25.4 / dpiX;
    screenHeight_mm = systemResolutionHeight * 25.4 / dpiY;

    // Wait for backbuffers to be resized
    renderer->WaitForFence();

    //! [Construct weaver]
    weaver = new SR::PredictingDX12Weaver(*context, renderer->GetDevice(), renderer->GetCommandAllocator(), renderer->GetCommandQueue(), renderer->GetFramebuffer(), renderer->GetBackbuffer(0), window.GetHandle());

    // Set command list
    weaver->setCommandList(renderer->GetCommandList());

    context->initialize();
    //! [Construct weaver]

    do {
        window.Update();

        if (renderer->GetSize().x != window.GetSize().x || renderer->GetSize().y != window.GetSize().y)
        {
            renderer->Resize(window.GetSize());
        }

        renderer->GetCommandAllocator()->Reset();
        renderer->GetCommandList()->Reset(renderer->GetCommandAllocator(), nullptr);

        // Bind framebuffer as render target
        D3D12_CPU_DESCRIPTOR_HANDLE FramebufferHandle = renderer->BindFramebufferAsTarget();

        // Clear framebuffer
        const float clearColor[] = { 0.5, 0.5, 0.5, 1 };
        renderer->GetCommandList()->ClearRenderTargetView(FramebufferHandle, clearColor, 0, nullptr);

        if (contextValid) {
            RenderScene(*renderer, renderWidth, renderHeight, eyes->left, eyes->right, pyramid, shader);
        }
        else {
            RenderScene(*renderer, renderWidth, renderHeight, { -30, 0, 600 }, { 30, 0, 600 }, pyramid, shader);
        }

        // Bind framebuffer for weaver usage
        renderer->BindFramebufferAsUAV();

        // Bind back buffer as render target
        renderer->BindBackbufferAsTarget();

        //! [Render woven image]
        // ClearRenderTargetView does not need to be called because weaving overwrites each pixel on the backbuffer
        D3D12_VIEWPORT Viewport;
        Viewport.TopLeftX = 0;
        Viewport.TopLeftY = 0;
        Viewport.Width = window.GetSize().x;
        Viewport.Height = window.GetSize().y;
        Viewport.MinDepth = 0;
        Viewport.MaxDepth = 1;

        D3D12_RECT ScissorRect;
        ScissorRect.top = 0;
        ScissorRect.left = 0;
        ScissorRect.right = window.GetSize().x;
        ScissorRect.bottom = window.GetSize().y;

        renderer->GetCommandList()->RSSetViewports(1, &Viewport);
        renderer->GetCommandList()->RSSetScissorRects(1, &ScissorRect);

        if (contextValid && weaver != nullptr) {
            weaver->weave(Viewport.Width, Viewport.Height, Viewport.TopLeftX, Viewport.TopLeftY);
        }
        //! [Render woven image]

        // Bind back buffer for presenting
        renderer->BindBackbufferForPresent();

        renderer->GetCommandList()->Close();

        renderer->ExecuteCommandList();

        // Present the frame.
        renderer->GetSwapChain()->Present(0, 0);

        // Wait for frame to finish
        renderer->WaitForFence();

        if (contextValid && weaver == nullptr) {
            //! [Construct weaver]
            weaver = new SR::PredictingDX12Weaver(*context, renderer->GetDevice(), renderer->GetCommandAllocator(), renderer->GetCommandQueue(), renderer->GetFramebuffer(), renderer->GetBackbuffer(0), window.GetHandle());

            // Set command list
            weaver->setCommandList(renderer->GetCommandList());
            context->initialize();
            //! [Construct weaver]
        }

    } while (
        !(GetKeyState(VK_ESCAPE) & 0x0800) && !window.ShouldClose()
        );

    // Wait for pending commands to finish before quitting
    renderer->WaitForFence();
    if (contextValid == false) {
        contextValid = true;
        std::this_thread::sleep_for(3000ms);
    }

    if (weaver != nullptr) {
        delete weaver;
        weaver = nullptr;
    }
}
