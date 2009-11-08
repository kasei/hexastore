#ifndef _STORE_H
#define _STORE_H

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

#include "mentok/mentok_types.h"
#include "mentok/algebra/variablebindings.h"
#include "mentok/engine/variablebindings_iter.h"
#include "mentok/engine/variablebindings_iter_sorting.h"
#include "mentok/misc/util.h"
#include "mentok/rdf/triple.h"

typedef struct {
	/* Create a new storage. */
	int (*init)(void* storage, void* options);
	
	/* Close storage/model context */
	int (*close)(void* storage);
	
	/* Return the number of triples in the storage for model */
	uint64_t (*size)(void* storage);	
	
	/* Return the number of triples matching a triple pattern */
	uint64_t (*count)(void* storage, hx_triple* triple);	
	
	/* Begin a bulk load processes. Indexes and counts won't be accurate again until finish_bulk_load is called. */
	int (*begin_bulk_load)( void* storage );
	
	/* Begin a bulk load processes. Indexes and counts won't be accurate again until end_bulk_load is called. */
	int (*end_bulk_load)( void* storage );
	
	/* Add a triple to the storage from the given model */
	int (*add_triple)(void* storage, hx_triple* triple);
	
	/* Remove a triple from the storage */
	int (*remove_triple)(void* storage, hx_triple* triple);
	
	/* Check if triple is in storage */
	int (*contains_triple)(void* storage, hx_triple* triple);
	
	/* Return a stream of triples matching a triple pattern */
	hx_variablebindings_iter* (*get_statements)(void* storage, hx_triple* triple, hx_node* sort_variable);
	
	/* Return a stream of triples matching a triple pattern with a specific index thunk (originating from the triple_orderings function) */
	hx_variablebindings_iter* (*get_statements_with_index)(void* storage, hx_triple* triple, void* index);
	
	/* Synchronise to underlying storage */
	int (*sync)(void* storage);
	
	/* Return a list of ordering arrays, giving the possible access patterns for the given triple */
	hx_container_t* (*triple_orderings)(void* storage, hx_triple*);
	
	/* Get a string representation of a triple ordering returned by triple_orderings */
	char* (*ordering_name)(void* storage, void* ordering);

	/* Return an ID value for a node. */
	hx_node_id (*node2id)(void* storage, hx_node* node);
	
	/* Return a node object for an ID. Caller is responsible for freeing the node. */
	hx_node* (*id2node)(void* storage, hx_node_id id);
	
	/* Return the sort ordering that will result from calling get_statements_with_index on a particular ordering thunk */
	/* The container contains hx_variablebindings_iter_sorting* objects */
	hx_container_t* (*iter_sorting)( void* storage, hx_triple* triple, void* ordering );

} hx_store_vtable;


typedef struct {
	void* world;
	hx_store_vtable* vtable;
	void* ptr;
} hx_store;


/* Called by a store implementation constructor (i.e. hx_new_store_hexastore) */
hx_store* hx_new_store ( void* world, hx_store_vtable* vtable, void* ptr );

int hx_free_store ( hx_store* store );

uint64_t hx_store_size ( hx_store* store );
uint64_t hx_store_count ( hx_store* store, hx_triple* triple );

int hx_store_add_triple ( hx_store* store, hx_triple* triple );
int hx_store_remove_triple ( hx_store* store, hx_triple* triple );
int hx_store_contains_triple ( hx_store* store, hx_triple* triple );

hx_container_t* hx_store_triple_orderings ( hx_store* store, hx_triple* triple );
hx_variablebindings_iter* hx_store_get_statements ( hx_store* store, hx_triple* triple, hx_node* sort_variable );
hx_variablebindings_iter* hx_store_get_statements_with_index (hx_store* store, hx_triple* triple, void* thunk);

hx_node_id hx_store_get_node_id ( hx_store* store, hx_node* node );
hx_node* hx_store_get_node ( hx_store* store, hx_node_id id );

int hx_store_begin_bulk_load ( hx_store* store );
int hx_store_end_bulk_load ( hx_store* store );

char* hx_store_ordering_name (hx_store* storage, void* ordering);
hx_container_t* hx_store_iter_sorting ( hx_store* storage, hx_triple* triple, void* ordering );


int hx_store_variablebindings_string ( hx_store* store, hx_variablebindings* b, char** string );
int hx_store_variablebindings_debug ( hx_store* store, hx_variablebindings* b );

hx_node* hx_variablebindings_node_for_binding ( hx_variablebindings* b, hx_store* store, int column );
hx_node* hx_variablebindings_node_for_binding_name ( hx_variablebindings* b, hx_store* store, char* name );

#ifdef __cplusplus
}
#endif

#endif
