#
# Copyright (C) 2025 Leia
#

cmake_minimum_required(VERSION 3.21)
project(example_directx_weaving)

set(LEIASR_SDKROOT "$ENV{LEIASR_SDKROOT}" CACHE PATH "Path to LEIASR SDK root")

# Tell CMake to look here for find_package()
list(APPEND CMAKE_PREFIX_PATH "${LEIASR_SDKROOT}/lib/cmake")


find_package(simulatedreality REQUIRED)
find_package(srDirectX REQUIRED)


add_definitions(
    -DTW_STATIC
    -DTW_NO_LIB_PRAGMA
    -DTW_NO_DIRECT3D
    -DGLEW_STATIC
    -D_CRT_SECURE_NO_WARNINGS
)

# DirectX10 example
add_executable(example_directx10_weaving
    WIN32
    ${PROJECT_SOURCE_DIR}/directx10_weaving/main.cpp
    ${PROJECT_SOURCE_DIR}/directx10_weaving/leia_math.h
    ${PROJECT_SOURCE_DIR}/directx10_weaving/resource.h
    ${PROJECT_SOURCE_DIR}/directx10_weaving/resource.rc
    ${PROJECT_SOURCE_DIR}/directx10_weaving/stb_image.h
    ${PROJECT_SOURCE_DIR}/directx10_weaving/StereoImage.jpg
    ${PROJECT_SOURCE_DIR}/directx10_weaving/targetver.h
    ${PROJECT_SOURCE_DIR}/directx10_weaving/icon_small.ico
    ${PROJECT_SOURCE_DIR}/directx10_weaving/icon_large.ico
)

target_include_directories(example_directx10_weaving PRIVATE
    ${PROJECT_SOURCE_DIR}/directx10_weaving
)

if (WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(BUILD_PLATFORM 32)
endif()

target_link_libraries(example_directx10_weaving PRIVATE
    simulatedreality
    srDirectX${BUILD_PLATFORM}::srDirectX
)

add_custom_command(
  TARGET example_directx10_weaving POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${PROJECT_SOURCE_DIR}/directx10_weaving/StereoImage.jpg
          $<TARGET_FILE_DIR:example_directx10_weaving>/StereoImage.jpg)

add_custom_command(TARGET example_directx10_weaving POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE_DIR:example_directx10_weaving> $<TARGET_RUNTIME_DLLS:example_directx10_weaving>
    ${CMAKE_CURRENT_BINARY_DIR}
)
