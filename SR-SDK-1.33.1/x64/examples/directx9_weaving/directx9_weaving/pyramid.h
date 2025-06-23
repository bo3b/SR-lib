/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#pragma once
#include <d3d9.h>
#include <DirectXMath.h>
#include "renderer.h"

class Pyramid {
public:
    Pyramid(Renderer&);
    ~Pyramid();
    void draw();
    void invalidateDeviceObjects();
    void restoreDeviceObjects();

private:
    struct Vertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Color;
    };

    IDirect3DVertexBuffer9* Buffer = nullptr;
    
    Renderer& renderer;
};
