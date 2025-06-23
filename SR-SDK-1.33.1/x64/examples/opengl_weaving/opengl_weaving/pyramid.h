/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#pragma once

#include <GL/glew.h>

class Pyramid {
    GLuint vertexArrayID;
    GLuint vertexbuffer;
    GLuint colorbuffer;

public:
    Pyramid();
    ~Pyramid();
    void draw();
};
