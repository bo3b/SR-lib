/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef SHADER_H
#define SHADER_H

/*!
 * \brief This function makes a shader program based on the vertex and fragment shader inputs
 * \param vertexShaderSource is a string to be compiled as the vertex shader
 * \param fragmentShaderSource is a string to be compiled as the fragment shader
 * \returns shader program ID to be used in glUseProgram
 */
GLuint makeShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource);

/*!
 * \brief This function reads the shader program input from files
 * 
 * You can customize shader program by creating two text files with similar content provided in the loadBasicShaders function.
 * This function works with a vertex shader file path and fragment shader file path relative to the working folder.
 * 
 * \param vertex_file_path is the address string to a file to  be compiled as the vertex shader
 * \param fragment_file_path is the address string to a file to be compiled as the fragment shader
 * \returns shader program ID to be used in glUseProgram (by running makeShaderProgram)
 */
GLuint loadShadersFromFile(const char* vertex_file_path, const char* fragment_file_path);

/*!
 * \brief Creates a shader that does not use any transform matrix (static) and that uses 1 texture.
 *
 * \returns Shader program ID to be used in glUseProgram (by running makeShaderProgram)
 */
GLuint createStaticTextureShader();
#endif
