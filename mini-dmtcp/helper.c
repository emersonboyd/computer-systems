#include <ucontext.h> // for reading / writing registers
#include <unistd.h>   // for read, close

#include "helper.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int read_to_end_of_line(int fd) {
  char c[1];
  int read_result = 1;

  // keep reading until we read the end of the line
  while (read_result) {
    read_result = read(fd, c, 1);
    if (c[0] == '\n') {
      break;
    }
  }

  return read_result;
}

int get_character_index(char str[], char c) {
  int i;

  for (i = 0; str[i] != '\0'; i++) {
    if (str[i] == c) {
      return i;
    }
  }

  return -1;
}

int get_dash_index(char str[]) { return get_character_index(str, '-'); }

int get_space_index(char str[]) { return get_character_index(str, ' '); }

MemoryRegion* convert_string_to_memory_region(char buffer[], int dash_index,
                                              int space_index) {
  int start_mem_length = dash_index + 1;         // includes null terminator
  int end_mem_length = space_index - dash_index; // includes null terminator
  char* start_mem_str = malloc(start_mem_length);
  char* end_mem_str = malloc(end_mem_length);
  memcpy(start_mem_str, &buffer[0], dash_index);
  memcpy(end_mem_str, &buffer[dash_index + 1], dash_index);
  start_mem_str[start_mem_length - 1] = '\0';
  end_mem_str[end_mem_length - 1] = '\0';

  unsigned long start_mem = strtoul(start_mem_str, NULL, 16);
  unsigned long end_mem = strtoul(end_mem_str, NULL, 16);
  unsigned long total_mem = end_mem - start_mem;

  char read_char = buffer[space_index + 1];
  char write_char = buffer[space_index + 2];
  char exec_char = buffer[space_index + 3];
  char priv_char = buffer[space_index + 4];
  bool readable = read_char == 'r';
  bool writeable = write_char == 'w';
  bool executable = exec_char == 'x';
  bool private = priv_char == 'p';

  MemoryRegion* mem_reg;
  mem_reg = (MemoryRegion*)malloc(sizeof(MemoryRegion));
  mem_reg->startAddress = (void*)start_mem;
  mem_reg->size = total_mem;
  mem_reg->isReadable = readable;
  mem_reg->isWriteable = writeable;
  mem_reg->isExecutable = executable;
  mem_reg->isPrivate = private;

  return mem_reg;
}

void write_memory_region_header(int writer, const MemoryRegion* mem_reg) {
  unsigned long memory_region_size = sizeof(MemoryRegion);
  int write_result = write(writer, mem_reg, memory_region_size);
  if (write_result < memory_region_size) {
    printf("Only %d memory header bytes were written instead of %ld\n",
           write_result, memory_region_size);
    exit(1);
  }
}

bool write_memory_region_memory(int writer, const MemoryRegion* mem_reg) {
  unsigned long char_length = mem_reg->size;
  char* buffer = (char*)mem_reg->startAddress;
  int write_result = write(writer, buffer, char_length);
  if (write_result < 0) {
    // here we know this memory region isn't writeable
    return false;
  } else if (write_result < char_length) {
    printf("Only %d memory block bytes were written instead of %ld\n",
           write_result, char_length);
    exit(1);
  }

  return true;
}

void write_register_data(int writer, ucontext_t context) {
  unsigned long context_size = sizeof(ucontext_t);
  int write_result = write(writer, &context, context_size);
  if (write_result < context_size) {
    printf("Only %d register data bytes were written instead of %ld\n",
           write_result, context_size);
    exit(1);
  }
}

bool read_memory_region_header(int reader, MemoryRegion* mem_reg) {
  unsigned long memory_region_size = sizeof(MemoryRegion);

  int read_result = read(reader, mem_reg, memory_region_size);

  if (read_result < 0) {
    printf("Unexpected error reading from file descriptor %d", reader);
    exit(1);
  } else if (read_result == 0) {
    // this means we have reached EOF
    return false;
  }

  return true;
}

char* read_memory_region_memory(int reader, size_t length) {
  char* buffer = (char*)malloc(sizeof(char) * length);
  int read_result = read(reader, buffer, length);

  if (read_result < 0) {
    printf("Unexpected error reading from file descriptor %d", reader);
    exit(1);
  }

  return buffer;
}

void read_register_data(int reader, ucontext_t* context) {
  unsigned long context_size = sizeof(ucontext_t);
  int read_result = read(reader, context, context_size);

  if (read_result < 0) {
    printf("Unexpected error reading from file descriptor %d", reader);
    exit(1);
  }
}

bool read_line_and_check_if_stack(int reader) {
  char curr_char[1];
  char prev_char[1];

  int read_result = 1;

  // keep reading until we read a "[s" block cause that refers to stack
  while (read_result) {
    read_result = read(reader, curr_char, 1);
    if (curr_char[0] == '\n') {
      return false;
    }

    if (prev_char[0] == '[' && curr_char[0] == 's') {
      return true;
    }

    prev_char[0] = curr_char[0];
  }

  return false;
}

MemoryRegion* get_stack_region(int reader) {
  char buffer[40];

  int read_result = 1;

  // this chunk keeps reading the file until the line wth "[stack]" is found
  while (read_result) {
    read_result = read(reader, buffer, 40);
    buffer[39] = '\0';

    if (read_result) {
      bool is_stack = read_line_and_check_if_stack(reader);

      if (is_stack) {
        int dash_index = get_dash_index(buffer);
        int space_index = get_space_index(buffer);
        return convert_string_to_memory_region(buffer, dash_index, space_index);
      }
    }
  }

  return NULL;
}

bool map_memory_region(const MemoryRegion* mem_reg) {
  int prot = 0;
  if (mem_reg->isReadable) {
    prot |= PROT_READ;
  }
  if (mem_reg->isWriteable) {
    prot |= PROT_WRITE;
  }
  if (mem_reg->isExecutable) {
    prot |= PROT_EXEC;
  }
  prot |= PROT_WRITE;

  int flags = 0;
  if (mem_reg->isPrivate) {
    flags = MAP_PRIVATE;
  } else {
    flags = MAP_SHARED;
  }
  flags |= MAP_ANONYMOUS;

  void* mem_address =
      mmap(mem_reg->startAddress, mem_reg->size, prot, flags, -1, 0);
  if (mem_address < 0) {
    printf("Error mapping memory of checkpoint file\n");
    exit(1);
  }
  if (mem_address != mem_reg->startAddress) {
    printf("Mismatched addresses for mmap region: %p and %p\n", mem_address,
           mem_reg->startAddress);
    return false;
  }

  return true;
}
