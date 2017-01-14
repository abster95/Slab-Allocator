#pragma once
#include "list.h"

typedef struct buddy {
	void * space; //memory on witch buddy operates
	int maxBlock; //2^maxBlock pages are in the biggest block
	list_head* avail; //array of available blocks
	buddy(void* space, size_t size); //buddy constructor
	void* kmem_getpages(size_t order); // order is how many 2^order pages we need
	int kmem_freepages(void* space, size_t order); //space is from where we're freeing and order is how much

} buddy;