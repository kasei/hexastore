#ifndef _MAP_H
#define _MAP_H

#include <stdlib.h>
#include "iterator.h"

typedef struct {
	void *key;
	void *value;
} map_entry_type;

typedef map_entry_type* map_entry_t;

typedef struct {
	void (*clear)(void*);
	int (*contains_key)(void*, void*);
	int (*contains_value)(void*, void*);
	iterator_t (*entry_iterator)(void*);
	void* (*get)(void*, void*);
	int (*is_empty)(void*);
	iterator_t (*key_iterator)(void*);
	void* (*put)(void*, void*, void*);
	void* (*remove)(void*, void*);
	size_t (*size)(void*);
	iterator_t (*value_iterator)(void*);
	void (*destroy)(void*);
	void *state;
} map_type;

typedef map_type* map_t;

map_t map_create(
	void (*clear)(void*),
	int (*contains_key)(void*, void*),
	int (*contains_value)(void*, void*),
	iterator_t (*entry_iterator)(void*),
	void* (*get)(void*, void*),
	int (*is_empty)(void*),
	iterator_t (*key_iterator)(void*),
	void* (*put)(void*, void*, void*),
	void* (*remove)(void*, void*),
	size_t (*size)(void*),
	iterator_t (*value_iterator)(void*),
	void (*destroy)(void*),
	void *state);

void map_destroy(map_t);

void map_clear(map_t);

int map_contains_key(map_t, void*);

int map_contains_value(map_t, void*);

iterator_t map_entry_iterator(map_t);

void* map_get(map_t, void*);

int map_is_empty(map_t);

iterator_t map_key_iterator(map_t);

void* map_put(map_t, void*, void*);

void* map_remove(map_t, void*);

size_t map_size(map_t);

iterator_t map_value_iterator(map_t);

#endif /* _MAP_H */
