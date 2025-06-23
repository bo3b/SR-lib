/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include "shader.h"
#include <string>
#include "shader_internal.h"

ColorShader::ColorShader(Renderer& renderer) : renderer(renderer)
{
    renderer.GetDevice()->CreatePixelShader(ShaderBlobPS, sizeof(ShaderBlobPS), nullptr, &PixelShader);
    renderer.GetDevice()->CreateVertexShader(ShaderBlobVS, sizeof(ShaderBlobVS), nullptr, &VertexShader);

    D3D11_INPUT_ELEMENT_DESC LayoutDesc[2];
    ZeroMemory(&LayoutDesc, sizeof(LayoutDesc));
    for(int i = 0; i < 2; i++)
    {
        LayoutDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    }
    LayoutDesc[0].SemanticName = "SV_Position";
    LayoutDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    LayoutDesc[0].AlignedByteOffset = 0;
    LayoutDesc[1].SemanticName = "COLOR";
    LayoutDesc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    LayoutDesc[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;

    renderer.GetDevice()->CreateInputLayout(LayoutDesc, 2, ShaderBlobVS, sizeof(ShaderBlobVS), &InputLayout);
}

ColorShader::~ColorShader()
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

void ColorShader::Bind()
{
    renderer.GetContext()->VSSetShader(VertexShader, nullptr, 0);
    renderer.GetContext()->GSSetShader(nullptr, nullptr, 0);
    renderer.GetContext()->PSSetShader(PixelShader, nullptr, 0);
    renderer.GetContext()->IASetInputLayout(InputLayout);
}
