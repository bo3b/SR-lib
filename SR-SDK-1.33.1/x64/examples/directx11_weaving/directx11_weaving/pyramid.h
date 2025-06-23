/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "renderer.h"

class Pyramid {
public:
    Pyramid(Renderer&);
    ~Pyramid();
    void draw();

private:
    struct Vertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Color;
    };

    ID3D11Buffer* Buffer = nullptr;

    Renderer& renderer;
};
