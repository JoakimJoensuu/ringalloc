cmake_minimum_required(VERSION 3.21)
project(ringalloc LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(CREATE_LINK "${CMAKE_BINARY_DIR}/compile_commands.json"
     "${CMAKE_SOURCE_DIR}/compile_commands.json" SYMBOLIC COPY_ON_ERROR)

add_library(ringalloc_warnings INTERFACE)
target_compile_options(
  ringalloc_warnings
  INTERFACE -Wall
            -Wextra
            -Wpedantic
            -Werror
            -Wconversion
            -Wformat=2
            -Wmissing-prototypes
            -Wshadow
            -Wstrict-prototypes)
