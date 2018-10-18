#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#include "helper.h"

static int PAGE_SIZE;
bool init = false;
head_t heads[3];

const char *ALLOC_SIZE_OUT_OF_RANGE = "Alloc size too large to get bin index\n";

void assert(bool condition) {
	if (!condition) {
		exit(1);
	}
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

	write(STDERR_FILENO, ALLOC_SIZE_OUT_OF_RANGE, strlen(ALLOC_SIZE_OUT_OF_RANGE) + 1);
	exit(1);
}

void *list_remove(size_t alloc_size) {
	int index = get_index_for_size(alloc_size);
	head_t *head = &heads[index];

	node_t *n = TAILQ_FIRST(head);

	// point further in memory to the farthest available memory block
	void *ret = ((void *) n) + n->num_free * n->size;

	if (n->num_free == 0) {
		TAILQ_REMOVE(head, n, nodes);
	}
	else {
		n->num_free -= 1;
	}

	return ret;
}

void list_insert(MallocHeader *free_hdr, int num_free_blocks) {
	assert(num_free_blocks > 0);
	assert(sizeof(node_t) <= free_hdr->size);

	// get the size on the stack before we overwrite it
	size_t size = free_hdr->size;
	node_t *n = (node_t *) free_hdr;
	n->size = size;
	n->num_free = num_free_blocks - 1;
	
	int index = get_index_for_size(free_hdr->size);
	head_t *head = &heads[index];

	TAILQ_INSERT_TAIL(head, n, nodes);
}

void init_bins() {
	PAGE_SIZE = getpagesize();

	int i;
	for (i = 0; i < NUM_BINS; i++) {
		TAILQ_INIT(&heads[i]);
	}

	init = true;
}

size_t round_up_to_page_size(size_t size) {
	if (size % PAGE_SIZE == 0) {
		return size;
	}

	return PAGE_SIZE * (size / PAGE_SIZE + 1);
}

bool is_init() {
	return init;
}