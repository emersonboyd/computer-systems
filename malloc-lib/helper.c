#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>

#include "helper.h"

head_t heads[4];

bool init = false;

int get_index_for_size(size_t alloc_size) {
	int i;
	for (i = 0; i < NUM_BINS; i++) {
		if (alloc_size <= BIN_SIZES[i]) {
			return i;
		}
	}

	return NUM_BINS;
}

void list_print(size_t bin_size) {
	int index = get_index_for_size(bin_size);
	head_t *head = &heads[index];

	char buf[1024];
	snprintf(buf, 1024, "Bin %lu\n", bin_size);
	write(STDOUT_FILENO, buf, strlen(buf) + 1);

    struct node * e = NULL;
    TAILQ_FOREACH(e, head, nodes)
    {
		char buf[1024];
		snprintf(buf, 1024, "Bin %lu: %lu bytes free at %p\n", bin_size, e->h->size, e->h);
		write(STDOUT_FILENO, buf, strlen(buf) + 1);
    }
}

void list_insert(MallocHeader *hdr) {
	node_t *n = (node_t *) my_malloc(sizeof(node_t));
	assert(n != NULL);
	
	n->h = hdr;

	int index = get_index_for_size(hdr->size);
	head_t *head = &heads[index];
	TAILQ_INSERT_TAIL(head, n, nodes);
}

void init_bins() {
	int i;
	for (i = 0; i <= NUM_BINS; i++) {
		TAILQ_INIT(&heads[i]);
	}

	init = true;
}

bool is_init() {
	return init;
}

void *my_malloc(size_t size) {
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
		}
		else {
			TAILQ_FOREACH(n, head, nodes)
	    	{
	        	if (alloc_size <= n->h->size) {
	        		TAILQ_REMOVE(head, n, nodes);
	        		ret = n->h;
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

void my_free(void *ptr) {
	if (ptr == NULL) {
		return;
	}

	MallocHeader *hdr = (MallocHeader *) (ptr - sizeof(MallocHeader));

	char buf[1024];
	snprintf(buf, 1024, "%s:%d free(): Freeing %lu bytes at %p\n",
					 __FILE__, __LINE__, hdr->size, hdr);
	write(STDOUT_FILENO, buf, strlen(buf) + 1);

	list_insert(hdr);

	// int munmap_result = munmap(hdr, hdr->size);
	// assert(munmap_result >= 0);

	return;
}

void *my_calloc(size_t nmemb, size_t size) {
	if (nmemb <= 0 || size <= 0) {
		return NULL;
	}

	size_t n_bytes = nmemb * size;
	void *ret = my_malloc(n_bytes);
	assert(ret != NULL);
	void *memset_result = memset(ret, 0, n_bytes);
	assert(memset_result != NULL);

	return ret;
}

void *my_realloc(void *ptr, size_t size) {
	if (ptr == NULL || size <= 0) {
		return NULL;
	}

	MallocHeader *hdr_original = (MallocHeader *) (ptr - sizeof(MallocHeader));
	void *ret = my_malloc(size);
	if (ret == NULL) {
		return NULL;
	}

	void *memcpy_result = memcpy(ret, ptr, hdr_original->size);
	assert(memcpy_result != NULL);

	my_free(hdr_original);
	return ret;
}