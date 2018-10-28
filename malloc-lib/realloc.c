#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "helper.h"

// pointer to the realloc hook that exists before the realloc hook is updated
void* (*original_realloc_hook)(void*, size_t, const void*);

size_t
min(size_t a, size_t b)
{
  return a < b ? a : b;
}

void*
realloc(void* ptr, size_t size)
{
  // check if we should call our realloc hook
  if (__realloc_hook != original_realloc_hook) {
    return __realloc_hook(ptr, size, __builtin_return_address(0));
  }

  if (ptr == NULL) {
    return malloc(size);
  } else if (size <= 0) {
    free(ptr);
    return NULL;
  }

  void* ret = malloc(size);
  assert(ret != NULL, __FILE__, __LINE__);

  // copy over the old data to the new data
  MallocHeader* hdr_original = (MallocHeader*)(ptr - sizeof(MallocHeader));
  assert(hdr_original->offset == 0, __FILE__, __LINE__);
  size_t data_size = hdr_original->size - sizeof(MallocHeader);
  void* memcpy_result = memcpy(ret, ptr, min(size, data_size));
  assert(memcpy_result != NULL, __FILE__, __LINE__);

  free(ptr);
  return ret;
}

static __attribute__((constructor)) void
init_original_realloc_hook(void)
{
  original_realloc_hook = __realloc_hook;
}
