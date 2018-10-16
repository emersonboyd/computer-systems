#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

int main() {	
	void *ptr0 = malloc(64);
	void *ptr1 = malloc(23432);
	free(ptr1);
	void *ptr2 = malloc(20);
	free(ptr0);
	free(ptr2);

	int *int1 = (int *) malloc(sizeof(int));
	*int1 = 4;
	free(int1);
	int *int2 = (int *) calloc(1, sizeof(int));

	char buf[1024];
	snprintf(buf, 1024, "%p %p %d\n", int1, int2, *int2);
	write(STDOUT_FILENO, buf, strlen(buf) + 1);

	malloc_stats();

	return 0;
}

// TODO READ OVER THE ASSIGNMENT TO MAKE SURE I'M FOLLOWING ALL OF THE RULES SPECIFIED
// TODO remove all of the write() calls
// TODO hide all TAILQ functions within the helper.c
// TODO initialize the bins within malloc.c rather than the main file
// TODO check manpages for all functions and ensure we fulfill the return value specified by each
