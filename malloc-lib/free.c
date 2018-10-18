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

	pthread_mutex_t mutex;
	int mutex_init_result = pthread_mutex_init(&mutex, NULL);
	assert(mutex_init_result == 0, __FILE__, __LINE__);
	pthread_mutex_lock(&mutex);

	if (!is_init()) {
		init_bins();
	}

	MallocHeader *hdr = (MallocHeader *) (ptr - sizeof(MallocHeader));
	assert(hdr->size == BIN_SIZES[0] || hdr->size == BIN_SIZES[1] || hdr->size == BIN_SIZES[2] || hdr->size > BIN_SIZES[2], __FILE__, __LINE__);

	// if we have an allocated size greater than the bin holds, we just "munmap" the area
	if (!use_bins_for_size(hdr->size)) {
		int munmap_result = munmap((void *) hdr - hdr->offset, hdr->offset + hdr->size);
		if (munmap_result < 0) {
			perror("failed to map region");
		}
		assert(munmap_result >= 0, __FILE__, __LINE__);
	}
	else {
		assert(hdr->offset == 0, __FILE__, __LINE__);
		list_insert(hdr, 1);
	}

	pthread_mutex_unlock(&mutex);
	int mutex_destroy_result = pthread_mutex_destroy(&mutex);
	assert(mutex_destroy_result == 0, __FILE__, __LINE__);

	return;
}
