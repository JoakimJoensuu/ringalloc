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
add_library(ringalloc_sanitize INTERFACE)
if(PROJECT_IS_TOP_LEVEL AND (CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU"))
  target_compile_options(ringalloc_sanitize INTERFACE -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_link_options(ringalloc_sanitize INTERFACE -fsanitize=address,undefined)
endif()

add_library(ringalloc src/ringalloc.c)
target_include_directories(ringalloc PUBLIC include)
target_compile_features(ringalloc PUBLIC c_std_23)
target_compile_options(ringalloc PRIVATE -ffreestanding)
target_link_libraries(ringalloc PRIVATE ringalloc_warnings ringalloc_sanitize)

option(RINGALLOC_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(RINGALLOC_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})

if(RINGALLOC_EXAMPLES)
  add_subdirectory(examples)
endif()

if(RINGALLOC_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
