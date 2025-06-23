/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include "pyramid.h"
#include <DirectXMath.h>
#include "d3dx12.h"
#include "shader.h"

//! [Pentahedron Vertices]
static const DirectX::XMFLOAT3 g_vertex_buffer_data[] = {
    {+1.0, -1.0, -1.0},    {0.0, +1.0,  0.0},    {-1.0, -1.0, -1.0},
    {0.0, +1.0,  0.0},     {-1.0, -1.0, +1.0},   {-1.0, -1.0, -1.0},
    {+1.0, -1.0, +1.0},    {0.0, +1.0,  0.0},    {+1.0, -1.0, -1.0},
    {+1.0, -1.0, +1.0},    {+1.0, -1.0, -1.0},   {-1.0, -1.0, -1.0},
    {+1.0, -1.0, +1.0},    {-1.0, -1.0, -1.0},   {-1.0, -1.0, +1.0},
    {+1.0, -1.0, +1.0},    {-1.0, -1.0, +1.0},   {0.0, +1.0,  0.0}
};
//! [Pentahedron Vertices]

//! [Pentahedron Colors]
// One color for each vertex. XYZ maps to RGB
static const  DirectX::XMFLOAT3 g_color_buffer_data[] = {
     {0.0,  1.0,  0.0},     {0.0,  1.0,  0.0},    {0.0,  1.0,  0.0}, //back
     {1.0,  0.0,  0.0},     {1.0,  0.0,  0.0},    { 1.0,  0.0,  0.0}, //left
     {0.0,  1.0,  1.0},     {0.0,  1.0,  1.0},    { 0.0,  1.0,  1.0}, //right
     {0.0,  0.0,  1.0},     {0.0,  0.0,  1.0},    { 0.0,  0.0,  1.0}, //up
     {0.0,  0.0,  1.0},     {0.0,  0.0,  1.0},    { 0.0,  0.0,  1.0}, //up
     {1.0,  1.0,  0.0},     {1.0,  1.0,  0.0},    { 1.0,  1.0,  0.0} //front
};
//! [Pentahedron Colors]

static const int g_vertex_count = 18;

Pyramid::Pyramid(Renderer& renderer) : renderer(renderer)
{
    ColorShader::Vertex Vertices[g_vertex_count];

    for (int i = 0; i < g_vertex_count; i++)
    {
        Vertices[i].Position = g_vertex_buffer_data[i];
        Vertices[i].Color = g_color_buffer_data[i];
    }

    const UINT VertexBufferSize = sizeof(Vertices);

    D3D12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
    renderer.GetDevice()->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&VertexBuffer));

    UINT8* GPUAddress = nullptr;
    CD3DX12_RANGE ReadRange(0, 0);
    VertexBuffer->Map(0, &ReadRange, reinterpret_cast<void**>(&GPUAddress));
    memcpy(GPUAddress, Vertices, sizeof(Vertices));
    VertexBuffer->Unmap(0, nullptr);

    VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
    VertexBufferView.StrideInBytes = sizeof(ColorShader::Vertex);
    VertexBufferView.SizeInBytes = VertexBufferSize;
}

Pyramid::~Pyramid()
{
    if (VertexBuffer != nullptr)
    {
        VertexBuffer->Release();
        VertexBuffer = nullptr;
    }
}

void Pyramid::draw(ID3D12GraphicsCommandList* CommandList)
{
    CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
    CommandList->DrawInstanced(18, 1, 0, 0);
}
