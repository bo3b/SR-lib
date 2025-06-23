/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include <renderer.h>

Renderer::Renderer(Window& window)
{
    DXGI_SWAP_CHAIN_DESC Desc;
    ZeroMemory(&Desc, sizeof(DXGI_SWAP_CHAIN_DESC));

    Desc.BufferCount = 2;
    Desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    Desc.OutputWindow = window.GetHandle();
    Desc.SampleDesc.Count = 1;
    Desc.SwapEffect = (DXGI_SWAP_EFFECT)4;  // DXGI_SWAP_EFFECT_FLIP_DISCARD;
    Desc.Windowed = window.IsWindowed();

    D3D11_CREATE_DEVICE_FLAG Flags = {};

#ifndef NDEBUG
    Flags = { D3D11_CREATE_DEVICE_DEBUG };
#endif

    if(!Init(Desc, Flags))
    {
        Flags = {};
        if(!Init(Desc, Flags))
        {
            exit(-1);
            return;
        }
    }

    CreateBackbuffer();

    SwapchainSize = window.GetSize();
}

Renderer::~Renderer()
{
    if (Backbuffer != nullptr)
    {
        Backbuffer->Release();
        Backbuffer = nullptr;
    }

    if (BackbufferView != nullptr)
    {
        BackbufferView->Release();
        BackbufferView = nullptr;
    }

    if (RenderContext != nullptr)
    {
        RenderContext->Release();
        RenderContext = nullptr;
    }

    if (RenderSwapChain != nullptr)
    {
        RenderSwapChain->SetFullscreenState(false, nullptr);
        RenderSwapChain->Release();
        RenderSwapChain = nullptr;
    }

    if (RenderDevice != nullptr)
    {
        RenderDevice->Release();
        RenderDevice = nullptr;
    }
}

ID3D11Device* Renderer::GetDevice()
{
    return RenderDevice;
}

ID3D11DeviceContext* Renderer::GetContext()
{
    return RenderContext;
}

IDXGISwapChain* Renderer::GetSwapChain()
{
    return RenderSwapChain;
}

ID3D11RenderTargetView* Renderer::GetBackbuffer()
{
    return BackbufferView;
}

void Renderer::Resize(DirectX::XMINT2 NewSize)
{
    if (NewSize.x > 0 && NewSize.y > 0)
    {
        if (Backbuffer != nullptr)
        {
            Backbuffer->Release();
            Backbuffer = nullptr;
        }

        if (BackbufferView != nullptr)
        {
            BackbufferView->Release();
            BackbufferView = nullptr;
        }

        SwapchainSize = NewSize;
        RenderSwapChain->ResizeBuffers(2, SwapchainSize.x, SwapchainSize.y, DXGI_FORMAT_UNKNOWN, 0);

        CreateBackbuffer();
    }
}

DirectX::XMINT2 Renderer::GetSize()
{
    return SwapchainSize;
}

void Renderer::CreateBackbuffer()
{
    RenderSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&Backbuffer));
    RenderDevice->CreateRenderTargetView(Backbuffer, nullptr, &BackbufferView);
}

bool Renderer::Init(DXGI_SWAP_CHAIN_DESC& Desc, D3D11_CREATE_DEVICE_FLAG Flags)
{
    return D3D11CreateDeviceAndSwapChain(nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        Flags,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &Desc,
        &RenderSwapChain,
        &RenderDevice,
        nullptr,
        &RenderContext) == S_OK;
}
