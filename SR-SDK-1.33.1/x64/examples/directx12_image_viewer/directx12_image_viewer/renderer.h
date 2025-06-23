/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef RENDERER_H
#define RENDERER_H
#include <d3d12.h>
#include <d3dx12.h>
#include <DirectXMath.h>
#include <dxgi1_4.h>
#include <string>
#include "window.h"

const UINT BackbufferCount = 2;

class Renderer
{
public:

    Renderer(Window&, UINT width, UINT height, char* fileName);
    ~Renderer();

    //getters
    ID3D12Device2* GetDevice() { return RenderDevice; }
    ID3D12CommandAllocator* GetCommandAllocator() { return CommandAllocator; }
    ID3D12GraphicsCommandList* GetCommandList() { return CommandList; }
    ID3D12CommandQueue* GetCommandQueue() { return CommandQueue; }
    ID3D12Resource* GetBackBuffer(int index) { return Backbuffers[index]; }
    ID3D12Resource* GetFirstBackBuffer() { return GetBackBuffer(0); }
    ID3D12DescriptorHeap* GetSRVHeap() { return SRVHeap; }
    IDXGISwapChain3* GetSwapChain() { return SwapChain; }

    
    void WaitForFence();
    std::string GetOutputDeviceName(HWND window);
    void Resize(DirectX::XMINT2 Size);
    DirectX::XMINT2 GetSize();
    D3D12_CPU_DESCRIPTOR_HANDLE BindBackbufferAsTarget();
    void BindBackbufferForPresent();
    void ExecuteCommandList();

private:
    static const UINT BackbufferCount = 2;

    //private functions
    bool InitPipeline(HWND hWnd, DirectX::XMINT2 windowSize);
    bool InitAssets(char* fileName);
    //InitPipeline functions
    bool CreateCommandQueue();
    bool CreateSwapChain(HWND hWnd, IDXGIFactory4* DXGIFactory, DirectX::XMINT2 windowSize);
    bool CreateRTVHeap();
    bool CreateSRVHeap();
    bool CreateCommandAllocator();
    bool CreateBackbuffers();
    //InitAssets functions
    bool CreateCommandList();
    bool CreateVertexBuffer();
    

    // Pipeline objects.
    IDXGISwapChain3* SwapChain;
    ID3D12Device2* RenderDevice;
    ID3D12Resource* Backbuffers[BackbufferCount];
    ID3D12CommandAllocator* CommandAllocator;
    ID3D12CommandQueue* CommandQueue;
    ID3D12DescriptorHeap* RTVHeap;
    ID3D12DescriptorHeap* SRVHeap;
    ID3D12GraphicsCommandList* CommandList;
    UINT RTVHeapSize;
    DirectX::XMINT2 SwapchainSize;

    // Synchronization objects.
    UINT FrameIndex;
    HANDLE FenceEvent;
    ID3D12Fence* Fence;
    UINT64 FenceValue;

    // Viewport dimensions.
    UINT Width;
    UINT Height;
};

#endif
