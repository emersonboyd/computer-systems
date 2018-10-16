#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>

#include "helper.h"

static int PAGE_SIZE;
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

void list_insert(MallocHeader *free_hdr) {
	size_t n_alloc_size = sizeof(MallocHeader) + sizeof(node_t);
	size_t n_request_size = round_up_to_page_size(n_alloc_size);
	MallocHeader *n_hdr = (MallocHeader *) mmap(0, n_request_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	assert(n_hdr != MAP_FAILED);
	n_hdr->size = n_request_size;
	n_hdr->is_mmaped = true;

	node_t *n = (node_t *) ((void *) n_hdr + sizeof(MallocHeader));
	
	n->h = free_hdr;

	int index = get_index_for_size(free_hdr->size);
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