#
# Copyright (C) 2025 Leia, Inc.
#

cmake_minimum_required(VERSION 3.12)
project(example_c C)
find_package(simulatedreality REQUIRED)
add_executable(example_c ${PROJECT_SOURCE_DIR}/src/main.c)
target_link_libraries(example_c simulatedreality)
