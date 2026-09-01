#include <ringalloc.h>

#include <cgreen/assertions.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/reporter.h>
#include <cgreen/runner.h>
#include <cgreen/suite.h>
#include <cgreen/text_reporter.h>
#include <cgreen/unit.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum : size_t {
  TINY_LENGTH    = 1,
  ODD_SIZE       = 7,
  SMALL_SIZE     = 8,
  GROW_SIZE      = 32,
  BLOCK_SIZE     = 400,
  STORAGE_LENGTH = 1024,
  MISALIGNMENT   = 1,
  BLOCK_COUNT    = 64,
};

enum : unsigned char {
  FILL_A = 0x11,
  FILL_B = 0x22,
  FILL_C = 0x33,
  FILL_D = 0x44,
};

static bool is_max_aligned(void *address) {
  return ((uintptr_t)address & (alignof(max_align_t) - 1U)) == 0;
}

Ensure(allocate_zero) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *allocator = ra_initialize(storage, sizeof(storage));
  void *zero = nullptr;

  assert_that(allocator, is_non_null);
  zero = ra_allocate(allocator, 0);
  assert_that(zero, is_non_null);
  assert_that(ra_allocate(allocator, SMALL_SIZE), is_non_null);
}

Ensure(reallocate_zero) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *allocator = ra_initialize(storage, sizeof(storage));
  unsigned char *block = nullptr;
  assert_that(allocator, is_non_null);
  block = ra_allocate(allocator, SMALL_SIZE);
  assert_that(block, is_non_null);
  block = ra_reallocate(allocator, block, 0);
  assert_that(block, is_non_null);
  assert_that(ra_reallocate(allocator, block, GROW_SIZE), is_equal_to(block));
}

Ensure(free_null) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *allocator = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *second = nullptr;

  assert_that(allocator, is_non_null);
  first = ra_allocate(allocator, SMALL_SIZE);
  second = ra_allocate(allocator, SMALL_SIZE);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  ra_free(allocator, nullptr);
  ra_free(allocator, first);
  assert_that(second, is_non_null);
}

Ensure(initialize_too_small) {
  unsigned char storage[TINY_LENGTH];
  assert_that(ra_initialize(storage, sizeof(storage)), is_null);
}

Ensure(initialize_smallest) {
  unsigned char storage[STORAGE_LENGTH];
  size_t capacity = 0;
  struct ringalloc *allocator = nullptr;

  for (; capacity <= sizeof(storage); capacity += 1) {
    allocator = ra_initialize(storage, capacity);
    if (allocator != nullptr) break;
  }
  assert_that(allocator, is_non_null);
  assert_that(ra_allocate(allocator, 0), is_null);
  assert_that(ra_allocate(allocator, SMALL_SIZE), is_null);
  assert_that(capacity, is_greater_than(0));
  assert_that(ra_initialize(storage, capacity - 1), is_null);
}

Ensure(initialize_unaligned) {
  alignas(max_align_t) unsigned char storage[STORAGE_LENGTH + MISALIGNMENT];
  struct ringalloc *allocator = ra_initialize(storage + MISALIGNMENT, STORAGE_LENGTH);

  assert_that(allocator, is_non_null);
  assert_that(ra_allocate(allocator, SMALL_SIZE), is_non_null);
}

Ensure(allocate_aligned) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *allocator = ra_initialize(storage, sizeof(storage));
  void *first = nullptr;
  void *second = nullptr;

  assert_that(allocator, is_non_null);
  first = ra_allocate(allocator, 1);
  second = ra_allocate(allocator, ODD_SIZE);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  assert_that(is_max_aligned(first), is_true);
  assert_that(is_max_aligned(second), is_true);
}

Ensure(allocate_odd_sizes) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *allocator = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *second = nullptr;
  unsigned char *third = nullptr;

  assert_that(allocator, is_non_null);
  first = ra_allocate(allocator, 1);
  second = ra_allocate(allocator, ODD_SIZE);
  third = ra_allocate(allocator, 1);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  assert_that(third, is_non_null);
  assert_that(is_max_aligned(third), is_true);
  assert_that(first, is_not_equal_to(second));
  assert_that(second, is_not_equal_to(third));
}

Ensure(initialize_zero_ring) {
  alignas(max_align_t) unsigned char storage[STORAGE_LENGTH + MISALIGNMENT];
  unsigned char *buffer = storage + MISALIGNMENT;
  struct ringalloc *allocator = nullptr;
  size_t capacity = 0;

  for (; capacity <= STORAGE_LENGTH; capacity += 1) {
    allocator = ra_initialize(buffer, capacity);
    if (allocator == nullptr) continue;
    if (ra_allocate(allocator, SMALL_SIZE) == nullptr) continue;
    assert_that(capacity, is_greater_than(1));
    allocator = ra_initialize(buffer, capacity - 1);
    assert_that(allocator, is_non_null);
    assert_that(ra_allocate(allocator, SMALL_SIZE), is_null);
    return;
  }
  assert_that(false, is_true);
}

Ensure(allocate_and_free_oldest) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *allocator = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *second = nullptr;
  unsigned char expect[SMALL_SIZE];

  assert_that(allocator, is_non_null);
  first = ra_allocate(allocator, SMALL_SIZE);
  second = ra_allocate(allocator, SMALL_SIZE);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  assert_that(first, is_not_equal_to(second));
  memset(first, FILL_A, SMALL_SIZE);
  memset(second, FILL_B, SMALL_SIZE);
  ra_free(allocator, first);
  memset(expect, FILL_B, sizeof(expect));
  assert_that(memcmp(second, expect, sizeof(expect)), is_equal_to(0));
  ra_free(allocator, second);
  assert_that(ra_allocate(allocator, SMALL_SIZE), is_non_null);
}

Ensure(reallocate_newest_only) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *allocator = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *second = nullptr;
  unsigned char *grown = nullptr;
  unsigned char expect[GROW_SIZE];

  assert_that(allocator, is_non_null);
  first = ra_allocate(allocator, SMALL_SIZE);
  memset(first, FILL_A, SMALL_SIZE);
  grown = ra_reallocate(allocator, first, GROW_SIZE);
  assert_that(grown, is_equal_to(first));
  memset(first, FILL_A, GROW_SIZE);
  second = ra_allocate(allocator, SMALL_SIZE);
  assert_that(second, is_non_null);
  assert_that(ra_reallocate(allocator, second, STORAGE_LENGTH), is_null);
  grown = ra_reallocate(allocator, second, GROW_SIZE);
  assert_that(grown, is_equal_to(second));
  memset(expect, FILL_A, sizeof(expect));
  assert_that(memcmp(first, expect, sizeof(expect)), is_equal_to(0));
}

Ensure(reallocate_failure_keeps_content) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *allocator = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *second = nullptr;
  unsigned char expect[SMALL_SIZE];

  assert_that(allocator, is_non_null);
  first = ra_allocate(allocator, SMALL_SIZE);
  second = ra_allocate(allocator, SMALL_SIZE);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  memset(second, FILL_B, SMALL_SIZE);
  assert_that(ra_reallocate(allocator, second, STORAGE_LENGTH), is_null);
  memset(expect, FILL_B, sizeof(expect));
  assert_that(memcmp(second, expect, sizeof(expect)), is_equal_to(0));
}

Ensure(reallocate_shrink_then_grow) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *allocator = ra_initialize(storage, sizeof(storage));
  unsigned char *block = nullptr;
  unsigned char expect[SMALL_SIZE];

  assert_that(allocator, is_non_null);
  block = ra_allocate(allocator, GROW_SIZE);
  assert_that(block, is_non_null);
  memset(block, FILL_C, GROW_SIZE);
  assert_that(ra_reallocate(allocator, block, SMALL_SIZE), is_equal_to(block));
  memset(expect, FILL_C, sizeof(expect));
  assert_that(memcmp(block, expect, sizeof(expect)), is_equal_to(0));
  assert_that(ra_reallocate(allocator, block, GROW_SIZE), is_equal_to(block));
  assert_that(memcmp(block, expect, sizeof(expect)), is_equal_to(0));
}

Ensure(wrap_reuses_oldest) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *allocator = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *second = nullptr;
  unsigned char *wrapped = nullptr;
  unsigned char expect[BLOCK_SIZE];

  assert_that(allocator, is_non_null);
  first = ra_allocate(allocator, BLOCK_SIZE);
  second = ra_allocate(allocator, BLOCK_SIZE);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  memset(first, FILL_A, BLOCK_SIZE);
  memset(second, FILL_B, BLOCK_SIZE);
  assert_that(ra_allocate(allocator, BLOCK_SIZE), is_null);
  ra_free(allocator, first);
  wrapped = ra_allocate(allocator, BLOCK_SIZE);
  assert_that(wrapped, is_equal_to(first));
  memset(wrapped, FILL_C, BLOCK_SIZE);
  memset(expect, FILL_B, sizeof(expect));
  assert_that(memcmp(second, expect, sizeof(expect)), is_equal_to(0));
  assert_that(ra_allocate(allocator, BLOCK_SIZE), is_null);
  ra_free(allocator, second);
  assert_that(ra_allocate(allocator, BLOCK_SIZE), is_non_null);
}

Ensure(wrap_keeps_alignment) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *allocator = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *second = nullptr;
  unsigned char *wrapped = nullptr;

  assert_that(allocator, is_non_null);
  first = ra_allocate(allocator, BLOCK_SIZE);
  second = ra_allocate(allocator, BLOCK_SIZE);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  assert_that(is_max_aligned(first), is_true);
  assert_that(is_max_aligned(second), is_true);
  ra_free(allocator, first);
  wrapped = ra_allocate(allocator, BLOCK_SIZE);
  assert_that(wrapped, is_equal_to(first));
  assert_that(is_max_aligned(wrapped), is_true);
}

Ensure(reallocate_wrapped_newest) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *allocator = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *second = nullptr;
  unsigned char *wrapped = nullptr;
  unsigned char expect[SMALL_SIZE];

  assert_that(allocator, is_non_null);
  first = ra_allocate(allocator, BLOCK_SIZE);
  second = ra_allocate(allocator, BLOCK_SIZE);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  ra_free(allocator, first);
  wrapped = ra_allocate(allocator, BLOCK_SIZE);
  assert_that(wrapped, is_equal_to(first));
  memset(wrapped, FILL_C, BLOCK_SIZE);
  assert_that(ra_reallocate(allocator, wrapped, SMALL_SIZE), is_equal_to(wrapped));
  memset(expect, FILL_C, sizeof(expect));
  assert_that(memcmp(wrapped, expect, sizeof(expect)), is_equal_to(0));
  assert_that(ra_reallocate(allocator, wrapped, BLOCK_SIZE), is_equal_to(wrapped));
}

Ensure(reallocate_wraps_newest) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *allocator = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *newest = nullptr;
  unsigned char *moved = nullptr;
  unsigned char *block = nullptr;
  unsigned char expect[SMALL_SIZE];

  assert_that(allocator, is_non_null);
  first = ra_allocate(allocator, BLOCK_SIZE);
  assert_that(first, is_non_null);
  newest = ra_allocate(allocator, SMALL_SIZE);
  assert_that(newest, is_non_null);
  while ((block = ra_allocate(allocator, SMALL_SIZE)) != nullptr) {
    newest = block;
  }
  memset(newest, FILL_C, SMALL_SIZE);
  ra_free(allocator, first);
  moved = ra_reallocate(allocator, newest, BLOCK_SIZE);
  assert_that(moved, is_non_null);
  assert_that(moved, is_not_equal_to(newest));
  memset(expect, FILL_C, sizeof(expect));
  assert_that(memcmp(moved, expect, sizeof(expect)), is_equal_to(0));
}

Ensure(reallocate_sole_block_fills_buffer) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *allocator = ra_initialize(storage, sizeof(storage));
  unsigned char *blocks[BLOCK_COUNT];
  unsigned char *sole = nullptr;
  unsigned char *filled = nullptr;
  size_t count = 0;
  size_t size = SMALL_SIZE;

  assert_that(allocator, is_non_null);
  blocks[count++] = ra_allocate(allocator, SMALL_SIZE);
  while (count < sizeof(blocks) / sizeof(blocks[0]) &&
         (blocks[count] = ra_allocate(allocator, SMALL_SIZE)) != nullptr) {
    count++;
  }
  assert_that(count, is_greater_than(1));
  for (size_t index = 0; index + 1 < count; index++) {
    ra_free(allocator, blocks[index]);
  }
  sole = blocks[count - 1];
  memset(sole, FILL_A, SMALL_SIZE);

  filled = sole;
  while (true) {
    unsigned char *grown = ra_reallocate(allocator, filled, size + SMALL_SIZE);
    if (grown == nullptr) break;
    filled = grown;
    size += SMALL_SIZE;
  }
  assert_that(filled, is_non_null);
  assert_that(filled, is_not_equal_to(sole));
  assert_that(filled[0], is_equal_to(FILL_A));
  assert_that(ra_allocate(allocator, 1), is_null);
}

Ensure(reallocate_wraps_only_live) {
  unsigned char storage[STORAGE_LENGTH];
  size_t capacity = 0;
  struct ringalloc *allocator = nullptr;
  unsigned char *first = nullptr;
  unsigned char *newest = nullptr;
  unsigned char *moved = nullptr;
  unsigned char expect[SMALL_SIZE];
  bool wrapped = false;

  for (; capacity <= sizeof(storage); capacity += 1) {
    allocator = ra_initialize(storage, capacity);
    if (allocator == nullptr) continue;

    first = ra_allocate(allocator, BLOCK_SIZE);
    newest = ra_allocate(allocator, SMALL_SIZE);
    if (first == nullptr || newest == nullptr) continue;

    ra_free(allocator, first);
    memset(newest, FILL_C, SMALL_SIZE);
    moved = ra_reallocate(allocator, newest, BLOCK_SIZE);
    if (moved == nullptr || moved == newest) continue;

    wrapped = true;
    break;
  }

  assert_that(wrapped, is_true);
  memset(expect, FILL_C, sizeof(expect));
  assert_that(memcmp(moved, expect, sizeof(expect)), is_equal_to(0));
}

Ensure(reset_drops_live) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *allocator = ra_initialize(storage, sizeof(storage));
  size_t count = 0;
  size_t again = 0;

  assert_that(allocator, is_non_null);
  while (ra_allocate(allocator, SMALL_SIZE) != nullptr) {
    count++;
  }
  assert_that(count, is_greater_than(0));
  ra_reset(allocator);
  while (ra_allocate(allocator, SMALL_SIZE) != nullptr) {
    again++;
  }
  assert_that(again, is_equal_to(count));
}

int main() {
  auto suite = create_test_suite();
  add_test(suite, initialize_too_small);
  add_test(suite, initialize_smallest);
  add_test(suite, initialize_unaligned);
  add_test(suite, initialize_zero_ring);
  add_test(suite, allocate_zero);
  add_test(suite, allocate_aligned);
  add_test(suite, allocate_odd_sizes);
  add_test(suite, allocate_and_free_oldest);
  add_test(suite, reallocate_newest_only);
  add_test(suite, reallocate_failure_keeps_content);
  add_test(suite, reallocate_zero);
  add_test(suite, reallocate_shrink_then_grow);
  add_test(suite, wrap_reuses_oldest);
  add_test(suite, wrap_keeps_alignment);
  add_test(suite, reallocate_wrapped_newest);
  add_test(suite, reallocate_wraps_newest);
  add_test(suite, reallocate_sole_block_fills_buffer);
  add_test(suite, reallocate_wraps_only_live);
  add_test(suite, free_null);
  add_test(suite, reset_drops_live);
  auto reporter = create_text_reporter();
  int result = run_test_suite(suite, reporter);
  destroy_test_suite(suite);
  destroy_reporter(reporter);
  return result;
}
