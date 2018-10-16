#ifndef EBOYD_HELPER_H
#define EBOYD_HELPER_H

#include <stdbool.h>
#include <stdlib.h>
#include <sys/queue.h>

static const int NUM_BINS = 3;
static const size_t BIN_SIZES[] = {32, 128, 512};

typedef struct
{
  size_t size; // the allocation size including the size of the header
  bool is_mmaped;
} MallocHeader;

typedef struct node
{
    MallocHeader *h;
    TAILQ_ENTRY(node) nodes;
} node_t;

typedef TAILQ_HEAD(head_s, node) head_t;

extern head_t heads[3];

void init_bins();
bool is_init();
void list_print(size_t bin_size);
void list_insert(MallocHeader *hdr);
bool use_bins_for_size(size_t alloc_size);
int get_index_for_size(size_t alloc_size);
size_t round_up_to_page_size(size_t size);

#endif