/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef SHADER_H
#define SHADER_H
#include <d3d10_1.h>
#include "renderer.h"

class TextureShader
{
public:
    TextureShader(Renderer&);
    ~TextureShader();

    void Bind();

private:
    Renderer& renderer;

    ID3D10VertexShader* VertexShader = nullptr;
    ID3D10PixelShader* PixelShader = nullptr;
    ID3D10InputLayout* InputLayout = nullptr;
};

#endif
