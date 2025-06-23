/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#pragma once

#include <GL/glew.h>

class ImagePlane {
    GLuint vertexArrayID;
    GLuint vertexbuffer;
    GLuint textureCoordBuffer;
    GLuint textureID; // used in: glBindTexture(GL_TEXTURE_2D, textureID);

public:
    // <dataFormat> describes the pixel format of the input image, <dataType> describes the type of the individual components of the input pixels, <textureFormat> describes the internal pixel format of the OpenGL texture.
    // These parameters will be used in the following order to generate a OpenGL texture:
    // glTexImage2D(GL_TEXTURE_2D, 0, textureFormat, imageWidth, imageHeight, 0, dataFormat, dataType, imageData);
    ImagePlane(const void* imageData, GLsizei imageWidth, GLsizei imageHeight, GLenum dataFormat, GLenum dataType, GLenum textureFormat);
    // Destructor does nothing, call destroy() before ImagePlane goes out of scope!
    ~ImagePlane() {};
    void draw() const;
    // Call this function before OpenGL is terminated or this object is destructed
    void destroy();
};
