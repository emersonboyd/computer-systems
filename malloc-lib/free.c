#include <malloc.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "helper.h"

void free(void *ptr) {
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

	// write(STDOUT_FILENO, "list_insert start\n", strlen("list_insert start\n") + 1);
	list_insert(hdr);
	// write(STDOUT_FILENO, "list_insert finish\n", strlen("list_insert finish\n") + 1);


	// int munmap_result = munmap(hdr, hdr->size);
	// assert(munmap_result >= 0);

	return;
}
