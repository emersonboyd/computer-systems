#include <fcntl.h> // for open
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h> // for reading / writing registers
#include <unistd.h>   // for read, close

#include "helper.h"

static const int SIGCKPT = 12;

void handle_signal(int sig) {
  if (sig != SIGCKPT) {
    printf("Bad signal given: %d\n", sig);
    return;
  }

  int fdw = open(RESTORE_FILE_PATH, RESTORE_FLAGS, RESTORE_MODE);
  if (fdw < 0) {
    printf("Failed to open the checkpoint file.\n");
    exit(1);
  }

  int run_counter = 0;

  ucontext_t context;
  getcontext(&context);

  // check to see if we have already entered this file more than once
  // if so, we know we're restarting the program
  if (run_counter++ > 0) {
    return;
  }

  write_register_data(fdw, context);

  int fdr = open("/proc/self/maps", O_RDONLY);
  if (fdr < 0) {
    printf("Failed to open file maps.\n");
    exit(1);
  }

  char buffer[40];
  int read_result = 1;

  // keep looping until we have reached eof
  while (read_result) {
    read_result = read(fdr, buffer, 40);
    buffer[39] = '\0';

    // push our file descriptor to the end of the line and update the read
    // result in case we reach eof
    if (read_result) {
      read_result = read_to_end_of_line(fdr);
    }

    int dash_index = -1;
    int space_index = -1;

    // if we just got a good read result from the newline character, then we
    // should print the buffer
    if (read_result) {
      dash_index = get_dash_index(buffer);
      space_index = get_space_index(buffer);
    }

    if (dash_index >= 0 && space_index >= 0) {
      MemoryRegion* mem_reg =
          convert_string_to_memory_region(buffer, dash_index, space_index);

      // ignore memory segments that are shared or that we can't read
      if (!mem_reg->isPrivate || !mem_reg->isReadable) {
        continue;
      }

      // we try to write out memory block. If it's a success, we write the
      // header then the memory. If not, move back the pointer as necessary.
      bool success = write_memory_region_memory(fdw, mem_reg);

      // if we know we succeeded at writing the memory, then we should put our
      // header first, then re-write the memory block
      if (success) {
        off_t offset = lseek(fdw, -1 * mem_reg->size, SEEK_CUR);
        if (offset < 0) {
          printf("Error retracing write offset after writing memory block\n");
          exit(1);
        }

        write_memory_region_header(fdw, mem_reg);
        write_memory_region_memory(fdw, mem_reg);
      }
    }
  }

  int close_fdr = close(fdr);
  int close_fdw = close(fdw);
  if (close_fdr < 0 || close_fdw < 0) {
    printf("Failed to close either the maps file descriptor or the checkpoint "
           "file descriptor.\n");
    exit(1);
  }

  printf("Created checkpoint file\n");
}

static __attribute__((constructor)) void init_signal_handler(void) {
  signal(SIGCKPT, handle_signal);
}
