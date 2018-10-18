#include <malloc.h>
#include <string.h>

#include "helper.h"

void *calloc(size_t nmemb, size_t size) {
	if (nmemb <= 0 || size <= 0) {
		return NULL;
	}

	size_t n_bytes = nmemb * size;
	void *ret = malloc(n_bytes);
	assert(ret != NULL, __FILE__, __LINE__);
	void *memset_result = memset(ret, 0, n_bytes);
	assert(memset_result != NULL, __FILE__, __LINE__);

	return ret;
}
