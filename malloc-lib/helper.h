#ifndef EBOYD_HELPER_H
#define EBOYD_HELPER_H

#include <stdbool.h>
#include <stdlib.h>
#include <sys/queue.h>

static const int NUM_BINS = 3;
static const size_t BIN_SIZES[] = {64, 128, 512};
static const unsigned long ALIGN_BYTES = 8;

typedef struct
{
  size_t size; // the allocation size to the end of the memory region allocation, including the size of the header
  size_t offset; // this represents the bytes offset from the beginning of the region allocation, often zero
} MallocHeader;

typedef struct node
{
    size_t size;
    int num_free;
    TAILQ_ENTRY(node) nodes;
} node_t;

typedef TAILQ_HEAD(head_s, node) head_t;

extern head_t heads[3];

void assert(bool condition, const char *fname, int lineno);
void init_bins();
bool is_init();
void *list_remove(size_t alloc_size);
void list_insert(MallocHeader *hdr, int num_free_blocks);
bool use_bins_for_size(size_t alloc_size);
bool has_free_block_for_size(size_t alloc_size);
int get_index_for_size(size_t alloc_size);
size_t round_up_to_page_size(size_t size);
bool is_aligned(void *ptr, size_t alignment);

#endif