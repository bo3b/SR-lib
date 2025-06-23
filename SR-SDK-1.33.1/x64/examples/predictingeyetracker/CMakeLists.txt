#
# Copyright (C) 2025 Leia, Inc.
#

cmake_minimum_required(VERSION 3.12)
project(example_predictingeyetracker)
find_package(simulatedreality REQUIRED)
add_executable(example_predictingeyetracker ${PROJECT_SOURCE_DIR}/src/predictingeyetracker.cpp)
target_link_libraries(example_predictingeyetracker simulatedreality)
