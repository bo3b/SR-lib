/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef SHADER_H
#define SHADER_H
#include <d3d9.h>
#include "renderer.h"

class TextureShader
{
public:
    TextureShader(Renderer&);
    ~TextureShader();

    void Bind();

private:
    Renderer& renderer;

    IDirect3DVertexShader9* VertexShader = nullptr;
    IDirect3DPixelShader9* PixelShader = nullptr;
    IDirect3DVertexDeclaration9* InputLayout = nullptr;
};

#endif
