#include <ringalloc.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main() {
  uint8_t storage[BUFSIZ];
  struct ringalloc *ringalloc = ringalloc_init(storage, sizeof storage);

  if (ringalloc == nullptr) return 1;
  char *hello = ringalloc_alloc(ringalloc, sizeof "hello");
  char *world = ringalloc_alloc(ringalloc, sizeof "world");
  if (hello == nullptr || world == nullptr) return 1;
  memcpy(hello, "hello", sizeof "hello");
  memcpy(world, "world", sizeof "world");
  printf("%s %s\n", hello, world);
  return 0;
}
