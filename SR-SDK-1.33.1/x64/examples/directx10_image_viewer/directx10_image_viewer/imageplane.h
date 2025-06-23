/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#pragma once
#include <d3d10_1.h>
#include <DirectXMath.h>
#include "renderer.h"

class ImagePlane {
public:
    ImagePlane(Renderer&, unsigned char* data, int width, int height);
    ~ImagePlane();
    void draw();

private:
    struct VertexColor
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Color;
    };

    struct VertexTexture
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT2 Texture;
    };

    ID3D10Buffer* Buffer = nullptr;

    Renderer& renderer;

    ID3D10ShaderResourceView* frameBufferView = nullptr;

    char* textureFileName = "";

    void loadTextureOntoPlane(unsigned char* data, int width, int height);
};
