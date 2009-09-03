#ifndef _STORE_TCINDEX_H
#define _STORE_TCINDEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
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
#include "algebra/variablebindings.h"
#include "engine/variablebindings_iter.h"

typedef struct {
	int order[3];
	TCBDB *bdb;
} hx_store_tokyocabinet_index;

enum {
	HX_STORE_TCINDEX_ITER_DUP_NONE	= 0,
	HX_STORE_TCINDEX_ITER_DUP_A		= 1,
	HX_STORE_TCINDEX_ITER_DUP_B		= 2,
	HX_STORE_TCINDEX_ITER_DUP_C		= 3
};

typedef struct {
	hx_store_tokyocabinet_index* index;
	int flags;
	hx_node_id node_mask_a, node_mask_b, node_mask_c;
	char node_dup_b;
	char node_dup_c;
	BDBCUR* cursor;
	int finished;
} hx_store_tokyocabinet_index_iter;

typedef struct {
	hx_store_tokyocabinet_index_iter* iter;
	int size;
	int free_names;
	char** names;
	int* triple_pos_to_index;
	int* index_to_triple_pos;
	char *subject, *predicate, *object;
	hx_variablebindings* current;
} _hx_store_tokyocabinet_index_iter_vb_info;

static int HX_STORE_TCINDEX_ORDER_SPO[3]	= { HX_SUBJECT, HX_PREDICATE, HX_OBJECT };
static int HX_STORE_TCINDEX_ORDER_SOP[3]	= { HX_SUBJECT, HX_OBJECT, HX_PREDICATE };
static int HX_STORE_TCINDEX_ORDER_PSO[3]	= { HX_PREDICATE, HX_SUBJECT, HX_OBJECT };
static int HX_STORE_TCINDEX_ORDER_POS[3]	= { HX_PREDICATE, HX_OBJECT, HX_SUBJECT };
static int HX_STORE_TCINDEX_ORDER_OSP[3]	= { HX_OBJECT, HX_SUBJECT, HX_PREDICATE };
static int HX_STORE_TCINDEX_ORDER_OPS[3]	= { HX_OBJECT, HX_PREDICATE, HX_SUBJECT };

/* hx_store_tokyocabinet_index* hx_new_index ( int a, int b, int c ); */

hx_store_tokyocabinet_index* hx_new_tokyocabinet_index ( void* world, int* order, const char* directory, const char* filename );
int hx_free_tokyocabinet_index ( hx_store_tokyocabinet_index* index );
int hx_store_tokyocabinet_index_debug ( hx_store_tokyocabinet_index* index );
int hx_store_tokyocabinet_index_add_triple ( hx_store_tokyocabinet_index* index, hx_node_id s, hx_node_id p, hx_node_id o );
int hx_store_tokyocabinet_index_remove_triple ( hx_store_tokyocabinet_index* i, hx_node_id s, hx_node_id p, hx_node_id o );
uint64_t hx_store_tokyocabinet_index_triples_count ( hx_store_tokyocabinet_index* index );

hx_store_tokyocabinet_index_iter* hx_store_tokyocabinet_index_new_iter ( hx_store_tokyocabinet_index* index );
hx_store_tokyocabinet_index_iter* hx_store_tokyocabinet_index_new_iter1 ( hx_store_tokyocabinet_index* index, hx_node_id s, hx_node_id p, hx_node_id o );
int hx_free_tokyocabinet_index_iter ( hx_store_tokyocabinet_index_iter* iter );

int hx_store_tokyocabinet_index_iter_finished ( hx_store_tokyocabinet_index_iter* iter );
int hx_store_tokyocabinet_index_iter_current ( hx_store_tokyocabinet_index_iter* iter, hx_node_id* s, hx_node_id* p, hx_node_id* o );
int hx_store_tokyocabinet_index_iter_next ( hx_store_tokyocabinet_index_iter* iter );

int hx_store_tokyocabinet_index_iter_is_sorted_by_index ( hx_store_tokyocabinet_index_iter* iter, int index );

hx_variablebindings_iter* hx_new_tokyocabinet_index_iter_variablebindings ( hx_store_tokyocabinet_index_iter* i, char* subj_name, char* pred_name, char* obj_name, int free_names );

#ifdef __cplusplus
}
#endif

#endif
