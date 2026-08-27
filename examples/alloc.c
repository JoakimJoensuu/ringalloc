#include <ringalloc.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main() {
  uint8_t storage[BUFSIZ];
  struct ringalloc *ringalloc = ringalloc_init(storage, sizeof storage);

  if (ringalloc == nullptr) return 1;
  char *hello = ringalloc_alloc(ringalloc, strlen("hello") + 1);
  char *world = ringalloc_alloc(ringalloc, strlen("world") + 1);
  if (hello == nullptr || world == nullptr) return 1;
  strcpy(hello, "hello");
  strcpy(world, "world");
  printf("%s %s\n", hello, world);
  return 0;
}
