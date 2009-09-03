#ifndef _STORE_HEXASTORE_H
#define _STORE_HEXASTORE_H

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

#include "store/store.h"
#include "store/hexastore/index.h"
#include "store/hexastore/btree.h"
#include "store/hexastore/head.h"
#include "store/hexastore/vector.h"
#include "store/hexastore/terminal.h"

typedef struct {
	hx_nodemap* map;
	hx_store_hexastore_index* spo;
	hx_store_hexastore_index* sop;
	hx_store_hexastore_index* pso;
	hx_store_hexastore_index* pos;
	hx_store_hexastore_index* osp;
	hx_store_hexastore_index* ops;
	hx_container_t* indexes;
	int next_var;
} hx_store_hexastore;

typedef struct {
	hx_store_hexastore_index_iter* iter;
	int size;
	char** names;
	int* triple_pos_to_index;
	int* index_to_triple_pos;
	char *subject, *predicate, *object;
	hx_variablebindings* current;
} _hx_store_hexastore_iter_vb_info;

hx_store* hx_new_store_hexastore ( void* world );
hx_store* hx_new_store_hexastore_with_nodemap ( void* world, hx_nodemap* map );

int hx_store_hexastore_init (hx_store* store, void* options);
hx_nodemap* hx_store_hexastore_get_nodemap ( hx_store* store );
int hx_store_hexastore_debug (hx_store* store);

int hx_store_hexastore_write( hx_store* store, FILE* f );
hx_store* hx_store_hexastore_read( void* world, FILE* f, int buffer );


/***** STORE METHODS: *****/


/* Close storage/model context */
int hx_store_hexastore_close (hx_store* store);

/* Return the number of triples in the storage for model */
uint64_t hx_store_hexastore_size (hx_store* store);	

/* Return the number of triples matching a triple pattern */
uint64_t hx_store_hexastore_count (hx_store* store, hx_triple* triple);	

/* Add a triple to the storage from the given model */
int hx_store_hexastore_add_triple (hx_store* store, hx_triple* triple);

/* Remove a triple from the storage */
int hx_store_hexastore_remove_triple (hx_store* store, hx_triple* triple);

/* Check if triple is in storage */
int hx_store_hexastore_contains_triple (hx_store* store, hx_triple* triple);

/* Return a stream of triples matching a triple pattern */
hx_variablebindings_iter* hx_store_hexastore_get_statements (hx_store* store, hx_triple* triple, hx_node* sort_variable);

/* Synchronise to underlying storage */
int hx_store_hexastore_sync (hx_store* store);

/* Return a list of ordering arrays, giving the possible access patterns for the given triple */
hx_container_t* hx_store_hexastore_triple_orderings (hx_store* store, hx_triple*);

/* Return an ID value for a node. */
hx_node_id hx_store_hexastore_node2id (hx_store* storage, hx_node* node);

/* Return a node object for an ID. Caller is responsible for freeing the node. */
hx_node* hx_store_hexastore_id2node (hx_store* storage, hx_node_id id);



#ifdef __cplusplus
}
#endif

#endif
