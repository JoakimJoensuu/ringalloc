# Ringalloc

Caller buffer. Ring of variable-size allocations. Free oldest, realloc newest
only. Wrap when the end is full.

## Build

```sh
cmake -B build && cmake --build build && ctest --test-dir build
```

CMake fetches [cgreen](https://github.com/cgreen-devs/cgreen) when tests are on.
Examples and tests are off when this project is not the CMake top level.

## Style

`--experimental-custom-checks` is required for `CustomChecks` in `.clang-tidy`.

```sh
clang-format-23 --dry-run --Werror $(find include src examples tests -type f -name '*.[ch]' | sort)
clang-tidy-23 --experimental-custom-checks $(jq -r '.[].file' compile_commands.json)
```
