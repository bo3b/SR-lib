/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include "renderer.h"

class Pyramid {
public:
    Pyramid(Renderer&);
    ~Pyramid();
    void draw(ID3D12GraphicsCommandList*);

private:
    ID3D12Resource* VertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;

    Renderer& renderer;
};
