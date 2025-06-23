#
# Copyright (C) 2025 Leia
#

cmake_minimum_required(VERSION 3.21)
project(example_opengl_image_viewer)

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

# OpenGL-image viewer
add_executable(example_opengl_image_viewer
    ${PROJECT_SOURCE_DIR}/opengl_image_viewer/main.cpp
    ${PROJECT_SOURCE_DIR}/opengl_image_viewer/imageplane.cpp
    ${PROJECT_SOURCE_DIR}/opengl_image_viewer/imageplane.h
    ${PROJECT_SOURCE_DIR}/opengl_image_viewer/shader.cpp
    ${PROJECT_SOURCE_DIR}/opengl_image_viewer/shader.h
    ${PROJECT_SOURCE_DIR}/opengl_image_viewer/srcontainer.h
    ${PROJECT_SOURCE_DIR}/opengl_image_viewer/stb_image.h
)
target_include_directories(example_opengl_image_viewer PRIVATE
    ${PROJECT_SOURCE_DIR}/opengl_image_viewer
    ${srOpenGL_INCLUDE_DIR}
    glfw-3.3.7/include/GLFW/
    glfw-3.3.7/include/
    glew-1.13.0/include/
)

if (WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(BUILD_PLATFORM 32)
endif()

target_link_libraries(example_opengl_image_viewer PRIVATE
    simulatedreality
    srOpenGL${BUILD_PLATFORM}::srOpenGL
    ${OPENGL_LIBRARY}
    glfw
    glew
    Shcore
)

add_custom_command(TARGET example_opengl_image_viewer POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE_DIR:example_opengl_image_viewer> $<TARGET_RUNTIME_DLLS:example_opengl_image_viewer>
    ${CMAKE_CURRENT_BINARY_DIR}
)
