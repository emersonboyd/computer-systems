#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#include "helper.h"

static const char *ERROR_MESSAGE_ALIGNMENT_NOT_POWER_OF_TWO = "Given alignment must be a power of two\n";

size_t max(size_t a, size_t b) {
	return a > b ? a : b;
}

void *memalign(size_t alignment, size_t size) {
	if ((alignment == 0) || (alignment & (alignment - 1)) != 0) {
		write(STDERR_FILENO, ERROR_MESSAGE_ALIGNMENT_NOT_POWER_OF_TWO, strlen(ERROR_MESSAGE_ALIGNMENT_NOT_POWER_OF_TWO) + 1);
		exit(1);
	}

	if (size <= 0) {
		return NULL;
	}

	void *ret;

	// we know malloc already aligns to ALIGN_BYTES so we can check if our alignment is at most ALIGN_BYTES
	if (alignment <= ALIGN_BYTES) {
		ret = malloc(size);
	}
	else {
		size_t size_factor = (alignment / ALIGN_BYTES);
		size_t single_data_size = max(size_factor * size, BIN_SIZES[NUM_BINS - 1] * 2);
		size_t data_size = single_data_size * 3;
		size_t request_size = data_size + 2 * sizeof(MallocHeader);

		char buf[1024];
		snprintf(buf, 1024, "requesting allocation of %zu bytes\n", request_size);
		write(STDOUT_FILENO, buf, strlen(buf) + 1);

		ret = malloc(data_size + 2 * sizeof(MallocHeader));

		snprintf(buf, 1024, "just allocated %zu bytes at %p\n", request_size, ret);
		write(STDOUT_FILENO, buf, strlen(buf) + 1);

		assert(ret != NULL, __FILE__, __LINE__);

		MallocHeader *first_hdr = (MallocHeader *) (ret - sizeof(MallocHeader));
		MallocHeader *second_hdr = (MallocHeader *) (ret + sizeof(MallocHeader));

		snprintf(buf, 1024, "first header size is %zu bytes\n", first_hdr->size);
		write(STDOUT_FILENO, buf, strlen(buf) + 1);

		size_t full_alloc_size = first_hdr->size;

		assert(first_hdr->size == full_alloc_size, __FILE__, __LINE__);
		assert(first_hdr->size > BIN_SIZES[NUM_BINS - 1] * 2, __FILE__, __LINE__);
		assert(second_hdr != NULL, __FILE__, __LINE__);

		// keep incrementing the 
		void *ptr;
		for (ptr = (void *) second_hdr; !is_aligned(ptr, alignment); ptr += sizeof(MallocHeader)) {
			write(STDOUT_FILENO, "k\n", 3);
		}

		// ptr is at the aligned memory address, so we create a MallocHeader one sizeof(MallocHeader) behind ptr
		second_hdr = (MallocHeader *) (ptr - sizeof(MallocHeader));
		first_hdr->size = (size_t) second_hdr - (size_t) first_hdr;
		second_hdr->size = full_alloc_size - first_hdr->size;
		second_hdr->offset = first_hdr->size;

		// make sure we're gonna mmap this ish
		assert(!use_bins_for_size(second_hdr->size), __FILE__, __LINE__);

		snprintf(buf, 1024, "first header is at %p with size %zu, second header at %p with size %zu\n", first_hdr, first_hdr->size, second_hdr, second_hdr->size);
		write(STDOUT_FILENO, buf, strlen(buf) + 1);

		write(STDOUT_FILENO, "Setting return pointer\n", strlen("Setting return pointer\n") + 1);
		ret = (void *) second_hdr + sizeof(MallocHeader);
	}

	assert((ret != NULL) && is_aligned(ret, alignment), __FILE__, __LINE__);

	return ret;
}
