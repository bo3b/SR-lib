/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include <renderer.h>

#include <exception>

Renderer::Renderer(Window& window)
{
    const DirectX::XMINT2 windowSize = window.GetSize();
    PresentParameters.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
    PresentParameters.BackBufferCount = 2;
    PresentParameters.BackBufferFormat = D3DFMT_X8R8G8B8;
    PresentParameters.BackBufferHeight = windowSize.y;
    PresentParameters.BackBufferWidth = windowSize.x;
    PresentParameters.EnableAutoDepthStencil = FALSE;
    PresentParameters.Flags = 0;
    PresentParameters.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    PresentParameters.hDeviceWindow = window.GetHandle();
    PresentParameters.MultiSampleQuality = 0;
    PresentParameters.MultiSampleType = D3DMULTISAMPLE_NONE;
    PresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    PresentParameters.SwapEffect = D3DSWAPEFFECT_FLIP;
    PresentParameters.Windowed = true;

    if (!Init(window.GetHandle(), PresentParameters))
    {
        exit(-1);
        return;
    }

    SwapchainSize = window.GetSize();

    CreateBackbuffer();
}

Renderer::~Renderer()
{
    if (BackbufferSurface != nullptr)
    {
        BackbufferSurface->Release();
        BackbufferSurface = nullptr;
    }

    if (RenderDevice != nullptr)
    {
        RenderDevice->Release();
        RenderDevice = nullptr;
    }
}

IDirect3DDevice9* Renderer::GetDevice()
{
    return RenderDevice;
}

IDirect3DSurface9* Renderer::GetBackbuffer()
{
    return BackbufferSurface;
}

void Renderer::Resize(DirectX::XMINT2 NewSize)
{
    if (NewSize.x > 0 && NewSize.y > 0)
    {
        if (BackbufferSurface != nullptr)
        {
            BackbufferSurface->Release();
            BackbufferSurface = nullptr;
        }

        SwapchainSize = NewSize;

        PresentParameters.BackBufferWidth = SwapchainSize.x;
        PresentParameters.BackBufferHeight = SwapchainSize.y;
        HRESULT hr = RenderDevice->Reset(&PresentParameters);
        if (FAILED(hr))
            throw std::exception("Failed to resize renderer.");

        CreateBackbuffer();
    }
}

DirectX::XMINT2 Renderer::GetSize()
{
    return SwapchainSize;
}

void Renderer::CreateBackbuffer()
{
    HRESULT hr = RenderDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &BackbufferSurface);
    if (FAILED(hr))
        throw std::exception("Failed to create renderer backbuffer.");
}

bool Renderer::Init(HWND hWnd, D3DPRESENT_PARAMETERS& PresentParameters)
{
    IDirect3D9* pD3D9 = Direct3DCreate9(D3D_SDK_VERSION);
    if (pD3D9 == nullptr)
        return false;

    HRESULT hr = pD3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_MIXED_VERTEXPROCESSING, &PresentParameters, &RenderDevice);

    pD3D9->Release();

    return S_OK == hr;
}
