cmake_minimum_required(VERSION 3.12)
project(glew VERSION 1.13.0)

find_package(OpenGL REQUIRED)

# GLEW
set(GLEW_SOURCE src/glew.c)
set(GLEW_HEADERS )
add_library(glew STATIC
    ${GLEW_SOURCE}
    ${GLEW_INCLUDE}
)
target_include_directories(glew PRIVATE
    include/
)
target_link_libraries(glew
    ${OPENGL_LIBRARY}
    ${EXTRA_LIBS}
)
