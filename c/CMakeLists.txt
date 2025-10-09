cmake_minimum_required(VERSION 3.10)
project(ImageAssemblyLibrary C)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# Build a static library from source files
add_library(imageassembly STATIC
    template.h
    template.c
    error.h
    error.c
    bytecode.h
    bytecode.c
    assembly.h
    assembly.c
)