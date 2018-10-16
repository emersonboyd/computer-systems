#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

int main() {	
	void *ptr0 = malloc(300);
	void *ptr1 = malloc(20);
	void *ptr2 = calloc(1, 200);
	void *ptr3 = calloc(1, 51234);

	free(ptr0);
	free(ptr1);
	free(ptr2);
	free(ptr3);

	// char buf[1024];
	// snprintf(buf, 1024, "%p %p %d\n", int1, int2, *int2);
	// write(STDOUT_FILENO, buf, strlen(buf) + 1);

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
