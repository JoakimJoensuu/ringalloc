#ifndef RINGALLOC_H
#define RINGALLOC_H

#include <stddef.h>

struct ringalloc;

/** @return Allocator in @p buf, or nullptr if @p buf is too small. */
struct ringalloc *ringalloc_init(void *buf, size_t cap);
void *ringalloc_alloc(struct ringalloc *ringalloc, size_t size);
/** Oldest allocation only. */
void ringalloc_free(struct ringalloc *ringalloc);
/** Newest allocation only. nullptr if @p ptr is not newest or it cannot grow. */
void *ringalloc_realloc(struct ringalloc *ringalloc, void *ptr, size_t size);
void ringalloc_reset(struct ringalloc *ringalloc);

#endif /* RINGALLOC_H */
