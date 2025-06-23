/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef SRWEAVERCONTEXT_H
#define SRWEAVERCONTEXT_H
#include <d3d9.h>
#include <DirectXMath.h>
#include "window.h"
#include <sr/weaver/dx9weaver.h>
#include "renderer.h"

class SRContainer
{
public:
    SRContainer();
    ~SRContainer();

    SR::SRContext* context;
    int weaverInputWidth;
    int weaverInputHeight;
    SR::PredictingDX9Weaver* weaver;

    bool isContextCreated();
    void createContext(Renderer& renderer, Window& window);
    void destroyContext();
};

#endif
