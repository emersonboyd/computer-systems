#include <malloc.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "helper.h"

void *malloc(size_t size) {
	if (!is_init()) {
		init_bins();
	}

	if (size <= 0) {
		return NULL;
	}

	// validate size.
	size_t alloc_size = size + sizeof(MallocHeader);


	void *ret = NULL;
	bool allocate_memory = true;

	// check if we can get away with re-using previously allocated memory
	if (use_bins_for_size(alloc_size)) {
		int index = get_index_for_size(alloc_size);
		head_t *head = &heads[index];

		// only use bins if there are bins available
		if (!TAILQ_EMPTY(head)) {
			node_t *n = NULL;

			n = TAILQ_FIRST(head);
			TAILQ_REMOVE(head, n, nodes);
			ret = n->h;
			assert(ret != NULL);

			free(n);
			
			allocate_memory = false;
			MallocHeader *hdr = (MallocHeader *) ret;
			alloc_size = hdr->size;
			
			char buf[1024];
			snprintf(buf, 1024, "Just got a header from bin of %zu bytes\n", alloc_size);
			write(STDOUT_FILENO, buf, strlen(buf) + 1);
		}
		
	}

	// allocate memory, if necessary
	if (allocate_memory) {
		// allocate the memory in a different way depending on the size
		if (use_bins_for_size(alloc_size)) {
			int index = get_index_for_size(alloc_size);
			alloc_size = BIN_SIZES[index];
			ret = sbrk(alloc_size);
			assert(ret >= 0);
		}
		else {
			ret = mmap(0, alloc_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
			assert(ret != MAP_FAILED);
		}

		MallocHeader *hdr = (MallocHeader *) ret;
		hdr->size = alloc_size;
	}
	
	char buf[1024];
	snprintf(buf, 1024, "%s:%d malloc(%zu): Allocated %zu bytes at %p\n",
					 __FILE__, __LINE__, size, alloc_size, ret);
	write(STDOUT_FILENO, buf, strlen(buf) + 1);

	return ret + sizeof(MallocHeader);
}
