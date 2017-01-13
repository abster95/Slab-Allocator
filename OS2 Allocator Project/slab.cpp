#include "slab.h"

struct list_head {
	struct list_head *next;
	struct list_head *prev;
};

struct kmem_cache_s {
	struct list_head *slabs_full; //list of full slabs
	struct list_head *slabs_partial; //list of partially filled slabs
	struct list_head *slabs_free; //list of free slabs
	unsigned int objsize; //size of object in slab
	unsigned int num; //number of objects in each slab
	unsigned int  gfporder; //size of the slab in blocks (each slab takes 2^gfporder blocks because of the buddy alloc)
	size_t colour; //number of different offsets that can be used
	void(*ctor)(void *); //constructor for the object
	void(*dtor)(void *); //destructor for the object
	char *name; //name of the cache
	struct list_head *next; //next cache
};

void kmem_init(void * space, int block_num) {
}

kmem_cache_t * kmem_cache_create(const char * name, size_t size, void(*ctor)(void *), void(*dtor)(void *)) {
	return nullptr;
}

int kmem_cache_shrink(kmem_cache_t * cachep) {
	return 0;
}

void * kmem_cache_alloc(kmem_cache_t * cachep) {
	return nullptr;
}

void kmem_cache_free(kmem_cache_t * cachep, void * objp) {
}

void * kmalloc(size_t size) {
	return nullptr;
}

void kfree(const void * objp) {
}

void kmem_cache_destroy(kmem_cache_t * cachep) {
}

void kmem_cache_info(kmem_cache_t * cachep) {
}

int kmem_cache_error(kmem_cache_t * cachep) {
	return 0;
}
