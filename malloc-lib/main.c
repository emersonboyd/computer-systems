#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "helper.h"

int main() {
	// void *ptr;
	// size_t alignment, size;
	// for (alignment = 1; alignment < 8192; alignment *= 2) {
	// 	for (size = 0; size < 10000; size += 4) {
	// 		ptr = malloc(size);
	// 	}
	// }

	malloc_stats();

	void *p0 = malloc(256);
	free(p0);
	void *p1 = malloc(2342);
	p1 = realloc(p1, 2990);

	char buf[1024];
	snprintf(buf, 1024, "\n\n%p\n\n", p1);
	write(STDOUT_FILENO, buf, strlen(buf) + 1);

	malloc_stats();

	return 0;
}

// TODO READ OVER THE ASSIGNMENT TO MAKE SURE I'M FOLLOWING ALL OF THE RULES SPECIFIED
// TODO remove all of the write() calls
// TODO hide all TAILQ functions within the helper.c
// TODO initialize the bins within malloc.c rather than the main file
// TODO check manpages for all functions and ensure we fulfill the return value specified by each
// TODO add locks for malloc and free
// TODO what happens if they request to malloc size 0?
// TODO make sure the ret* is a multiple of 8 bytes
// TODO get ride of assertions lying around
// TODO delete the stack values around line 37 in free.c
