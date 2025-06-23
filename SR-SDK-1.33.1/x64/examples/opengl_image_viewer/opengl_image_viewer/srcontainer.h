/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef SRCONTAINER_H
#define SRCONTAINER_H

// External dependencies
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include "glfw3native.h" // glfwGetWin32Window(window)

// Simulated Reality includes
#include "sr/world/display/display.h"
#include "sr/weaver/glweaver.h"

// Holds PredictingGLWeaver and SRContext so that SRContext can easily and safely be repeatedly created and destroyed, so that camera turns on and off.
class SRContainer {
public:
    SR::SRContext* context;
    SR::PredictingGLWeaver* weaver;
    size_t weaverInputWidth;
    size_t weaverInputHeight;

    // Context is initialized as not created
    SRContainer() : context(NULL), weaver(NULL), weaverInputWidth(0), weaverInputHeight(0) {}

    // Context needs to be created before container members can be accessed
    bool contextCreated() { return context; }

    // Create context
    void createContext(GLFWwindow* window) {
        // Construct context
        context = SR::SRContext::create();

        //! [Get weaver input width and height]
        // Render to lower resolution than native display resolution because three-dimensional image won't convey any more detail and we can save processing power.
        // The input buffer of the weaver is twice as wide as the weaved image. The weaved image will be native display resolution.
        // Full-HD resolution for each view is ideal for 4K displays.
        SR::Display* display = SR::Display::create(*context);
        weaverInputWidth = 2 * display->getRecommendedViewsTextureWidth();
        weaverInputHeight = display->getRecommendedViewsTextureHeight();
        //! [Get weaver input width and height]

        //! [Construct weaver]
        HWND windowHandle = glfwGetWin32Window(window);
        weaver = new SR::PredictingGLWeaver(*context, weaverInputWidth, weaverInputHeight, windowHandle);
        //! [Construct weaver]

        // Initialize context
        context->initialize();
    }

    // Destroy context (return to state before creation)
    void destroyContext() {
        delete weaver;
        weaver = NULL;
        SR::SRContext::deleteSRContext(context);
        context = NULL;
        weaverInputWidth = 0;
        weaverInputHeight = 0;
    }
};

#endif
