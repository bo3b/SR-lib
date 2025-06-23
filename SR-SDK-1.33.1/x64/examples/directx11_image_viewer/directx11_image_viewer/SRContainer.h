/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef SRWEAVERCONTEXT_H
#define SRWEAVERCONTEXT_H
#include <d3d11.h>
#include <DirectXMath.h>
#include "window.h"
#include <sr/weaver/dx11weaver.h>
#include "renderer.h"

class SRContainer
{
public:
    SRContainer();
    ~SRContainer();

    SR::SRContext* context;
    size_t weaverInputWidth;
    size_t weaverInputHeight;
    SR::PredictingDX11Weaver* weaver;

    bool isContextCreated();
    void createContext(Renderer& renderer, Window& window);
    void destroyContext();
};

#endif
