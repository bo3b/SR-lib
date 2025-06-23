/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef SRWEAVERCONTEXT_H
#define SRWEAVERCONTEXT_H
#include <d3d10_1.h>
#include <DirectXMath.h>
#include "window.h"
#include <sr/weaver/dx10weaver.h>
#include "renderer.h"

class SRContainer
{
public:
    SRContainer();
    ~SRContainer();

    SR::SRContext* context;
    int weaverInputWidth;
    int weaverInputHeight;
    SR::PredictingDX10Weaver* weaver;

    bool isContextCreated();
    void createContext(Renderer& renderer, Window& window);
    void destroyContext();
};

#endif
