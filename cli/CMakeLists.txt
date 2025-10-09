cmake_minimum_required(VERSION 3.14)
project(ImageAssemblyCLI C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

# Add the library subdirectory
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../c ${CMAKE_CURRENT_BINARY_DIR}/imageassembly)

# Fetch STB libraries (header-only)
include(FetchContent)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# Fetch msf_gif for animated GIF support (single-header C library)
FetchContent_Declare(
    msf_gif
    GIT_REPOSITORY https://github.com/notnullnotvoid/msf_gif.git
    GIT_TAG master
)
FetchContent_MakeAvailable(msf_gif)

# Build CLI executable
add_executable(imgasm
    main.c
)

# Link the imageassembly library
target_link_libraries(imgasm 
    imageassembly
)

# Add include directories for STB and msf_gif
target_include_directories(imgasm PRIVATE
    ${stb_SOURCE_DIR}
    ${msf_gif_SOURCE_DIR}
)

# Link math library (needed on some platforms)
if(UNIX AND NOT APPLE)
    target_link_libraries(imgasm m)
endif()