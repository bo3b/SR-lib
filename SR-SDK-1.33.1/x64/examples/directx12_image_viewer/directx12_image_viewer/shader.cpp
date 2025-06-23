/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include "shader.h"
#include <string>
#include "shader_internal.h"

TextureShader::TextureShader(Renderer& renderer) :
    renderer(renderer),
    RootSignature(nullptr),
    PipelineState(nullptr)
{
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

    // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

    CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    CD3DX12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 0;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ID3DBlob* signature;
    ID3DBlob* error;
    D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error);
    if (signature == nullptr)
    {
        return;
    }
        
    renderer.GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature));
    signature->Release();
    if (RootSignature == nullptr)
    {
        return;
    }

    // Create the pipeline state
    D3D12_SHADER_BYTECODE VS_Data = { ShaderBlobVS, sizeof(ShaderBlobVS) };
    D3D12_SHADER_BYTECODE PS_Data = { ShaderBlobPS, sizeof(ShaderBlobPS) };

    D3D12_INPUT_ELEMENT_DESC InputLayout[] =
    {
        { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { InputLayout, _countof(InputLayout) };
    psoDesc.pRootSignature = RootSignature;
    psoDesc.VS = VS_Data;
    psoDesc.PS = PS_Data;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    renderer.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PipelineState));

    if (PipelineState == nullptr)
    {
        return;
    }
}

TextureShader::~TextureShader()
{
    if (RootSignature != nullptr)
    {
        RootSignature->Release();
        RootSignature = nullptr;
    }

    if (PipelineState != nullptr)
    {
        PipelineState->Release();
        PipelineState = nullptr;
    }
}

void TextureShader::Bind(ID3D12GraphicsCommandList* CommandList)
{
    CommandList->SetPipelineState(PipelineState);
    CommandList->SetGraphicsRootSignature(RootSignature);

    ID3D12DescriptorHeap* ppHeaps[] = { renderer.GetSRVHeap() };
    CommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    CommandList->SetGraphicsRootDescriptorTable(0, renderer.GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
}
