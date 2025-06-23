/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>

#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>

#include "shader.h"

GLuint makeShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource) {
    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    glShaderSource(VertexShaderID, 1, &vertexShaderSource, NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        printf("%s\n", &VertexShaderErrorMessage[0]);
    }

    // Compile Fragment Shader
    glShaderSource(FragmentShaderID, 1, &fragmentShaderSource, NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
        printf("%s\n", &FragmentShaderErrorMessage[0]);
    }

    // Link the program
    printf("Linking program\n");
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        printf("%s\n", &ProgramErrorMessage[0]);
    }

    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}


GLuint loadShadersFromFile(const char* vertex_file_path, const char* fragment_file_path) {
    std::string VertexShaderCode;
    std::string FragmentShaderCode;
    // Read the Vertex Shader code from the file
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if (VertexShaderStream.is_open()) {
        std::stringstream sstr;
        sstr << VertexShaderStream.rdbuf();
        VertexShaderCode = sstr.str();
        VertexShaderStream.close();
        printf("Vertex Shader code is taken from : %s\n", vertex_file_path);
    }

    // Read the Fragment Shader code from the file
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if (FragmentShaderStream.is_open()) {
        std::stringstream sstr;
        sstr << FragmentShaderStream.rdbuf();
        FragmentShaderCode = sstr.str();
        FragmentShaderStream.close();
        printf("Fragment Shader code is taken from : %s\n", fragment_file_path);
    }
    char const* vertexShaderSource = VertexShaderCode.c_str();
    char const* fragmentShaderSource = FragmentShaderCode.c_str();
    // Common shader program construction code
    return makeShaderProgram(vertexShaderSource, fragmentShaderSource);
}

GLuint loadBasicShaders() {
    const char* vertexShaderSource =
        "#version 330 core\n"
        // Input vertex data, different for all executions of this shader.
        "layout(location = 0) in vec3 vertexPosition_modelspace;\n"
        "layout(location = 1) in vec3 vertexColor;\n"
        // Output data; will be interpolated for each fragment.
        "out vec3 fragmentColor;\n"
        // Values that stay constant for the whole mesh.
        "uniform mat4 MVP;\n"
        "void main() {\n"
            // Output position of the vertex, in clip space : MVP * position
            "gl_Position = MVP * vec4(vertexPosition_modelspace, 1);\n"
            // The color of each vertex will be interpolated
            // to produce the color of each fragment
            "fragmentColor = vertexColor;\n"
        "}\n";
    const char* fragmentShaderSource =
        "#version 330 core\n"
        // Interpolated values from the vertex shaders
        "in vec3 fragmentColor;\n"
        // Ouput data
        "out vec3 color;\n"
        "void main() {\n"
            // Output color = color specified in the vertex shader,
            // interpolated between all 3 surrounding vertices
            "color = fragmentColor;\n"
        "}\n";

    // Common shader program construction code
    return makeShaderProgram(vertexShaderSource, fragmentShaderSource);
}
