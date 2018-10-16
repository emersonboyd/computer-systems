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

	int index = get_index_for_size(alloc_size);
	head_t *head = &heads[index];

	void *ret = NULL;
	bool allocate_memory = true;

	// check if we can get away with re-using previously allocated memory
	if (!TAILQ_EMPTY(head)) {
		node_t *n = NULL;

		if (index < NUM_BINS) {
			n = TAILQ_FIRST(head);
			TAILQ_REMOVE(head, n, nodes);
			ret = n->h;
			free(n);
		}
		else {
			TAILQ_FOREACH(n, head, nodes)
	    	{
	        	if (alloc_size <= n->h->size) {
	        		TAILQ_REMOVE(head, n, nodes);
	        		ret = n->h;
	        		free(n);
	        		break;
	        	}

	       		// if there were not options large enough to hold our alloc_size, we need to allocate new memory
	    	}
		}
	    
		if (ret != NULL) {
			allocate_memory = false;
			MallocHeader *hdr = (MallocHeader *) ret;
			alloc_size = hdr->size;
		}
	}

	// allocate memory, if necessary
	if (allocate_memory) {
		// allocate the memory in a different way depending on the size
		if (index < NUM_BINS) {
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
	
	// We can't use printf here because printf internally calls `malloc` and thus
	// we'll get into an infinite recursion leading to a segfault.
	// Instead, we first write the message into a string and then use the `write`
	// system call to display it on the console.
	char buf[1024];
	snprintf(buf, 1024, "%s:%d malloc(%zu): Allocated %zu bytes at %p\n",
					 __FILE__, __LINE__, size, alloc_size, ret);
	write(STDOUT_FILENO, buf, strlen(buf) + 1);

	return ret + sizeof(MallocHeader);
}
