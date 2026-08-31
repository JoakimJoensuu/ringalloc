#include <ringalloc.h>

#include <cgreen/assertions.h>
#include <cgreen/constraint_syntax_helpers.h>
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
  TIGHT_STORAGE  = 512,
  MISALIGNMENT   = 1,
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
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof(storage));
  void *zero = nullptr;

  assert_that(ringalloc, is_non_null);
  zero = ra_allocate(ringalloc, 0);
  assert_that(zero, is_non_null);
  assert_that(ra_allocate(ringalloc, SMALL_SIZE), is_non_null);
}

Ensure(reallocate_zero) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof(storage));
  unsigned char *block = nullptr;
  assert_that(ringalloc, is_non_null);
  block = ra_allocate(ringalloc, SMALL_SIZE);
  assert_that(block, is_non_null);
  block = ra_reallocate(ringalloc, block, 0);
  assert_that(block, is_non_null);
  assert_that(ra_reallocate(ringalloc, block, GROW_SIZE), is_equal_to(block));
}

Ensure(free_null) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *second = nullptr;

  assert_that(ringalloc, is_non_null);
  first = ra_allocate(ringalloc, SMALL_SIZE);
  second = ra_allocate(ringalloc, SMALL_SIZE);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  ra_free(ringalloc, nullptr);
  ra_free(ringalloc, first);
  assert_that(second, is_non_null);
}

Ensure(initialize_too_small) {
  unsigned char storage[TINY_LENGTH];
  assert_that(ra_initialize(storage, sizeof(storage)), is_null);
}

Ensure(initialize_smallest) {
  unsigned char storage[STORAGE_LENGTH];
  size_t capacity = 0;
  struct ringalloc *ringalloc = nullptr;

  for (; capacity <= sizeof(storage); capacity += 1) {
    ringalloc = ra_initialize(storage, capacity);
    if (ringalloc != nullptr) break;
  }
  assert_that(ringalloc, is_non_null);
  assert_that(ra_allocate(ringalloc, 0), is_null);
  assert_that(ra_allocate(ringalloc, SMALL_SIZE), is_null);
  assert_that(capacity, is_greater_than(0));
  assert_that(ra_initialize(storage, capacity - 1), is_null);
}

Ensure(initialize_unaligned) {
  alignas(max_align_t) unsigned char storage[STORAGE_LENGTH + MISALIGNMENT];
  struct ringalloc *ringalloc = ra_initialize(storage + MISALIGNMENT, STORAGE_LENGTH);

  assert_that(ringalloc, is_non_null);
  assert_that(ra_allocate(ringalloc, SMALL_SIZE), is_non_null);
}

Ensure(allocate_aligned) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof(storage));
  void *first = nullptr;
  void *second = nullptr;

  assert_that(ringalloc, is_non_null);
  first = ra_allocate(ringalloc, 1);
  second = ra_allocate(ringalloc, ODD_SIZE);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  assert_that(is_max_aligned(first), is_true);
  assert_that(is_max_aligned(second), is_true);
}

Ensure(allocate_odd_sizes) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *second = nullptr;
  unsigned char *third = nullptr;

  assert_that(ringalloc, is_non_null);
  first = ra_allocate(ringalloc, 1);
  second = ra_allocate(ringalloc, ODD_SIZE);
  third = ra_allocate(ringalloc, 1);
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
  struct ringalloc *ringalloc = nullptr;
  size_t capacity = 0;

  for (; capacity <= STORAGE_LENGTH; capacity += 1) {
    ringalloc = ra_initialize(buffer, capacity);
    if (ringalloc == nullptr) continue;
    if (ra_allocate(ringalloc, SMALL_SIZE) == nullptr) continue;
    assert_that(capacity, is_greater_than(1));
    ringalloc = ra_initialize(buffer, capacity - 1);
    assert_that(ringalloc, is_non_null);
    assert_that(ra_allocate(ringalloc, SMALL_SIZE), is_null);
    return;
  }
  assert_that(false, is_true);
}

Ensure(allocate_and_free_oldest) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *second = nullptr;
  unsigned char expect[SMALL_SIZE];

  assert_that(ringalloc, is_non_null);
  first = ra_allocate(ringalloc, SMALL_SIZE);
  second = ra_allocate(ringalloc, SMALL_SIZE);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  assert_that(first, is_not_equal_to(second));
  memset(first, FILL_A, SMALL_SIZE);
  memset(second, FILL_B, SMALL_SIZE);
  ra_free(ringalloc, first);
  memset(expect, FILL_B, sizeof(expect));
  assert_that(memcmp(second, expect, sizeof(expect)), is_equal_to(0));
  ra_free(ringalloc, second);
  assert_that(ra_allocate(ringalloc, SMALL_SIZE), is_non_null);
}

Ensure(reallocate_newest_only) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *second = nullptr;
  unsigned char *grown = nullptr;
  unsigned char expect[GROW_SIZE];

  assert_that(ringalloc, is_non_null);
  first = ra_allocate(ringalloc, SMALL_SIZE);
  memset(first, FILL_A, SMALL_SIZE);
  grown = ra_reallocate(ringalloc, first, GROW_SIZE);
  assert_that(grown, is_equal_to(first));
  memset(first, FILL_A, GROW_SIZE);
  second = ra_allocate(ringalloc, SMALL_SIZE);
  assert_that(second, is_non_null);
  assert_that(ra_reallocate(ringalloc, second, STORAGE_LENGTH), is_null);
  grown = ra_reallocate(ringalloc, second, GROW_SIZE);
  assert_that(grown, is_equal_to(second));
  memset(expect, FILL_A, sizeof(expect));
  assert_that(memcmp(first, expect, sizeof(expect)), is_equal_to(0));
}

Ensure(reallocate_failure_keeps_content) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *second = nullptr;
  unsigned char expect[SMALL_SIZE];

  assert_that(ringalloc, is_non_null);
  first = ra_allocate(ringalloc, SMALL_SIZE);
  second = ra_allocate(ringalloc, SMALL_SIZE);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  memset(second, FILL_B, SMALL_SIZE);
  assert_that(ra_reallocate(ringalloc, second, STORAGE_LENGTH), is_null);
  memset(expect, FILL_B, sizeof(expect));
  assert_that(memcmp(second, expect, sizeof(expect)), is_equal_to(0));
}

Ensure(reallocate_shrink_then_grow) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof(storage));
  unsigned char *block = nullptr;
  unsigned char expect[SMALL_SIZE];

  assert_that(ringalloc, is_non_null);
  block = ra_allocate(ringalloc, GROW_SIZE);
  assert_that(block, is_non_null);
  memset(block, FILL_C, GROW_SIZE);
  assert_that(ra_reallocate(ringalloc, block, SMALL_SIZE), is_equal_to(block));
  memset(expect, FILL_C, sizeof(expect));
  assert_that(memcmp(block, expect, sizeof(expect)), is_equal_to(0));
  assert_that(ra_reallocate(ringalloc, block, GROW_SIZE), is_equal_to(block));
  assert_that(memcmp(block, expect, sizeof(expect)), is_equal_to(0));
}

Ensure(wrap_reuses_oldest) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *second = nullptr;
  unsigned char *wrapped = nullptr;
  unsigned char expect[BLOCK_SIZE];

  assert_that(ringalloc, is_non_null);
  first = ra_allocate(ringalloc, BLOCK_SIZE);
  second = ra_allocate(ringalloc, BLOCK_SIZE);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  memset(first, FILL_A, BLOCK_SIZE);
  memset(second, FILL_B, BLOCK_SIZE);
  assert_that(ra_allocate(ringalloc, BLOCK_SIZE), is_null);
  ra_free(ringalloc, first);
  wrapped = ra_allocate(ringalloc, BLOCK_SIZE);
  assert_that(wrapped, is_equal_to(first));
  memset(wrapped, FILL_C, BLOCK_SIZE);
  memset(expect, FILL_B, sizeof(expect));
  assert_that(memcmp(second, expect, sizeof(expect)), is_equal_to(0));
  assert_that(ra_allocate(ringalloc, BLOCK_SIZE), is_null);
  ra_free(ringalloc, second);
  assert_that(ra_allocate(ringalloc, BLOCK_SIZE), is_non_null);
}

Ensure(wrap_keeps_alignment) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *second = nullptr;
  unsigned char *wrapped = nullptr;

  assert_that(ringalloc, is_non_null);
  first = ra_allocate(ringalloc, BLOCK_SIZE);
  second = ra_allocate(ringalloc, BLOCK_SIZE);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  assert_that(is_max_aligned(first), is_true);
  assert_that(is_max_aligned(second), is_true);
  ra_free(ringalloc, first);
  wrapped = ra_allocate(ringalloc, BLOCK_SIZE);
  assert_that(wrapped, is_equal_to(first));
  assert_that(is_max_aligned(wrapped), is_true);
}

Ensure(reallocate_wrapped_newest) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *second = nullptr;
  unsigned char *wrapped = nullptr;
  unsigned char expect[SMALL_SIZE];

  assert_that(ringalloc, is_non_null);
  first = ra_allocate(ringalloc, BLOCK_SIZE);
  second = ra_allocate(ringalloc, BLOCK_SIZE);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  ra_free(ringalloc, first);
  wrapped = ra_allocate(ringalloc, BLOCK_SIZE);
  assert_that(wrapped, is_equal_to(first));
  memset(wrapped, FILL_C, BLOCK_SIZE);
  assert_that(ra_reallocate(ringalloc, wrapped, SMALL_SIZE), is_equal_to(wrapped));
  memset(expect, FILL_C, sizeof(expect));
  assert_that(memcmp(wrapped, expect, sizeof(expect)), is_equal_to(0));
  assert_that(ra_reallocate(ringalloc, wrapped, BLOCK_SIZE), is_equal_to(wrapped));
}

Ensure(reallocate_wraps_newest) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *newest = nullptr;
  unsigned char *moved = nullptr;
  unsigned char *block = nullptr;
  unsigned char expect[SMALL_SIZE];

  assert_that(ringalloc, is_non_null);
  first = ra_allocate(ringalloc, BLOCK_SIZE);
  assert_that(first, is_non_null);
  newest = ra_allocate(ringalloc, SMALL_SIZE);
  assert_that(newest, is_non_null);
  while ((block = ra_allocate(ringalloc, SMALL_SIZE)) != nullptr) {
    newest = block;
  }
  memset(newest, FILL_C, SMALL_SIZE);
  ra_free(ringalloc, first);
  moved = ra_reallocate(ringalloc, newest, BLOCK_SIZE);
  assert_that(moved, is_non_null);
  assert_that(moved, is_not_equal_to(newest));
  memset(expect, FILL_C, sizeof(expect));
  assert_that(memcmp(moved, expect, sizeof(expect)), is_equal_to(0));
}

Ensure(reallocate_sole_block_fills_buffer) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof(storage));
  unsigned char *blocks[64];
  unsigned char *sole = nullptr;
  unsigned char *filled = nullptr;
  size_t count = 0;
  size_t size = SMALL_SIZE;

  assert_that(ringalloc, is_non_null);
  blocks[count++] = ra_allocate(ringalloc, SMALL_SIZE);
  while (count < sizeof(blocks) / sizeof(blocks[0]) &&
         (blocks[count] = ra_allocate(ringalloc, SMALL_SIZE)) != nullptr) {
    count++;
  }
  assert_that(count, is_greater_than(1));
  for (size_t index = 0; index + 1 < count; index++) {
    ra_free(ringalloc, blocks[index]);
  }
  sole = blocks[count - 1];
  memset(sole, FILL_A, SMALL_SIZE);

  filled = sole;
  while (true) {
    unsigned char *grown = ra_reallocate(ringalloc, filled, size + SMALL_SIZE);
    if (grown == nullptr) break;
    filled = grown;
    size += SMALL_SIZE;
  }
  assert_that(filled, is_non_null);
  assert_that(filled, is_not_equal_to(sole));
  assert_that(((unsigned char *)filled)[0], is_equal_to(FILL_A));
  assert_that(ra_allocate(ringalloc, 1), is_null);
}

Ensure(reallocate_wraps_only_live) {
  unsigned char storage[TIGHT_STORAGE];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof(storage));
  unsigned char *first = nullptr;
  unsigned char *newest = nullptr;
  unsigned char *moved = nullptr;
  unsigned char expect[SMALL_SIZE];

  assert_that(ringalloc, is_non_null);
  first = ra_allocate(ringalloc, BLOCK_SIZE);
  newest = ra_allocate(ringalloc, SMALL_SIZE);
  assert_that(first, is_non_null);
  assert_that(newest, is_non_null);
  ra_free(ringalloc, first);
  memset(newest, FILL_C, SMALL_SIZE);
  moved = ra_reallocate(ringalloc, newest, BLOCK_SIZE);
  assert_that(moved, is_non_null);
  assert_that(moved, is_not_equal_to(newest));
  memset(expect, FILL_C, sizeof(expect));
  assert_that(memcmp(moved, expect, sizeof(expect)), is_equal_to(0));
}

Ensure(reset_drops_live) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof(storage));
  unsigned char *again = nullptr;
  unsigned char expect[SMALL_SIZE];

  assert_that(ringalloc, is_non_null);
  assert_that(ra_allocate(ringalloc, SMALL_SIZE), is_non_null);
  assert_that(ra_allocate(ringalloc, SMALL_SIZE), is_non_null);
  ra_reset(ringalloc);
  again = ra_allocate(ringalloc, SMALL_SIZE);
  assert_that(again, is_non_null);
  memset(again, FILL_D, SMALL_SIZE);
  memset(expect, FILL_D, sizeof(expect));
  assert_that(memcmp(again, expect, sizeof(expect)), is_equal_to(0));
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
  return run_test_suite(suite, create_text_reporter());
}
