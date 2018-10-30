#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "helper.h"

int PAGE_SIZE;
int NUM_ARENAS;
pthread_mutex_t BASE_MUTEX = PTHREAD_MUTEX_INITIALIZER;

bool init = false;

head_t** heads;
int** used_blocks; // each index represents the number of used blocks
                   // in the corresponding bin
size_t* mmap_size;
int** num_malloc_requests;
int** num_free_requests;
__thread MallocArena arena;

int thread_num_counter = 0;
pthread_t* threads;

const char* ALLOC_SIZE_OUT_OF_RANGE = "Alloc size too large to get bin index\n";

void*
my_mmap(size_t alloc_size)
{
  return mmap(0, alloc_size, PROT_READ | PROT_WRITE,
              MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
}

void*
pthread_start(void* arg)
{
  // we need the lock to increment the counter for the thread number
  pthread_mutex_lock(&BASE_MUTEX);

  arena.num = thread_num_counter++;
  pthread_mutex_init(&arena.mutex, NULL);

  int i;
  for (i = 0; i < NUM_BINS; i++) {
    TAILQ_INIT(&(heads[arena.num][i]));
    used_blocks[arena.num][i] = 0;
    num_malloc_requests[arena.num][i] = 0;
    num_free_requests[arena.num][i] = 0;
  }
  mmap_size[arena.num] = 0;

  char buf[1024];
  snprintf(buf, 1024,
           "new thread with arena number %d and mmap_size pointer at %p\n",
           arena.num, &mmap_size);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);

  pthread_mutex_unlock(&BASE_MUTEX);

  return NULL;
}

void
assert(bool condition, const char* fname, int lineno)
{
  if (!condition) {
    char buf[1024];
    snprintf(buf, 1024, "assertion failed in file %s at line %d\n", fname,
             lineno);
    write(STDERR_FILENO, buf, strlen(buf) + 1);
    exit(1);
  }
}

bool
is_aligned(void* ptr, size_t alignment)
{
  return (((size_t)ptr) & (alignment - 1)) == 0;
}

// returns whether bins should be used for the given allocation size
bool
use_bins_for_size(size_t alloc_size)
{
  return alloc_size <= BIN_SIZES[NUM_BINS - 1];
}

bool
has_free_block_for_size(size_t alloc_size)
{
  assert(arena.num >= 0 && arena.num <= NUM_ARENAS, __FILE__, __LINE__);

  int index = get_index_for_size(alloc_size);
  return !TAILQ_EMPTY(&heads[arena.num][index]);
}

// returns the bin index associated with the given allocation size
int
get_index_for_size(size_t alloc_size)
{
  int i;
  for (i = 0; i < NUM_BINS; i++) {
    if (alloc_size <= BIN_SIZES[i]) {
      return i;
    }
  }

  write(STDERR_FILENO, ALLOC_SIZE_OUT_OF_RANGE,
        strlen(ALLOC_SIZE_OUT_OF_RANGE) + 1);
  exit(1);
}

void*
list_remove(size_t alloc_size)
{
  assert(arena.num >= 0 && arena.num <= NUM_ARENAS, __FILE__, __LINE__);

  int index = get_index_for_size(alloc_size);
  head_t* head = &heads[arena.num][index];

  node_t* n = TAILQ_FIRST(head);

  // point further in memory to the farthest available memory block
  void* ret = ((void*)n) + n->num_free * n->size;

  if (n->num_free == 0) {
    TAILQ_REMOVE(head, n, nodes);
  } else {
    n->num_free -= 1;
  }

  return ret;
}

void
list_insert(MallocHeader* free_hdr, int num_free_blocks)
{
  assert(arena.num >= 0 && arena.num <= NUM_ARENAS, __FILE__, __LINE__);

  assert(num_free_blocks > 0, __FILE__, __LINE__);
  assert(sizeof(node_t) <= free_hdr->size, __FILE__, __LINE__);
  assert(free_hdr->offset == 0, __FILE__, __LINE__);

  // get the size on the stack before we overwrite it
  size_t size = free_hdr->size;
  node_t* n = (node_t*)free_hdr;
  n->size = size;
  n->num_free = num_free_blocks - 1;

  char buf[1024];
  snprintf(
    buf, 1024,
    "Have header with block size %zu which corresponds to bin index %d\n",
    free_hdr->size, get_index_for_size(free_hdr->size));
  write(STDOUT_FILENO, buf, strlen(buf) + 1);

  int index = get_index_for_size(free_hdr->size);
  head_t* head = &heads[arena.num][index];

  TAILQ_INSERT_TAIL(head, n, nodes);
}

void
helper_initialize()
{
  assert(!is_init(), __FILE__, __LINE__);

  init = true;

  write(STDOUT_FILENO, "initializing helper class...\n",
        strlen("initializing helper class...\n") + 1);

  char buf[1024];
  snprintf(buf, 1024, "Unlock at file %s line %d\n", __FILE__, __LINE__);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);
  pthread_mutex_unlock(&BASE_MUTEX);

  PAGE_SIZE = sysconf(_SC_PAGESIZE);
  NUM_ARENAS = sysconf(
    _SC_NPROCESSORS_ONLN); // set the number of arenas to the nubmer of cores

  arena.num = 0;

  // make sure we malloc the arenas with mmap because the bins will not have
  // been set up
  size_t arena_head_array_malloc_size =
    round_up_to_page_size(sizeof(head_t*) * NUM_ARENAS);
  size_t arena_counters_array_malloc_size =
    round_up_to_page_size(sizeof(int*) * NUM_ARENAS);
  size_t arena_counters_mmap_size_size =
    round_up_to_page_size(sizeof(size_t) * NUM_ARENAS);
  heads = (head_t**)my_mmap(arena_head_array_malloc_size);
  used_blocks = (int**)my_mmap(arena_counters_array_malloc_size);
  num_malloc_requests = (int**)my_mmap(arena_counters_array_malloc_size);
  num_free_requests = (int**)my_mmap(arena_counters_array_malloc_size);
  mmap_size = (size_t*)my_mmap(arena_counters_mmap_size_size);
  assert(heads != MAP_FAILED, __FILE__, __LINE__);
  assert(used_blocks != MAP_FAILED, __FILE__, __LINE__);
  assert(num_malloc_requests != MAP_FAILED, __FILE__, __LINE__);
  assert(num_free_requests != MAP_FAILED, __FILE__, __LINE__);
  assert(mmap_size != MAP_FAILED, __FILE__, __LINE__);

  write(STDOUT_FILENO, "k\n", 3);

  int i;
  for (i = 0; i < NUM_ARENAS; i++) {
    mmap_size[i] = 0;

    size_t head_array_malloc_size =
      round_up_to_page_size(sizeof(head_t) * NUM_BINS);
    size_t counters_array_malloc_size =
      round_up_to_page_size(sizeof(int) * NUM_BINS);
    heads[i] = (head_t*)my_mmap(head_array_malloc_size);
    used_blocks[i] = (int*)my_mmap(counters_array_malloc_size);
    num_malloc_requests[i] = (int*)my_mmap(counters_array_malloc_size);
    num_free_requests[i] = (int*)my_mmap(counters_array_malloc_size);
    assert(heads[i] != MAP_FAILED, __FILE__, __LINE__);
    assert(used_blocks[i] != MAP_FAILED, __FILE__, __LINE__);
    assert(num_malloc_requests != MAP_FAILED, __FILE__, __LINE__);
    assert(num_free_requests != MAP_FAILED, __FILE__, __LINE__);

    int j;
    for (j = 0; j < NUM_BINS; j++) {
      TAILQ_INIT(&(heads[i][j]));
      used_blocks[i][j] = 0;
      num_malloc_requests[i][j] = 0;
      num_free_requests[i][j] = 0;
    }
  }

  size_t threads_malloc_size =
    round_up_to_page_size(sizeof(pthread_t) * NUM_ARENAS);
  threads = (pthread_t*)my_mmap(threads_malloc_size);
  assert(threads != MAP_FAILED, __FILE__, __LINE__);
  for (i = 0; i < NUM_ARENAS; i++) {
    int pthread_create_result =
      pthread_create(&threads[i], NULL, pthread_start, NULL);
    assert(pthread_create_result == 0, __FILE__, __LINE__);
  }

  // update our sizes
  increment_mmap_size(arena_head_array_malloc_size);
  increment_mmap_size(arena_counters_array_malloc_size);
  increment_mmap_size(arena_counters_array_malloc_size);
  increment_mmap_size(arena_counters_array_malloc_size);
  increment_mmap_size(arena_counters_mmap_size_size);
  for (i = 0; i < NUM_ARENAS; i++) {
    size_t head_array_malloc_size =
      round_up_to_page_size(sizeof(head_t) * NUM_BINS);
    size_t counters_array_malloc_size =
      round_up_to_page_size(sizeof(int) * NUM_BINS);
    increment_mmap_size(head_array_malloc_size);
    increment_mmap_size(counters_array_malloc_size);
    increment_mmap_size(counters_array_malloc_size);
    increment_mmap_size(counters_array_malloc_size);
  }
  increment_mmap_size(threads_malloc_size);

  // initialize the information for this helper
  // int i;
  // for (i = 0; i < NUM_BINS; i++) {
  //   TAILQ_INIT(&(heads[i]));
  //   used_blocks[i] = 0;
  //   num_malloc_requests[i] = 0;
  //   num_free_requests[i] = 0;
  // }
  // mmap_size = 0;

  // // loop over each pthread_t and start it up
  // size_t threads_malloc_size =
  //   round_up_to_page_size(sizeof(pthread_t) * NUM_ARENAS);

  // pthread_mutex_unlock(&BASE_MUTEX);

  // threads = (pthread_t*)malloc(threads_malloc_size);

  // pthread_mutex_lock(&BASE_MUTEX);

  // threads = (pthread_t*)mmap(0, threads_malloc_size, PROT_READ | PROT_WRITE,
  //                            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  // assert(threads != MAP_FAILED, __FILE__, __LINE__);

  // for (i = 0; i < NUM_ARENAS; i++) {
  //   char buf[1024];
  //   snprintf(buf, 1024, "Unlock at file %s line %d\n", __FILE__, __LINE__);
  //   write(STDOUT_FILENO, buf, strlen(buf) + 1);
  //   pthread_mutex_unlock(&BASE_MUTEX);

  //   int pthread_create_result =
  //     pthread_create(&(threads[i]), NULL, pthread_start, NULL);
  //   assert(pthread_create_result == 0, __FILE__, __LINE__);

  //   snprintf(buf, 1024, "Lock at file %s line %d\n", __FILE__, __LINE__);
  //   write(STDOUT_FILENO, buf, strlen(buf) + 1);
  //   pthread_mutex_lock(&BASE_MUTEX);
  // }

  snprintf(buf, 1024, "Lock at file %s line %d\n", __FILE__, __LINE__);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);
  pthread_mutex_lock(&BASE_MUTEX);

  write(STDOUT_FILENO, "finished initializing helper class...\n",
        strlen("finished initializing helper class...\n") + 1);
}

size_t
round_up_to_page_size(size_t size)
{
  if (size % PAGE_SIZE == 0) {
    return size;
  }

  return PAGE_SIZE * (size / PAGE_SIZE + 1);
}

bool
is_init()
{
  return init;
}

void
increment_used_blocks(int index)
{
  assert(arena.num >= 0 && arena.num <= NUM_ARENAS, __FILE__, __LINE__);
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  used_blocks[arena.num][index]++;
}

void
decrement_used_blocks(int index)
{
  assert(arena.num >= 0 && arena.num <= NUM_ARENAS, __FILE__, __LINE__);
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  used_blocks[arena.num][index]--;
}

int
get_num_used_blocks(int index)
{
  assert(arena.num >= 0 && arena.num <= NUM_ARENAS, __FILE__, __LINE__);
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  return used_blocks[arena.num][index];
}

int
get_num_free_blocks(int index)
{
  assert(arena.num >= 0 && arena.num <= NUM_ARENAS, __FILE__, __LINE__);
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  int num_free_blocks = 0;
  head_t* head = &heads[arena.num][index];
  node_t* n;
  TAILQ_FOREACH(n, head, nodes)
  {
    // we add one because the node itself is converted to a free block once
    // num_free reaches zero
    num_free_blocks += n->num_free + 1;
  }

  return num_free_blocks;
}

void
increment_mmap_size(size_t alloc_size)
{
  assert(arena.num >= 0 && arena.num <= NUM_ARENAS, __FILE__, __LINE__);

  mmap_size[arena.num] += alloc_size;
}

void
decrement_mmap_size(size_t alloc_size)
{
  assert(arena.num >= 0 && arena.num <= NUM_ARENAS, __FILE__, __LINE__);

  mmap_size[arena.num] -= alloc_size;
}

size_t
get_mmap_size()
{
  assert(arena.num >= 0 && arena.num <= NUM_ARENAS, __FILE__, __LINE__);

  return mmap_size[arena.num];
}

void
increment_num_malloc_requests(int index)
{
  assert(arena.num >= 0 && arena.num <= NUM_ARENAS, __FILE__, __LINE__);
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  num_malloc_requests[arena.num][index] += 1;
}

void
increment_num_free_requests(int index)
{
  assert(arena.num >= 0 && arena.num <= NUM_ARENAS, __FILE__, __LINE__);
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  num_free_requests[arena.num][index] += 1;
}

int
get_num_malloc_requests(int index)
{
  assert(arena.num >= 0 && arena.num <= NUM_ARENAS, __FILE__, __LINE__);
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  return num_malloc_requests[arena.num][index];
}

int
get_num_free_requests(int index)
{
  assert(arena.num >= 0 && arena.num <= NUM_ARENAS, __FILE__, __LINE__);
  assert(index < NUM_BINS, __FILE__, __LINE__);

  return num_free_requests[arena.num][index];
}