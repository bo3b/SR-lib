/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include "imageplane.h"

#include <GL/glew.h>

//! [Triangle Vertices]
// Z-range = [-1, 1>
static const GLfloat g_vertex_buffer_data[] = {
    -1.0, 1.0, 0.0,     3.0, 1.0, 0.0,    -1.0, -3.0, 0.0,
};
//! [Triangle Vertices]

//! [Triangle Texture Coords]
// One color for each vertex. XYZ maps to RGB
static const GLfloat g_texture_coord_data[] = {
     0.0,  0.0,     2.0,  0.0,     0.0,  2.0,
};
//! [Triangle Texture Coords]

ImagePlane::ImagePlane(const void* imageData, GLsizei imageWidth, GLsizei imageHeight, GLenum dataFormat, GLenum dataType, GLenum textureFormat) {
    //! [Generate vertices and texture coordinates]
    glGenVertexArrays(1, &vertexArrayID);
    glBindVertexArray(vertexArrayID);

    // Bind vertex buffer to vertex array
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Bind texture coord buffer to vertex array
    glGenBuffers(1, &textureCoordBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, textureCoordBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_texture_coord_data), g_texture_coord_data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    //! [Generate vertices and texture coordinates]

    //! [Generate image texture]
    // Generate ID and load data
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Don't interpolate image pixels as the border between the left and right image should remain sharp.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Load the image data into the GPU.
    glTexImage2D(GL_TEXTURE_2D, 0, textureFormat, imageWidth, imageHeight, 0, dataFormat, dataType, imageData);
    //! [Generate image texture]

    glBindVertexArray(0);
}

void ImagePlane::draw() const {
    // Ensure active texture is set to 0 (default) as we use only one texture in our shader and GLWeaver might set this to another value.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glBindVertexArray(vertexArrayID);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void ImagePlane::destroy() {
    glDeleteTextures(1, &textureID);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &textureCoordBuffer);

    glDeleteVertexArrays(1, &vertexArrayID);
}
