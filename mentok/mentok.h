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

#include "mentok/mentok_types.h"
#include "mentok/rdf/triple.h"
#include "mentok/algebra/variablebindings.h"
#include "mentok/misc/nodemap.h"
#include "mentok/misc/util.h"
#include "mentok/engine/variablebindings_iter.h"
#include "mentok/store/store.h"

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

#include "mentok/algebra/bgp.h"

typedef struct {
	void* world;
	hx_model* hx;
	hx_container_t* remote_sources;
	int64_t remote_latency_cost;
	int64_t nestedloopjoin_penalty;
	int64_t hashjoin_penalty;
	int64_t unsorted_mergejoin_penalty;
	hx_variablebindings_iter* (*bgp_exec_func)( void*, hx_model*, void* thunk );
	hx_node* (*lookup_node)( void*, hx_node_id );
	hx_container_t* (*optimizer_access_plans) ( void* ctx, hx_triple* t );
	hx_container_t* (*optimizer_join_plans) ( void* ctx, hx_container_t* lhs, hx_container_t* rhs, int leftjoin );
	void* bgp_exec_func_thunk;
} hx_execution_context;

hx_execution_context* hx_new_execution_context ( void* world, hx_model* hx );
int hx_execution_context_init ( hx_execution_context* c, void* world, hx_model* hx );
int hx_execution_context_set_bgp_exec_func ( hx_execution_context* ctx, hx_variablebindings_iter* (*)( void*, hx_model*, void* ), void* thunk );
hx_node* hx_execution_context_lookup_node ( hx_execution_context* ctx, hx_node_id nodeid );
int hx_free_execution_context ( hx_execution_context* c );


hx_model* hx_new_model ( void* world );
hx_model* hx_new_model_with_store ( void* world, hx_store* store );
int hx_free_model ( hx_model* hx );

int hx_model_add_triple( hx_model* hx, hx_node* s, hx_node* p, hx_node* o );

int hx_model_remove_triple( hx_model* hx, hx_node* s, hx_node* p, hx_node* o );
int hx_model_debug ( hx_model* hx );

uint64_t hx_model_triples_count( hx_model* hx );
uint64_t hx_model_count_statements( hx_model* hx, hx_node* s, hx_node* p, hx_node* o );

hx_node* hx_model_new_variable ( hx_model* hx );
hx_node* hx_model_new_named_variable ( hx_model* hx, char* name );

hx_variablebindings_iter* hx_model_new_variablebindings_iter_for_triple ( hx_model* hx, hx_triple* t, hx_node_position_t sort_position );

#ifdef __cplusplus
}
#endif

#endif
