#include <malloc.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include "helper.h"

void *malloc(size_t size) {
	pthread_mutex_t mutex;
	int mutex_init_result = pthread_mutex_init(&mutex, NULL);
	assert(mutex_init_result == 0);
	pthread_mutex_lock(&mutex);

	if (!is_init()) {
		init_bins();
	}

	if (size <= 0) {
		return NULL;
	}

	// validate sizes
	size_t alloc_size = size + sizeof(MallocHeader);

	void *ret = NULL;

	// check if we can get away with re-using previously allocated memory, else we allocate new memory
	if (use_bins_for_size(alloc_size) && has_free_block_for_size(alloc_size)) {
		ret = list_remove(alloc_size);
		assert(ret != NULL);
		MallocHeader *hdr = (MallocHeader *) ret;
		hdr->size = BIN_SIZES[get_index_for_size(alloc_size)];
		
		char buf[1024];
		snprintf(buf, 1024, "Just got a header at %p from bin of %zu bytes\n", hdr, hdr->size);
		write(STDOUT_FILENO, buf, strlen(buf) + 1);
	}
	else {
		// get the size we need to request from sbrk or mmap
		size_t request_size = round_up_to_page_size(alloc_size);

		// allocate the memory in a different way depending on the size
		if (use_bins_for_size(alloc_size)) {
			int index = get_index_for_size(alloc_size);
			alloc_size = BIN_SIZES[index];
			ret = sbrk(request_size);
			assert(ret >= 0);
			
			char buf[1024];
			snprintf(buf, 1024, "Just requested %zu bytes for an allocation size of %zu and MallocHeader size of %zu\n", request_size, alloc_size, sizeof(MallocHeader));
			write(STDOUT_FILENO, buf, strlen(buf) + 1);

			// here, we have to do the work of adding the remaining parts of request_size to empty bins
			request_size -= alloc_size;
			int num_free_blocks = request_size / alloc_size;
			void *free_ptr = (void *) (ret + alloc_size);
			MallocHeader *free_ptr_hdr = (MallocHeader *) free_ptr;
			free_ptr_hdr->size = alloc_size;
			list_insert(free_ptr_hdr, num_free_blocks);
		}
		else {
			ret = mmap(0, request_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
			assert(ret != MAP_FAILED);
			alloc_size = request_size;
		}

		MallocHeader *hdr = (MallocHeader *) ret;
		hdr->size = alloc_size;
	}

	pthread_mutex_unlock(&mutex);
	int mutex_destroy_result = pthread_mutex_destroy(&mutex);
	assert(mutex_destroy_result == 0);
	
	char buf[1024];
	snprintf(buf, 1024, "%s:%d malloc(%zu): Allocated %zu bytes at %p\n",
					 __FILE__, __LINE__, size, alloc_size, ret);
	write(STDOUT_FILENO, buf, strlen(buf) + 1);

	// make sure our malloc is aligned to 8 bytes
	assert(((unsigned long) (ret + sizeof(MallocHeader)) & 7) == 0);
	return ret + sizeof(MallocHeader);
}
