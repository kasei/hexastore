#ifndef _STORE_TOKYOCABINET_H
#define _STORE_TOKYOCABINET_H

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
#include <tcutil.h>
#include <tcbdb.h>

#include "store/tokyocabinet/tcindex.h"
#include "store/tokyocabinet/tokyocabinet.h"
#include "store/store.h"

typedef struct {
	void* world;
	const char* directory;
	hx_node_id next_id;
	int bulk_load;
	hx_store_tokyocabinet_index* bulk_load_index;
	TCBDB* id2node;
	TCBDB* node2id;
	TCBDB* counts;
	hx_store_tokyocabinet_index* spo;
	hx_store_tokyocabinet_index* sop;
	hx_store_tokyocabinet_index* pso;
	hx_store_tokyocabinet_index* pos;
	hx_store_tokyocabinet_index* osp;
	hx_store_tokyocabinet_index* ops;
} hx_store_tokyocabinet;

typedef struct {
	hx_store_tokyocabinet_index_iter* iter;
	int size;
	char** names;
	int* triple_pos_to_index;
	int* index_to_triple_pos;
	char *subject, *predicate, *object;
	hx_variablebindings* current;
} _hx_store_tokyocabinet_iter_vb_info;

hx_store* hx_new_store_tokyocabinet ( void* world, const char* directory );

int hx_store_tokyocabinet_init (hx_store* store, void* options);
int hx_store_tokyocabinet_debug (hx_store* store);

TCBDB* _hx_new_tokyocabinet_nodemap ( hx_store_tokyocabinet* hx );


/***** STORE METHODS: *****/


/* Close storage/model context */
int hx_store_tokyocabinet_close (hx_store* store);

/* Return the number of triples in the storage for model */
uint64_t hx_store_tokyocabinet_size (hx_store* store);	

/* Return the number of triples matching a triple pattern */
uint64_t hx_store_tokyocabinet_count (hx_store* store, hx_triple* triple);	

/* Begin a bulk load processes. Indexes and counts won't be accurate again until finish_bulk_load is called. */
int hx_store_tokyocabinet_begin_bulk_load ( hx_store* storage );

/* Begin a bulk load processes. Indexes and counts won't be accurate again until end_bulk_load is called. */
int hx_store_tokyocabinet_end_bulk_load ( hx_store* storage );

/* Add a triple to the storage from the given model */
int hx_store_tokyocabinet_add_triple (hx_store* store, hx_triple* triple);

/* Remove a triple from the storage */
int hx_store_tokyocabinet_remove_triple (hx_store* store, hx_triple* triple);

/* Check if triple is in storage */
int hx_store_tokyocabinet_contains_triple (hx_store* store, hx_triple* triple);

/* Return a stream of triples matching a triple pattern */
hx_variablebindings_iter* hx_store_tokyocabinet_get_statements (hx_store* store, hx_triple* triple, hx_node* sort_variable);

/* Return a stream of triples matching a triple pattern with a specific index thunk (originating from the triple_orderings function) */
hx_variablebindings_iter* hx_store_tokyocabinet_get_statements_with_index (hx_store* storage, hx_triple* triple, hx_store_tokyocabinet_index* index);

/* Synchronise to underlying storage */
int hx_store_tokyocabinet_sync (hx_store* store);

/* Return a list of ordering arrays, giving the possible access patterns for the given triple */
hx_container_t* hx_store_tokyocabinet_triple_orderings (hx_store* store, hx_triple*);

/* Get a string representation of a triple ordering returned by triple_orderings */
char* hx_store_tokyocabinet_ordering_name (hx_store* store, void* ordering);

/* Return an ID value for a node. */
hx_node_id hx_store_tokyocabinet_node2id (hx_store* storage, hx_node* node);

/* Return a node object for an ID. Caller is responsible for freeing the node. */
hx_node* hx_store_tokyocabinet_id2node (hx_store* storage, hx_node_id id);



#ifdef __cplusplus
}
#endif

#endif
