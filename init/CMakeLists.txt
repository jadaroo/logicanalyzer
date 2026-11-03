# Host-side program (runs on the PC, not the Pico).
# Build separately from the firmware:
#   cmake -S init -B build-host && cmake --build build-host
cmake_minimum_required(VERSION 3.13)

project(logicanalyzer_host C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(logicanalyzer_host
    main.c
    serial.c
)

# Shares protocols.h with the firmware
target_include_directories(logicanalyzer_host PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_compile_options(logicanalyzer_host PRIVATE -Wall -Wextra)
