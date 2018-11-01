#include <malloc.h>
#include "helper.h"

struct mallinfo mallinfo() {
	initialize_helper_if_necessary();

	int arena = get_arena();

	int used_blocks[3];
    int free_blocks[3];
    int total_free_ordinary_blocks = 0;
    size_t total_free_size = 0;
    int i;
    for (i = 0; i < NUM_BINS; i++) {
      used_blocks[i] = get_num_used_blocks(arena, i);
      free_blocks[i] = get_num_free_blocks(arena, i);
      if (i > 0) {
      	total_free_ordinary_blocks += free_blocks[i];
      }
      total_free_size += free_blocks[i] * BIN_SIZES[i];
    }

    size_t mmap_size = get_mmap_size(arena);
    int num_mmaped_regions = get_num_mmaped_regions(arena);

    size_t total_size = 0;
    for (i = 0; i < NUM_BINS; i++) {
      total_size += used_blocks[i] * BIN_SIZES[i];
      total_size += free_blocks[i] * BIN_SIZES[i];
    }
    total_size += mmap_size;

    // convert the acquired statistics to a struct mallinfo
	struct mallinfo m;
	m.arena = total_size - mmap_size;
	m.ordblks = total_free_ordinary_blocks;
	m.smblks = used_blocks[0];
	m.hblks = num_mmaped_regions;
	m.hblkhd = mmap_size;
	m.usmblks = -1; // not used for threaded environments
	m.fsmblks = free_blocks[0] * BIN_SIZES[0];
	m.uordblks = total_size - total_free_size;
	m.fordblks = total_free_size;
	m.keepcost = m.arena;

	return m;
}