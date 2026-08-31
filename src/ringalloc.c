#include "ringalloc.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct header {
  size_t size;
};

/**
 *   ----------------------++--------+-----------------+---------+------------------++------------
 *   prev payload_padding  || header | header_padding  | payload | payload_padding  || next header
 *   ----------------------++--------+-----------------+---------+------------------++------------
 */
struct frame_layout {
  size_t header_size;
  size_t header_padding;
  size_t payload_size;
  size_t payload_padding;
};

struct frame {
  unsigned char *start;
  struct frame_layout layout;
};

static struct header load_header(const unsigned char *start) {
  struct header header;
  memcpy(&header, start, sizeof(header));
  return header;
}

static void store_header(unsigned char *start, const struct header *header) {
  memcpy(start, header, sizeof(*header));
}

static void store_header_size(unsigned char *start, size_t size) {
  store_header(start, &(struct header){.size = size});
}

/**
 * A, B, C and D are allocated:
 *
 *                    caller provided buffer
 *                              v
 *   +-------------------------------------------------------+
 *   v     allocator state               ring buffer         v
 *               v                            v
 *   +---+------------------+---+<---------capacity--------->+
 *   |pad| struct ringalloc |pad|[    A    ][B][ C  ][ D ]   |
 *   +---+------------------+---+----------------------------+
 *                               ^                   ^    ^
 *                               base                last next
 *                               first
 *                               wrap = nullptr
 *
 * A is freed:
 *
 *   +---+------------------+---+<---------capacity--------->+
 *   |pad| struct ringalloc |pad|           [B][ C  ][ D ]   |
 *   +---+------------------+---+----------------------------+
 *                               ^          ^        ^    ^
 *                               base       first    last next
 *                               wrap = nullptr
 *
 * E wraps:
 *
 *   +---+------------------+---+<---------capacity--------->+
 *   |pad| struct ringalloc |pad|[ E  ]     [B][ C  ][ D ]   |
 *   +---+------------------+---+----------------------------+
 *                               ^     ^    ^             ^
 *                               base  next first         wrap
 *                               last
 */
struct ringalloc {
  unsigned char *base;
  size_t capacity;
  unsigned char *first;
  unsigned char *last;
  unsigned char *next;
  unsigned char *wrap;
  bool empty;
};

static struct ringalloc load_state(const struct ringalloc *handle) {
  struct ringalloc state;
  memcpy(&state, handle, sizeof(state));
  return state;
}

static void store_state(struct ringalloc *handle, const struct ringalloc *state) {
  memcpy(handle, state, sizeof(*state));
}

static size_t alignment_padding(size_t length, size_t alignment) {
  size_t mask = alignment - 1U;
  return (alignment - (length & mask)) & mask;
}

static size_t space_between(const unsigned char *start, const unsigned char *end) {
  if (start > end) return 0;
  return (size_t)(end - start);
}

static struct frame_layout frame_layout(size_t payload_size) {
  size_t alignment = alignof(max_align_t);
  size_t header_size = sizeof(struct header);

  return (struct frame_layout){
      .header_size = header_size,
      .header_padding = alignment_padding(header_size, alignment),
      .payload_size = payload_size,
      .payload_padding = alignment_padding(payload_size, alignment),
  };
}

static struct frame frame(unsigned char *start, size_t payload_size) {
  struct frame_layout layout = frame_layout(payload_size);
  store_header_size(start, layout.payload_size);
  return (struct frame){
      .start = start,
      .layout = layout,
  };
}

static struct frame existing_frame(unsigned char *start) {
  struct header header = load_header(start);
  return (struct frame){
      .start = start,
      .layout = frame_layout(header.size),
  };
}

static size_t header_length(struct frame_layout layout) {
  return layout.header_size + layout.header_padding;
}

static bool frame_layout_fits(struct frame_layout layout, size_t limit) {
  size_t header = header_length(layout);
  if (header > limit) return false;
  if (layout.payload_size > limit - header) return false;
  if (layout.payload_padding > limit - header - layout.payload_size) return false;
  return true;
}

static size_t frame_length(struct frame_layout layout) {
  return layout.header_size + layout.header_padding + layout.payload_size + layout.payload_padding;
}

static void *payload_address(struct frame frame) {
  return frame.start + header_length(frame.layout);
}

static bool has_wrapped(const struct ringalloc *ringalloc) {
  return ringalloc->wrap != nullptr;
}

static bool has_one_frame(const struct ringalloc *ringalloc) {
  return !ringalloc->empty && ringalloc->first == ringalloc->last;
}

static size_t room_at_next(const struct ringalloc *ringalloc) {
  if (!has_wrapped(ringalloc)) {
    return space_between(ringalloc->next, ringalloc->base + ringalloc->capacity);
  }
  if (ringalloc->next < ringalloc->first) {
    return space_between(ringalloc->next, ringalloc->first);
  }
  return 0;
}

static bool can_fit_frame_at_next(const struct ringalloc *ringalloc, size_t payload_size) {
  return frame_layout_fits(frame_layout(payload_size), room_at_next(ringalloc));
}

static bool can_fit_frame_at_base(const struct ringalloc *ringalloc, size_t payload_size) {
  if (has_wrapped(ringalloc)) return false;
  if (ringalloc->next <= ringalloc->first) return false;
  return frame_layout_fits(frame_layout(payload_size),
                           space_between(ringalloc->base, ringalloc->first));
}

static bool can_reallocate_at_base(const struct ringalloc *ringalloc, size_t payload_size) {
  if (has_wrapped(ringalloc)) return false;
  if (ringalloc->next <= ringalloc->first) return false;
  size_t room = has_one_frame(ringalloc) ? ringalloc->capacity
                                         : space_between(ringalloc->base, ringalloc->first);
  return frame_layout_fits(frame_layout(payload_size), room);
}

struct ringalloc *ra_initialize(unsigned char *buffer, size_t capacity) {
  if (buffer == nullptr) unreachable();

  size_t state_padding = alignment_padding((uintptr_t)buffer, alignof(struct ringalloc));
  size_t state_padded_size = state_padding + sizeof(struct ringalloc);
  if (state_padded_size > capacity) {
    return nullptr;
  }

  size_t remaining_capacity = capacity - state_padded_size;
  size_t ring_buffer_padding =
      alignment_padding((uintptr_t)buffer + state_padded_size, alignof(max_align_t));

  size_t ring_buffer_capacity =
      remaining_capacity < ring_buffer_padding ? 0 : remaining_capacity - ring_buffer_padding;
  unsigned char *ring_buffer_base = remaining_capacity < ring_buffer_padding
                                        ? buffer + state_padded_size
                                        : buffer + state_padded_size + ring_buffer_padding;

  struct ringalloc *handle = (struct ringalloc *)(buffer + state_padding);
  store_state(handle, &(struct ringalloc){
                          .base = ring_buffer_base,
                          .capacity = ring_buffer_capacity,
                          .first = ring_buffer_base,
                          .last = ring_buffer_base,
                          .next = ring_buffer_base,
                          .empty = true,
                      });
  return handle;
}

void ra_reset(struct ringalloc *ringalloc) {
  if (ringalloc == nullptr) unreachable();

  struct ringalloc state = load_state(ringalloc);
  state.first = state.base;
  state.last = state.base;
  state.next = state.base;
  state.wrap = nullptr;
  state.empty = true;
  store_state(ringalloc, &state);
}

void *ra_allocate(struct ringalloc *ringalloc, size_t size) {
  if (ringalloc == nullptr) unreachable();

  struct ringalloc state = load_state(ringalloc);

  if (can_fit_frame_at_next(&state, size)) {
    state.last = state.next;
  } else if (can_fit_frame_at_base(&state, size)) {
    state.wrap = state.next;
    state.last = state.base;
  } else {
    return nullptr;
  }

  struct frame new = frame(state.last, size);

  state.next = new.start + frame_length(new.layout);
  state.empty = false;
  store_state(ringalloc, &state);

  return payload_address(new);
}

static size_t min(size_t left, size_t right) {
  return left < right ? left : right;
}

void *ra_reallocate(struct ringalloc *ringalloc, void *allocation, size_t size) {
  if (ringalloc == nullptr) unreachable();
  if (allocation == nullptr) unreachable();

  struct ringalloc state = load_state(ringalloc);
  if (state.empty) unreachable();

  struct frame last = existing_frame(state.last);
  if (allocation != payload_address(last)) unreachable();

  struct frame_layout new_layout = frame_layout(size);
  size_t room = frame_length(last.layout) + room_at_next(&state);
  if (frame_layout_fits(new_layout, room)) {
    store_header_size(last.start, new_layout.payload_size);
    state.next = last.start + frame_length(new_layout);
    store_state(ringalloc, &state);
    return allocation;
  }

  if (!can_reallocate_at_base(&state, size)) return nullptr;

  unsigned char *old_start = last.start;
  size_t copy_length = min(size, last.layout.payload_size);

  if (has_one_frame(&state)) {
    state.first = state.base;
  } else {
    state.wrap = old_start;
  }
  state.last = state.base;

  struct frame new = frame(state.last, size);
  memmove(payload_address(new), allocation, copy_length);
  state.next = new.start + frame_length(new.layout);
  store_state(ringalloc, &state);

  return payload_address(new);
}

void ra_free(struct ringalloc *ringalloc, void *allocation) {
  if (ringalloc == nullptr) unreachable();
  if (allocation == nullptr) return;

  struct ringalloc state = load_state(ringalloc);
  if (state.empty) unreachable();

  struct frame first = existing_frame(state.first);
  if (allocation != payload_address(first)) unreachable();

  if (has_one_frame(&state)) {
    ra_reset(ringalloc);
    return;
  }

  state.first += frame_length(first.layout);
  if (has_wrapped(&state) && state.first == state.wrap) {
    state.first = state.base;
    state.wrap = nullptr;
  }
  store_state(ringalloc, &state);
}
