#
# Copyright (C) 2025 Leia
#

cmake_minimum_required(VERSION 3.21)
project(example_opengl_weaving)

find_package(simulatedreality REQUIRED)
find_package(srOpenGL REQUIRED)
find_package(OpenGL REQUIRED)

add_definitions(
    -DTW_STATIC
    -DTW_NO_LIB_PRAGMA
    -DTW_NO_DIRECT3D
    -DGLEW_STATIC
    -D_CRT_SECURE_NO_WARNINGS
)

# GLEW
add_subdirectory(glew-1.13.0)

# GLFW
add_subdirectory(glfw-3.3.7 EXCLUDE_FROM_ALL)

# Opengl-pyramids
add_executable(example_opengl_weaving
    ${PROJECT_SOURCE_DIR}/opengl_weaving/main.cpp
    ${PROJECT_SOURCE_DIR}/opengl_weaving/pyramid.cpp
    ${PROJECT_SOURCE_DIR}/opengl_weaving/pyramid.h
    ${PROJECT_SOURCE_DIR}/opengl_weaving/shader.cpp
    ${PROJECT_SOURCE_DIR}/opengl_weaving/shader.h
)

target_include_directories(example_opengl_weaving PRIVATE
    ${PROJECT_SOURCE_DIR}/opengl_weaving
    ${srOpenGL_INCLUDE_DIR}
    glfw-3.3.7/include/GLFW/
    glfw-3.3.7/include/
    glm-0.9.7.1/
    glew-1.13.0/include/
)

if (WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(BUILD_PLATFORM 32)
endif()

target_link_libraries(example_opengl_weaving PRIVATE
    simulatedreality
    srOpenGL${BUILD_PLATFORM}::srOpenGL
    ${OPENGL_LIBRARY}
    glfw
    glew
    Shcore
)

add_custom_command(TARGET example_opengl_weaving POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE_DIR:example_opengl_weaving> $<TARGET_RUNTIME_DLLS:example_opengl_weaving>
    ${CMAKE_CURRENT_BINARY_DIR}
)
