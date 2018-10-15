#include "helper.h"
#include <unistd.h>
#include <stdio.h>

int main() {
	init_bins();
	
	void *ptr1 = my_malloc(23432);
	my_free(ptr1);
	void *ptr4 = my_malloc(55555);
	my_free(ptr4);
	list_print(513);
	void *ptr3 = my_malloc(32);
	my_free(ptr3);
	void *ptr2 = my_malloc(15000);
	void *ptr5 = my_malloc(400);
	void *ptr6 = my_malloc(500);
	my_free(ptr5);
	my_free(ptr6);
	void *ptr7 = my_malloc(300);

	void *ptr8 = my_calloc(4, 32);
	// my_free(ptr8);

	list_print(32);
	list_print(128);
	list_print(512);
	list_print(513);

	int *int1 = (int *) my_malloc(sizeof(int));
	*int1 = 4;
	my_free(int1);
	int *int2 = (int *) my_malloc(sizeof(int));
	int *int3 = (int *) my_calloc(1, sizeof(int));
	int *int4 = (int *) my_realloc(int2, sizeof(int));
	printf("%p %p %p %p\n", int1, int2, int3, int4);

	return 0;
}