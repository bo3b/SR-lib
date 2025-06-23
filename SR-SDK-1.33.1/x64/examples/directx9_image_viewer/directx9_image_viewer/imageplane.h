/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#pragma once
#include <d3d9.h>
#include <DirectXMath.h>
#include "renderer.h"

class ImagePlane {
public:
    ImagePlane(Renderer&, unsigned char* data, int width, int height);
    ~ImagePlane();
    void draw();
    void invalidateDeviceObjects();
    void restoreDeviceObjects();

private:
    /*struct VertexColor
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Color;
    };*/

    struct VertexTexture
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT2 Texture;
    };

    IDirect3DVertexBuffer9* Buffer = nullptr;

    Renderer& renderer;

    IDirect3DTexture9* frameBufferTexture = nullptr;

    char* textureFileName = "";

    unsigned char* imageData = nullptr;
    int imageWidth = 0;
    int imageHeight = 0;

    void loadTextureOntoPlane(unsigned char* data, int width, int height);
};
