#include "ringalloc.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum : size_t { ALIGN = alignof(max_align_t) };

static_assert((ALIGN & (ALIGN - 1U)) == 0);

enum : size_t {
  ITEM_HDR = (sizeof(size_t) + (ALIGN - 1U)) & ~(ALIGN - 1U),
};

struct ringalloc {
  uint8_t *base;
  size_t cap;
  size_t head;
  size_t tail;
  size_t newest;
  size_t wrap;
  size_t live_cnt;
};

struct item {
  size_t size;
};

static size_t align_up(size_t nbytes, size_t align) {
  return (nbytes + (align - 1U)) & ~(align - 1U);
}

static bool item_bytes(size_t size, size_t *bytes) {
  size_t total = 0;
  if (size > ((size_t)-1) - ITEM_HDR) {
    return false;
  }
  total = ITEM_HDR + size;
  if (total > ((size_t)-1) - (ALIGN - 1U)) {
    return false;
  }
  *bytes = align_up(total, ALIGN);
  return true;
}

static struct item *item_at(struct ringalloc *ringalloc, size_t offset) {
  return (struct item *)(ringalloc->base + offset);
}

static void *user_at(struct ringalloc *ringalloc, size_t offset) {
  return ringalloc->base + offset + ITEM_HDR;
}

static size_t gap_after_head(const struct ringalloc *ringalloc) {
  if (ringalloc->live_cnt == 0) {
    return ringalloc->cap;
  }
  if (ringalloc->head < ringalloc->tail) {
    return ringalloc->tail - ringalloc->head;
  }
  if (ringalloc->wrap != 0) {
    return 0;
  }
  return ringalloc->cap - ringalloc->head;
}

static size_t align_gap(const uint8_t *ptr, size_t align) {
  size_t rem = (uintptr_t)ptr % align;
  return (rem == 0) ? 0 : align - rem;
}

struct ringalloc *ringalloc_init(void *buf, size_t cap) {
  uint8_t *raw = nullptr;
  size_t skip = 0;
  size_t after = 0;
  size_t ring_skip = 0;
  size_t ring_cap = 0;
  struct ringalloc *ringalloc = nullptr;

  if (buf == nullptr) abort();
  raw = (uint8_t *)buf;
  skip = align_gap(raw, alignof(struct ringalloc));
  if (skip > cap || sizeof(struct ringalloc) > cap - skip) {
    return nullptr;
  }
  after = skip + sizeof(struct ringalloc);
  ring_skip = align_gap(raw + after, ALIGN);
  if (ring_skip > cap - after) {
    return nullptr;
  }
  ring_cap = cap - after - ring_skip;
  if (ring_cap < ITEM_HDR) {
    return nullptr;
  }
  ringalloc = (struct ringalloc *)(raw + skip);
  memset(ringalloc, 0, sizeof *ringalloc);
  ringalloc->base = raw + after + ring_skip;
  ringalloc->cap = ring_cap;
  ringalloc_reset(ringalloc);
  return ringalloc;
}

void ringalloc_reset(struct ringalloc *ringalloc) {
  if (ringalloc == nullptr) abort();
  ringalloc->head = 0;
  ringalloc->tail = 0;
  ringalloc->newest = 0;
  ringalloc->wrap = 0;
  ringalloc->live_cnt = 0;
}

void *ringalloc_alloc(struct ringalloc *ringalloc, size_t size) {
  size_t block = 0;
  size_t offset = 0;

  if (ringalloc == nullptr) abort();
  if (!item_bytes(size, &block)) {
    return nullptr;
  }
  if (block > ringalloc->cap) {
    return nullptr;
  }
  if (ringalloc->live_cnt == 0) {
    ringalloc_reset(ringalloc);
  }

  if (gap_after_head(ringalloc) >= block) {
    offset = ringalloc->head;
  } else if (ringalloc->wrap == 0 && ringalloc->head > ringalloc->tail &&
             ringalloc->tail >= block) {
    ringalloc->wrap = ringalloc->head;
    offset = 0;
  } else {
    return nullptr;
  }
  item_at(ringalloc, offset)->size = size;
  ringalloc->newest = offset;
  ringalloc->head = offset + block;
  ringalloc->live_cnt += 1;
  return user_at(ringalloc, offset);
}

void ringalloc_free(struct ringalloc *ringalloc, void *ptr) {
  size_t block = 0;
  struct item *item = nullptr;

  if (ringalloc == nullptr) abort();
  if (ptr == nullptr) abort();
  if (ringalloc->live_cnt == 0) abort();
  if (ptr != user_at(ringalloc, ringalloc->tail)) abort();
  item = item_at(ringalloc, ringalloc->tail);
  if (!item_bytes(item->size, &block)) abort();
  ringalloc->tail += block;
  ringalloc->live_cnt -= 1;
  if (ringalloc->wrap != 0 && ringalloc->tail == ringalloc->wrap) {
    ringalloc->tail = 0;
    ringalloc->wrap = 0;
  }
  if (ringalloc->live_cnt == 0) {
    ringalloc_reset(ringalloc);
  }
}

void *ringalloc_realloc(struct ringalloc *ringalloc, void *ptr, size_t size) {
  size_t old_block = 0;
  size_t new_block = 0;
  size_t extra = 0;
  struct item *item = nullptr;

  if (ringalloc == nullptr) abort();
  if (ptr == nullptr) abort();
  if (ringalloc->live_cnt == 0) abort();
  if (ptr != user_at(ringalloc, ringalloc->newest)) abort();
  item = item_at(ringalloc, ringalloc->newest);
  if (!item_bytes(item->size, &old_block)) abort();
  if (!item_bytes(size, &new_block)) {
    return nullptr;
  }
  if (new_block <= old_block) {
    item->size = size;
    ringalloc->head = ringalloc->newest + new_block;
    return ptr;
  }
  extra = new_block - old_block;
  if (gap_after_head(ringalloc) < extra) {
    return nullptr;
  }
  item->size = size;
  ringalloc->head += extra;
  return ptr;
}
