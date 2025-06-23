/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef SRWEAVERCONTEXT_H
#define SRWEAVERCONTEXT_H
#include <d3d12.h>
#include <DirectXMath.h>
#include <sr/weaver/dx12weaver.h>
#include "sr/sense/display/switchablehint.h"
#include "sr/world/display/screen.h"

class SRContainer
{
private:
    SR::SRContext* context;
    SR::PredictingDX12Weaver* weaver;
public:
    SRContainer();
    ~SRContainer();

    SR::SRContext* getContext() { return context; }
    bool isContextCreated() { return (getContext() != nullptr); }
    void createContext(ID3D12Device2* RenderDevice,
        ID3D12CommandAllocator* CommandAllocator,
        ID3D12GraphicsCommandList* CommandList,
        ID3D12CommandQueue* CommandQueue,
        ID3D12Resource* WeaverInputBuffer,
        ID3D12Resource* WeaverOutputBuffer,
        HWND hWnd);
    void destroyContext();

    SR::PredictingDX12Weaver* getWeaver() { return weaver; }
};

#endif
