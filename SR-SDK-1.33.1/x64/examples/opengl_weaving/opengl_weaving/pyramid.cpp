/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include "pyramid.h"

#include <GL/glew.h>

//! [Pentahedron Vertices]
static const GLfloat g_vertex_buffer_data[] = {
    +1.0, -1.0, -1.0,     0.0, +1.0,  0.0,    -1.0, -1.0, -1.0,
     0.0, +1.0,  0.0,    -1.0, -1.0, +1.0,    -1.0, -1.0, -1.0,
    +1.0, -1.0, +1.0,     0.0, +1.0,  0.0,    +1.0, -1.0, -1.0,
    +1.0, -1.0, +1.0,    +1.0, -1.0, -1.0,    -1.0, -1.0, -1.0,
    +1.0, -1.0, +1.0,    -1.0, -1.0, -1.0,    -1.0, -1.0, +1.0,
    +1.0, -1.0, +1.0,    -1.0, -1.0, +1.0,     0.0, +1.0,  0.0,
};
//! [Pentahedron Vertices]

//! [Pentahedron Colors]
// One color for each vertex. XYZ maps to RGB
static const GLfloat g_color_buffer_data[] = {
     0.0,  1.0,  0.0,     0.0,  1.0,  0.0,     0.0,  1.0,  0.0, //back
     1.0,  0.0,  0.0,     1.0,  0.0,  0.0,     1.0,  0.0,  0.0, //left
     0.0,  1.0,  1.0,     0.0,  1.0,  1.0,     0.0,  1.0,  1.0, //right
     0.0,  0.0,  1.0,     0.0,  0.0,  1.0,     0.0,  0.0,  1.0, //up
     0.0,  0.0,  1.0,     0.0,  0.0,  1.0,     0.0,  0.0,  1.0, //up
     1.0,  1.0,  0.0,     1.0,  1.0,  0.0,     1.0,  1.0,  0.0, //front
};
//! [Pentahedron Colors]

Pyramid::Pyramid() {
    glGenVertexArrays(1, &vertexArrayID);
    glBindVertexArray(vertexArrayID);

    // Bind vertex buffer to vertex array
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Bind color buffer to vertex array
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
}

Pyramid::~Pyramid() {
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &colorbuffer);

    glDeleteVertexArrays(1, &vertexArrayID);
}

void Pyramid::draw() {
    glBindVertexArray(vertexArrayID);
    glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
}
