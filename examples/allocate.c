#include <ringalloc.h>

#include <stdio.h>
#include <string.h>

int main() {
  unsigned char storage[BUFSIZ];
  struct ringalloc *allocator = ra_initialize(storage, sizeof(storage));

  if (allocator == nullptr) return 1;
  char *hello = ra_allocate(allocator, sizeof("hello"));
  char *world = ra_allocate(allocator, sizeof("world"));
  if (hello == nullptr || world == nullptr) return 1;
  memcpy(hello, "hello", sizeof("hello"));
  memcpy(world, "world", sizeof("world"));
  printf("%s %s\n", hello, world);
  return 0;
}
