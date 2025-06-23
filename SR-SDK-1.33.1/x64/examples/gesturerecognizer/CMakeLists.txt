#
# Copyright (C) 2025 Leia, Inc.
#

cmake_minimum_required(VERSION 3.12)
project(example_gesturerecognizer)
find_package(simulatedreality REQUIRED)
add_executable(example_gesturerecognizer ${PROJECT_SOURCE_DIR}/src/gesturerecognizer.cpp)
target_link_libraries(example_gesturerecognizer simulatedreality)
