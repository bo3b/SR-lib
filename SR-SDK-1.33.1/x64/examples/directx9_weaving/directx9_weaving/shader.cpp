/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include "shader.h"
#include <string>
#include "shader_internal.h"

ColorShader::ColorShader(Renderer& renderer) : renderer(renderer)
{
    restoreDeviceObjects();
}

ColorShader::~ColorShader()
{
    invalidateDeviceObjects();
}

void ColorShader::Bind()
{
    renderer.GetDevice()->SetVertexShader(VertexShader);
    renderer.GetDevice()->SetPixelShader(PixelShader);
    renderer.GetDevice()->SetVertexDeclaration(InputLayout);
}

void ColorShader::invalidateDeviceObjects()
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

void ColorShader::restoreDeviceObjects()
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
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0},
        D3DDECL_END()
    };

    hr = renderer.GetDevice()->CreateVertexDeclaration(dwDecl, &InputLayout);
    if (FAILED(hr))
    {
        throw std::exception("Failed to create vertex declaration.");
    }
}
