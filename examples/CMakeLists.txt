add_executable(example alloc.c)
target_link_libraries(example PRIVATE ringalloc ringalloc_warnings)
