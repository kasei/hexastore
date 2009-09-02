#ifndef _VECTOR_H
#define _VECTOR_H

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
#include "store/hexastore/terminal.h"

#define VECTOR_TREE_BRANCHING_SIZE				28

typedef struct {
	uintptr_t triples_count;
	uintptr_t tree;
} hx_vector;

typedef struct {
	hx_vector* vector;
	hx_btree_iter* t;
} hx_vector_iter;

hx_vector* hx_new_vector ( void* world );
int hx_free_vector ( hx_vector* list );

int hx_vector_debug ( const char* header, const hx_vector* v );
int hx_vector_add_terminal ( hx_vector* v, const hx_node_id n, hx_terminal* t );
hx_terminal* hx_vector_get_terminal ( hx_vector* v, hx_node_id n );
int hx_vector_remove_terminal ( hx_vector* v, hx_node_id n );
list_size_t hx_vector_size ( hx_vector* v );
uintptr_t hx_vector_triples_count ( hx_vector* v );
void hx_vector_triples_count_add ( hx_vector* v, int c );

int hx_vector_write( hx_vector* t, FILE* f );
hx_vector* hx_vector_read( FILE* f, int buffer );

hx_vector_iter* hx_vector_new_iter ( hx_vector* vector );
int hx_free_vector_iter ( hx_vector_iter* iter );
int hx_vector_iter_finished ( hx_vector_iter* iter );
int hx_vector_iter_current ( hx_vector_iter* iter, hx_node_id* n, hx_terminal** t );
int hx_vector_iter_next ( hx_vector_iter* iter );
int hx_vector_iter_seek( hx_vector_iter* iter, hx_node_id n );

#ifdef __cplusplus
}
#endif

#endif
