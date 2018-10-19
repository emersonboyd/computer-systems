#include <malloc.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "helper.h"

static const char* ERROR_MESSAGE_ALIGNMENT_NOT_POWER_OF_TWO =
    "Given alignment must be a power of two\n";

// pointer to the memalign hook that exists before the memalign hook is updated
void* (*original_memalign_hook)(size_t, size_t, const void*);

size_t max(size_t a, size_t b) { return a > b ? a : b; }

void* memalign(size_t alignment, size_t size) {
  // check if we should call our memalign hook
  if (__memalign_hook != original_memalign_hook) {
    return __memalign_hook(alignment, size, __builtin_return_address(0));
  }

  // check if alignment is a power of two
  if ((alignment == 0) || (alignment & (alignment - 1)) != 0) {
    write(STDERR_FILENO, ERROR_MESSAGE_ALIGNMENT_NOT_POWER_OF_TWO,
          strlen(ERROR_MESSAGE_ALIGNMENT_NOT_POWER_OF_TWO) + 1);
    exit(1);
  }

  if (size <= 0) {
    return NULL;
  }

  void* ret;

  // we know malloc already aligns to ALIGN_BYTES so we can check if our
  // alignment is at most ALIGN_BYTES
  if (alignment <= ALIGN_BYTES) {
    ret = malloc(size);
  } else {
    // here, we are basically ensuring that we allocate enough space to make
    // alignment possible
    size_t size_factor = (alignment / ALIGN_BYTES);
    size_t single_data_size =
        max(size_factor * size, BIN_SIZES[NUM_BINS - 1] * 2);
    size_t data_size = single_data_size * 3;
    size_t request_size = data_size + 2 * sizeof(MallocHeader);

    ret = malloc(request_size);

    assert(ret != NULL, __FILE__, __LINE__);

    // first_hdr is the header populated by malloc
    // second_hdr is the header that we will populate in front of the aligned
    // memory
    MallocHeader* first_hdr = (MallocHeader*)(ret - sizeof(MallocHeader));
    MallocHeader* second_hdr = (MallocHeader*)(ret + sizeof(MallocHeader));

    size_t full_alloc_size = first_hdr->size;

    assert(first_hdr->size == full_alloc_size, __FILE__, __LINE__);
    assert(first_hdr->size > BIN_SIZES[NUM_BINS - 1] * 2, __FILE__, __LINE__);
    assert(second_hdr != NULL, __FILE__, __LINE__);

    // keep incrementing our pointertto the second header until the pointer is
    // properly aligned
    void* ptr;
    for (ptr = (void*)second_hdr; !is_aligned(ptr, alignment);
         ptr += sizeof(MallocHeader)) {
    }

    // ptr is at the aligned memory address, so we create a MallocHeader one
    // sizeof(MallocHeader) behind ptr
    second_hdr = (MallocHeader*)(ptr - sizeof(MallocHeader));
    first_hdr->size = (size_t)second_hdr - (size_t)first_hdr;
    second_hdr->size = full_alloc_size - first_hdr->size;
    second_hdr->offset = first_hdr->size;

    // ensure we will munmap this pointer (because we know we mmaped it to begin
    // with)
    assert(!use_bins_for_size(second_hdr->size), __FILE__, __LINE__);

    ret = (void*)second_hdr + sizeof(MallocHeader);
  }

  assert((ret != NULL) && is_aligned(ret, alignment), __FILE__, __LINE__);

  return ret;
}

static __attribute__((constructor)) void init_original_memalign_hook(void) {
  original_memalign_hook = __memalign_hook;
}
