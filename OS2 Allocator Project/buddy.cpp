#include "buddy.h"
#include "slab.h"
#include "page.h"
#include "macros.h"
#include <cmath>
#include <iostream>
using namespace std;

void* memory;
page* pagesBase;

buddy::buddy(void * space, unsigned long long size) {
	//this->maxBlock = (int)log2(size-1) + 1;
	//unsigned long long neededSpace = (sizeof(buddy) + sizeof(page) * (size - 1) + sizeof(list_head) * maxBlock) / BLOCK_SIZE + 1;
	
	// //take up 2 blocks just in case
	//size -= 2;
	int neededSpace = 1;
	while ((sizeof(buddy) + sizeof(page) * (size - neededSpace) + sizeof(list_head) * ((int)log2(size - neededSpace)+1)) > BLOCK_SIZE*neededSpace) {
		neededSpace++;
	}
	size -= neededSpace;
	this->usable = (int)size;
	memory = (void*)((unsigned long long)space + neededSpace * BLOCK_SIZE);
	pagesBase = new((void*)((unsigned long long)space + sizeof(buddy))) page[size];
	for (int i = 0; i < size; i++) {
		pagesBase[i].init_page();
	}
	this->space = memory;
	this->maxBlock = (int)log2(size) + 1;
	void * tmp = memory;
	
	this->avail = new((void*)((unsigned long long)space + sizeof(buddy) + sizeof(page) * (size))) list_head[maxBlock];
	for (int i = maxBlock - 1; i >= 0; i--) {
		avail[i].list_init();
		if ((size >> i ) &1) {
			avail[i].next = avail[i].prev = (list_head*)tmp;
			unsigned long long index = ((unsigned long long) tmp - (unsigned long long) memory) >> (unsigned long long)log2(BLOCK_SIZE);
			pagesBase[index].order = i;
			((list_head*)tmp)->next = nullptr;
			((list_head*)tmp)->prev = &(avail[i]);
			cout << exp2(i) << " blok te velicine je stavljlen na adresu: " << hex << (unsigned long long) tmp <<endl;
			tmp = (void*)((unsigned long long) tmp + (BLOCK_SIZE << i));
		}
	}
	for (int i = 0; i < maxBlock; i++) {
		cout << "Nivo: " << i;
		if (avail[i].next != nullptr) {
			cout << " Krece na adresi: " << hex << (unsigned long long) avail[i].next << endl;
		} else {
			cout << endl;
		}
	}
}

void * buddy::kmem_getpages(unsigned long long order) {
	if (order < 0 || order > maxBlock) return 0; //ERROR
	int bestAvail = order;
	while ((bestAvail < maxBlock) && ((avail[bestAvail].next) == nullptr)) {
		bestAvail++; //try to find best fit
	}
	if (bestAvail > maxBlock) return nullptr; //can't allocate this at the moment
	
	//remove the page from the level
	list_head* ret = avail[bestAvail].next;
	list_head* tmp = ret->next;
	avail[bestAvail].next = tmp;
	if (tmp != nullptr) {
		tmp->prev = &avail[bestAvail];
	}
	unsigned long long index = ((unsigned long long) ret - (unsigned long long) memory) >> (unsigned long long) log2(BLOCK_SIZE);
	pagesBase[index].order = ~0; //so we know that this is now taken

	while (bestAvail > order) { //if we split higher order blocks to get the one we need
		bestAvail--;
		tmp = (list_head*)((unsigned long long) ret + (BLOCK_SIZE << bestAvail));
		tmp->next = nullptr;
		tmp->prev = &avail[bestAvail]; //avail[bestAvail] is surely empty, so we can put tmp at the begining
		avail[bestAvail].next = avail[bestAvail].prev = tmp;
		index = ((unsigned long long) tmp - (unsigned long long) memory) >> (unsigned long long) log2(BLOCK_SIZE);
		pagesBase[index].order = bestAvail;
	}
	cout << "Novo stanje nakon alokacije bloka: " << endl;
	for (int i = 0; i < maxBlock; i++) {
		cout << "Nivo: " << i;
		if (avail[i].next != nullptr) {
			cout << " Krece na adresi: " << hex << (unsigned long long) avail[i].next << endl;
		} else {
			cout << endl;
		}
	}
	return (void*)ret;
}

int buddy::kmem_freepages(void * space, unsigned long long order) {
	if( order < 0 || order > maxBlock || space == nullptr ) return 0; //ERROR
	list_head* tmp;
	while (true) { //while there's buddys to join
		unsigned long long mask = BLOCK_SIZE << order; // mask to figure out the buddy
		tmp = (list_head*)((unsigned long long) memory + (((unsigned long long) space - (unsigned long long) memory) ^ mask)); //find the adress of space's buddy
		unsigned long long index = ((unsigned long long)tmp - (unsigned long long) memory) >> (unsigned long long) log2(BLOCK_SIZE); //page with this index will tell us if the buddy is free or taken (sort of like a map)
		if (index >= BLOCK_NUMBER) {
			cout << "Taj buddy uopste ne postoji u memoriji(nije ni alociran ni free)" << endl;
			break;
		}
		if (tmp != nullptr && pagesBase[index].order == order) { //if the buddy's free
			tmp->prev->next = tmp->next;//take it out of the current buddy level
			if (tmp->next != nullptr) {
				tmp->next->prev = tmp->prev;
			}
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
	if (tmp != nullptr) {
		tmp->prev = (list_head*)space;
	}
	((list_head*)space)->prev = &avail[order];
	avail[order].next = (list_head*)space;
	unsigned long long index = ((unsigned long long)space - (unsigned long long)memory) >> (unsigned long long) log2(BLOCK_SIZE);
	pagesBase[index].order = order; //from this page on it's free for 2^order

	for (int i = 0; i < maxBlock; i++) {
		cout << "Nivo: " << i;
		if (avail[i].next != nullptr) {
			cout << " Krece na adresi: " << hex << (unsigned long long) avail[i].next << endl;
		} else {
			cout << endl;
		}
	}

	return 1;
	
}
