#ifndef _STORE_INDEX_H
#define _STORE_INDEX_H

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

#include "mentok_types.h"
#include "store/hexastore/head.h"

typedef struct {
	int size;
	int order[3];
	uintptr_t head;
} hx_store_hexastore_index;

enum {
	HX_INDEX_ITER_DUP_NONE	= 0,
	HX_INDEX_ITER_DUP_A		= 1,
	HX_INDEX_ITER_DUP_B		= 2,
	HX_INDEX_ITER_DUP_C		= 3
};

typedef struct {
	hx_store_hexastore_index* index;
	int flags;
	hx_node_id node_mask_a, node_mask_b, node_mask_c;
	char node_dup_b;
	char node_dup_c;
	hx_head_iter* head_iter;
	hx_vector_iter* vector_iter;
	hx_terminal_iter* terminal_iter;
	int started;
	int finished;
} hx_store_hexastore_index_iter;

static int HX_INDEX_ORDER_SPO[3]	= { HX_SUBJECT, HX_PREDICATE, HX_OBJECT };
static int HX_INDEX_ORDER_SOP[3]	= { HX_SUBJECT, HX_OBJECT, HX_PREDICATE };
static int HX_INDEX_ORDER_PSO[3]	= { HX_PREDICATE, HX_SUBJECT, HX_OBJECT };
static int HX_INDEX_ORDER_POS[3]	= { HX_PREDICATE, HX_OBJECT, HX_SUBJECT };
static int HX_INDEX_ORDER_OSP[3]	= { HX_OBJECT, HX_SUBJECT, HX_PREDICATE };
static int HX_INDEX_ORDER_OPS[3]	= { HX_OBJECT, HX_PREDICATE, HX_SUBJECT };

/* hx_store_hexastore_index* hx_new_index ( int a, int b, int c ); */
hx_store_hexastore_index* hx_new_index ( void* world, int* index_order );
int hx_free_index ( hx_store_hexastore_index* index );
int hx_store_hexastore_index_debug ( hx_store_hexastore_index* index );
int hx_store_hexastore_index_add_triple ( hx_store_hexastore_index* index, hx_node_id s, hx_node_id p, hx_node_id o );
int hx_store_hexastore_index_add_triple_terminal ( hx_store_hexastore_index* index, hx_node_id s, hx_node_id p, hx_node_id o, hx_terminal** t );
int hx_store_hexastore_index_add_triple_with_terminal ( hx_store_hexastore_index* index, hx_terminal* t, hx_node_id s, hx_node_id p, hx_node_id o, int _new );
int hx_store_hexastore_index_remove_triple ( hx_store_hexastore_index* i, hx_node_id s, hx_node_id p, hx_node_id o );
uintptr_t hx_store_hexastore_index_triples_count ( hx_store_hexastore_index* index );
hx_head* hx_store_hexastore_index_head ( hx_store_hexastore_index* index );

int hx_store_hexastore_index_write( hx_store_hexastore_index* t, FILE* f );
hx_store_hexastore_index* hx_store_hexastore_index_read( FILE* f, int buffer );

hx_store_hexastore_index_iter* hx_store_hexastore_index_new_iter ( hx_store_hexastore_index* index );
hx_store_hexastore_index_iter* hx_store_hexastore_index_new_iter1 ( hx_store_hexastore_index* index, hx_node_id s, hx_node_id p, hx_node_id o );
int hx_free_index_iter ( hx_store_hexastore_index_iter* iter );

int hx_store_hexastore_index_iter_finished ( hx_store_hexastore_index_iter* iter );
int hx_store_hexastore_index_iter_current ( hx_store_hexastore_index_iter* iter, hx_node_id* s, hx_node_id* p, hx_node_id* o );
int hx_store_hexastore_index_iter_next ( hx_store_hexastore_index_iter* iter );

int hx_store_hexastore_index_iter_is_sorted_by_index ( hx_store_hexastore_index_iter* iter, int index );

char* hx_store_hexastore_index_name ( hx_store_hexastore_index* idx );

#ifdef __cplusplus
}
#endif

#endif
