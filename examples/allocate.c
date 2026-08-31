#include <ringalloc.h>

#include <stdio.h>
#include <string.h>

int main() {
  unsigned char storage[BUFSIZ];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof storage);

  if (ringalloc == nullptr) return 1;
  char *hello = ra_allocate(ringalloc, sizeof("hello"));
  char *world = ra_allocate(ringalloc, sizeof("world"));
  if (hello == nullptr || world == nullptr) return 1;
  memcpy(hello, "hello", sizeof("hello"));
  memcpy(world, "world", sizeof("world"));
  printf("%s %s\n", hello, world);
  return 0;
}
