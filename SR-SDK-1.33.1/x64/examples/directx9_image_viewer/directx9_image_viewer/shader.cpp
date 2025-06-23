/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include "shader.h"
#include <string>
#include "shader_internal.h"

TextureShader::TextureShader(Renderer& renderer) : renderer(renderer)
{
    HRESULT hr = renderer.GetDevice()->CreatePixelShader((const DWORD*)ShaderBlobPS, &PixelShader);
    if (FAILED(hr))
    {
        throw std::exception("Failed to create pixel shader.");
    }

    hr = renderer.GetDevice()->CreateVertexShader((const DWORD*)ShaderBlobVS, &VertexShader);
    if (FAILED(hr))
    {
        throw std::exception("Failed to create vertex shader.");
    }

    const D3DVERTEXELEMENT9 dwDecl[] =
    {
        // Stream, Offset, Type, Method, Usage, UsageIndex
        {0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };

    hr = renderer.GetDevice()->CreateVertexDeclaration(dwDecl, &InputLayout);
    if (FAILED(hr))
    {
        throw std::exception("Failed to create vertex declaration.");
    }
}

TextureShader::~TextureShader()
{
    if (VertexShader != nullptr)
    {
        VertexShader->Release();
        VertexShader = nullptr;
    }

    if (PixelShader != nullptr)
    {
        PixelShader->Release();
        PixelShader = nullptr;
    }

    if (InputLayout != nullptr)
    {
        InputLayout->Release();
        InputLayout = nullptr;
    }
}

void TextureShader::Bind()
{
    renderer.GetDevice()->SetVertexShader(VertexShader);
    renderer.GetDevice()->SetPixelShader(PixelShader);
    renderer.GetDevice()->SetVertexDeclaration(InputLayout);
}
