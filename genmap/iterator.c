#include <stdlib.h>
#include "iterator.h"

iterator_t iterator_create(
		int (*has_next)(void*),
		void* (*next)(void*),
		void (*destroy)(void*),
		void* state) {
	iterator_t iter = malloc(sizeof(iterator_type));
	if(iter != NULL) {
		iter->has_next = has_next;
		iter->next = next;
		iter->destroy = destroy;
		iter->state = state;
	}
	return iter;
}

void iterator_destroy(iterator_t iter) {
	(*(iter->destroy))(iter->state);
	free(iter);
}

int iterator_has_next(iterator_t iter) {
	return (*(iter->has_next))(iter->state);
}

void* iterator_next(iterator_t iter) {
	return (*(iter->next))(iter->state);
}
