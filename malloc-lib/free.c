#include <malloc.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include "helper.h"

void free(void *ptr) {
	pthread_mutex_t mutex;
	pthread_mutex_lock(&mutex);

	if (!is_init()) {
		init_bins();
	}

	if (ptr == NULL) {
		return;
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

	return;
}
