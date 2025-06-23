/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include "shader.h"
#include <string>
#include "shader_internal.h"

TextureShader::TextureShader(Renderer& renderer) : renderer(renderer)
{
    HRESULT hr = renderer.GetDevice()->CreatePixelShader(ShaderBlobPS, sizeof(ShaderBlobPS), &PixelShader);
    if (FAILED(hr))
    {
        throw std::exception("Failed to create pixel shader.");
    }

    hr = renderer.GetDevice()->CreateVertexShader(ShaderBlobVS, sizeof(ShaderBlobVS), &VertexShader);
    if (FAILED(hr))
    {
        throw std::exception("Failed to create vertex shader.");
    }

    D3D10_INPUT_ELEMENT_DESC LayoutDesc[2] = {};
    LayoutDesc[0].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
    LayoutDesc[0].SemanticName = "SV_Position";
    LayoutDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    LayoutDesc[0].AlignedByteOffset = 0;
    LayoutDesc[1].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
    LayoutDesc[1].SemanticName = "TEXCOORD";
    LayoutDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    LayoutDesc[1].AlignedByteOffset = D3D10_APPEND_ALIGNED_ELEMENT;

    hr = renderer.GetDevice()->CreateInputLayout(LayoutDesc, 2, ShaderBlobVS, sizeof(ShaderBlobVS), &InputLayout);
    if (FAILED(hr))
    {
        throw std::exception("Failed to create input layout.");
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
    renderer.GetDevice()->VSSetShader(VertexShader);
    renderer.GetDevice()->GSSetShader(nullptr);
    renderer.GetDevice()->PSSetShader(PixelShader);
    renderer.GetDevice()->IASetInputLayout(InputLayout);
}
