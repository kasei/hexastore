#ifndef _ITERATOR_H
#define _ITERATOR_H

typedef struct {
	int (*has_next)(void*);
	void* (*next)(void*);
	void (*destroy)(void*);
	void* state;
} iterator_type;

typedef iterator_type* iterator_t;

iterator_t iterator_create(
	int (*has_next)(void*),
	void* (*next)(void*),
	void (*destroy)(void*),
	void* state);

void iterator_destroy(iterator_t);

int iterator_has_next(iterator_t);

void* iterator_next(iterator_t);

#endif /* _ITERATOR_H */
