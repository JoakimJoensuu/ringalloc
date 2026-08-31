#ifndef RINGALLOC_H
#define RINGALLOC_H

#include <stddef.h>

struct ringalloc;

/**
 * @return Allocator in @p buffer, or nullptr if @p buffer is too small.
 */
struct ringalloc *ra_initialize(unsigned char *buffer, size_t capacity);

/** @return address of newly allocated block, or nullptr if no room. */
void *ra_allocate(struct ringalloc *ringalloc, size_t size);

/**
 * @param allocation Must be the newest one.
 * @param size Zero leaves a live zero-size block; it does not free @p allocation.
 *
 * @return address of the reallocated block, might be a new one, or nullptr if no room; @p
 * allocation stays valid on failure.
 */
void *ra_reallocate(struct ringalloc *ringalloc, void *allocation, size_t size);

/**
 * No-op if @p allocation is nullptr.
 *
 * @param allocation Must be the oldest one or nullptr
 */
void ra_free(struct ringalloc *ringalloc, void *allocation);

/**
 * Drops all allocations.
 */
void ra_reset(struct ringalloc *ringalloc);

#endif /* RINGALLOC_H */
