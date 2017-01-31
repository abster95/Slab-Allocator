#pragma once
#include "macros.h"
#include "list.h"
#include "slab.h"
#include "slabstruct.h"
#include <cmath>


extern void* memory;
extern struct page * pagesBase;


typedef struct page {
	list_head list;
	unsigned int order;
	void init_page();
	static void set_cache(page * pagep, kmem_cache_t *cachep) {
		pagep->list.next = &cachep->next;
	}

	static kmem_cache_t* get_cache(page* pagep) {
		return list_entry(pagep->list.next, kmem_cache_t, next);
	}

	static void set_slab(page * page, slab* slab) {
		page->list.prev = &slab->list;
	}

	static slab* get_slab(page* pagep) {
		return list_entry(pagep->list.prev, slab, list);
	}

	static page* virtual_to_page(void* vir) {
		unsigned long long index = ((unsigned long long) vir - (unsigned long long) memory) >> (unsigned long long) log2(BLOCK_SIZE);
		return &(pagesBase[index]);
	}
} page;

