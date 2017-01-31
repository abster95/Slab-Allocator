#include "slab.h"
#include "slabstruct.h"
#include "list.h"
#include "page.h"
#include "buddy.h"
#include "macros.h"
#include <iostream>
#include <cstring>
using namespace std;





static kmem_cache_t cache_cache;
static kmem_cache_t* buffers[13];
extern void* memory;
extern page* pagesBase;
buddy* b;

//in page struct next points to the cache and prev points to the slab that the page belongs


void kmem_init(void * space, int block_num) {
	b = new(space) buddy(space, block_num);
	cache_cache = {
		list_head(), //empty
		list_head(), //empty
		list_head(), //empty
		sizeof(kmem_cache_t), //how big is the object
		((BLOCK_SIZE - sizeof(slab)) / (sizeof(kmem_cache_t) + sizeof(unsigned int))), //calculate how much can fit on a block after reserving space for slab desc and an int for indexing
		0, // one block is enough
		((BLOCK_SIZE - sizeof(slab)) % (sizeof(kmem_cache_t) + sizeof(unsigned int))) / 64 + 1, //calculate how many offsets
		0,
		nullptr,
		nullptr,
		"cache_cache\0",
		false,
		list_head()
	};

	cache_cache.next.list_init();
	cache_cache.slabs_free.list_init();
	cache_cache.slabs_full.list_init();
	cache_cache.slabs_partial.list_init();
	
	for (int i = 0; i < 13; i++) {
		buffers[i] = kmem_cache_create("SizeN\0", 32 << i, nullptr, nullptr);
	}
}

kmem_cache_t * kmem_cache_create(const char * name, size_t size, void(*ctor)(void *), void(*dtor)(void *)) {
	kmem_cache_t* adr = (kmem_cache_t*)kmem_cache_alloc(&(cache_cache));
	adr->ctor = ctor;
	adr->dtor = dtor;
	adr->objsize = size;
	adr->colour_next = 0; //default
	adr->growing = false; //default
	adr->slabs_free.list_init();
	adr->slabs_full.list_init();
	adr->slabs_partial.list_init();
	if (strlen(name) < 63) {
		strcpy_s(adr->name, name);
	} else {
		cout << "ERROR: Name too long" << endl;
		exit(1);
	}
	int i = 0;
	while ((BLOCK_SIZE << i) < size) i++;
	adr->gfporder = i;
	if (size < (BLOCK_SIZE >> 3)) {
		adr->num = ((BLOCK_SIZE - sizeof(slab)) / (size + sizeof(unsigned int)));
		adr->colour = ((BLOCK_SIZE - sizeof(slab)) % (size + sizeof(unsigned int))) / 64 + 1;
	} else {
		adr->num = (BLOCK_SIZE << i) / size;
		adr->colour = ((BLOCK_SIZE << i) % size) / 64 + 1;
	}
	return adr;
}

int kmem_cache_shrink(kmem_cache_t * cachep) {
	if (cachep->growing) {
		cachep->growing = false;
		return 0;
	}
	unsigned long long cnt = 0;
	while (cachep->slabs_free.next != nullptr) { //while there are free slabs
		slab* tmp = list_entry(cachep->slabs_free.next, slab, list);
		tmp->list.prev->next = tmp->list.next;
		if (tmp->list.next != nullptr) {
			tmp->list.next->prev = tmp->list.prev;
		}
		void* adr;
		if (cachep->objsize < (BLOCK_SIZE >> 3)) {
			adr = tmp;
		} else {
			adr = (void*)((unsigned long long)(tmp->s_mem) - tmp->colouroff);
		}
		b->kmem_freepages(adr, cachep->gfporder);
		cnt += cachep->gfporder;
	}
	cachep->growing = false;
	return (int) cnt;
}

void * kmem_cache_alloc(kmem_cache_t * cachep) {
	if (cachep == nullptr) return nullptr; //ERROR
	slab* tmp;
	if (cachep->slabs_partial.next != nullptr) { //if there's a partial slab first fill it
		tmp = list_entry(cachep->slabs_partial.next, slab, list);
		cachep->slabs_partial.next = tmp->list.next;
		if (tmp->list.next != nullptr) {
			tmp->list.next->prev = tmp->list.prev;
		}
	} else if (cachep->slabs_free.next != nullptr) {//else, check if there are any free slabs and fill them
		tmp = list_entry(cachep->slabs_free.next, slab, list);
		cachep->slabs_free.next = tmp->list.next;
		if (tmp->list.next != nullptr) {
			tmp->list.next->prev = tmp->list.prev;
		}
	} else {//we need to alloc a new slab from buddy
		void* adr = b->kmem_getpages(cachep->gfporder);
		if (cachep->objsize < (BLOCK_SIZE >> 3)) { //if objsize is an eight of BLOCK_SIZE we store slab descriptor on the slab
			((slab*)adr)->init(cachep);
			tmp = (slab*)adr;
		} else {// else put it in a bufferr
			tmp = (slab*)kmalloc(sizeof(slab)+ (cachep->num * sizeof(unsigned int)));
			if (cachep->num <= 8) {
				tmp->initBig(cachep, adr);
			} else {
				cout << "ERROR!" << endl;
				exit(1);
			}
		}
		unsigned long long index = ((unsigned long long) adr - (unsigned long long) memory) >> (unsigned long long) log2(BLOCK_SIZE);
		page* pagep = &pagesBase[index];
		for (int i = 0; i < (1 << cachep->gfporder); i++) {
			page::set_cache(&pagep[i], cachep);
			page::set_slab(&pagep[i], tmp);
		}
	}
	
	void* adr = (void*)((unsigned long long) tmp->s_mem + tmp->free*cachep->objsize);
	tmp->free = slab_buffer(tmp)[tmp->free];
	tmp->inuse++;
	list_head* toPut;
	if ((tmp->inuse < cachep->num) && tmp->free != BUFCTL_END) { //if slab is partialy filled put it back in partial
		toPut = &cachep->slabs_partial;
	} else {//else put it back in full
		toPut = &cachep->slabs_full;
	}

	//update list
	tmp->list.prev = toPut;
	tmp->list.next = toPut->next;
	if (toPut->next != nullptr) {
		toPut->next->prev = &tmp->list;
	}
	toPut->next = &tmp->list;

	//set the flag for reaping
	cachep->growing = true;

	return adr;
}

void kmem_cache_free(kmem_cache_t * cachep, void * objp) {
	if (cachep == nullptr || objp == nullptr) return; //ERROR
	slab* slabp = page::get_slab(page::virtual_to_page(objp));
	slabp->list.prev->next = slabp->list.next;
	if (slabp->list.next != nullptr) {
		slabp->list.next->prev = slabp->list.prev;
	}
	unsigned long long objNo = ((unsigned long long) objp - (unsigned long long) slabp->s_mem) / cachep->objsize;
	slab_buffer(slabp)[objNo] = slabp->free;
	slabp->free = objNo;

	slabp->inuse--;
	list_head * toPut;
	if (slabp->inuse > 0) { //if there's still objects in slab return it to partial list
		toPut = &cachep->slabs_partial;
	} else { //else put it in free slabs
		toPut = &cachep->slabs_free;
	}

	slabp->list.next = toPut->next;
	slabp->list.prev = toPut;
	if (toPut->next) {
		toPut->next->prev = &slabp->list;
	}
	toPut->next = &slabp->list;
}

void * kmalloc(size_t size) {
	unsigned long long min = 32;
	for (int i = 0; i < 13; i++) {
		if (size <= min << i) return kmem_cache_alloc(buffers[i]);
	}
	cout << "Trazen prevelik buffer" << endl;
	return nullptr;
}

void kfree(const void * objp) {
	kmem_cache_t* cachep = page::get_cache(page::virtual_to_page((void*)objp));
	kmem_cache_free(cachep, (void*)objp);
}

void kmem_cache_destroy(kmem_cache_t * cachep) { 
	if (cachep == nullptr) return; //ERROR
	if (cachep->slabs_full.next == nullptr && cachep->slabs_partial.next == nullptr) { // can only destroy if all the slabs are empty
		cachep->growing = false;
		kmem_cache_shrink(cachep); //return all the slabs to buddy
		
		//cachep->next.next->prev = cachep->next.prev;
		//cachep->next.prev->next = cachep->next.next;

		kmem_cache_free(&cache_cache, (void*)cachep);
	}
}

void kmem_cache_info(kmem_cache_t * cachep) {
}

int kmem_cache_error(kmem_cache_t * cachep) {
	return 0;
}
