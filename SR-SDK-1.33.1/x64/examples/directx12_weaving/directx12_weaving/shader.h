/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef SHADER_H
#define SHADER_H
#include <d3d12.h>
#include "renderer.h"

class ColorShader
{
public:
    struct Vertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Color;
    };

    ColorShader(Renderer&);
    ~ColorShader();

    void Bind(ID3D12GraphicsCommandList*, UINT ViewSide);

    struct TransformationStruct
    {
        DirectX::XMMATRIX Transformation;
    };
    TransformationStruct TransformationData[2];

private:
    void UploadConstantBuffer(UINT ViewSide);

    Renderer& renderer;
    ID3D12RootSignature* RootSignature;
    ID3D12PipelineState* PipelineState;

    ID3D12DescriptorHeap* ConstantDescHeap;
    ID3D12Resource* ConstantUploadHeap;
    UINT ConstantHeapSize;
    UINT8* ConstantGPUBuffer;
};

#endif
