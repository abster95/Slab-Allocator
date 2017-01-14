#pragma once
#include "slab.h"
#include "list.h"
#include "macros.h"
typedef struct slab {
	list_head list;
	size_t inuse;//how many objects are currently in this slab
	size_t free; //next free slot in slab
	size_t colouroff; //colour offset for this slab
	void * s_mem; //where the slots start

	void init(kmem_cache_t* cachep);
	void initBig(kmem_cache_t* cachep, void* buf);

}slab;