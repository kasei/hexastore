#include "map.h"

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
		void *state) {
	map_t map = malloc(sizeof(map_type));
	if(map != NULL) {
		map->clear = clear;
		map->contains_key = contains_key;
		map->contains_value = contains_value;
		map->entry_iterator = entry_iterator;
		map->get = get;
		map->is_empty = is_empty;
		map->key_iterator = key_iterator;
		map->put = put;
		map->remove = remove;
		map->size = size;
		map->value_iterator = value_iterator;
		map->destroy = destroy;
		map->state = state;
	}
	return map;
}

void map_destroy(map_t map) {
	(*(map->destroy))(map->state);
	free(map);
}

void map_clear(map_t map) {
	(*(map->clear))(map->state);
}

int map_contains_key(map_t map, void* key) {
	return (*(map->contains_key))(map->state, key);
}

int map_contains_value(map_t map, void* value) {
	return (*(map->contains_value))(map->state, value);
}

iterator_t map_entry_iterator(map_t map) {
	return (*(map->entry_iterator))(map->state);
}

void* map_get(map_t map, void* key) {
	return (*(map->get))(map->state, key);
}

int map_is_empty(map_t map) {
	return (*(map->is_empty))(map->state);
}

iterator_t map_key_iterator(map_t map) {
	return (*(map->key_iterator))(map->state);
}

void* map_put(map_t map, void* key, void* value) {
	return (*(map->put))(map->state, key, value);
}

void* map_remove(map_t map, void* key) {
	return (*(map->remove))(map->state, key);
}

size_t map_size(map_t map) {
	return (*(map->size))(map->state);
}

iterator_t map_value_iterator(map_t map) {
	return (*(map->value_iterator))(map->state);
}
