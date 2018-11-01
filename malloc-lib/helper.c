#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include "helper.h"

int PAGE_SIZE;
int NUM_ARENAS;
pthread_mutex_t BASE_MUTEX = PTHREAD_MUTEX_INITIALIZER;

bool init = false;

head_t** heads;
MallocInfo* malloc_infos;
pthread_mutex_t* mutexs;

pthread_t* threads;

const char* ALLOC_SIZE_OUT_OF_RANGE = "Alloc size too large to get bin index\n";

void*
my_mmap(size_t alloc_size)
{
  return mmap(0, alloc_size, PROT_READ | PROT_WRITE,
              MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
}

int
get_arena()
{
  // the arena for this thread is the cpu affinity of the thread
  int arena = -1;
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  pthread_t current_thread = pthread_self();
  pthread_getaffinity_np(current_thread, sizeof(cpu_set_t), &cpu_set);
  int i;
  for (i = 0; i < NUM_ARENAS; i++) {
    if (CPU_ISSET(i, &cpu_set)) {
      arena = i;
      break;
    }
  }

  assert(arena >= 0 && arena < NUM_ARENAS, __FILE__, __LINE__);
  return arena;
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
is_init()
{
  return init;
}

void
initialize_helper_if_necessary()
{
  // return immediately if we've already initialized
  if (is_init()) {
    return;
  }

  // if we're not init, we should acquire a lock and check again if we need to
  // initialize once we have our lock

  int lock_result = pthread_mutex_lock(&BASE_MUTEX);
  assert(lock_result == 0, __FILE__, __LINE__);

  if (!is_init()) {

    helper_initialize();
  }

  int unlock_result = pthread_mutex_unlock(&BASE_MUTEX);
  assert(unlock_result == 0, __FILE__, __LINE__);
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
  int index = get_index_for_size(alloc_size);
  return !TAILQ_EMPTY(&heads[get_arena()][index]);
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
  int index = get_index_for_size(alloc_size);
  head_t* head = &heads[get_arena()][index];

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
  assert(num_free_blocks > 0, __FILE__, __LINE__);
  assert(sizeof(node_t) <= free_hdr->size, __FILE__, __LINE__);
  assert(free_hdr->offset == 0, __FILE__, __LINE__);

  // get the size on the stack before we overwrite it
  size_t size = free_hdr->size;
  node_t* n = (node_t*)free_hdr;
  n->size = size;
  n->num_free = num_free_blocks - 1;

  int index = get_index_for_size(free_hdr->size);
  head_t* head = &heads[get_arena()][index];

  TAILQ_INSERT_TAIL(head, n, nodes);
}

void
fork_child_handler()
{
  // ensure that we unlock the global lock mutex if it's been locked
  int unlock_result = pthread_mutex_unlock(&BASE_MUTEX);
  assert(unlock_result == 0, __FILE__, __LINE__);
}

void
helper_initialize()
{
  assert(!is_init(), __FILE__, __LINE__);

  PAGE_SIZE = sysconf(_SC_PAGESIZE);
  NUM_ARENAS = sysconf(
    _SC_NPROCESSORS_ONLN); // set the number of arenas to the nubmer of cores

  pthread_atfork(NULL, NULL, fork_child_handler);

  // make sure we malloc the arenas with mmap because the bins will not have
  // been set up
  size_t arena_head_array_malloc_size =
    round_up_to_page_size(sizeof(head_t*) * NUM_ARENAS);
  size_t arena_malloc_info_array_size =
    round_up_to_page_size(sizeof(MallocInfo) * NUM_ARENAS);
  heads = (head_t**)my_mmap(arena_head_array_malloc_size);
  malloc_infos = (MallocInfo*)my_mmap(arena_malloc_info_array_size);
  assert(heads != MAP_FAILED, __FILE__, __LINE__);
  assert(malloc_infos != MAP_FAILED, __FILE__, __LINE__);

  int i;
  for (i = 0; i < NUM_ARENAS; i++) {
    malloc_infos[i].mmap_size = 0;

    size_t head_array_malloc_size =
      round_up_to_page_size(sizeof(head_t) * NUM_BINS);
    heads[i] = (head_t*)my_mmap(head_array_malloc_size);
    assert(heads[i] != MAP_FAILED, __FILE__, __LINE__);

    int j;
    for (j = 0; j < NUM_BINS; j++) {
      TAILQ_INIT(&(heads[i][j]));
      malloc_infos[i].used_blocks[j] = 0;
      malloc_infos[i].num_malloc_requests[j] = 0;
      malloc_infos[i].num_free_requests[j] = 0;
    }
  }

  // make sure we have one mutex for every core arena
  size_t arena_mutex_array_size =
    round_up_to_page_size(sizeof(pthread_mutex_t) * NUM_ARENAS);
  mutexs = (pthread_mutex_t*)my_mmap(arena_mutex_array_size);
  for (i = 0; i < NUM_ARENAS; i++) {
    int mutex_init_result = pthread_mutex_init(&(mutexs[i]), NULL);
    assert(mutex_init_result == 0, __FILE__, __LINE__);
  }
  assert(mutexs != MAP_FAILED, __FILE__, __LINE__);

  // update our sizes
  increment_mmap_size(arena_head_array_malloc_size);
  increment_mmap_size(arena_malloc_info_array_size);
  for (i = 0; i < NUM_ARENAS; i++) {
    size_t head_array_malloc_size =
      round_up_to_page_size(sizeof(head_t) * NUM_BINS);
    increment_mmap_size(head_array_malloc_size);
  }
  increment_mmap_size(arena_mutex_array_size);

  init = true;
}

size_t
round_up_to_page_size(size_t size)
{
  if (size % PAGE_SIZE == 0) {
    return size;
  }

  return PAGE_SIZE * (size / PAGE_SIZE + 1);
}

void
increment_used_blocks(int index)
{
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  malloc_infos[get_arena()].used_blocks[index]++;
}

void
decrement_used_blocks(int index)
{
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  malloc_infos[get_arena()].used_blocks[index]--;
}

int
get_num_used_blocks(int arena, int index)
{
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  return malloc_infos[arena].used_blocks[index];
}

int
get_num_free_blocks(int arena, int index)
{
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  int num_free_blocks = 0;
  head_t* head = &heads[arena][index];
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
  malloc_infos[get_arena()].mmap_size += alloc_size;
}

void
decrement_mmap_size(size_t alloc_size)
{
  malloc_infos[get_arena()].mmap_size -= alloc_size;
}

size_t
get_mmap_size(int arena)
{
  return malloc_infos[arena].mmap_size;
}

void
increment_num_malloc_requests(int index)
{
  malloc_infos[get_arena()].num_malloc_requests[index] += 1;
}

void
increment_num_free_requests(int index)
{
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  malloc_infos[get_arena()].num_free_requests[index] += 1;
}

int
get_num_malloc_requests(int arena, int index)
{
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  return malloc_infos[arena].num_malloc_requests[index];
}

int
get_num_free_requests(int arena, int index)
{
  assert(index < NUM_BINS, __FILE__, __LINE__);

  return malloc_infos[arena].num_free_requests[index];
}