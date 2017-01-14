#pragma once
#include "macros.h"
#include "list.h"

typedef struct page {
	list_head list;
	unsigned int order;
	void init_page();
} page;