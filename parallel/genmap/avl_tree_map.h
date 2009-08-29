#ifndef _AVL_TREE_MAP_H
#define _AVL_TREE_MAP_H

#include "map.h"

typedef int avl_tree_map_comparator(const void*, const void*, void*);

typedef void avl_tree_map_entry_destroy(void*, void*, void*);

typedef void avl_tree_map_params_destroy(void*);

map_t avl_tree_map_create(avl_tree_map_comparator*, avl_tree_map_comparator*, avl_tree_map_entry_destroy*, avl_tree_map_params_destroy*, void*);

#endif /* _AVL_TREE_MAP_H */
