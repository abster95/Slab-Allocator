#include "buddy.h"
#include "slab.h"
#include "page.h"
#include "macros.h"
#include <cmath>
#include <iostream>
using namespace std;

void* memory;
page pagesBase[BLOCK_NUMBER];

buddy::buddy(void * space, size_t size) {
	memory = space;
	for (int i = 0; i < BLOCK_NUMBER; i++) {
		pagesBase[i].init_page();
	}
	this->space = space;
	void * tmp = space;
	this->maxBlock = (int)log2(size) + 1;
	avail = new list_head[maxBlock];
	for (int i = maxBlock - 1; i >= 0; i--) {
		if ((size >> i ) &1) {
			avail[i].next = avail[i].prev = (list_head*)tmp;
			pagesBase[((unsigned long long) tmp - (unsigned long long) memory) >> 12].order = i;
			((list_head*)tmp)->prev = ((list_head*)tmp)->next = &(avail[i]);
			cout << exp2(i) << " blok te velicine je stavljlen na adresu: " << hex << (unsigned long long) tmp;
			tmp = (void*)((unsigned long long)tmp + (BLOCK_SIZE << i));
		}
	}
	for (int i = 0; i < maxBlock; i++) {
		cout << "Nivo: " << i;
		if (avail[i].next != &avail[i]) {
			cout << " Krece na adresi: " << hex << (unsigned long long) avail[i].next << endl;
		} else {
			cout << endl;
		}
	}
}

void * buddy::kmem_getpages(size_t order) {
	if (order < 0) return 0; //ERROR
	int bestAvail = order;
	while ((bestAvail < maxBlock) && ((avail[bestAvail].next) == &(avail[bestAvail]))) {
		bestAvail++; //try to find best fit
	}
	if (bestAvail > maxBlock) return 0; //can't allocate this at the moment
	
	list_head* ret = avail[bestAvail].next;
	list_head* tmp = ret->next;
	avail[bestAvail].next = tmp;
	tmp->prev = &avail[bestAvail];
	pagesBase[((unsigned long long) ret - (unsigned long long) memory) >> 12].order = ~0;

	while (bestAvail > order) {
		bestAvail--;
		tmp = (list_head*)((unsigned long long) ret + (BLOCK_SIZE << bestAvail));
		tmp->next = tmp->prev = &avail[bestAvail];
		avail[bestAvail].next = avail[bestAvail].prev = tmp;
		pagesBase[((unsigned long long) tmp - (unsigned long long) memory) >> 12].order = bestAvail;
	}
	cout << "Novo stanje nakon alokacije bloka: " << endl;
	for (int i = 0; i < maxBlock; i++) {
		cout << "Nivo: " << i;
		if (avail[i].next != &avail[i]) {
			cout << " Krece na adresi: " << hex << (unsigned long long) avail[i].next << endl;
		} else {
			cout << endl;
		}
	}
	return (void*)ret;
}

int buddy::kmem_freepages(void * space, size_t order) {
	if( order < 0 || order > maxBlock || space == nullptr ) return 0; //ERROR
	list_head* tmp;
	while (true) { //while there's buddys to join
		unsigned long long mask = BLOCK_SIZE << order; // mask to figure out the buddy
		tmp = (list_head*)((unsigned long long) memory + (((unsigned long long) space - (unsigned long long) memory) ^ mask)); //find the adress of space's buddy
		unsigned long long index = ((unsigned long long)tmp - (unsigned long long) memory) >> 12; //page with this index will tell us if the buddy is free or taken (sort of like a map)
		if (index >= BLOCK_NUMBER) {
			cout << "Taj buddy uopste ne postoji u memoriji(nije ni alociran ni free)" << endl;
			break;
		}
		if (pagesBase[index].order == order) { //if the buddy's free
			tmp->prev->next = tmp->next;//take it out of the current buddy level
			tmp->next->prev = tmp->prev;
			pagesBase[index].order = ~0; //the page isn't currently in the buddy free list
			order++; // to check for higher orders
			if ((void*)tmp < space) { //if the found buddy comes before  given buddy
				space = (void*)tmp;//adjust
			}
		} else {
			break;
		}
	}
	//put in available list
	tmp = avail[order].next;
	((list_head*)space)->next = tmp;
	tmp->prev = (list_head*)space;
	((list_head*)space)->prev = &avail[order];
	avail[order].next = (list_head*)space;
	pagesBase[((unsigned long long) space - (unsigned long long) memory) >> 12].order = order; //from this page on it's free for 2^order

	for (int i = 0; i < maxBlock; i++) {
		cout << "Nivo: " << i;
		if (avail[i].next != &avail[i]) {
			cout << " Krece na adresi: " << hex << (unsigned long long) avail[i].next << endl;
		} else {
			cout << endl;
		}
	}

	return 1;
	
}
