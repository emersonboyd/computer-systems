#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

int main(int argc, char **argv) {
  size_t size = 12;

  void *mem = malloc(size);

  printf("Successfully malloc'd %zu bytes at addr %p\n", size, mem);
  assert(mem != NULL);

  free(mem);

  printf("Successfully free'd %zu bytes from addr %p\n", size, mem);

  return 0;
}

// TODO READ OVER THE ASSIGNMENT TO MAKE SURE I'M FOLLOWING ALL OF THE RULES SPECIFIED
// TODO format these files using clang-format
