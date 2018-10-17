#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

int main() {	
	int *ptr0 = malloc(sizeof(int));
	*ptr0 = 1023;
	int *ptr1 = realloc(ptr0, sizeof(int) - 2);

	char buf[1024];
	snprintf(buf, 1024, "p0 at %p and p1 at %p with value of %d\n", ptr0, ptr1, *ptr1);
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
