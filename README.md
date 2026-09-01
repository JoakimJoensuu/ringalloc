# Ringalloc

Caller buffer. Ring of variable-size allocations.
For a stream of items that are freed in allocation order. API in [include/ringalloc.h](include/ringalloc.h).

Wrap when the next allocation or reallocation does not fit at the end and the start has room.
Leftover at the end stays unused until wrap is cleared.

Freestanding library (almost): no heap; a caller-provided buffer holds the
allocator and all allocations. Targets a flat address space. Object pointers
convert to `uintptr_t` and back without changing the address, and low bits
reflect byte alignment.

## Build

```sh
cmake -B build && cmake --build build && ctest --test-dir build
```

See [CMakeLists.txt](CMakeLists.txt) and [tests/CMakeLists.txt](tests/CMakeLists.txt) for
options and dependencies.

## Style

`--experimental-custom-checks` is required for `CustomChecks` in `.clang-tidy`.

```sh
clang-format-23 --dry-run --Werror $(find include src examples tests -type f -name '*.[ch]' | sort)
clang-tidy-23 --experimental-custom-checks $(jq -r '.[].file' compile_commands.json)
```
