#include <ringalloc.h>

#include <cgreen/assertions.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/runner.h>
#include <cgreen/suite.h>
#include <cgreen/text_reporter.h>
#include <cgreen/unit.h>
#include <stddef.h>
#include <string.h>

enum : size_t {
  TINY_LENGTH    = 1,
  SMALL_SIZE     = 8,
  GROW_SIZE      = 32,
  BLOCK_SIZE     = 400,
  STORAGE_LENGTH = 1024,
  MISALIGNMENT   = 1,
};

enum : unsigned char {
  FILL_A = 0x11,
  FILL_B = 0x22,
  FILL_C = 0x33,
  FILL_D = 0x44,
};

Ensure(allocate_zero) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof storage);
  void *zero = nullptr;

  assert_that(ringalloc, is_non_null);
  zero = ra_allocate(ringalloc, 0);
  assert_that(zero, is_non_null);
  assert_that(ra_allocate(ringalloc, SMALL_SIZE), is_non_null);
}

Ensure(reallocate_zero) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof storage);
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
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof storage);
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
  assert_that(ra_initialize(storage, sizeof storage), is_null);
}

Ensure(initialize_smallest) {
  unsigned char storage[STORAGE_LENGTH];
  size_t capacity = 0;
  struct ringalloc *ringalloc = nullptr;

  for (; capacity <= sizeof storage; capacity += 1) {
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
  unsigned char storage[STORAGE_LENGTH + MISALIGNMENT];
  struct ringalloc *ringalloc = ra_initialize(storage + MISALIGNMENT, STORAGE_LENGTH);

  assert_that(ringalloc, is_non_null);
  assert_that(ra_allocate(ringalloc, SMALL_SIZE), is_non_null);
}

Ensure(allocate_and_free_oldest) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof storage);
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
  memset(expect, FILL_B, sizeof expect);
  assert_that(memcmp(second, expect, sizeof expect), is_equal_to(0));
  ra_free(ringalloc, second);
  assert_that(ra_allocate(ringalloc, SMALL_SIZE), is_non_null);
}

Ensure(reallocate_newest_only) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof storage);
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
  memset(expect, FILL_A, sizeof expect);
  assert_that(memcmp(first, expect, sizeof expect), is_equal_to(0));
}

Ensure(reallocate_shrink_then_grow) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof storage);
  unsigned char *block = nullptr;
  unsigned char expect[SMALL_SIZE];

  assert_that(ringalloc, is_non_null);
  block = ra_allocate(ringalloc, GROW_SIZE);
  assert_that(block, is_non_null);
  memset(block, FILL_C, GROW_SIZE);
  assert_that(ra_reallocate(ringalloc, block, SMALL_SIZE), is_equal_to(block));
  memset(expect, FILL_C, sizeof expect);
  assert_that(memcmp(block, expect, sizeof expect), is_equal_to(0));
  assert_that(ra_reallocate(ringalloc, block, GROW_SIZE), is_equal_to(block));
  assert_that(memcmp(block, expect, sizeof expect), is_equal_to(0));
}

Ensure(wrap_reuses_oldest) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof storage);
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
  memset(expect, FILL_B, sizeof expect);
  assert_that(memcmp(second, expect, sizeof expect), is_equal_to(0));
  assert_that(ra_allocate(ringalloc, BLOCK_SIZE), is_null);
  ra_free(ringalloc, second);
  assert_that(ra_allocate(ringalloc, BLOCK_SIZE), is_non_null);
}

Ensure(reallocate_wrapped_newest) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof storage);
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
  memset(expect, FILL_C, sizeof expect);
  assert_that(memcmp(wrapped, expect, sizeof expect), is_equal_to(0));
  assert_that(ra_reallocate(ringalloc, wrapped, BLOCK_SIZE), is_equal_to(wrapped));
}

Ensure(reallocate_wraps_newest) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof storage);
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
  memset(expect, FILL_C, sizeof expect);
  assert_that(memcmp(moved, expect, sizeof expect), is_equal_to(0));
}

Ensure(reset_drops_live) {
  unsigned char storage[STORAGE_LENGTH];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof storage);
  unsigned char *again = nullptr;
  unsigned char expect[SMALL_SIZE];

  assert_that(ringalloc, is_non_null);
  assert_that(ra_allocate(ringalloc, SMALL_SIZE), is_non_null);
  assert_that(ra_allocate(ringalloc, SMALL_SIZE), is_non_null);
  ra_reset(ringalloc);
  again = ra_allocate(ringalloc, SMALL_SIZE);
  assert_that(again, is_non_null);
  memset(again, FILL_D, SMALL_SIZE);
  memset(expect, FILL_D, sizeof expect);
  assert_that(memcmp(again, expect, sizeof expect), is_equal_to(0));
}

int main() {
  auto suite = create_test_suite();
  add_test(suite, initialize_too_small);
  add_test(suite, initialize_smallest);
  add_test(suite, initialize_unaligned);
  add_test(suite, allocate_zero);
  add_test(suite, allocate_and_free_oldest);
  add_test(suite, reallocate_newest_only);
  add_test(suite, reallocate_zero);
  add_test(suite, reallocate_shrink_then_grow);
  add_test(suite, wrap_reuses_oldest);
  add_test(suite, reallocate_wrapped_newest);
  add_test(suite, reallocate_wraps_newest);
  add_test(suite, free_null);
  add_test(suite, reset_drops_live);
  return run_test_suite(suite, create_text_reporter());
}
