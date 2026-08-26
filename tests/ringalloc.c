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
  TINY_LEN    = 1,
  SMALL_LEN   = 8,
  GROW_LEN    = 32,
  BLOCK_LEN   = 400,
  STORAGE_LEN = 1024,
};

enum : uint8_t {
  FILL_A = 0x11,
  FILL_B = 0x22,
  FILL_C = 0x33,
  FILL_D = 0x44,
};

Ensure(init_too_small) {
  uint8_t storage[TINY_LEN];
  assert_that(ringalloc_init(storage, sizeof storage), is_null);
}

Ensure(alloc_and_free_oldest) {
  uint8_t storage[STORAGE_LEN];
  struct ringalloc *ringalloc = ringalloc_init(storage, sizeof storage);
  uint8_t *first = nullptr;
  uint8_t *second = nullptr;
  uint8_t expect[SMALL_LEN];

  assert_that(ringalloc, is_non_null);
  first = ringalloc_alloc(ringalloc, SMALL_LEN);
  second = ringalloc_alloc(ringalloc, SMALL_LEN);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  assert_that(first, is_not_equal_to(second));
  memset(first, FILL_A, SMALL_LEN);
  memset(second, FILL_B, SMALL_LEN);
  ringalloc_free(ringalloc);
  memset(expect, FILL_B, sizeof expect);
  assert_that(memcmp(second, expect, sizeof expect), is_equal_to(0));
  ringalloc_free(ringalloc);
  assert_that(ringalloc_alloc(ringalloc, SMALL_LEN), is_non_null);
}

Ensure(realloc_newest_only) {
  uint8_t storage[STORAGE_LEN];
  struct ringalloc *ringalloc = ringalloc_init(storage, sizeof storage);
  uint8_t *first = nullptr;
  uint8_t *second = nullptr;
  uint8_t *grown = nullptr;
  uint8_t expect[GROW_LEN];

  assert_that(ringalloc, is_non_null);
  first = ringalloc_alloc(ringalloc, SMALL_LEN);
  memset(first, FILL_A, SMALL_LEN);
  grown = ringalloc_realloc(ringalloc, first, GROW_LEN);
  assert_that(grown, is_equal_to(first));
  memset(first, FILL_A, GROW_LEN);
  second = ringalloc_alloc(ringalloc, SMALL_LEN);
  assert_that(second, is_non_null);
  assert_that(ringalloc_realloc(ringalloc, first, GROW_LEN + SMALL_LEN), is_null);
  grown = ringalloc_realloc(ringalloc, second, GROW_LEN);
  assert_that(grown, is_equal_to(second));
  memset(expect, FILL_A, sizeof expect);
  assert_that(memcmp(first, expect, sizeof expect), is_equal_to(0));
}

Ensure(realloc_shrink_then_grow) {
  uint8_t storage[STORAGE_LEN];
  struct ringalloc *ringalloc = ringalloc_init(storage, sizeof storage);
  uint8_t *block = nullptr;
  uint8_t expect[SMALL_LEN];

  assert_that(ringalloc, is_non_null);
  block = ringalloc_alloc(ringalloc, GROW_LEN);
  assert_that(block, is_non_null);
  memset(block, FILL_C, GROW_LEN);
  assert_that(ringalloc_realloc(ringalloc, block, SMALL_LEN), is_equal_to(block));
  memset(expect, FILL_C, sizeof expect);
  assert_that(memcmp(block, expect, sizeof expect), is_equal_to(0));
  assert_that(ringalloc_realloc(ringalloc, block, GROW_LEN), is_equal_to(block));
  assert_that(memcmp(block, expect, sizeof expect), is_equal_to(0));
}

Ensure(wrap_reuses_oldest) {
  uint8_t storage[STORAGE_LEN];
  struct ringalloc *ringalloc = ringalloc_init(storage, sizeof storage);
  uint8_t *first = nullptr;
  uint8_t *second = nullptr;
  uint8_t *wrapped = nullptr;
  uint8_t expect[BLOCK_LEN];

  assert_that(ringalloc, is_non_null);
  first = ringalloc_alloc(ringalloc, BLOCK_LEN);
  second = ringalloc_alloc(ringalloc, BLOCK_LEN);
  assert_that(first, is_non_null);
  assert_that(second, is_non_null);
  memset(first, FILL_A, BLOCK_LEN);
  memset(second, FILL_B, BLOCK_LEN);
  assert_that(ringalloc_alloc(ringalloc, BLOCK_LEN), is_null);
  ringalloc_free(ringalloc);
  wrapped = ringalloc_alloc(ringalloc, BLOCK_LEN);
  assert_that(wrapped, is_equal_to(first));
  memset(wrapped, FILL_C, BLOCK_LEN);
  memset(expect, FILL_B, sizeof expect);
  assert_that(memcmp(second, expect, sizeof expect), is_equal_to(0));
  assert_that(ringalloc_alloc(ringalloc, BLOCK_LEN), is_null);
}

Ensure(reset_drops_live) {
  uint8_t storage[STORAGE_LEN];
  struct ringalloc *ringalloc = ringalloc_init(storage, sizeof storage);
  uint8_t *again = nullptr;
  uint8_t expect[SMALL_LEN];

  assert_that(ringalloc, is_non_null);
  assert_that(ringalloc_alloc(ringalloc, SMALL_LEN), is_non_null);
  assert_that(ringalloc_alloc(ringalloc, SMALL_LEN), is_non_null);
  ringalloc_reset(ringalloc);
  again = ringalloc_alloc(ringalloc, SMALL_LEN);
  assert_that(again, is_non_null);
  memset(again, FILL_D, SMALL_LEN);
  memset(expect, FILL_D, sizeof expect);
  assert_that(memcmp(again, expect, sizeof expect), is_equal_to(0));
}

int main() {
  auto suite = create_test_suite();
  add_test(suite, init_too_small);
  add_test(suite, alloc_and_free_oldest);
  add_test(suite, realloc_newest_only);
  add_test(suite, realloc_shrink_then_grow);
  add_test(suite, wrap_reuses_oldest);
  add_test(suite, reset_drops_live);
  return run_test_suite(suite, create_text_reporter());
}
