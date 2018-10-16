#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "helper.h"

void *realloc(void *ptr, size_t size) {
	if (ptr == NULL || size <= 0) {
		return NULL;
	}

	void *ret = malloc(size);
	if (ret == NULL) {
		return NULL;
	}

	MallocHeader *hdr_original = (MallocHeader *) (ptr - sizeof(MallocHeader));
	size_t data_size = hdr_original->size - sizeof(MallocHeader);
	void *memcpy_result = memcpy(ret, ptr, data_size);
	assert(memcpy_result != NULL);

	free((void *) hdr_original + sizeof(MallocHeader));
	return ret;
}
