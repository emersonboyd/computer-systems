#include <fcntl.h> // for open
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h> // for mmap
#include <ucontext.h> // for reading / writing registers
#include <unistd.h>   // for read, close

#include "helper.h"

char checkpoint_path[1000];
void* new_stack_addr = (void*)0x5300000;

void unmap_old_stack() {
  int fdr = open("/proc/self/maps", O_RDONLY);
  if (fdr < 0) {
    printf("Error opening maps\n");
    exit(1);
  }

  MemoryRegion* old_stack_mem_reg = get_stack_region(fdr);
  if (old_stack_mem_reg == NULL) {
    printf("Error finding the old stack memory region\n");
    exit(1);
  }

  int unmap_result =
      munmap(old_stack_mem_reg->startAddress, old_stack_mem_reg->size);
  if (unmap_result < 0) {
    printf("Failed to unmap the old stack memory\n");
    exit(1);
  }

  int close_result = close(fdr);
  if (close_result < 0) {
    printf("Failed to close the maps file\n");
    exit(1);
  }
}

void restore_memory() {
  unmap_old_stack();

  int fdr = open(checkpoint_path, O_RDONLY);
  if (fdr < 0) {
    printf("Failed to open restore file.\n");
    exit(1);
  }

  ucontext_t context;
  read_register_data(fdr, &context);

  // keep looping until we have reached eof
  while (true) {
    MemoryRegion mem_reg;
    bool read_success = read_memory_region_header(fdr, &mem_reg);

    // if we couldn't read another header, then we have reached eof
    if (!read_success) {
      break;
    }

    // this whole chunk of code puts our memory block from the checkpoint into
    // its spot
    char* mem = read_memory_region_memory(fdr, mem_reg.size);
    bool map_success = map_memory_region(&mem_reg);
    if (map_success) {
      memcpy(mem_reg.startAddress, mem, mem_reg.size);
    } else {
      printf("Error mapping new memory\n");
      exit(1);
    }
  }

  int close_fdr = close(fdr);
  if (close_fdr < 0) {
    printf("Failed to close the restore file.\n");
    exit(1);
  }

  printf("Restarting from checkpoint\n");
  setcontext(&context);
}

int main(int argc, char** argv) {
  // update our stack point to an unused part of memory
  void* stack_addr = mmap(new_stack_addr, STACK_LENGTH, STACK_PROT, STACK_FLAGS,
                          STACK_FD, STACK_OFFSET);
  if (stack_addr != new_stack_addr) {
    printf("Mapping new memory did not work at address %p", new_stack_addr);
    exit(1);
  }

  if (argc < 2) {
    printf("Please pass the checkpoint filename as an argument\n");
    exit(1);
  }

  // copy the path from the argument to our variable
  strcpy(checkpoint_path, argv[1]);

  asm volatile("mov %0, %%rsp"
               :
               : "g"(new_stack_addr + STACK_LENGTH)
               : "memory");
  restore_memory();

  // if we've reached this line of code, the setcontext failed
  printf("Call to setcontext() returned, failed to deviate as expected.\n");
  exit(1);
}
