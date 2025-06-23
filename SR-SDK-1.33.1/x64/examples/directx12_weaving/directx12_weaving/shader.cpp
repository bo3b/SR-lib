/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include "shader.h"
#include "d3dx12.h"
#include "shader_internal.h"

ColorShader::ColorShader(Renderer& renderer) : renderer(renderer)
{
    // Assign constant buffers to a root parameter (B0)
    D3D12_ROOT_DESCRIPTOR RootDescriptor;
    RootDescriptor.RegisterSpace = 0;
    RootDescriptor.ShaderRegister = 0;

    D3D12_ROOT_PARAMETER RootParameters[1];
    RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    RootParameters[0].Descriptor = RootDescriptor;
    RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
    RootSignatureDesc.Init(_countof(RootParameters), nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    RootSignatureDesc.pParameters = &RootParameters[0];

    ID3DBlob* Signature;
    ID3DBlob* Error;
    D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &Signature, &Error);
    renderer.GetDevice()->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature));

    // Create the pipeline state
    D3D12_SHADER_BYTECODE VS_Data = { ShaderBlobVS, sizeof(ShaderBlobVS) };
    D3D12_SHADER_BYTECODE PS_Data = { ShaderBlobPS, sizeof(ShaderBlobPS) };

    D3D12_INPUT_ELEMENT_DESC InputLayout[] =
    {
        { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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

    // Create upload buffer for constant data
    D3D12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(1024 * 64);
    renderer.GetDevice()->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&ConstantUploadHeap));
    ConstantUploadHeap->SetName(L"TransformationMatrix");

    // Create descriptor heap for constant data
    D3D12_DESCRIPTOR_HEAP_DESC ConstantDescHeapDesc = {};
    ConstantDescHeapDesc.NumDescriptors = 1;
    ConstantDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ConstantDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    renderer.GetDevice()->CreateDescriptorHeap(&ConstantDescHeapDesc, IID_PPV_ARGS(&ConstantDescHeap));

    ConstantHeapSize = (sizeof(TransformationStruct) + 255) & ~255;

    // Create buffer view on descriptor heap
    D3D12_CONSTANT_BUFFER_VIEW_DESC Desc = {};
    Desc.BufferLocation = ConstantUploadHeap->GetGPUVirtualAddress();
    Desc.SizeInBytes = ConstantHeapSize;
    renderer.GetDevice()->CreateConstantBufferView(&Desc, ConstantDescHeap->GetCPUDescriptorHandleForHeapStart());

    // Map CPU buffer to GPU data
    CD3DX12_RANGE ReadRange(0, 0);
    ConstantUploadHeap->Map(0, &ReadRange, reinterpret_cast<void**>(&ConstantGPUBuffer));
}

ColorShader::~ColorShader()
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

    if (ConstantUploadHeap != nullptr)
    {
        ConstantUploadHeap->Release();
        ConstantUploadHeap = nullptr;
    }

    if (ConstantDescHeap != nullptr)
    {
        ConstantDescHeap->Release();
        ConstantDescHeap = nullptr;
    }
}

void ColorShader::Bind(ID3D12GraphicsCommandList* CommandList, UINT ViewSide)
{
    CommandList->SetPipelineState(PipelineState);
    CommandList->SetGraphicsRootSignature(RootSignature);

    CommandList->SetGraphicsRootConstantBufferView(0, ConstantUploadHeap->GetGPUVirtualAddress() + ViewSide * ConstantHeapSize);

    UploadConstantBuffer(ViewSide);
}

void ColorShader::UploadConstantBuffer(UINT ViewSide)
{
    memcpy(ConstantGPUBuffer + ViewSide * ConstantHeapSize, &TransformationData[ViewSide], sizeof(TransformationData));
}
