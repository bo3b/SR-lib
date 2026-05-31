#
# Copyright (C) 2025 Leia, Inc.
#

cmake_minimum_required(VERSION 3.12)
project(example_cpp)
find_package(simulatedreality REQUIRED)
add_executable(example_cpp ${PROJECT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(example_cpp simulatedreality)
