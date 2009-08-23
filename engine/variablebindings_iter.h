#ifndef _VARIABLEBINDINGS_ITER_H
#define _VARIABLEBINDINGS_ITER_H

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
#include "zlib.h"

#include "algebra/variablebindings.h"
#include "hexastore_types.h"
#include "misc/nodemap.h"
#include "misc/util.h"
#include "rdf/node.h"

typedef struct {
	int (*finished) ( void* iter );
	int (*current) ( void* iter, void* results );
	int (*next) ( void* iter );	
	int (*free) ( void* iter );
	int (*size) ( void* iter );
	char** (*names) ( void* iter );
	int (*sorted_by_index) ( void* iter, int index );
	int (*debug) ( void* iter, char* header, int indent );
} hx_variablebindings_iter_vtable;

typedef struct {
	int size;
	char** names;
	hx_variablebindings_iter_vtable* vtable;
	void* ptr;
} hx_variablebindings_iter;

#include "algebra/expr.h"
#include "engine/materialize.h"

typedef enum {
	HX_VARIABLEBINDINGS_ITER_SORT_ASCENDING		= 1,
	HX_VARIABLEBINDINGS_ITER_SORT_DESCENDING	= 2
} hx_variablebindings_iter_sort_order;

typedef struct {
	hx_variablebindings_iter_sort_order order;
	int sparql_order; /* is this ordering based on the SPARQL comparison function? if so, we can use it for 'ORDER BY' clauses, otherwise it's still useful for things like merge-join. */
	hx_expr* expr;
} hx_variablebindings_iter_sorting;

hx_variablebindings_iter_sorting* hx_variablebindings_iter_new_node_sorting ( hx_variablebindings_iter_sort_order order, int sparql_order, hx_node* node );
hx_variablebindings_iter_sorting* hx_variablebindings_iter_new_sorting ( hx_variablebindings_iter_sort_order order, int sparql_order, hx_expr* expr );
int hx_free_variablebindings_iter_sorting ( hx_variablebindings_iter_sorting* sorting );
int hx_variablebindings_iter_sorting_string ( hx_variablebindings_iter_sorting* sorting, char** string );

hx_variablebindings_iter* hx_variablebindings_new_empty_iter ( void );
hx_variablebindings_iter* hx_variablebindings_new_empty_iter_with_names ( int size, char** names );

hx_variablebindings_iter* hx_variablebindings_new_iter ( hx_variablebindings_iter_vtable* vtable, void* ptr );
int hx_free_variablebindings_iter ( hx_variablebindings_iter* iter );
int hx_variablebindings_iter_finished ( hx_variablebindings_iter* iter );
int hx_variablebindings_iter_current ( hx_variablebindings_iter* iter, hx_variablebindings** b );
int hx_variablebindings_iter_next ( hx_variablebindings_iter* iter );
int hx_variablebindings_iter_size ( hx_variablebindings_iter* iter );
char** hx_variablebindings_iter_names ( hx_variablebindings_iter* iter );
int hx_variablebindings_column_index ( hx_variablebindings_iter* iter, char* column );
int hx_variablebindings_iter_is_sorted_by_index ( hx_variablebindings_iter* iter, int index );
int hx_variablebindings_iter_debug ( hx_variablebindings_iter* iter, char* header, int indent );

hx_variablebindings_iter* hx_variablebindings_sort_iter( hx_variablebindings_iter* iter, int index );

#ifdef __cplusplus
}
#endif

#endif
