# Host-side program (runs on the PC, not the Pico).
# Build separately from the firmware, from this folder:
#   cmake -S . -B build && cmake --build build
cmake_minimum_required(VERSION 3.13)

project(logicanalyzer_host C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)   # -g, so gdb can show source lines
endif()

# Every .c and .cpp file under src/ (including subfolders like src/i2c/) is compiled.
# CONFIGURE_DEPENDS makes CMake re-scan when you add or remove a file.
file(GLOB_RECURSE HOST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/src/*.c
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)

add_executable(logicanalyzer_host ${HOST_SOURCES})

# include/ and every folder inside it (include/i2c, include/uart, ...),
# so headers can be included by name: #include "i2c_decoder.h"
set(HOST_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include)
file(GLOB_RECURSE HOST_INCLUDE_CHILDREN LIST_DIRECTORIES true CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/include/*
)
foreach(child ${HOST_INCLUDE_CHILDREN})
    if(IS_DIRECTORY ${child})
        list(APPEND HOST_INCLUDE_DIRS ${child})
    endif()
endforeach()

target_include_directories(logicanalyzer_host PRIVATE
    ${HOST_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}/../include   # protocols.h, shared with the firmware
)

target_compile_options(logicanalyzer_host PRIVATE -Wall -Wextra)

# main.c uses lround() from math.h
target_link_libraries(logicanalyzer_host PRIVATE m)
