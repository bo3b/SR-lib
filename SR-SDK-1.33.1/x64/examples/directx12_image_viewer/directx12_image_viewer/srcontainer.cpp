/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include <sr/world/display/screen.h>
#include <sr/weaver/dx12weaver.h>
#include "srcontainer.h"


 // Holds PredictingGLWeaver and SRContext so that SRContext can easily and safely be repeatedly enabled and disabled, so that camera/ turns on and off.
  // SRWeaverContext is initialized as not created
SRContainer::SRContainer() : context(nullptr), weaver(nullptr) {}

// Make sure SRWeaverContext is destroyed before object is deleted
SRContainer::~SRContainer() 
{
    if (isContextCreated())
        destroyContext();
}


// Create SRWeaverContext
void SRContainer::createContext(ID3D12Device2* RenderDevice, ID3D12CommandAllocator* CommandAllocator, ID3D12GraphicsCommandList* CommandList, ID3D12CommandQueue* CommandQueue, ID3D12Resource* WeaverInputBuffer, ID3D12Resource* WeaverOutputBuffer, HWND hWnd)
{
    // Construct context
    context = SR::SRContext::create();

    //! [Construct weaver]+
    try {
        weaver = new SR::PredictingDX12Weaver(*context, RenderDevice, CommandAllocator, CommandQueue, WeaverInputBuffer, WeaverOutputBuffer, hWnd);
    }
    catch (const std::exception& e) {
        std::string errorMessage = e.what();
        std::cout << "Could not construct PredictingDX12Weaver: " << errorMessage << std::endl << "Aborting." << std::endl;
        exit(1);
    }
    //! [Construct weaver]

    // Set command list
    weaver->setCommandList(CommandList);

    // Initialize context
    context->initialize();
}

// Destroy SRWeaverContext (return to state before creation)
void SRContainer::destroyContext() 
{
    if (weaver != nullptr)
    {
        delete weaver;
        weaver = NULL;
    }

    if (context != nullptr)
    {
        SR::SRContext::deleteSRContext(context);
        context = NULL;
    }
}

