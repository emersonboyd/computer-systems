#include <malloc.h>
#include <string.h>
#include <assert.h>

#include "helper.h"

void *calloc(size_t nmemb, size_t size) {
	if (nmemb <= 0 || size <= 0) {
		return NULL;
	}

	size_t n_bytes = nmemb * size;
	void *ret = malloc(n_bytes);
	assert(ret != NULL);
	void *memset_result = memset(ret, 0, n_bytes);
	assert(memset_result != NULL);

	list_print(32);
	list_print(128);
	list_print(512);

	return ret;
}
