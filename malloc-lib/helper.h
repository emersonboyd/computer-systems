#ifndef EBOYD_HELPER_H
#define EBOYD_HELPER_H

#include <stdbool.h>
#include <stdlib.h>
#include <sys/queue.h>

static const int NUM_BINS = 3;
static const size_t BIN_SIZES[] = {32, 128, 512};

typedef struct
{
  size_t size;
} MallocHeader;

typedef struct node
{
    MallocHeader *h;
    TAILQ_ENTRY(node) nodes;
} node_t;

typedef TAILQ_HEAD(head_s, node) head_t;

void init_bins();
bool is_init();
void list_print(size_t bin_size);

void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_calloc(size_t nmemb, size_t size);

#endif