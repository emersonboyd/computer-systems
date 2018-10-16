#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "helper.h"

bool init = false;
head_t heads[3];

const char *ALLOC_SIZE_OUT_OF_RANGE = "Alloc size too large to get bin index\n";

// returns whether bins should be used for the given allocation size
bool use_bins_for_size(size_t alloc_size) {
	return alloc_size <= BIN_SIZES[NUM_BINS - 1];
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

void list_insert(MallocHeader *hdr) {
	node_t *n = (node_t *) malloc(sizeof(node_t));
	assert(n != NULL);
	
	n->h = hdr;

	int index = get_index_for_size(hdr->size);

	head_t *head = &heads[index];

	// char buf[1024];
	// snprintf(buf, 1024, "Inserting at bin index %d with size %lu at head pointer %p\n", index, hdr->size, head);
	// write(STDOUT_FILENO, buf, strlen(buf) + 1);

	TAILQ_INSERT_TAIL(head, n, nodes);

	// write(STDOUT_FILENO, "finished insert\n", strlen("finished insert\n"));
}

void list_print(size_t bin_size) {
	int index = get_index_for_size(bin_size);
	head_t *head = &heads[index];

	char buf[1024];
	snprintf(buf, 1024, "Bin %lu head at pointer %p\n", bin_size, head);
	write(STDOUT_FILENO, buf, strlen(buf) + 1);

    struct node * e = NULL;
    TAILQ_FOREACH(e, head, nodes)
    {
		char buf[1024];
		snprintf(buf, 1024, "Bin %lu: %lu bytes free at %p\n", bin_size, e->h->size, e->h);
		write(STDOUT_FILENO, buf, strlen(buf) + 1);
    }
}

void init_bins() {
	int i;
	for (i = 0; i < NUM_BINS; i++) {
		TAILQ_INIT(&heads[i]);
		char buf[1024];
		snprintf(buf, 1024, "Initialized head at %p\n", &heads[i]);
		write(STDOUT_FILENO, buf, strlen(buf) + 1);
	}

	init = true;
}

bool is_init() {
	return init;
}