/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include "imageplane.h"
#include <DirectXMath.h>
#include <d3d12.h>
#include <DirectXPackedVector.h>
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

ImagePlane::ImagePlane(Renderer& renderer, unsigned char* data, int width, int height) : renderer(renderer) 
{
    VertexTexture Vertices[g_vertex_count];

    for (int i = 0; i < g_vertex_count; i++)
    {
        Vertices[i].Position = g_vertex_buffer_data[i];
        Vertices[i].Texture = g_texture_buffer_data[i];
    }

    const UINT vertexBufferSize = sizeof(Vertices);

    renderer.GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&VertexBuffer));

    if (VertexBuffer == nullptr)
    {
        return;
    }

    // Copy the triangle data to the vertex buffer.
    UINT8* pVertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
    VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    memcpy(pVertexDataBegin, Vertices, sizeof(Vertices));
    VertexBuffer->Unmap(0, nullptr);

    // Initialize the vertex buffer view.
    VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
    VertexBufferView.StrideInBytes = sizeof(VertexTexture);
    VertexBufferView.SizeInBytes = vertexBufferSize;

    loadTextureOntoPlane(data, width, height);
}

ImagePlane::~ImagePlane() 
{
    if (Texture != nullptr)
    {
        Texture->Release();
        Texture = nullptr;
    }

    if (VertexBuffer != nullptr)
    {
        VertexBuffer->Release();
        VertexBuffer = nullptr;
    }
}

/***
* Method used to draw a unique texture onto the plane or "triangle".
* Filename should be like "path/to/file/filename.jpg".
* Width and height should be the width and height of the sent image.
* **/
void ImagePlane::loadTextureOntoPlane(unsigned char* data, int width, int height) 
{
    ID3D12Resource* textureUploadHeap;
    // Create the texture.

    // Describe and create a Texture2D.
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    renderer.GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&Texture));

    if (Texture == nullptr)
    {
        return;
    }

    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(Texture, 0, 1);

    // Create the GPU upload buffer.
    renderer.GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&textureUploadHeap));

    if (textureUploadHeap == nullptr)
    {
        return;
    }

    // Copy data to the intermediate upload heap and then schedule a copy 
    // from the upload heap to the Texture2D.

    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = data;
    textureData.RowPitch = width * 4;
    textureData.SlicePitch = textureData.RowPitch * height;

    UpdateSubresources(renderer.GetCommandList(), Texture, textureUploadHeap, 0, 0, 1, &textureData);
    //Texture is now loaded into video memory, free memory.
    renderer.GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(Texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

    // Describe and create a SRV for the texture.
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    renderer.GetDevice()->CreateShaderResourceView(Texture, &srvDesc, renderer.GetSRVHeap()->GetCPUDescriptorHandleForHeapStart());

    // Close the command list and execute it to begin the initial GPU setup.
    renderer.GetCommandList()->Close();
    ID3D12CommandList* ppCommandLists[] = { renderer.GetCommandList() };
    renderer.GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void ImagePlane::draw() 
{
    renderer.GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    renderer.GetCommandList()->IASetVertexBuffers(0, 1, &VertexBufferView);
    renderer.GetCommandList()->DrawInstanced(g_vertex_count, 1, 0, 0);
}
