/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef SHADER_H
#define SHADER_H
#include <d3d11.h>
#include "renderer.h"

class TextureShader
{
public:
    TextureShader(Renderer&);
    ~TextureShader();

    void Bind(ID3D12GraphicsCommandList*);

private:
    Renderer& renderer;
    ID3D12RootSignature* RootSignature;
    ID3D12PipelineState* PipelineState;
};

#endif
