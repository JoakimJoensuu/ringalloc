#include <ringalloc.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main() {
  uint8_t storage[BUFSIZ];
  struct ringalloc *ringalloc = ringalloc_init(storage, sizeof storage);
  char *hello = nullptr;
  char *world = nullptr;

  if (ringalloc == nullptr) return 1;
  hello = ringalloc_alloc(ringalloc, sizeof "hello");
  world = ringalloc_alloc(ringalloc, sizeof "world");
  if (hello == nullptr || world == nullptr) return 1;
  strcpy(hello, "hello");
  strcpy(world, "world");
  printf("%s %s\n", hello, world);
  return 0;
}
