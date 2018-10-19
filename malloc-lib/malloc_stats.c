#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "helper.h"

void malloc_stats() {
  pthread_mutex_t mutex;
  int mutex_init_result = pthread_mutex_init(&mutex, NULL);
  assert(mutex_init_result == 0, __FILE__, __LINE__);
  pthread_mutex_lock(&mutex);

  if (!is_init()) {
    initialize();
  }

  pthread_mutex_unlock(&mutex);
  int mutex_destroy_result = pthread_mutex_destroy(&mutex);
  assert(mutex_destroy_result == 0, __FILE__, __LINE__);

  const char* STR_ARENA_0 = "Arena 0:";

  const char* STR_TOTAL_SIZE_BEGIN = "Total size of arena: ";
  const char* STR_TOTAL_SIZE_END = " bytes";
  const char* STR_NUM_BINS = "Total number of bins: ";
  const char* STR_BIN_BEGIN = "Bin ";
  const char* STR_BIN_END = ":";
  const char* STR_NUM_BLOCKS = "Total number of blocks: ";
  const char* STR_NUM_BLOCKS_USED = "Used blocks: ";
  const char* STR_NUM_BLOCKS_FREE = "Free blocks: ";
  const char* STR_ALLOCATION_REQUESTS = "Total allocation requests: ";
  const char* STR_FREE_REQUESTS = "Total free requests: ";

  int used_blocks[3];
  int free_blocks[3];
  int num_malloc_requests[3];
  int num_free_requests[3];
  int i;
  for (i = 0; i < NUM_BINS; i++) {
    used_blocks[i] = get_num_used_blocks(i);
    free_blocks[i] = get_num_free_blocks(i);
    num_malloc_requests[i] = get_num_malloc_requests(i);
    num_free_requests[i] = get_num_free_requests(i);
  }

  size_t mmap_size = get_mmap_size();

  size_t total_size = 0;
  for (i = 0; i < NUM_BINS; i++) {
    total_size += used_blocks[i] * BIN_SIZES[i];
    total_size += free_blocks[i] * BIN_SIZES[i];
  }
  total_size += mmap_size;

  char buf[1024];
  int BUF_LEN = 1024;

  snprintf(buf, BUF_LEN, "%s\n", STR_ARENA_0);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);

  snprintf(buf, BUF_LEN, "%s%zu%s\n", STR_TOTAL_SIZE_BEGIN, total_size,
           STR_TOTAL_SIZE_END);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);

  snprintf(buf, BUF_LEN, "%s%d\n", STR_NUM_BINS, NUM_BINS);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);

  for (i = 0; i < NUM_BINS; i++) {
    snprintf(buf, BUF_LEN, "\n%s%d%s\n", STR_BIN_BEGIN, i + 1, STR_BIN_END);
    write(STDOUT_FILENO, buf, strlen(buf) + 1);

    snprintf(buf, BUF_LEN, "%s%d\n", STR_NUM_BLOCKS,
             used_blocks[i] + free_blocks[i]);
    write(STDOUT_FILENO, buf, strlen(buf) + 1);

    snprintf(buf, BUF_LEN, "%s%d\n", STR_NUM_BLOCKS_USED, used_blocks[i]);
    write(STDOUT_FILENO, buf, strlen(buf) + 1);

    snprintf(buf, BUF_LEN, "%s%d\n", STR_NUM_BLOCKS_FREE, free_blocks[i]);
    write(STDOUT_FILENO, buf, strlen(buf) + 1);

    snprintf(buf, BUF_LEN, "%s%d\n", STR_ALLOCATION_REQUESTS,
             num_malloc_requests[i]);
    write(STDOUT_FILENO, buf, strlen(buf) + 1);

    snprintf(buf, BUF_LEN, "%s%d\n", STR_FREE_REQUESTS, num_free_requests[i]);
    write(STDOUT_FILENO, buf, strlen(buf) + 1);
  }
}