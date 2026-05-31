#
# Copyright (C) 2025 Leia, Inc.
#

cmake_minimum_required(VERSION 3.12)
project(example_systemsense)
find_package(simulatedreality REQUIRED)
add_executable(example_systemsense ${PROJECT_SOURCE_DIR}/src/systemsense.cpp)
target_link_libraries(example_systemsense simulatedreality)
