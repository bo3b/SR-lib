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

Pyramid::Pyramid(Renderer& renderer) : renderer(renderer) {

    Vertex Vertices[g_vertex_count];

    for(int i=0; i < g_vertex_count; i++)
    {
        Vertices[i].Position = g_vertex_buffer_data[i];
        Vertices[i].Color = g_color_buffer_data[i];
    }

    D3D11_BUFFER_DESC Desc = { };
    Desc.ByteWidth = sizeof(Vertex) * g_vertex_count;
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA Resource = { };
    Resource.pSysMem = &Vertices;

    renderer.GetDevice()->CreateBuffer(&Desc, &Resource, &Buffer);
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

    renderer.GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    renderer.GetContext()->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);
    renderer.GetContext()->Draw(g_vertex_count, 0);
}
