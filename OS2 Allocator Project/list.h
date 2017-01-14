#pragma once
typedef struct list_head {
	struct list_head *next;
	struct list_head *prev;
	list_head() {
		next = prev = this;
	}
	void list_init() {
		next = prev = this;
	}

} list_head;