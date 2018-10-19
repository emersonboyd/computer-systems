#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "helper.h"

size_t min(size_t a, size_t b) {
	return a < b ? a : b;
}

void *realloc(void *ptr, size_t size) {
	if (ptr == NULL) {
		return malloc(size);
	}
	else if (size <= 0) {
		free(ptr);
		return NULL;
	}

	void *ret = malloc(size);
	assert(ret != NULL, __FILE__, __LINE__);

	MallocHeader *hdr_original = (MallocHeader *) (ptr - sizeof(MallocHeader));
	assert(hdr_original->offset == 0, __FILE__, __LINE__);
	size_t data_size = hdr_original->size - sizeof(MallocHeader);
	void *memcpy_result = memcpy(ret, ptr, min(size, data_size));
	assert(memcpy_result != NULL, __FILE__, __LINE__);

	free(ptr);
	return ret;
}
