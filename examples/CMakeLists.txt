add_executable(example allocate.c)
target_link_libraries(example PRIVATE ringalloc ringalloc_warnings)
