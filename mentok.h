#ifndef _HEXASTORE_H
#define _HEXASTORE_H

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

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif

#include "mentok_types.h"
#include "rdf/triple.h"
#include "algebra/variablebindings.h"
#include "misc/nodemap.h"
#include "misc/util.h"
#include "engine/variablebindings_iter.h"
#include "store/hexastore/head.h"
#include "store/hexastore/terminal.h"
#include "store/hexastore/vector.h"
#include "store/store.h"

typedef enum {
	RDF_ITER_FLAGS_BOUND_A	= 1,
	RDF_ITER_FLAGS_BOUND_B	= 2,
	RDF_ITER_FLAGS_BOUND_C	= 4
} hx_iter_flag_t;

#define THREADED_BATCH_SIZE	5000

typedef struct {
	hx_store* store;
	int next_var;
	hx_container_t* indexes;
} hx_model;

#include "algebra/bgp.h"

typedef struct {
	void* world;
	hx_model* hx;
	int64_t nestedloopjoin_penalty;
	int64_t hashjoin_penalty;
	int64_t unsorted_mergejoin_penalty;
	hx_variablebindings_iter* (*bgp_exec_func)( void*, hx_model*, void* thunk );
	hx_node* (*lookup_node)( void*, hx_node_id );
	void* bgp_exec_func_thunk;
} hx_execution_context;

hx_execution_context* hx_new_execution_context ( void* world, hx_model* hx );
int hx_execution_context_init ( hx_execution_context* c, void* world, hx_model* hx );
int hx_execution_context_set_bgp_exec_func ( hx_execution_context* ctx, hx_variablebindings_iter* (*)( void*, hx_model*, void* ), void* thunk );
hx_node* hx_execution_context_lookup_node ( hx_execution_context* ctx, hx_node_id nodeid );
int hx_free_execution_context ( hx_execution_context* c );


hx_model* hx_new_hexastore ( void* world );
hx_model* hx_new_hexastore_with_store ( void* world, hx_store* store );
int hx_free_hexastore ( hx_model* hx );

int hx_add_triple( hx_model* hx, hx_node* s, hx_node* p, hx_node* o );

int hx_remove_triple( hx_model* hx, hx_node* s, hx_node* p, hx_node* o );
int hx_debug ( hx_model* hx );

uint64_t hx_triples_count( hx_model* hx );
uint64_t hx_count_statements( hx_model* hx, hx_node* s, hx_node* p, hx_node* o );

hx_node* hx_new_variable ( hx_model* hx );
hx_node* hx_new_named_variable ( hx_model* hx, char* name );
hx_container_t* hx_get_indexes ( hx_model* hx );

hx_variablebindings_iter* hx_new_variablebindings_iter_for_triple ( hx_model* hx, hx_triple* t, hx_node_position_t sort_position );

#ifdef __cplusplus
}
#endif

#endif
