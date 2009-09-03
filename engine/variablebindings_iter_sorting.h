#ifndef _VARIABLEBINDINGS_ITER_SORTING_H
#define _VARIABLEBINDINGS_ITER_SORTING_H

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

#include "algebra/variablebindings.h"
#include "hexastore_types.h"
#include "misc/util.h"
#include "rdf/node.h"
#include "algebra/expr.h"
#include "engine/materialize.h"
#include "engine/variablebindings_iter.h"

typedef enum {
	HX_VARIABLEBINDINGS_ITER_SORT_ASCENDING		= 1,
	HX_VARIABLEBINDINGS_ITER_SORT_DESCENDING	= 2
} hx_variablebindings_iter_sort_order;

typedef struct {
	hx_variablebindings_iter_sort_order order;
	int sparql_order; /* is this ordering based on the SPARQL comparison function? if so, we can use it for 'ORDER BY' clauses, otherwise it's still useful for things like merge-join. */
	hx_expr* expr;
} hx_variablebindings_iter_sorting;

hx_variablebindings_iter_sorting* hx_copy_variablebindings_iter_sorting ( hx_variablebindings_iter_sorting* );
hx_variablebindings_iter_sorting* hx_variablebindings_iter_new_node_sorting ( hx_variablebindings_iter_sort_order order, int sparql_order, hx_node* node );
hx_variablebindings_iter_sorting* hx_variablebindings_iter_new_sorting ( hx_variablebindings_iter_sort_order order, int sparql_order, hx_expr* expr );
int hx_free_variablebindings_iter_sorting ( hx_variablebindings_iter_sorting* sorting );
int hx_variablebindings_iter_sorting_string ( hx_variablebindings_iter_sorting* sorting, char** string );

#ifdef __cplusplus
}
#endif

#endif
