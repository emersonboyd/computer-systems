#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "helper.h"

// pointer to the free hook that exists before the free hook is updated
void (*original_free_hook)(void*, const void*);

void
free(void* ptr)
{
  // check if we should call our free hook
  if (__free_hook != original_free_hook) {
    return __free_hook(ptr, __builtin_return_address(0));
  }

  if (ptr == NULL) {
    return;
  }

  initialize_helper_if_necessary();

  char buf[1024];
  snprintf(buf, 1024, "Lock at file %s line %d\n", __FILE__, __LINE__);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);
  int lock_result = pthread_mutex_lock(&mutexs[get_arena()]);
  assert(lock_result == 0, __FILE__, __LINE__);

  MallocHeader* hdr = (MallocHeader*)(ptr - sizeof(MallocHeader));

  if (hdr->size == 0) {
    int unlock_result = pthread_mutex_unlock(&mutexs[get_arena()]);
    assert(unlock_result == 0, __FILE__, __LINE__);
    return;
  }

  assert(hdr->size == BIN_SIZES[0] || hdr->size == BIN_SIZES[1] ||
           hdr->size == BIN_SIZES[2] || hdr->size > BIN_SIZES[2],
         __FILE__, __LINE__);

  // if we have an allocated size greater than the bin holds, we just "munmap"
  // the area
  if (!use_bins_for_size(hdr->size)) {
    // hold these values on the stack so we can update our malloc_stats later
    size_t offset = hdr->offset;
    size_t size = hdr->size;

    lock_result = pthread_mutex_lock(&BASE_MUTEX);
    assert(lock_result == 0, __FILE__, __LINE__);

    int munmap_result =
      munmap((void*)hdr - hdr->offset, hdr->offset + hdr->size);

    int unlock_result = pthread_mutex_unlock(&BASE_MUTEX);
    assert(unlock_result == 0, __FILE__, __LINE__);

    if (munmap_result < 0) {
      perror("failed to map region");
    }

    assert(munmap_result >= 0, __FILE__, __LINE__);
    decrement_mmap_size(offset + size); // update our malloc_stats
  } else {
    assert(hdr->offset == 0, __FILE__, __LINE__);
    list_insert(hdr, 1);

    int index = get_index_for_size(hdr->size);
    decrement_used_blocks(index);       // update our malloc_stats
    increment_num_free_requests(index); // update our malloc_stats
  }

  snprintf(buf, 1024, "Unlock at file %s line %d\n", __FILE__, __LINE__);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);
  int unlock_result = pthread_mutex_unlock(&mutexs[get_arena()]);
  assert(unlock_result == 0, __FILE__, __LINE__);

  return;
}

static __attribute__((constructor)) void
init_original_free_hook(void)
{
  original_free_hook = __free_hook;
}
