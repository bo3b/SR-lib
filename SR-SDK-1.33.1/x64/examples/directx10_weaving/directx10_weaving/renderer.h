/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef RENDERER_H
#define RENDERER_H
#include <d3d10_1.h>
#include <DirectXMath.h>
#include "window.h"

class Renderer
{
public:
    Renderer(Window&);
    ~Renderer();

    ID3D10Device* GetDevice();
    IDXGISwapChain* GetSwapChain();

    ID3D10RenderTargetView* GetBackbuffer();

    void Resize(DirectX::XMINT2 Size);

    DirectX::XMINT2 GetSize();

private:
    void CreateBackbuffer();

    ID3D10Device* RenderDevice = nullptr;
    IDXGISwapChain* RenderSwapChain = nullptr;

    ID3D10Texture2D* Backbuffer = nullptr;
    ID3D10RenderTargetView* BackbufferView = nullptr;

    DirectX::XMINT2 SwapchainSize;

    bool Init(DXGI_SWAP_CHAIN_DESC& Desc, D3D10_CREATE_DEVICE_FLAG Flags);
};

#endif
