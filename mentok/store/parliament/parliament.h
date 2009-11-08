#ifndef _STORE_PARLIAMENT_H
#define _STORE_PARLIAMENT_H

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
#include <unistd.h>

#include "mentok/store/store.h"

typedef uint32_t hx_store_parliament_offset;
typedef uint32_t hx_store_parliament_id;

enum {
	HX_STORE_PARLIAMENT_STATEMENT_DELETED	= 0x1
} hx_store_parliament_statement_flags;

typedef struct {
	hx_store_parliament_offset fs;
	hx_store_parliament_offset fp;
	hx_store_parliament_offset fo;
	hx_store_parliament_offset sc;
	hx_store_parliament_offset pc;
	hx_store_parliament_offset oc;
	hx_node* value;
} hx_store_parliament_resource_record;

typedef struct {
	uint8_t flags;
	hx_store_parliament_id s;
	hx_store_parliament_id p;
	hx_store_parliament_id o;
	hx_store_parliament_offset ns;
	hx_store_parliament_offset np;
	hx_store_parliament_offset no;
} hx_store_parliament_statement_record;

typedef struct {
	int allocated;
	int used;
	hx_store_parliament_resource_record* records;
} hx_store_parliament_resource_table;

typedef struct {
	int allocated;
	int used;
	hx_store_parliament_statement_record* records;
} hx_store_parliament_statement_list;

typedef struct {
	void* world;
	uint64_t count;
	struct avl_table* node2id;
	hx_store_parliament_resource_table* resources;
	hx_store_parliament_statement_list* statements;
} hx_store_parliament;

hx_store* hx_new_store_parliament ( void* world );

/***** STORE METHODS: *****/


/* Close storage/model context */
int hx_store_parliament_close (hx_store* store);

/* Return the number of triples in the storage for model */
uint64_t hx_store_parliament_size (hx_store* store);	

/* Return the number of triples matching a triple pattern */
uint64_t hx_store_parliament_count (hx_store* store, hx_triple* triple);	

/* Begin a bulk load processes. Indexes and counts won't be accurate again until finish_bulk_load is called. */
int hx_store_parliament_begin_bulk_load ( hx_store* storage );

/* Begin a bulk load processes. Indexes and counts won't be accurate again until end_bulk_load is called. */
int hx_store_parliament_end_bulk_load ( hx_store* storage );

/* Add a triple to the storage from the given model */
int hx_store_parliament_add_triple (hx_store* store, hx_triple* triple);

/* Remove a triple from the storage */
int hx_store_parliament_remove_triple (hx_store* store, hx_triple* triple);

/* Check if triple is in storage */
int hx_store_parliament_contains_triple (hx_store* store, hx_triple* triple);

/* Return a stream of triples matching a triple pattern */
hx_variablebindings_iter* hx_store_parliament_get_statements (hx_store* store, hx_triple* triple, hx_node* sort_variable);

/* Return a stream of triples matching a triple pattern with a specific index thunk (originating from the triple_orderings function) */
hx_variablebindings_iter* hx_store_parliament_get_statements_with_index (hx_store* storage, hx_triple* triple, void* thunk);

/* Synchronise to underlying storage */
int hx_store_parliament_sync (hx_store* store);

/* Return a list of ordering arrays, giving the possible access patterns for the given triple */
hx_container_t* hx_store_parliament_triple_orderings (hx_store* store, hx_triple*);

/* Get a string representation of a triple ordering returned by triple_orderings */
char* hx_store_parliament_ordering_name (hx_store* store, void* ordering);

/* Return an ID value for a node. */
hx_node_id hx_store_parliament_node2id (hx_store* storage, hx_node* node);

/* Return a node object for an ID. Caller is responsible for freeing the node. */
hx_node* hx_store_parliament_id2node (hx_store* storage, hx_node_id id);



#ifdef __cplusplus
}
#endif

#endif
