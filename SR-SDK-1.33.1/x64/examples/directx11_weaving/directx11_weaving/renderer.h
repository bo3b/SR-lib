/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef RENDERER_H
#define RENDERER_H
#include <d3d11.h>
#include <DirectXMath.h>
#include "window.h"

class Renderer
{
public:
    Renderer(Window&);
    ~Renderer();

    ID3D11Device* GetDevice();
    ID3D11DeviceContext* GetContext();
    IDXGISwapChain* GetSwapChain();

    ID3D11RenderTargetView* GetBackbuffer();

    void Resize(DirectX::XMINT2 Size);

    DirectX::XMINT2 GetSize();

private:
    void CreateBackbuffer();

    ID3D11Device* RenderDevice = nullptr;
    ID3D11DeviceContext* RenderContext = nullptr;
    IDXGISwapChain* RenderSwapChain = nullptr;

    ID3D11Texture2D* Backbuffer = nullptr;
    ID3D11RenderTargetView* BackbufferView = nullptr;

    DirectX::XMINT2 SwapchainSize;

    bool Init(DXGI_SWAP_CHAIN_DESC& Desc, D3D11_CREATE_DEVICE_FLAG Flags);
};

#endif
