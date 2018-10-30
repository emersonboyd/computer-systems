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

static void*
my_malloc_hook(size_t size, const void* caller)
{
  write(STDOUT_FILENO, "Malloc hook\n", strlen("Malloc hook\n") + 1);

  /* Restore all old hooks */
  __malloc_hook = old_malloc_hook;

  /* Call recursively */
  void* result = malloc(size);

  /* Save underlying hooks */
  old_malloc_hook = __malloc_hook;

  /* Restore our own hooks */
  __malloc_hook = my_malloc_hook;

  return result;
}

static void
my_free_hook(void* ptr, const void* caller)
{
  write(STDOUT_FILENO, "Free hook\n", strlen("Free hook\n") + 1);

  /* Restore all old hooks */
  __free_hook = old_free_hook;

  /* Call recursively */
  free(ptr);

  /* Save underlying hooks */
  old_free_hook = __free_hook;

  /* Restore our own hooks */
  __free_hook = my_free_hook;
}

static void*
my_realloc_hook(void* ptr, size_t size, const void* caller)
{
  write(STDOUT_FILENO, "Realloc hook\n", strlen("Realloc hook\n") + 1);

  /* Restore all old hooks */
  __realloc_hook = old_realloc_hook;

  /* Call recursively */
  void* result = realloc(ptr, size);

  /* Save underlying hooks */
  old_realloc_hook = __realloc_hook;

  /* Restore our own hooks */
  __realloc_hook = my_realloc_hook;

  return result;
}

static void*
my_memalign_hook(size_t alignment, size_t size, const void* caller)
{
  write(STDOUT_FILENO, "Memalign hook\n", strlen("Memalign hook\n") + 1);

  /* Restore all old hooks */
  __memalign_hook = old_memalign_hook;

  /* Call recursively */
  void* result = memalign(alignment, size);

  /* Save underlying hooks */
  old_memalign_hook = __memalign_hook;

  /* Restore our own hooks */
  __memalign_hook = my_memalign_hook;

  return result;
}

int
main(int argc, char** argv)
{
  write(STDOUT_FILENO, "starting main...\n", strlen("starting main...\n") + 1);

  old_malloc_hook = __malloc_hook;
  __malloc_hook = my_malloc_hook;
  old_free_hook = __free_hook;
  __free_hook = my_free_hook;
  old_realloc_hook = __realloc_hook;
  __realloc_hook = my_realloc_hook;
  old_memalign_hook = __memalign_hook;
  __memalign_hook = my_memalign_hook;

  malloc_stats();

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

// TODO set the core affinity of threads using the pthread_attr_t struct and
// using the function pthread_attr_setaffinity_np(3)
// TODO figure out a better way to allocate data for the pthread_t array in
// helper.c helper_initialize() other than mmap because we aren't storing stats
// for mmap
// TODO get write of any snprintf() calls and any write() calls
// TODO figure out the true meaning of extern and get rid of as many externs as
// possible
// TODO creating new threads affects the bin numbers for the current bin - is
// that okay?
// TODO in each thread i need a global lock surrounding sbrk(), mmap(), munmap()
//

// size_t threads_malloc_size =
//   round_up_to_page_size(sizeof(pthread_t) * NUM_ARENAS);
// threads = (pthread_t*)my_mmap(threads_malloc_size);
// assert(threads != MAP_FAILED, __FILE__, __LINE__);
// for (i = 0; i < NUM_ARENAS; i++) {
//   int pthread_create_result =
//     pthread_create(&threads[i], NULL, pthread_start, NULL);
//   assert(pthread_create_result == 0, __FILE__, __LINE__);
// }