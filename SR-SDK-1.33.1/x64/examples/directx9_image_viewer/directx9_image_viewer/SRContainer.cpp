/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include <sr/world/display/display.h>
#include <sr/weaver/dx9weaver.h>
#include "window.h"
#include "renderer.h"
#include "SRContainer.h"

// Holds PredictingGLWeaver and SRContext so that SRContext can easily and safely be repeatedly created and destroyed, so that camera turns on and off.
// SRWeaverContext is initialized as not created
SRContainer::SRContainer() : context(NULL), weaverInputWidth(0), weaverInputHeight(0), weaver(NULL) {}

// Make sure SRWeaverContext is destroyed before object is deleted
SRContainer::~SRContainer() {
    if (isContextCreated()) destroyContext();
}

// SRWeaverContext needs to be created before members can be accessed
bool SRContainer::isContextCreated() { return context; }

// Create SRWeaverContext
void SRContainer::createContext(Renderer& renderer, Window& window) {
    // Construct context
    context = SR::SRContext::create();

    //! [Get weaver input width and height]
    // Render to lower resolution than native display resolution because three-dimensional image won't convey any more detail and we can save processing power.
    // The input buffer of the weaver is twice as wide as the weaved image. The weaved image will be native display resolution.
    // Full-HD resolution for each view is ideal for 4K displays.
    SR::Display* display = SR::Display::create(*context);
    weaverInputWidth = display->getRecommendedViewsTextureWidth();
    weaverInputHeight = display->getRecommendedViewsTextureHeight();
    //! The DX9 Weaver may not be able to deal with textures which are larger than 4096
    if (weaverInputWidth > 4096 / 2 || weaverInputHeight > 4096) {
        std::cout << "Warning: texture size in PredictingDX9Weaver may exceed the limits on this system." << std::endl;
    }
    //! [Get weaver input width and height]

    //! [Construct weaver]
    try {
        weaver = new SR::PredictingDX9Weaver(*context, renderer.GetDevice(), weaverInputWidth * 2, weaverInputHeight, window.GetHandle());
    }
    catch (const std::exception& e) {
        std::string errorMessage = e.what();
        std::cout << "Could not construct PredictingDX9Weaver: " << errorMessage << std::endl << "Aborting." << std::endl;
        exit(1);
    }
    //! [Construct weaver]

    // Initialize context
    context->initialize();
}

// Destroy SRWeaverContext (return to state before creation)
void SRContainer::destroyContext() {
    delete weaver;
    weaver = NULL;
    SR::SRContext::deleteSRContext(context);
    context = NULL;
    weaverInputWidth = 0;
    weaverInputHeight = 0;
}
