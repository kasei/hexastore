#ifndef _TCINDEX_H
#define _TCINDEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <tcbdb.h>
#include <tcutil.h>
#include <unistd.h>

#include "hexastore_types.h"
#include "variablebindings.h"
#include "head.h"

typedef struct {
	int order[3];
	TCBDB *bdb;
} hx_tcindex;

enum {
	HX_TCINDEX_ITER_DUP_NONE	= 0,
	HX_TCINDEX_ITER_DUP_A		= 1,
	HX_TCINDEX_ITER_DUP_B		= 2,
	HX_TCINDEX_ITER_DUP_C		= 3
};

typedef struct {
	hx_storage_manager* storage;
	hx_tcindex* index;
	int flags;
	hx_node_id node_mask_a, node_mask_b, node_mask_c;
	char node_dup_b;
	char node_dup_c;
	BDBCUR* cursor;
	int finished;
} hx_tcindex_iter;

typedef struct {
	hx_storage_manager* s;
	hx_tcindex_iter* iter;
	int size;
	int free_names;
	char** names;
	int* triple_pos_to_index;
	int* index_to_triple_pos;
	char *subject, *predicate, *object;
	hx_variablebindings* current;
} _hx_tcindex_iter_vb_info;

static int HX_TCINDEX_ORDER_SPO[3]	= { HX_SUBJECT, HX_PREDICATE, HX_OBJECT };
static int HX_TCINDEX_ORDER_SOP[3]	= { HX_SUBJECT, HX_OBJECT, HX_PREDICATE };
static int HX_TCINDEX_ORDER_PSO[3]	= { HX_PREDICATE, HX_SUBJECT, HX_OBJECT };
static int HX_TCINDEX_ORDER_POS[3]	= { HX_PREDICATE, HX_OBJECT, HX_SUBJECT };
static int HX_TCINDEX_ORDER_OSP[3]	= { HX_OBJECT, HX_SUBJECT, HX_PREDICATE };
static int HX_TCINDEX_ORDER_OPS[3]	= { HX_OBJECT, HX_PREDICATE, HX_SUBJECT };

/* hx_tcindex* hx_new_index ( int a, int b, int c ); */
hx_tcindex* hx_new_tcindex ( hx_storage_manager* s, int* index_order, const char* filename );
int hx_free_tcindex ( hx_tcindex* index, hx_storage_manager* s );
int hx_tcindex_debug ( hx_tcindex* index, hx_storage_manager* s );
int hx_tcindex_add_triple ( hx_tcindex* index, hx_storage_manager* st, hx_node_id s, hx_node_id p, hx_node_id o );
int hx_tcindex_remove_triple ( hx_tcindex* i, hx_storage_manager* st, hx_node_id s, hx_node_id p, hx_node_id o );
hx_storage_id_t hx_tcindex_triples_count ( hx_tcindex* index, hx_storage_manager* st );

hx_tcindex_iter* hx_tcindex_new_iter ( hx_tcindex* index, hx_storage_manager* st );
hx_tcindex_iter* hx_tcindex_new_iter1 ( hx_tcindex* index, hx_storage_manager* st, hx_node_id s, hx_node_id p, hx_node_id o );
int hx_free_tcindex_iter ( hx_tcindex_iter* iter );

int hx_tcindex_iter_finished ( hx_tcindex_iter* iter );
int hx_tcindex_iter_current ( hx_tcindex_iter* iter, hx_node_id* s, hx_node_id* p, hx_node_id* o );
int hx_tcindex_iter_next ( hx_tcindex_iter* iter );

int hx_tcindex_iter_is_sorted_by_index ( hx_tcindex_iter* iter, int index );

hx_variablebindings_iter* hx_new_tcindex_iter_variablebindings ( hx_tcindex_iter* i, hx_storage_manager* s, char* subj_name, char* pred_name, char* obj_name, int free_names );

#ifdef __cplusplus
}
#endif

#endif
