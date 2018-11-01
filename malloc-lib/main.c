#define _GNU_SOURCE
#include "helper.h"
#include <malloc.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// void* (*old_malloc_hook)(size_t, const void*);
// void (*old_free_hook)(void*, const void*);
// void* (*old_realloc_hook)(void*, size_t, const void*);
// void* (*old_memalign_hook)(size_t, size_t, const void*);

// static void*
// my_malloc_hook(size_t size, const void* caller)
// {
//   write(STDOUT_FILENO, "Malloc hook\n", strlen("Malloc hook\n") + 1);

//   /* Restore all old hooks */
//   __malloc_hook = old_malloc_hook;

//   /* Call recursively */
//   void* result = malloc(size);

//   /* Save underlying hooks */
//   old_malloc_hook = __malloc_hook;

//   /* Restore our own hooks */
//   __malloc_hook = my_malloc_hook;

//   return result;
// }

// static void
// my_free_hook(void* ptr, const void* caller)
// {
//   write(STDOUT_FILENO, "Free hook\n", strlen("Free hook\n") + 1);

//   /* Restore all old hooks */
//   __free_hook = old_free_hook;

//   /* Call recursively */
//   free(ptr);

//   /* Save underlying hooks */
//   old_free_hook = __free_hook;

//   /* Restore our own hooks */
//   __free_hook = my_free_hook;
// }

// static void*
// my_realloc_hook(void* ptr, size_t size, const void* caller)
// {
//   write(STDOUT_FILENO, "Realloc hook\n", strlen("Realloc hook\n") + 1);

//   /* Restore all old hooks */
//   __realloc_hook = old_realloc_hook;

//   /* Call recursively */
//   void* result = realloc(ptr, size);

//   /* Save underlying hooks */
//   old_realloc_hook = __realloc_hook;

//   /* Restore our own hooks */
//   __realloc_hook = my_realloc_hook;

//   return result;
// }

// static void*
// my_memalign_hook(size_t alignment, size_t size, const void* caller)
// {
//   write(STDOUT_FILENO, "Memalign hook\n", strlen("Memalign hook\n") + 1);

//   /* Restore all old hooks */
//   __memalign_hook = old_memalign_hook;

//   /* Call recursively */
//   void* result = memalign(alignment, size);

//   /* Save underlying hooks */
//   old_memalign_hook = __memalign_hook;

//   /* Restore our own hooks */
//   __memalign_hook = my_memalign_hook;

//   return result;
// }

void*
pthread_start(void* arg)
{
  const int cpu_affinity = *((int*)arg);

  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(cpu_affinity, &cpu_set);

  pthread_t current_thread = pthread_self();

  pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpu_set);

  char buf[1024];
  snprintf(buf, 1024, "Thread %p has cpu affinity %d\n", &current_thread,
           cpu_affinity);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);

  size_t alignment = 64;
  size_t size = 12;

  void* mem0 = malloc(size);
  // printf("wowza2 %d\n", cpu_affinity);

  // printf("Successfully malloc'd %zu bytes at addr %p\n", size, mem0);
  assert(mem0 != NULL, __FILE__, __LINE__);

  void* mem1 = realloc(mem0, size);
  assert(mem1 != NULL, __FILE__, __LINE__);

  // printf("Successfully realloc'd %zu bytes from addr %p to %p\n", size, mem0,
  //        mem1);

  snprintf(buf, 1024, "\n\n\nSHOULD BE ARENA%d\n\n\n", cpu_affinity);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);

  void* mem2 = memalign(alignment, size);
  assert(is_aligned(mem2, alignment), __FILE__, __LINE__);

  // printf(
  //   "Successfully memalign'd %zu bytes to a %zu-byte alignment at addr %p\n",
  //   size, alignment, mem2);

  return NULL;
}

int
main(int argc, char** argv)
{
  // write(STDOUT_FILENO, "starting main...\n", strlen("starting main...\n") +
  // 1);

  // old_malloc_hook = __malloc_hook;
  // __malloc_hook = my_malloc_hook;
  // old_free_hook = __free_hook;
  // __free_hook = my_free_hook;
  // old_realloc_hook = __realloc_hook;
  // __realloc_hook = my_realloc_hook;
  // old_memalign_hook = __memalign_hook;
  // __memalign_hook = my_memalign_hook;

  int num_arenas = sysconf(_SC_NPROCESSORS_ONLN);
  pthread_t* threads = malloc(num_arenas * sizeof(pthread_t));
  int* cpu_affinities = malloc(num_arenas * sizeof(int));
  int i;
  for (i = 0; i < num_arenas; i++) {
    cpu_affinities[i] = i;

    // tell the thread that its cpu affinity should be equal to i
    int pthread_create_result =
      pthread_create(&threads[i], NULL, pthread_start, &cpu_affinities[i]);
    assert(pthread_create_result == 0, __FILE__, __LINE__);
  }

  sleep(1);

  // for (i = 0; i < num_arenas; i++) {
  //   int arena = -1;
  //   cpu_set_t cpu_set;
  //   CPU_ZERO(&cpu_set);
  //   pthread_t current_thread = threads[i];
  //   pthread_getaffinity_np(current_thread, sizeof(cpu_set_t), &cpu_set);
  //   int j;
  //   for (j = 0; j < num_arenas; j++) {
  //     if (CPU_ISSET(j, &cpu_set)) {
  //       arena = j;
  //       break;
  //     }
  //   }

  //   printf("Arena: %d\n", arena);
  //   assert(arena == i, __FILE__, __LINE__);
  // }

  malloc_stats();

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
// Remove "incluce hlper.h" at top of main if possible
// TODO make sure malloc hooks works
// TDODO get rid of excess write statements
// TODO get rid of unecessary assert in my_mmap
// TODO get rid of my_mmap_calls extern in helper.h and in helper.c
// TODO make strucvts for malloc stats and for arena (mutex, arena_num)