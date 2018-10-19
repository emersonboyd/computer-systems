#include <malloc.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "helper.h"

void free(void *ptr) {
	if (ptr == NULL) {
		return;
	}

	pthread_mutex_lock(&MUTEX);

	if (!is_init()) {
		init_bins();
	}

	MallocHeader *hdr = (MallocHeader *) (ptr - sizeof(MallocHeader));
	assert(hdr->size == BIN_SIZES[0] || hdr->size == BIN_SIZES[1] || hdr->size == BIN_SIZES[2] || hdr->size > BIN_SIZES[2], __FILE__, __LINE__);

	// if we have an allocated size greater than the bin holds, we just "munmap" the area
	if (!use_bins_for_size(hdr->size)) {
		// hold these values on the stack so we can update our malloc_stats later
		size_t offset = hdr->offset;
		size_t size = hdr->size;

		int munmap_result = munmap((void *) hdr - hdr->offset, hdr->offset + hdr->size);
		if (munmap_result < 0) {
			perror("failed to map region");
		}
		assert(munmap_result >= 0, __FILE__, __LINE__);
		decrement_mmap_size(offset + size); // update our malloc_stats
	}
	else {
		assert(hdr->offset == 0, __FILE__, __LINE__);
		list_insert(hdr, 1);
		int index = get_index_for_size(hdr->size);
		decrement_used_blocks(index); // update our malloc_stats
		increment_num_free_requests(index); // update our malloc_stats
	}

	pthread_mutex_unlock(&MUTEX);
	return;
}
