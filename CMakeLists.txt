cmake_minimum_required(VERSION 3.25)
project(learn-opengl-sdl3 
VERSION 1.0.0 
LANGUAGES C CXX
DESCRIPTION "A modern OpenGL (SDL3 version) graphics rendering project."
)

include(cmake/ProjectOptions.cmake)

add_subdirectory(vendor)
add_subdirectory(learn-opengl-sdl3)

include(CTest)
enable_testing()
add_subdirectory(test)