#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "helper.h"

int main() {
	// int i;
	// for (i = 0; i < 20000; i++) {
	// 	size_t size = 234235;
	// 	void *ptr1 = malloc(size);
	// 	void *ptr2 = realloc(ptr1, size);
	// 	free(ptr2);
	// }
	// for (i = 0; i < 20000; i++) {
	// 	void *ptr1 = malloc(234);
	// 	void *ptr2 = calloc(2, 234);
	// }

	void *ptr = calloc(1, 300);
	ptr = malloc(234234);
	// ptr = malloc(3400);
	// free(ptr);

	char buf[1024];
	snprintf(buf, 1024, "%p\n", ptr);
	write(STDOUT_FILENO, buf, strlen(buf) + 1);

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
