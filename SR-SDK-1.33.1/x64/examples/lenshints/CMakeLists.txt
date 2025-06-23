#
# Copyright (C) 2025 Leia, Inc.
#

cmake_minimum_required(VERSION 3.12)
project(example_lenshints)
find_package(simulatedreality REQUIRED)
add_executable(example_lenshints ${PROJECT_SOURCE_DIR}/src/lenshints.cpp)
target_link_libraries(example_lenshints simulatedreality)
