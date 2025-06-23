/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include "imageplane.h"
#include <DirectXMath.h>
#include <d3d9.h>
#include <DirectXPackedVector.h>
#include <stb_image.h>
#include <iostream>

using namespace DirectX;
using namespace DirectX::PackedVector;

//Todo: Make this scale with screen aspect ratio?
//Render a single triangle with a right angle pointing to the right and down. (+X, -Y)
//! [Pentahedron Vertices]
static const DirectX::XMFLOAT3 g_vertex_buffer_data[] = {
    {-1.0, 1.0, 0.0},    {3.0, 1.0, 0.0},   {-1.0, -3.0,  0.0} //Right angle triangle, scaled and translated to work in fullscreen.
};
//! [Pentahedron Vertices]

//This array is used for weaving two color gradients. Goes unused while using a texture instead.
//One color for each vertex. XYZ maps to RGB
//! [Pentahedron Colors]
static const  DirectX::XMFLOAT3 g_color_buffer_data[] = {
     {0.0,  0.0,  1.0},     {0.0,  0.0,  0.25},    { 0.0,  0.0,  0.75}, //Gradient Blue
     {1.0,  0.0,  0.0},     {0.75,  0.0,  0.0},    { 0.25,  0.0,  0.0} //Gradient Red
};

//! [Texture Coords]
static const  DirectX::XMFLOAT2 g_texture_buffer_data[] = {
    //Texture coordinates for projecting the image twice across the full length of the triangle. 
    //This is because the final triangle is double the width and height of the screen so that we only need one triangle to render a fullscreen image.
    {0.0,  0.0},     {2,  0.0},    { 0.0,  2}
};

static const int g_vertex_count = 3;

ImagePlane::ImagePlane(Renderer& renderer, unsigned char* data, int width, int height) : renderer(renderer) {

    const int imageDataSize = width * height * 4;
    imageData = new unsigned char[imageDataSize];
    memcpy(imageData, data, imageDataSize);
    imageWidth = width;
    imageHeight = height;

    restoreDeviceObjects();    
}

ImagePlane::~ImagePlane() {
    invalidateDeviceObjects();
}

/***
* Method used to draw a unique texture onto the plane or "triangle".
* Filename should be like "path/to/file/filename.jpg".
* Width and height should be the width and height of the sent image.
* **/
void ImagePlane::loadTextureOntoPlane(unsigned char* data, int width, int height) {

    bool success = false;

    // Set sampler state.
    renderer.GetDevice()->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    renderer.GetDevice()->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    renderer.GetDevice()->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
    renderer.GetDevice()->SetSamplerState(0, D3DSAMP_BORDERCOLOR, 0);
    renderer.GetDevice()->SetSamplerState(0, D3DSAMP_DMAPOFFSET, 0);
    renderer.GetDevice()->SetSamplerState(0, D3DSAMP_ELEMENTINDEX, 0);
    renderer.GetDevice()->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    renderer.GetDevice()->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 1);
    renderer.GetDevice()->SetSamplerState(0, D3DSAMP_MAXMIPLEVEL, 0);
    renderer.GetDevice()->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    renderer.GetDevice()->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    renderer.GetDevice()->SetSamplerState(0, D3DSAMP_MIPMAPLODBIAS, 0);
    renderer.GetDevice()->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0);

    // Create correction texture.
    HRESULT hr = renderer.GetDevice()->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &frameBufferTexture, NULL);
    if (SUCCEEDED(hr))
    {
        // Create staging texture used to update contents of correction texture.
        IDirect3DTexture9* stagingTexture = nullptr;
        hr = renderer.GetDevice()->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &stagingTexture, NULL);
        if (SUCCEEDED(hr))
        {
            // Lock staging texture.
            D3DLOCKED_RECT lockedRect = {};
            hr = stagingTexture->LockRect(0, &lockedRect, NULL, 0);
            if (SUCCEEDED(hr))
            {
                // Fill staging texture.
                char* pDstRow = (char*)lockedRect.pBits;
                const char* pSrcRow = (const char*)data;
                const int pixelSize = 4 * sizeof(unsigned char);
                const int srcRowSize = width * pixelSize;

                // Convert from RGBA -> BGRA
                for (int y = 0; y < height; y++)
                {
                    const char* pSrcPixel = pSrcRow;
                    char* pDstPixel = pDstRow;

                    for (int x = 0; x < width; x++)
                    {
                        pDstPixel[0] = pSrcPixel[2];
                        pDstPixel[1] = pSrcPixel[1];
                        pDstPixel[2] = pSrcPixel[0];
                        pDstPixel[3] = pSrcPixel[3];

                        pDstPixel += pixelSize;
                        pSrcPixel += pixelSize;
                    }

                    pSrcRow += srcRowSize;
                    pDstRow += lockedRect.Pitch;
                }

                // Unlock staging texture.
                stagingTexture->UnlockRect(0);

                // Update correction texture.
                hr = renderer.GetDevice()->UpdateTexture(stagingTexture, frameBufferTexture);
                if (SUCCEEDED(hr))
                {
                    success = true;
                }
                else
                {
                    //SR::Log::error("Failed to update correction texture.");
                    assert(false);
                }
            }
            else
            {
                //SR::Log::error("Failed to lock correction texture.");
                assert(false);
            }

            stagingTexture->Release();
        }
        else
        {
            //SR::Log::error("Failed to create staging texture.");
            assert(false);
        }
    }
    else
    {
        //SR::Log::error("Failed to create correction texture.");
        assert(false);
    }
}

void ImagePlane::draw() {
    UINT Stride = sizeof(VertexTexture);
    UINT Offset = 0;

    renderer.GetDevice()->SetTexture(0, frameBufferTexture);

    //Old code
    renderer.GetDevice()->SetStreamSource(0, Buffer, Offset, Stride);
    renderer.GetDevice()->DrawPrimitive(D3DPT_TRIANGLELIST, 0, g_vertex_count / 3);
}

void ImagePlane::invalidateDeviceObjects()
{
    if (Buffer != nullptr)
    {
        Buffer->Release();
        Buffer = nullptr;
    }

    if (frameBufferTexture != nullptr)
    {
        frameBufferTexture->Release();
        frameBufferTexture = nullptr;
    }
}

void ImagePlane::restoreDeviceObjects()
{
    VertexTexture Vertices[g_vertex_count];

    for (int i = 0; i < g_vertex_count; i++)
    {
        Vertices[i].Position = g_vertex_buffer_data[i];
        Vertices[i].Texture = g_texture_buffer_data[i];
    }

    HRESULT hr = renderer.GetDevice()->CreateVertexBuffer(sizeof(Vertices), D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &Buffer, NULL);
    if (SUCCEEDED(hr))
    {
        void* pbData = nullptr;
        hr = Buffer->Lock(0, sizeof(Vertices), &pbData, D3DLOCK_DISCARD);
        if (SUCCEEDED(hr))
        {
            memcpy(pbData, Vertices, sizeof(Vertices));
            Buffer->Unlock();
        }
    }

    loadTextureOntoPlane(imageData, imageWidth, imageHeight);
}