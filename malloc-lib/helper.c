#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "helper.h"

int PAGE_SIZE;
pthread_mutex_t MUTEX = PTHREAD_MUTEX_INITIALIZER;
bool init = false;
head_t heads[3];
int used_blocks[3]; // each index represents the number of used blocks in the
                    // corresponding bin
size_t mmap_size;
int num_malloc_requests[3];
int num_free_requests[3];

const char* ALLOC_SIZE_OUT_OF_RANGE = "Alloc size too large to get bin index\n";

void assert(bool condition, const char* fname, int lineno) {
  if (!condition) {
    char buf[1024];
    snprintf(buf, 1024, "assertion failed in file %s at line %d\n", fname,
             lineno);
    write(STDERR_FILENO, buf, strlen(buf) + 1);
    exit(1);
  }
}

bool is_aligned(void* ptr, size_t alignment) {
  return (((size_t)ptr) & (alignment - 1)) == 0;
}

// returns whether bins should be used for the given allocation size
bool use_bins_for_size(size_t alloc_size) {
  return alloc_size <= BIN_SIZES[NUM_BINS - 1];
}

bool has_free_block_for_size(size_t alloc_size) {
  int index = get_index_for_size(alloc_size);
  return !TAILQ_EMPTY(&heads[index]);
}

// returns the bin index associated with the given allocation size
int get_index_for_size(size_t alloc_size) {
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

void* list_remove(size_t alloc_size) {
  int index = get_index_for_size(alloc_size);
  head_t* head = &heads[index];

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

void list_insert(MallocHeader* free_hdr, int num_free_blocks) {
  assert(num_free_blocks > 0, __FILE__, __LINE__);
  assert(sizeof(node_t) <= free_hdr->size, __FILE__, __LINE__);
  assert(free_hdr->offset == 0, __FILE__, __LINE__);

  // get the size on the stack before we overwrite it
  size_t size = free_hdr->size;
  node_t* n = (node_t*)free_hdr;
  n->size = size;
  n->num_free = num_free_blocks - 1;

  int index = get_index_for_size(free_hdr->size);
  head_t* head = &heads[index];

  TAILQ_INSERT_TAIL(head, n, nodes);
}

void initialize() {
  assert(!is_init(), __FILE__, __LINE__);

  PAGE_SIZE = sysconf(_SC_PAGESIZE);

  int i;
  for (i = 0; i < NUM_BINS; i++) {
    TAILQ_INIT(&heads[i]);
    used_blocks[i] = 0;
    num_malloc_requests[i] = 0;
    num_free_requests[i] = 0;
  }

  mmap_size = 0;

  init = true;
}

size_t round_up_to_page_size(size_t size) {
  if (size % PAGE_SIZE == 0) {
    return size;
  }

  return PAGE_SIZE * (size / PAGE_SIZE + 1);
}

bool is_init() { return init; }

void increment_used_blocks(int index) {
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  used_blocks[index]++;
}

void decrement_used_blocks(int index) {
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  used_blocks[index]--;
}

int get_num_used_blocks(int index) {
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  return used_blocks[index];
}

int get_num_free_blocks(int index) {
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  int num_free_blocks = 0;
  head_t* head = &heads[index];
  node_t* n;
  TAILQ_FOREACH(n, head, nodes) {
    // we add one because the node itself is converted to a free block once
    // num_free reaches zero
    num_free_blocks += n->num_free + 1;
  }

  return num_free_blocks;
}

void increment_mmap_size(size_t alloc_size) { mmap_size += alloc_size; }

void decrement_mmap_size(size_t alloc_size) { mmap_size -= alloc_size; }

size_t get_mmap_size() { return mmap_size; }

void increment_num_malloc_requests(int index) {
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  num_malloc_requests[index] += 1;
}

void increment_num_free_requests(int index) {
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  num_free_requests[index] += 1;
}

int get_num_malloc_requests(int index) {
  assert(index < NUM_BINS && index >= 0, __FILE__, __LINE__);

  return num_malloc_requests[index];
}

int get_num_free_requests(int index) {
  assert(index < NUM_BINS, __FILE__, __LINE__);

  return num_free_requests[index];
}