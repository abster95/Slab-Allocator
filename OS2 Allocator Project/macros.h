#pragma once
/*
	Returns the pointer to the begining of "type". 
	Takes a pointer to member of the type
	Calculates offset of the member from the begining of type
	Substracts offset from the given pointer
	Works with linux, should work here too
*/
#define list_entry(ptr, type, member) ((type *)((char *)(ptr)-(unsigned long long)(&((type *)0)->member)))

/*
	Word size used for alignment
	Should be 4 on x86 and 8 on x64
	Just in case
*/
#define WORD_SIZE (sizeof(void*))
#define BUFCTL_END (~0)

#define slab_buffer(slabp) ((unsigned int *)(((slab*)slabp)+1))

#define BLOCK_NUMBER (1024)
#define BLOCK_SIZE (4096)
#define CACHE_L1_LINE_SIZE (64)

