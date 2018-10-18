#include <malloc.h>
#include <string.h>
#include "helper.h"

size_t min(size_t a, size_t b) {
	return a < b ? a : b;
}

void *realloc(void *ptr, size_t size) {
	if (ptr == NULL && size <= 0) {
		return NULL;
	}
	else if (size <= 0) {
		free(ptr);
		return NULL;
	}
	else if (ptr == NULL) {
		return malloc(size);
	}

	void *ret = malloc(size);
	assert(ret != NULL);

	MallocHeader *hdr_original = (MallocHeader *) (ptr - sizeof(MallocHeader));
	size_t data_size = hdr_original->size - sizeof(MallocHeader);
	void *memcpy_result = memcpy(ret, ptr, min(size, data_size));
	assert(memcpy_result != NULL);

	free(ptr);
	return ret;
}
