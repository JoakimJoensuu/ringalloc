# Ringalloc

Caller buffer. Ring of variable-size allocations. Free oldest, reallocate newest.
For a stream of items that are freed in allocation order.

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

CMake fetches [cgreen](https://github.com/cgreen-devs/cgreen) when tests are on.
Examples and tests default off when this project is not the CMake top level.

## Style

`--experimental-custom-checks` is required for `CustomChecks` in `.clang-tidy`.

```sh
clang-format-23 --dry-run --Werror $(find include src examples tests -type f -name '*.[ch]' | sort)
clang-tidy-23 --experimental-custom-checks $(jq -r '.[].file' compile_commands.json)
```
