#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void* (*old_malloc_hook)(size_t, const void*);
void (*old_free_hook)(void*, const void*);
void* (*old_realloc_hook)(void*, size_t, const void*);
void* (*old_memalign_hook)(size_t, size_t, const void*);

static void* my_malloc_hook(size_t size, const void* caller) {
  /* Restore all old hooks */
  __malloc_hook = old_malloc_hook;

  /* Call recursively */
  void* result = malloc(size);

  /* Save underlying hooks */
  old_malloc_hook = __malloc_hook;

  /* Restore our own hooks */
  __malloc_hook = my_malloc_hook;

  write(STDOUT_FILENO, "Malloc hook\n", strlen("Malloc hook\n") + 1);

  return result;
}

static void my_free_hook(void* ptr, const void* caller) {
  /* Restore all old hooks */
  __free_hook = old_free_hook;

  /* Call recursively */
  free(ptr);

  /* Save underlying hooks */
  old_free_hook = __free_hook;

  /* Restore our own hooks */
  __free_hook = my_free_hook;

  write(STDOUT_FILENO, "Free hook\n", strlen("Free hook\n") + 1);
}

static void* my_realloc_hook(void* ptr, size_t size, const void* caller) {
  /* Restore all old hooks */
  __realloc_hook = old_realloc_hook;

  /* Call recursively */
  void* result = realloc(ptr, size);

  /* Save underlying hooks */
  old_realloc_hook = __realloc_hook;

  /* Restore our own hooks */
  __realloc_hook = my_realloc_hook;

  write(STDOUT_FILENO, "Realloc hook\n", strlen("Realloc hook\n") + 1);

  return result;
}

static void* my_memalign_hook(size_t alignment, size_t size,
                              const void* caller) {
  /* Restore all old hooks */
  __memalign_hook = old_memalign_hook;

  /* Call recursively */
  void* result = memalign(alignment, size);

  /* Save underlying hooks */
  old_memalign_hook = __memalign_hook;

  /* Restore our own hooks */
  __memalign_hook = my_memalign_hook;

  write(STDOUT_FILENO, "Memalign hook\n", strlen("Memalign hook\n") + 1);

  return result;
}

int main(int argc, char** argv) {
  old_malloc_hook = __malloc_hook;
  __malloc_hook = my_malloc_hook;
  old_free_hook = __free_hook;
  __free_hook = my_free_hook;
  old_realloc_hook = __realloc_hook;
  __realloc_hook = my_realloc_hook;
  old_memalign_hook = __memalign_hook;
  __memalign_hook = my_memalign_hook;

  size_t alignment = 64;
  size_t size = 12;

  void* mem0 = malloc(size);

  printf("Successfully malloc'd %zu bytes at addr %p\n", size, mem0);
  assert(mem0 != NULL);

  void* mem1 = realloc(mem0, size);

  printf("Successfully realloc'd %zu bytes from addr %p to %p\n", size, mem0,
         mem1);

  void* mem2 = memalign(alignment, size);

  printf(
      "Successfully memalign'd %zu bytes to a %zu-byte alignment at addr %p\n",
      size, alignment, mem2);

  return 0;
}
