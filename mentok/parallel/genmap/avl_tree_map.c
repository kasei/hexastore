#include <stdio.h>
#include "avl_tree_map.h"
#include "mentok/misc/avl.h"

#define _AVL_TREE_MAP_ITERATOR_RETURN_KEYS 0
#define _AVL_TREE_MAP_ITERATOR_RETURN_VALUES 1
#define _AVL_TREE_MAP_ITERATOR_RETURN_ENTRIES 2

typedef struct {
	struct avl_traverser traverser;
	void* next_item;
	unsigned char return_what;
} _avl_tree_map_iterator_state_type;

typedef _avl_tree_map_iterator_state_type* _avl_tree_map_iterator_state_t;

typedef struct {
	avl_tree_map_comparator *key_compare;
	avl_tree_map_comparator *value_compare;
	avl_tree_map_entry_destroy *entry_destroy;
	avl_tree_map_params_destroy *params_destroy;
	void *user_params;
} _avl_tree_map_state_params_type;

typedef _avl_tree_map_state_params_type* _avl_tree_map_state_params_t;

typedef struct {
	struct avl_table *table;
	_avl_tree_map_state_params_t params;
} _avl_tree_map_state_type;

typedef _avl_tree_map_state_type* _avl_tree_map_state_t;

void _avl_tree_map_destroy(void*);
void _avl_tree_map_clear(void*);
int _avl_tree_map_contains_key(void*, void*);
int _avl_tree_map_contains_value(void*, void*);
iterator_t _avl_tree_map_entry_iterator(void*);
void* _avl_tree_map_get(void*, void*);
int _avl_tree_map_is_empty(void*);
iterator_t _avl_tree_map_key_iterator(void*);
void* _avl_tree_map_put(void*, void*, void*);
void* _avl_tree_map_remove(void*, void*);
size_t _avl_tree_map_size(void*);
iterator_t _avl_tree_map_value_iterator(void*);

int _avl_tree_map_iterator_has_next(void*);
void* _avl_tree_map_iterator_next(void*);
void _avl_tree_map_iterator_destroy(void*);

int _avl_tree_map_key_compare(const void*, const void*, void*);
void _avl_tree_map_entry_destroy(void*, void*);


map_t avl_tree_map_create(avl_tree_map_comparator *key_compare, avl_tree_map_comparator *value_compare, avl_tree_map_entry_destroy *entry_destroy, avl_tree_map_params_destroy *params_destroy, void* user_params) {
	map_t map = NULL;
	_avl_tree_map_state_t state = malloc(sizeof(_avl_tree_map_state_type));
	if(state == NULL) {
		return NULL;
	}
	_avl_tree_map_state_params_t params = malloc(sizeof(_avl_tree_map_state_params_type));
	if(params == NULL) {
		free(state);
		return NULL;
	}
	params->key_compare = key_compare;
	params->value_compare = value_compare;
	params->entry_destroy = entry_destroy;
	params->params_destroy = params_destroy;
	params->user_params = user_params;
	state->params = params;
	state->table = avl_create(&_avl_tree_map_key_compare, params, &avl_allocator_default);
	if(state->table == NULL) {
		free(params);
		free(state);
		return NULL;
	}
	map = map_create(
			&_avl_tree_map_clear,
			&_avl_tree_map_contains_key,
			&_avl_tree_map_contains_value,
			&_avl_tree_map_entry_iterator,
			&_avl_tree_map_get,
			&_avl_tree_map_is_empty,
			&_avl_tree_map_key_iterator,
			&_avl_tree_map_put,
			&_avl_tree_map_remove,
			&_avl_tree_map_size,
			&_avl_tree_map_value_iterator,
			&_avl_tree_map_destroy,
			state);
	if(map == NULL) {
		avl_destroy(state->table, &_avl_tree_map_entry_destroy);
		if(params->params_destroy != NULL) {
			(*(params->params_destroy))(params->user_params);
		}
		free(params);
		free(state);
		return NULL;
	}
	return map;
}

void _avl_tree_map_destroy(void* s) {
	_avl_tree_map_state_t state = (_avl_tree_map_state_t)s;
	avl_destroy(state->table, &_avl_tree_map_entry_destroy);
	if(state->params->params_destroy != NULL) {
		(*(state->params->params_destroy))(state->params->user_params);
	}
	free(state->params);
	free(state);
}

void _avl_tree_map_clear(void* s) {
	_avl_tree_map_state_t state = (_avl_tree_map_state_t)s;
	avl_destroy(state->table, &_avl_tree_map_entry_destroy);
	state->table = avl_create(&_avl_tree_map_key_compare, state->params, &avl_allocator_default);
	if(state->table == NULL) {
		fprintf(stderr, "%s:%u: Error in _avl_tree_map_clear; unable to allocate new avl_table.\n", __FILE__, __LINE__);
	}
}

int _avl_tree_map_contains_key(void* s, void* key) {
	_avl_tree_map_state_t state = (_avl_tree_map_state_t)s;
	map_entry_type entry;
	entry.key = key;
	entry.value = NULL;
	return avl_find(state->table, &entry) != NULL;
}

int _avl_tree_map_contains_value(void* s, void* value) {
	_avl_tree_map_state_t state = (_avl_tree_map_state_t)s;
	if(state->params->value_compare == NULL) {
		fprintf(stderr, "%s:%u: Error in _avl_tree_map_contains_value; NULL value comparator.\n", __FILE__, __LINE__);
	} else {
		struct avl_traverser traverser;
		map_entry_t entry = (map_entry_t)avl_t_first(&traverser, state->table);
		while(entry != NULL) {
			if((*(state->params->value_compare))(value, entry->value, state->params->user_params) == 0) {
				return 1;
			}
			entry = (map_entry_t)avl_t_next(&traverser);
		}
	}
	return 0;
}

void* _avl_tree_map_get(void* s, void* key) {
	_avl_tree_map_state_t state = (_avl_tree_map_state_t)s;
	void *value = NULL;
	map_entry_type key_entry;
	key_entry.key = key;
	key_entry.value = NULL;
	map_entry_t entry = (map_entry_t)avl_find(state->table, &key_entry);
	if(entry != NULL) {
		value = entry->value;
	}
	return value;
}

int _avl_tree_map_is_empty(void* s) {
	return _avl_tree_map_size(s) == 0;
}

void* _avl_tree_map_put(void* s, void* key, void* value) {
	_avl_tree_map_state_t state = (_avl_tree_map_state_t)s;
	void* old_value = NULL;
	map_entry_t new_entry = malloc(sizeof(map_entry_type));
	if(new_entry == NULL) {
		fprintf(stderr, "%s:%u: Error in _avl_tree_map_put; unable to allocate new map_entry_type.\n", __FILE__, __LINE__);
	} else {
		new_entry->key = key;
		new_entry->value = value;

		map_entry_t old_entry = (map_entry_t)avl_replace(state->table, new_entry);
		if(old_entry != NULL) {
			old_value = old_entry->value;
			if(key != old_entry->key) {
				(*(state->params->entry_destroy))(old_entry->key, NULL, state->params->user_params);
			}
			free(old_entry);
		}
	}
	return old_value;
}

void* _avl_tree_map_remove(void* s, void* key) {
	_avl_tree_map_state_t state = (_avl_tree_map_state_t)s;
	void *old_value = NULL;
	map_entry_type key_entry;
	key_entry.key = key;
	key_entry.value = NULL;
	map_entry_t entry = avl_delete(state->table, &key_entry);
	if(entry != NULL) {
		old_value = entry->value;
		if(key != entry->key) {
			(*(state->params->entry_destroy))(entry->key, NULL, state->params->user_params);
		}
		free(entry);
	}
	return old_value;
}

size_t _avl_tree_map_size(void* s) {
	_avl_tree_map_state_t state = (_avl_tree_map_state_t)s;
	return avl_count(state->table);
}

iterator_t _avl_tree_map_value_iterator(void* s) {
	iterator_t iter = _avl_tree_map_entry_iterator(s);
	((_avl_tree_map_iterator_state_t)iter->state)->return_what = _AVL_TREE_MAP_ITERATOR_RETURN_VALUES;
	return iter;
}

iterator_t _avl_tree_map_entry_iterator(void* s) {
	_avl_tree_map_state_t state = (_avl_tree_map_state_t)s;
	_avl_tree_map_iterator_state_t iter_state = malloc(sizeof(_avl_tree_map_iterator_state_type));
	if(iter_state == NULL) {
		return NULL;
	}
	iter_state->return_what = _AVL_TREE_MAP_ITERATOR_RETURN_ENTRIES;
	iter_state->next_item = avl_t_first(&(iter_state->traverser), state->table);
	iterator_t iter = iterator_create(&_avl_tree_map_iterator_has_next, &_avl_tree_map_iterator_next, &_avl_tree_map_iterator_destroy, iter_state);
	if(iter == NULL) {
		free(iter_state);
		return NULL;
	}
	return iter;
}

iterator_t _avl_tree_map_key_iterator(void* s) {
	iterator_t iter = _avl_tree_map_entry_iterator(s);
	((_avl_tree_map_iterator_state_t)iter->state)->return_what = _AVL_TREE_MAP_ITERATOR_RETURN_KEYS;
	return iter;
}

int _avl_tree_map_iterator_has_next(void* is) {
	_avl_tree_map_iterator_state_t iter_state = (_avl_tree_map_iterator_state_t)is;
	return iter_state->next_item != NULL;
}

void* _avl_tree_map_iterator_next(void* is) {
	_avl_tree_map_iterator_state_t iter_state = (_avl_tree_map_iterator_state_t)is;
	void *item = NULL;
	if(iter_state->next_item != NULL) {
		map_entry_t entry = (map_entry_t)(iter_state->next_item);
		switch(iter_state->return_what) {
			case _AVL_TREE_MAP_ITERATOR_RETURN_KEYS: {
				item = entry->key;
				break;
			}
			case _AVL_TREE_MAP_ITERATOR_RETURN_VALUES: {
				item = entry->value;
				break;
			}
			case _AVL_TREE_MAP_ITERATOR_RETURN_ENTRIES: {
				item = entry;
				break;
			}
			default: {
				fprintf(stderr, "%s:%u: Error in _avl_tree_map_iterator_next; unexpected return_what %i\n", __FILE__, __LINE__, (int)(iter_state->return_what));
			}
		}
		iter_state->next_item = avl_t_next(&(iter_state->traverser));
	}
	return item;
}

void _avl_tree_map_iterator_destroy(void* is) {
	_avl_tree_map_iterator_state_t iter_state = (_avl_tree_map_iterator_state_t)is;
	free(iter_state);
}

int _avl_tree_map_key_compare(const void* e1, const void* e2, void* p) {
	map_entry_t entry1 = (map_entry_t)e1;
	map_entry_t entry2 = (map_entry_t)e2;
	_avl_tree_map_state_params_t params = (_avl_tree_map_state_params_t)p;
	return (*(params->key_compare))(entry1->key, entry2->key, params->user_params);
}

void _avl_tree_map_entry_destroy(void* e, void* p) {
	map_entry_t entry = (map_entry_t)e;
	_avl_tree_map_state_params_t params = (_avl_tree_map_state_params_t)p;
	(*(params->entry_destroy))(entry->key, entry->value, params->user_params);
	free(entry);
}
