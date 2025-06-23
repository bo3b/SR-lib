/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef RENDERER_H
#define RENDERER_H
#include <d3d12.h>
#include <DirectXMath.h>
#include <dxgi1_4.h>
#include <string>
#include "window.h"

const UINT BackbufferCount = 2;

class Renderer
{
public:
    Renderer(Window&, DirectX::XMINT2 FramebufferSize);
    ~Renderer();

    D3D12_CPU_DESCRIPTOR_HANDLE BindFramebufferAsTarget();
    void BindFramebufferAsUAV();

    D3D12_CPU_DESCRIPTOR_HANDLE BindBackbufferAsTarget();
    void BindBackbufferForPresent();

    ID3D12Device2* GetDevice();
    IDXGISwapChain3* GetSwapChain();
    ID3D12CommandAllocator* GetCommandAllocator();
    ID3D12CommandQueue* GetCommandQueue();
    ID3D12GraphicsCommandList* GetCommandList();
    void ExecuteCommandList();

    ID3D12DescriptorHeap* GetRtvHeap();
    UINT GetRtvHeapSize();

    ID3D12Resource* GetBackbuffer(UINT Index);
    UINT GetBackbufferOffset(UINT Index);
    ID3D12Resource* GetFramebuffer();
    UINT GetFramebufferOffset();
    DirectX::XMINT2 GetFramebufferSize();

    void Resize(DirectX::XMINT2 Size);

    DirectX::XMINT2 GetSize();

    void WaitForFence();

    std::string GetOutputDeviceName(HWND window);

private:
    void CreateBackbuffers();

    IDXGISwapChain3* SwapChain = nullptr;
    ID3D12Device2* RenderDevice = nullptr;
    ID3D12CommandQueue* CommandQueue = nullptr;
    ID3D12CommandAllocator* CommandAllocator = nullptr;
    ID3D12Resource* Backbuffers[BackbufferCount];
    ID3D12Resource* Framebuffer = nullptr;
    DirectX::XMINT2 FramebufferSize;

    ID3D12GraphicsCommandList* CommandList = nullptr;

    ID3D12DescriptorHeap* RTVHeap = nullptr;
    UINT RTVHeapSize = 0;

    ID3D12DescriptorHeap* SRVHeap = nullptr;
    UINT SRVHeapSize = 0;

    DirectX::XMINT2 SwapchainSize;

    ID3D12Fence* Fence = nullptr;
    UINT16 FenceValue = 0;
    HANDLE FenceEvent = nullptr;
};

#endif
