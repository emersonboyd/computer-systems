#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "helper.h"

// pointer to the malloc hook that exists before the malloc hook is updated
void* (*original_malloc_hook)(size_t, const void*);

void*
malloc(size_t size)
{
  // check if we should call our malloc hook
  if (__malloc_hook != original_malloc_hook) {
    return __malloc_hook(size, __builtin_return_address(0));
  }

  if (size <= 0) {
    return NULL;
  }

  initialize_helper_if_necessary();

  int lock_result = pthread_mutex_lock(&BASE_MUTEX);
  assert(lock_result == 0, __FILE__, __LINE__);
  char buf[1024];
  snprintf(buf, 1024, "Lock at file %s line %d\n", __FILE__, __LINE__);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);

  // make sure our MallocHeader will align our memory to 8 bytes
  assert(sizeof(MallocHeader) % ALIGN_BYTES == 0, __FILE__, __LINE__);

  // validate sizes
  size_t alloc_size = size + sizeof(MallocHeader);

  void* ret = NULL;

  write(STDOUT_FILENO, "k1\n", 4);

  // check if we can get away with re-using previously allocated memory, else we
  // allocate new memory
  if (use_bins_for_size(alloc_size) && has_free_block_for_size(alloc_size)) {
    ret = list_remove(alloc_size);
    assert(ret != NULL, __FILE__, __LINE__);
    MallocHeader* hdr = (MallocHeader*)ret;
    int index = get_index_for_size(alloc_size);
    alloc_size = BIN_SIZES[index]; // update the alloc_size based on the bin
                                   // we're pulling from
    hdr->size = alloc_size;
    hdr->offset = 0; // if we're getting a memory address from a bin, we know
                     // that it's inplace, so offset is zero
    assert(round_up_to_page_size(alloc_size) % alloc_size == 0, __FILE__,
           __LINE__);
    increment_used_blocks(index);         // update our malloc_stats
    increment_num_malloc_requests(index); // update our malloc_stats
  } else {
    // get the size we need to request from sbrk or mmap
    size_t request_size = round_up_to_page_size(alloc_size);

    // allocate the memory in a different way depending on the size
    if (use_bins_for_size(alloc_size)) {
      int index = get_index_for_size(alloc_size);
      alloc_size = BIN_SIZES[index];
      ret = sbrk(request_size);
      if (ret < 0) {
        errno = ENOMEM;
        return NULL;
      }
      assert(ret >= 0, __FILE__, __LINE__);

      // here, we have to do the work of adding the remaining parts of
      // request_size to empty bins
      request_size -= alloc_size;
      assert(request_size % alloc_size == 0, __FILE__, __LINE__);
      int num_free_blocks = request_size / alloc_size;
      void* free_ptr = (void*)(ret + alloc_size);
      MallocHeader* free_ptr_hdr = (MallocHeader*)free_ptr;
      free_ptr_hdr->size = alloc_size;
      free_ptr_hdr->offset = 0;
      write(STDOUT_FILENO, "k2\n", 4);
      snprintf(
        buf, 1024,
        "Have a free pointer header at %p with number of free blocks %d\n",
        free_ptr_hdr, num_free_blocks);
      write(STDOUT_FILENO, buf, strlen(buf) + 1);
      list_insert(free_ptr_hdr, num_free_blocks);
      write(STDOUT_FILENO, "k3\n", 4);
      increment_used_blocks(index); // update our malloc_stats
      write(STDOUT_FILENO, "k4\n", 4);
      increment_num_malloc_requests(index); // update our malloc_stats
    } else {

      ret = mmap(0, request_size, PROT_READ | PROT_WRITE,
                 MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
      assert(ret != MAP_FAILED, __FILE__, __LINE__);
      alloc_size = request_size;
      increment_mmap_size(alloc_size); // update our malloc_stats
    }

    // update the information in our header accordingly
    MallocHeader* hdr = (MallocHeader*)ret;
    hdr->size = alloc_size;
    hdr->offset = 0;
  }

  snprintf(buf, 1024, "Unlock at file %s line %d\n", __FILE__, __LINE__);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);
  int unlock_result = pthread_mutex_unlock(&BASE_MUTEX);
  assert(unlock_result == 0, __FILE__, __LINE__);

  // make sure our malloc is aligned to 8 bytes
  assert(is_aligned(ret + sizeof(MallocHeader), ALIGN_BYTES), __FILE__,
         __LINE__);
  return ret + sizeof(MallocHeader);
}

static __attribute__((constructor)) void
init_original_malloc_hook(void)
{
  original_malloc_hook = __malloc_hook;
}
