/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include "pyramid.h"
#include <DirectXMath.h>

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
static const DirectX::XMFLOAT3 g_color_buffer_data[] = {
     {0.0,  1.0,  0.0},     {0.0,  1.0,  0.0},    {0.0,  1.0,  0.0}, //back
     {1.0,  0.0,  0.0},     {1.0,  0.0,  0.0},    { 1.0,  0.0,  0.0}, //left
     {0.0,  1.0,  1.0},     {0.0,  1.0,  1.0},    { 0.0,  1.0,  1.0}, //right
     {0.0,  0.0,  1.0},     {0.0,  0.0,  1.0},    { 0.0,  0.0,  1.0}, //up
     {0.0,  0.0,  1.0},     {0.0,  0.0,  1.0},    { 0.0,  0.0,  1.0}, //up
     {1.0,  1.0,  0.0},     {1.0,  1.0,  0.0},    { 1.0,  1.0,  0.0} //front
};
//! [Pentahedron Colors]

static const int g_vertex_count = 18;

Pyramid::Pyramid(Renderer& renderer) : renderer(renderer) {

    restoreDeviceObjects();
}

Pyramid::~Pyramid() {
    if(Buffer != nullptr)
    {
        Buffer->Release();
        Buffer = nullptr;
    }
}

void Pyramid::draw() {
    UINT Stride = sizeof(Vertex);
    UINT Offset = 0;

    renderer.GetDevice()->SetStreamSource(0, Buffer, Offset, Stride);
    renderer.GetDevice()->DrawPrimitive(D3DPT_TRIANGLELIST, 0, g_vertex_count / 3);
}

void Pyramid::invalidateDeviceObjects()
{
    if (Buffer != nullptr)
    {
        Buffer->Release();
        Buffer = nullptr;
    }
}

void Pyramid::restoreDeviceObjects()
{
    Vertex Vertices[g_vertex_count];

    for (int i = 0; i < g_vertex_count; i++)
    {
        Vertices[i].Position = g_vertex_buffer_data[i];
        Vertices[i].Color = g_color_buffer_data[i];
    }

    const DWORD bufferSize = sizeof(Vertex) * g_vertex_count;

    HRESULT hr = renderer.GetDevice()->CreateVertexBuffer(bufferSize, 0, 0, D3DPOOL_DEFAULT, &Buffer, NULL);
    if (SUCCEEDED(hr))
    {
        void* pbData = nullptr;
        hr = Buffer->Lock(0, bufferSize, &pbData, 0);
        if (SUCCEEDED(hr))
        {
            memcpy(pbData, Vertices, bufferSize);
            Buffer->Unlock();
        }
    }
}