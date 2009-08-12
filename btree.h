#ifndef _BTREE_H
#define _BTREE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include "hexastore_types.h"

typedef struct {
	hx_node_id key;
	uintptr_t child;
} hx_btree_child;

typedef struct {
	uint32_t type;
	uint32_t flags;
	uintptr_t parent;
	uintptr_t prev;
	uintptr_t next;
	uint32_t used;
	hx_btree_child ptr[];
} hx_btree_node;

typedef struct {
	uint32_t branching_size;
	uintptr_t root;
} hx_btree;

typedef struct {
	int started;
	int finished;
	hx_btree* tree;
	hx_btree_node* page;
	uint32_t index;
} hx_btree_iter;

typedef void hx_btree_node_visitor ( hx_btree_node* node, int level, uint32_t branching_size, void* param );

hx_btree* hx_new_btree ( void* world, uint32_t branching_size );
int hx_free_btree ( hx_btree* tree );

uintptr_t hx_btree_search ( hx_btree* tree, hx_node_id key );
int hx_btree_insert ( hx_btree* tree, hx_node_id key, uintptr_t value );
int hx_btree_remove ( hx_btree* tree, hx_node_id key );
void hx_btree_traverse ( hx_btree* tree, hx_btree_node_visitor* before, hx_btree_node_visitor* after, int level, void* param );

hx_btree_iter* hx_btree_new_iter ( hx_btree* tree );
int hx_free_btree_iter ( hx_btree_iter* iter );
int hx_btree_iter_finished ( hx_btree_iter* iter );
int hx_btree_iter_current ( hx_btree_iter* iter, hx_node_id* n, uintptr_t* v );
int hx_btree_iter_next ( hx_btree_iter* iter );
int hx_btree_iter_seek( hx_btree_iter* iter, hx_node_id n );

#include "btree_internal.h"

#ifdef __cplusplus
}
#endif

#endif
