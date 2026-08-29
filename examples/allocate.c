#include <ringalloc.h>

#include <stdio.h>
#include <string.h>

int main() {
  unsigned char storage[BUFSIZ];
  struct ringalloc *ringalloc = ra_initialize(storage, sizeof storage);

  if (ringalloc == nullptr) return 1;
  char *hello = ra_allocate(ringalloc, strlen("hello") + 1);
  char *world = ra_allocate(ringalloc, strlen("world") + 1);
  if (hello == nullptr || world == nullptr) return 1;
  strcpy(hello, "hello");
  strcpy(world, "world");
  printf("%s %s\n", hello, world);
  return 0;
}
