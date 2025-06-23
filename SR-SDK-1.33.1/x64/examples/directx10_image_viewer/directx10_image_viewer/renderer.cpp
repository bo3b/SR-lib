/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include <renderer.h>
#include <exception>

Renderer::Renderer(Window& window)
{
    DXGI_SWAP_CHAIN_DESC Desc = {};
    Desc.BufferCount = 2;
    Desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    Desc.OutputWindow = window.GetHandle();
    Desc.SampleDesc.Count = 1;
    Desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    Desc.Windowed = true;

    D3D10_CREATE_DEVICE_FLAG Flags = {};

#ifndef NDEBUG
    Flags = { D3D10_CREATE_DEVICE_DEBUG };
#endif

    if (!Init(Desc, Flags))
    {
        Flags = {};
        if (!Init(Desc, Flags))
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

ID3D10Device* Renderer::GetDevice()
{
    return RenderDevice;
}

IDXGISwapChain* Renderer::GetSwapChain()
{
    return RenderSwapChain;
}

ID3D10RenderTargetView* Renderer::GetBackbuffer()
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
        if (FAILED(RenderSwapChain->ResizeBuffers(2, SwapchainSize.x, SwapchainSize.y, DXGI_FORMAT_UNKNOWN, 0)))
            throw std::exception("Failed to resize buffers.");

        CreateBackbuffer();
    }
}

DirectX::XMINT2 Renderer::GetSize()
{
    return SwapchainSize;
}

void Renderer::CreateBackbuffer()
{
    if (FAILED(RenderSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), reinterpret_cast<void**>(&Backbuffer))))
        throw std::exception("Failed to get swapchain buffer.");

    if (FAILED(RenderDevice->CreateRenderTargetView(Backbuffer, nullptr, &BackbufferView)))
        throw std::exception("Failed to create render-target view.");
}

bool Renderer::Init(DXGI_SWAP_CHAIN_DESC& Desc, D3D10_CREATE_DEVICE_FLAG Flags)
{
    return D3D10CreateDeviceAndSwapChain(nullptr,
        D3D10_DRIVER_TYPE_HARDWARE,
        nullptr,
        Flags,
        D3D10_SDK_VERSION,
        &Desc,
        &RenderSwapChain,
        &RenderDevice) == S_OK;
}
