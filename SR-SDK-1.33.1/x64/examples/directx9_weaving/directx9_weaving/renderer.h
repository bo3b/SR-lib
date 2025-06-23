/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef RENDERER_H
#define RENDERER_H
#include <d3d9.h>
#include <DirectXMath.h>
#include "window.h"

class Renderer
{
public:
    Renderer(Window&);
    ~Renderer();

public:
    IDirect3DDevice9* GetDevice();
    IDirect3DSurface9* GetBackbuffer();
    void Resize(DirectX::XMINT2 Size);
    DirectX::XMINT2 GetSize();

private:
    void CreateBackbuffer();
    bool Init(HWND hWnd, D3DPRESENT_PARAMETERS& PresentParameters);

private:
    IDirect3DDevice9* RenderDevice = nullptr;
    IDirect3DSurface9* BackbufferSurface = nullptr;
    DirectX::XMINT2 SwapchainSize = {};
    D3DPRESENT_PARAMETERS PresentParameters = {};
};

#endif
