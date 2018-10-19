A very basic main file can be run by calling "make" to demonstrate the malloc hooks, while "make test" can be called to run the t-test1 program provided by the Professor Arya.

There are two structs, one that defines the used memory (MallocHeader) and one that defines the free memory (node_t).

MallocHeader always lies directly before the data section of the memory region. It has an attribute size that represents the size of memory from the MallocHeader to the end of the memory region. It also has an attribute offset that represents the size between the beginning of the memory region and the MallocHeader (this is almost always zero unless we are allocating aligned memory with memalign).

node_t represents a free node in the node list. It has one attribute to describe the size of the memory region it can manifest, and another attribute to describe how many free multiples of the size follow after the node in memory. So, for example, if the user requests 512 bytes from sbrk, and malloc internally requests a page size from sbrk of 4096, there will be 512 bytes returned from malloc, and a node will be created at the memory region after the 512-byte memory region returned from malloc. This node will then also have size of 512, and will have a num_free of 6, as there are 6 more regions of 512 bytes after the node_t due to the sbrk request of 4096.

Using three different TAILQ lists from sys/queue.h, one for each bin size, each node_t is appended to the appropriate bin list head as necessary to keep track of the free list. When pulling a block from the free list, the list_remove function will look at the num_free attribute on the first available node, and determine the farthest memory block to pull after the node, and then decrement num_free by one. Once num_free is zero, the next call to list_remove will then remove the node from the TAILQ and return the node's memory address to malloc.

There are a lot of assert statements placed around the code to ensure proper code functionality and to detect the source of any bugs as immediately as possible. If this were production code, I would remove the assert statements.

Currently, I am using only one arena of memory because I do not yet know how to properly assign arenas as the bins grow. I will use more than one arena in the next assignment to optimize the use of different cores.

There are no known bugs/errors within the code.
