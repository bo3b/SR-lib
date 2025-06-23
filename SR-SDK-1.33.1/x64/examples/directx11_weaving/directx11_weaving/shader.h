/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef SHADER_H
#define SHADER_H
#include <d3d11.h>
#include "renderer.h"

class ColorShader
{
public:
    ColorShader(Renderer&);
    ~ColorShader();

    void Bind();

private:
    Renderer& renderer;

    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;
    ID3D11InputLayout* InputLayout = nullptr;
};

#endif
