#ifndef EBOYD_HELPER_H
#define EBOYD_HELPER_H

#include <fcntl.h>
#include <stdbool.h>
#include <sys/mman.h> // for mmap
#include <ucontext.h> // for reading / writing registers

static const int RESTORE_FLAGS = O_WRONLY | O_CREAT;
static const mode_t RESTORE_MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
static const char RESTORE_FILE_PATH[] = "./myckpt";
static const size_t STACK_LENGTH = 0x10000;
static const int STACK_PROT = PROT_READ | PROT_WRITE;
static const int STACK_FLAGS = MAP_PRIVATE | MAP_ANONYMOUS;
static const int STACK_FD = -1;
static const int STACK_OFFSET = 0;

typedef struct {
  void* startAddress;
  size_t size;
  int isReadable;
  int isWriteable;
  int isExecutable;
  int isPrivate;
} MemoryRegion;

int read_to_end_of_line(int fd);

int get_dash_index(char str[]);
int get_space_index(char str[]);

MemoryRegion* convert_string_to_memory_region(char line[], int dash_index,
                                              int space_index);

void write_memory_region_header(int writer, const MemoryRegion* mem_reg);
bool write_memory_region_memory(int writer, const MemoryRegion* mem_reg);
void write_register_data(int writer, const ucontext_t context);

bool read_memory_region_header(int reader, MemoryRegion* mem_reg);
char* read_memory_region_memory(int reader, size_t length);
void read_register_data(int reader, ucontext_t* context);

MemoryRegion* get_stack_region(int reader);

bool map_memory_region(const MemoryRegion* mem_reg);

#endif
