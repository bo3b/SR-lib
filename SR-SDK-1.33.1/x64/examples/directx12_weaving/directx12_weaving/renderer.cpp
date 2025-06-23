/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include <renderer.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <stdexcept>

using namespace Microsoft::WRL;

void GetHardwareAdapter(ComPtr<IDXGIFactory4> Factory, IDXGIAdapter1** Adapter)
{
    std::vector<IDXGIAdapter1*> AvailableAdapters;

    // Find available GPUs
    IDXGIAdapter* CurrentAdapter;
    SIZE_T highestDedicatedGpuMemory = 0;
    for (UINT AdapterIndex = 0; Factory->EnumAdapters(AdapterIndex, &CurrentAdapter) != DXGI_ERROR_NOT_FOUND; AdapterIndex++)
    {
        DXGI_ADAPTER_DESC AdapterDesc;
        CurrentAdapter->GetDesc(&AdapterDesc);

        bool IsAMD = AdapterDesc.VendorId == 0x1002;
        bool IsIntel = AdapterDesc.VendorId == 0x8086;
        bool IsNVIDIA = AdapterDesc.VendorId == 0x10DE;
        bool IsMicrosoft = AdapterDesc.VendorId == 0x1414;

        const bool SkipAdapter = IsMicrosoft;

        // Decide whether this adapter should be considered for usage
        if (!SkipAdapter)
        {
            //Note the highest amount of dedicated GPU memory:
            if (AdapterDesc.DedicatedVideoMemory > highestDedicatedGpuMemory)
            {
                highestDedicatedGpuMemory = AdapterDesc.DedicatedVideoMemory;

                //Put adapter at the start of the list.
                AvailableAdapters.insert(AvailableAdapters.begin(), (IDXGIAdapter1*)CurrentAdapter);
            }
            else
            {
                //Put the adapter at the end of the list.
                AvailableAdapters.push_back((IDXGIAdapter1*)CurrentAdapter);
            }
        }
        else
        {
            CurrentAdapter->Release();
        }
    }

    // Select an adapter capable of running DirectX12, always take the first compatible adapter in the list since it will have the most dedicated memory.
    for (IDXGIAdapter1* CurrentAdapter : AvailableAdapters)
    {
        if (SUCCEEDED(D3D12CreateDevice(CurrentAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)) && *Adapter == nullptr)
        {
            *Adapter = CurrentAdapter;
            break;
        }
        else
        {
            CurrentAdapter->Release();
        }
    }
}

void GetHardwareAdapterByPreference(ComPtr<IDXGIFactory6> Factory, IDXGIAdapter1** Adapter) {
    IDXGIAdapter1* tempAdapter = nullptr;

    for (UINT adapterIndex = 0;
        DXGI_ERROR_NOT_FOUND != Factory->EnumAdapterByGpuPreference(
            adapterIndex,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(Adapter));
        adapterIndex++)
    {
        DXGI_ADAPTER_DESC1 desc;
        tempAdapter = *Adapter;
        tempAdapter->GetDesc1(&desc);

        // Don't select the Basic Render Driver adapter.
        if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
        {
            break;
        }
    }
}

//Throws: Runtime error when no suitable graphics adapter can be found.
Renderer::Renderer(Window& window, DirectX::XMINT2 FramebufferSize) : FramebufferSize(FramebufferSize)
{
    // Enable DX12 debug layer in debug mode
#ifndef NDEBUG
    ID3D12Debug* DebugInterface;
    if (D3D12GetDebugInterface(__uuidof(ID3D12Debug), reinterpret_cast<void**>(&DebugInterface)) == S_OK)
    {
        DebugInterface->EnableDebugLayer();
        DebugInterface->Release();
    }
#endif

    IDXGIFactory4* DXGIFactory = nullptr;
    IDXGIFactory6* DXGIFactory6 = nullptr;
    DWORD DXGIFactoryFlags = 0;
#ifndef NDEBUG
    DXGIFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    CreateDXGIFactory2(DXGIFactoryFlags, IID_PPV_ARGS(&DXGIFactory));

    IDXGIAdapter1* DXGIAdapter = nullptr;
    HRESULT hr = DXGIFactory->QueryInterface(&DXGIFactory6);
    if (SUCCEEDED(hr))
    {
        GetHardwareAdapterByPreference(DXGIFactory6, &DXGIAdapter);
        DXGIFactory6->Release();
    }
    else
    {
        GetHardwareAdapter(DXGIFactory, &DXGIAdapter);
    }

    //Check if the hardware adapter was found
    if (DXGIAdapter == NULL) {
        //Release ownership of factory
        if (DXGIFactory != nullptr) {
            DXGIFactory->Release();
        }
        //Throw an exception to the top level.
        throw std::runtime_error("No compatble DirectX 12 graphics adapter found.");
    }
    D3D12CreateDevice(DXGIAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&RenderDevice));

#ifndef NDEBUG
    ID3D12InfoQueue* InfoQueue = nullptr;
    RenderDevice->QueryInterface(IID_PPV_ARGS(&InfoQueue));

    InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);

    D3D12_MESSAGE_SEVERITY Severities[] =
    {
        D3D12_MESSAGE_SEVERITY_INFO
    };

    D3D12_MESSAGE_ID DenyIds[] = 
    {
        D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE
    };

    D3D12_INFO_QUEUE_FILTER NewFilter = {};
    NewFilter.DenyList.NumSeverities = _countof(Severities);
    NewFilter.DenyList.pSeverityList = Severities;
    NewFilter.DenyList.NumIDs = _countof(DenyIds);
    NewFilter.DenyList.pIDList = DenyIds;

    InfoQueue->PushStorageFilter(&NewFilter);
    InfoQueue->Release();
#endif

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
    QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    RenderDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&CommandQueue));

    if (CommandQueue == nullptr)
    {
        return;
    }

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
    SwapChainDesc.BufferCount = BackbufferCount;
    SwapChainDesc.BufferDesc.Width = window.GetSize().x;
    SwapChainDesc.BufferDesc.Height = window.GetSize().y;
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    SwapChainDesc.OutputWindow = window.GetHandle();
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.Windowed = true;
    SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    DXGIFactory->CreateSwapChain(CommandQueue, &SwapChainDesc, (IDXGISwapChain**)&SwapChain);

    if(SwapChain == nullptr)
    {
        return;
    }

    DXGIFactory->MakeWindowAssociation(window.GetHandle(), DXGI_MWA_NO_ALT_ENTER);
    DXGIFactory->Release();

    D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc = {};
    RTVHeapDesc.NumDescriptors = BackbufferCount + 1;
    RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    RenderDevice->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(&RTVHeap));
    RTVHeapSize = RenderDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    if (RTVHeap == nullptr)
    {
        return;
    }

    D3D12_DESCRIPTOR_HEAP_DESC SRVHeapDesc = {};
    SRVHeapDesc.NumDescriptors = 1;
    SRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    SRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    RenderDevice->CreateDescriptorHeap(&SRVHeapDesc, IID_PPV_ARGS(&SRVHeap));
    SRVHeapSize = RenderDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    if (SRVHeap == nullptr)
    {
        return;
    }

    RenderDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator));

    if (CommandAllocator == nullptr)
    {
        return;
    }

    CreateBackbuffers();

    SwapchainSize = window.GetSize();

    D3D12_CLEAR_VALUE ClearMask = {};
    ClearMask.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    D3D12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(ClearMask.Format, FramebufferSize.x, FramebufferSize.y, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    RenderDevice->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &ClearMask, IID_PPV_ARGS(&Framebuffer));

    // Create framebuffer RTV at slot 0
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(RTVHeap->GetCPUDescriptorHandleForHeapStart());
    RenderDevice->CreateRenderTargetView(Framebuffer, nullptr, rtvHandle);

    // Create framebuffer SRV at slot 0
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(SRVHeap->GetCPUDescriptorHandleForHeapStart());
    RenderDevice->CreateShaderResourceView(Framebuffer, nullptr, srvHandle);

    // Create command list
    RenderDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator, nullptr, IID_PPV_ARGS(&CommandList));
    CommandList->Close();

    // Create fence
    RenderDevice->CreateFence(FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
    FenceValue = 0;

    // Create an event handle to use for frame synchronization.
    FenceEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::BindFramebufferAsTarget()
{
    CD3DX12_RESOURCE_BARRIER BufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(GetFramebuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);
    GetCommandList()->ResourceBarrier(1, &BufferBarrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE BufferHandle(GetRtvHeap()->GetCPUDescriptorHandleForHeapStart(), GetFramebufferOffset(), GetRtvHeapSize());
    GetCommandList()->OMSetRenderTargets(1, &BufferHandle, FALSE, nullptr);

    return BufferHandle;
}

void Renderer::BindFramebufferAsUAV()
{
    CD3DX12_RESOURCE_BARRIER BufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(GetFramebuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GetCommandList()->ResourceBarrier(1, &BufferBarrier);
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::BindBackbufferAsTarget()
{
    UINT FrameIndex = SwapChain->GetCurrentBackBufferIndex();

    CD3DX12_RESOURCE_BARRIER BufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(GetBackbuffer(FrameIndex), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    GetCommandList()->ResourceBarrier(1, &BufferBarrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE BufferHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(GetRtvHeap()->GetCPUDescriptorHandleForHeapStart(), GetBackbufferOffset(FrameIndex), GetRtvHeapSize());
    GetCommandList()->OMSetRenderTargets(1, &BufferHandle, FALSE, nullptr);

    return BufferHandle;
}

void Renderer::BindBackbufferForPresent()
{
    UINT FrameIndex = SwapChain->GetCurrentBackBufferIndex();

    CD3DX12_RESOURCE_BARRIER BufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(GetBackbuffer(FrameIndex), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    GetCommandList()->ResourceBarrier(1, &BufferBarrier);
}

Renderer::~Renderer()
{
    if (Fence != nullptr)
    {
        Fence->Release();
        Fence = nullptr;
    }

    if(CommandList != nullptr)
    {
        CommandList->Release();
        CommandList = nullptr;
    }

    if (CommandAllocator != nullptr)
    {
        CommandAllocator->Release();
        CommandAllocator = nullptr;
    }

    if (CommandQueue != nullptr)
    {
        CommandQueue->Release();
        CommandQueue = nullptr;
    }

    if (SwapChain != nullptr)
    {
        SwapChain->SetFullscreenState(false, nullptr);
        SwapChain->Release();
        SwapChain = nullptr;
    }

    if (RTVHeap != nullptr)
    {
        RTVHeap->Release();
        RTVHeap = nullptr;
        RTVHeapSize = 0;
    }

    if (SRVHeap != nullptr)
    {
        SRVHeap->Release();
        SRVHeap = nullptr;
        SRVHeapSize = 0;
    }

    if (Framebuffer != nullptr)
    {
        Framebuffer->Release();
        Framebuffer = nullptr;
    }

    for(int i = 0; i < BackbufferCount; i++)
    {
        if (Backbuffers[i] != nullptr)
        {
            Backbuffers[i]->Release();
            Backbuffers[i] = nullptr;
        }
    }

    if (RenderDevice != nullptr)
    {
#ifndef NDEBUG
        ID3D12DebugDevice* DebugDevice = nullptr;
        HRESULT res = RenderDevice->QueryInterface(&DebugDevice);
        if (DebugDevice != nullptr)
        {
            RenderDevice->Release();
            RenderDevice = nullptr;

            DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
            DebugDevice->Release();
        }
        else
        {
            RenderDevice->Release();
            RenderDevice = nullptr;
        }
#else
        RenderDevice->Release();
        RenderDevice = nullptr;
#endif
    }
}

ID3D12Device2* Renderer::GetDevice()
{
    return RenderDevice;
}

IDXGISwapChain3* Renderer::GetSwapChain()
{
    return SwapChain;
}

ID3D12CommandAllocator* Renderer::GetCommandAllocator()
{
    return CommandAllocator;
}

ID3D12CommandQueue* Renderer::GetCommandQueue()
{
    return CommandQueue;
}

ID3D12DescriptorHeap* Renderer::GetRtvHeap()
{
    return RTVHeap;
}

UINT Renderer::GetRtvHeapSize()
{
    return RTVHeapSize;
}

ID3D12Resource* Renderer::GetBackbuffer(UINT Index)
{
    return Backbuffers[Index];
}

UINT Renderer::GetBackbufferOffset(UINT Index)
{
    return Index + 1;
}

ID3D12Resource* Renderer::GetFramebuffer()
{
    return Framebuffer;
}

UINT Renderer::GetFramebufferOffset()
{
    return 0;
}

DirectX::XMINT2 Renderer::GetFramebufferSize()
{
    return FramebufferSize;
}

ID3D12GraphicsCommandList* Renderer::GetCommandList()
{
    return CommandList;
}

void Renderer::ExecuteCommandList()
{
    ID3D12CommandList* CommandLists[] = { CommandList };
    CommandQueue->ExecuteCommandLists(_countof(CommandLists), CommandLists);
}

void Renderer::Resize(DirectX::XMINT2 NewSize)
{
    if (NewSize.x > 0 && NewSize.y > 0)
    {
        WaitForFence();
        for (int i = 0; i < BackbufferCount; i++)
        {
            if (Backbuffers[i] != nullptr)
            {
                Backbuffers[i]->Release();
                Backbuffers[i] = nullptr;
            }
        }

        SwapchainSize = NewSize;
        SwapChain->ResizeBuffers(BackbufferCount, SwapchainSize.x, SwapchainSize.y, DXGI_FORMAT_UNKNOWN, 0);

        CreateBackbuffers();

        WaitForFence();
    }
}

DirectX::XMINT2 Renderer::GetSize()
{
    return SwapchainSize;
}

void Renderer::WaitForFence()
{
    FenceValue++;

    GetCommandQueue()->Signal(Fence, FenceValue);

    if (Fence->GetCompletedValue() < FenceValue)
    {
        Fence->SetEventOnCompletion(FenceValue, FenceEvent);
        WaitForSingleObject(FenceEvent, INFINITE);
    }
}

std::string Renderer::GetOutputDeviceName(HWND window)
{
    IDXGIOutput* Output;
    if (SUCCEEDED(SwapChain->GetContainingOutput(&Output))) {
        DXGI_OUTPUT_DESC Desc;
        Output->GetDesc(&Desc);
        std:std::wstring DeviceName(Desc.DeviceName);

        Output->Release();

        return std::string(DeviceName.begin(), DeviceName.end());
    }
    else {
        HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONULL);
        MONITORINFOEX monitorInfo;
        monitorInfo.cbSize = sizeof(monitorInfo);
        BOOL b = GetMonitorInfoA(monitor, &monitorInfo);
        return std::string(monitorInfo.szDevice);
    }
}

// Create backbuffers at slot 1 & 2
void Renderer::CreateBackbuffers()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(RTVHeap->GetCPUDescriptorHandleForHeapStart());
    rtvHandle.Offset(1, RTVHeapSize);

    for (UINT n = 0; n < BackbufferCount; n++)
    {
        SwapChain->GetBuffer(n, IID_PPV_ARGS(&Backbuffers[n]));
        RenderDevice->CreateRenderTargetView(Backbuffers[n], nullptr, rtvHandle);
        rtvHandle.Offset(1, RTVHeapSize);
    }
}

