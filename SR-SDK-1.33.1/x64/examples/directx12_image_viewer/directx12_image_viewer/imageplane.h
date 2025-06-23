/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "renderer.h"

class ImagePlane 
{
public:
    ImagePlane(Renderer&, unsigned char* data, int width, int height);
    ~ImagePlane();
    void draw();

    ID3D12Resource* GetTexture() { return Texture; }

private:
    struct VertexTexture
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT2 Texture;
    };

    ID3D12Resource* VertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
    ID3D12Resource* Texture;

    Renderer& renderer;

    void loadTextureOntoPlane(unsigned char* data, int width, int height);
};
