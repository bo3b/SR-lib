/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include "imageplane.h"
#include <DirectXMath.h>
#include <d3d11.h>
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
    VertexTexture Vertices[g_vertex_count];

    for (int i = 0; i < g_vertex_count; i++)
    {
        Vertices[i].Position = g_vertex_buffer_data[i];
        Vertices[i].Texture = g_texture_buffer_data[i];
    }

    D3D11_BUFFER_DESC Desc = { };
    Desc.ByteWidth = sizeof(VertexTexture) * g_vertex_count;
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA Resource = { };
    Resource.pSysMem = &Vertices;

    renderer.GetDevice()->CreateBuffer(&Desc, &Resource, &Buffer);

    loadTextureOntoPlane(data, width, height);
}

ImagePlane::~ImagePlane() {
    if(Buffer != nullptr)
    {
        Buffer->Release();
        Buffer = nullptr;
    }
}

/***
* Method used to draw a unique texture onto the plane or "triangle".
* Filename should be like "path/to/file/filename.jpg".
* Width and height should be the width and height of the sent image.
* **/
void ImagePlane::loadTextureOntoPlane(unsigned char* data, int width, int height) {
    //Create a texture sampler state description.
    D3D11_SAMPLER_DESC samplerDesc = { };
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.BorderColor[0] = 0;
    samplerDesc.BorderColor[1] = 0;
    samplerDesc.BorderColor[2] = 0;
    samplerDesc.BorderColor[3] = 0;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    //Create the texture sampler state.
    ID3D11SamplerState* pSampler = NULL;
    renderer.GetDevice()->CreateSamplerState(&samplerDesc, &pSampler);
    renderer.GetContext()->PSSetSamplers(NULL, 1, &pSampler);

    D3D11_SUBRESOURCE_DATA initData = { 0 };
    initData.pSysMem = (const void*)data;
    initData.SysMemPitch = width * 4;
    initData.SysMemSlicePitch = height * width * 4;

    D3D11_TEXTURE2D_DESC desc;
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;

    //Trying to load texture into pTexture, if this doesn't work we abort the code since pTexture would be NULL.
    ID3D11Texture2D* pTexture = NULL;
    HRESULT hr = renderer.GetDevice()->CreateTexture2D(&desc, &initData, &pTexture);

    if (FAILED(hr)) {
        std::cout << "Unable to load 2D texture. Check if the supplied images are valid and loaded correctly.";
        //Texture was unable to load. Abort.
        return;
    }

    frameBufferView = nullptr;

    renderer.GetDevice()->CreateShaderResourceView(pTexture, nullptr, &frameBufferView);
}

void ImagePlane::draw() {
    UINT Stride = sizeof(VertexTexture);
    UINT Offset = 0;

    renderer.GetContext()->PSSetShaderResources(0, 1, &frameBufferView);

    //Old code
    renderer.GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    renderer.GetContext()->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);
    renderer.GetContext()->Draw(g_vertex_count, 0);
}
