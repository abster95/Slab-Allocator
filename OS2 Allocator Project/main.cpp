#include "buddy.h"
#include "list.h"
#include "macros.h"
#include "page.h"
#include "slab.h"
#include <iostream>
#include <stdlib.h>
using namespace std;

int main() {
	void* space = malloc(BLOCK_NUMBER*BLOCK_SIZE);
	
	//buddy * b = new buddy(space, BLOCK_NUMBER);
	//void* adr = b->kmem_getpages(0);
	//b->kmem_freepages(adr, 0);
	//cout<<""
	kmem_init(space, BLOCK_NUMBER);
	kmem_cache_t* mycache = kmem_cache_create("moj kesa\0", 7, nullptr, nullptr);
	void* object = kmem_cache_alloc(mycache);
	cout << "alociran objekat velicine 7" << endl;
	kmem_cache_free(mycache, object);
	cout << "vracen objekat kesu" << endl;
	
	kmem_cache_destroy(mycache);
	cout << "kes destrojd" << endl;
	return 0;
}