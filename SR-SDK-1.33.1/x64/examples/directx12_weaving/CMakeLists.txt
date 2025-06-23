#
# Copyright (C) 2025 Leia
#

cmake_minimum_required(VERSION 3.21)
project(example_directx_weaving)

find_package(simulatedreality REQUIRED)
find_package(srDirectX REQUIRED)

find_program(FXC fxc DOC "fx compiler")
if(NOT FXC)
    message(SEND_ERROR "Cannot find fxc.")
endif(NOT FXC)

add_definitions(
    -DTW_STATIC
    -DTW_NO_LIB_PRAGMA
    -DTW_NO_DIRECT3D
    -DGLEW_STATIC
    -D_CRT_SECURE_NO_WARNINGS
)

# compile shader to CSO
execute_process(
    COMMAND ${FXC} /T "vs_5_0" /E "VSMain" /O3 /Zpr /Qstrip_debug /Qstrip_reflect /Qstrip_rootsignature /Zi /Fo "${CMAKE_CURRENT_BINARY_DIR}/ColorShader_vs.cso" "${PROJECT_SOURCE_DIR}/directx12_weaving/ColorShader.hlsl"
    COMMAND ${FXC} /T "ps_5_0" /E "PSMain" /O3 /Zpr /Qstrip_debug /Qstrip_reflect /Qstrip_rootsignature /Zi /Fo "${CMAKE_CURRENT_BINARY_DIR}/ColorShader_ps.cso" "${PROJECT_SOURCE_DIR}/directx12_weaving/ColorShader.hlsl"
)

# Pack CSO blobs into shader_internal.h
set(VERTEXSHADERFILE "${CMAKE_CURRENT_BINARY_DIR}/ColorShader_vs.cso")
file(READ ${VERTEXSHADERFILE} VERTEXSHADER HEX)
STRING(REGEX REPLACE "([0-9a-z][0-9a-z])" "0x\\1," VERTEXSHADER "${VERTEXSHADER}")
set(FRAGMENTSHADERFILE "${CMAKE_CURRENT_BINARY_DIR}/ColorShader_ps.cso")
file(READ ${FRAGMENTSHADERFILE} FRAGMENTSHADER HEX)
STRING(REGEX REPLACE "([0-9a-z][0-9a-z])" "0x\\1," FRAGMENTSHADER "${FRAGMENTSHADER}")
configure_file(${PROJECT_SOURCE_DIR}/directx12_weaving/shader_internal.h.in ${PROJECT_SOURCE_DIR}/directx12_weaving/shader_internal.h)

# directx-pyramids
add_executable(example_directx12_weaving
    ${PROJECT_SOURCE_DIR}/directx12_weaving/main.cpp
    ${PROJECT_SOURCE_DIR}/directx12_weaving/pyramid.cpp
    ${PROJECT_SOURCE_DIR}/directx12_weaving/pyramid.h
    ${PROJECT_SOURCE_DIR}/directx12_weaving/renderer.cpp
    ${PROJECT_SOURCE_DIR}/directx12_weaving/renderer.h
    ${PROJECT_SOURCE_DIR}/directx12_weaving/shader.cpp
    ${PROJECT_SOURCE_DIR}/directx12_weaving/shader.h
    ${PROJECT_SOURCE_DIR}/directx12_weaving/shader_internal.h
    ${PROJECT_SOURCE_DIR}/directx12_weaving/window.cpp
    ${PROJECT_SOURCE_DIR}/directx12_weaving/window.h
)

target_include_directories(example_directx12_weaving PRIVATE
    ${PROJECT_SOURCE_DIR}/directx12_weaving
    ${srDirectX_INCLUDE_DIR}
)

if (WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(BUILD_PLATFORM 32)
endif()

target_link_libraries(example_directx12_weaving PRIVATE
    simulatedreality
    srDirectX${BUILD_PLATFORM}::srDirectX
    d3d12
    DXGI
    Shcore.lib
)

add_custom_command(TARGET example_directx12_weaving POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE_DIR:example_directx12_weaving> $<TARGET_RUNTIME_DLLS:example_directx12_weaving>
    ${CMAKE_CURRENT_BINARY_DIR}
)
