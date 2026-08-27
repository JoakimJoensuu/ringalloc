#ifndef RINGALLOC_H
#define RINGALLOC_H

#include <stddef.h>

struct ringalloc;

/** @return Allocator in @p buf, or nullptr if @p buf cannot hold one allocation. */
struct ringalloc *ringalloc_init(void *buf, size_t cap);
/** @return nullptr if the block does not fit or @p size overflows. */
void *ringalloc_alloc(struct ringalloc *ringalloc, size_t size);
/** Oldest allocation only. */
void ringalloc_free(struct ringalloc *ringalloc, void *ptr);
/** Newest only. Shrinks or grows in place. nullptr if it does not fit or @p size overflows. */
void *ringalloc_realloc(struct ringalloc *ringalloc, void *ptr, size_t size);
/** Drops all live allocations. */
void ringalloc_reset(struct ringalloc *ringalloc);

#endif /* RINGALLOC_H */
