#include <malloc.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include "helper.h"

void free(void *ptr) {
	if (ptr == NULL) {
		return;
	}

	pthread_mutex_t mutex;
	int mutex_init_result = pthread_mutex_init(&mutex, NULL);
	assert(mutex_init_result == 0);
	pthread_mutex_lock(&mutex);

	if (!is_init()) {
		init_bins();
	}

	MallocHeader *hdr = (MallocHeader *) (ptr - sizeof(MallocHeader));

	char buf[1024];
	snprintf(buf, 1024, "%s:%d free(): Freeing %lu bytes at %p\n",
					 __FILE__, __LINE__, hdr->size, hdr);
	write(STDOUT_FILENO, buf, strlen(buf) + 1);

	// if we have an allocated size greater than the bin holds, we just "munmap" the area
	if (!use_bins_for_size(hdr->size)) {
		write(STDOUT_FILENO, "freeing with unmap\n", strlen("freeing with unmap\n") + 1);
		int munmap_result = munmap(hdr, hdr->size);
		assert(munmap_result >= 0);
	}
	else {
		write(STDOUT_FILENO, "freeing by adding to bin\n", strlen("freeing by adding to bin\n") + 1);
		list_insert(hdr, 1);
	}

	pthread_mutex_unlock(&mutex);
	int mutex_destroy_result = pthread_mutex_destroy(&mutex);
	assert(mutex_destroy_result == 0);

	return;
}
