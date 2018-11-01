#define _GNU_SOURCE
#include "helper.h"
#include <malloc.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

void*
pthread_start_spam(void* arg)
{
  // set the cpu affinity based on the given argument
  const int cpu_affinity = *((int*)arg);
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(cpu_affinity, &cpu_set);
  pthread_t current_thread = pthread_self();
  pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpu_set);

  int i;
  for (i = 0; i < 1000; i++) {
    void* mem0 = malloc(100);
    free(mem0);
  }

  return NULL;
}

void*
perform_forks(const int cpu_affinity)
{
  // set the cpu affinity based on the given argument
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(cpu_affinity, &cpu_set);
  pthread_t current_thread = pthread_self();
  pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpu_set);

  bool parent = true;

  int i;
  for (i = 0; i < 10; i++) {
    int fork_result = fork();
    assert(fork_result >= 0, __FILE__, __LINE__);

    // stop forking new threads if we are the child
    if (fork_result == 0) {
      parent = false;
      break;
    }
  }

  // try to access the base mutex and kill the children silently
  if (!parent) {
    char* args[] = { "./hello", NULL };
    execvp("./hello", args);
    exit(0);
  }

  return NULL;
}

int
main(int argc, char** argv)
{
  old_malloc_hook = __malloc_hook;
  __malloc_hook = my_malloc_hook;
  old_free_hook = __free_hook;
  __free_hook = my_free_hook;
  old_realloc_hook = __realloc_hook;
  __realloc_hook = my_realloc_hook;
  old_memalign_hook = __memalign_hook;
  __memalign_hook = my_memalign_hook;

  // showcase the malloc hooks
  void* ptr0 = malloc(100);
  void* ptr1 = realloc(ptr0, 100);
  void* ptr2 = memalign(64, 12);
  free(ptr1);
  free(ptr2);

  // reset the malloc hooks back to nothing so we don't spam output
  __malloc_hook = old_malloc_hook;
  __free_hook = old_free_hook;
  __realloc_hook = old_realloc_hook;
  __memalign_hook = old_memalign_hook;

  // now, show off fork() capabilities by creating a bunch of working threads
  int num_arenas = sysconf(_SC_NPROCESSORS_ONLN);
  pthread_t* threads = malloc(num_arenas * sizeof(pthread_t));
  int* cpu_affinities = malloc(num_arenas * sizeof(int));

  // spawn working threads
  int i;
  for (i = 0; i < num_arenas; i++) {
    cpu_affinities[i] = i;

    // tell the thread that its cpu affinity should be equal to i
    int pthread_create_result =
      pthread_create(&threads[i], NULL, pthread_start_spam, &cpu_affinities[i]);
    assert(pthread_create_result == 0, __FILE__, __LINE__);
  }

  // give our forker a random cpu affinity
  srand(time(NULL));
  int fork_cpu_affinity = rand() % num_arenas;
  perform_forks(fork_cpu_affinity);

  sleep(2);

  return 0;
}

// TODO get write of any snprintf() calls and any write() calls
