#ifndef _BGP_H
#define _BGP_H

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
#include "hexastore.h"
#include "node.h"

typedef struct {
	uint64_t cost;
	hx_triple* triple;
} _hx_bgp_selectivity_t;

typedef struct {
	int size;
	int variables;
	char** variable_names;
	hx_triple** triples;
} hx_bgp;

hx_bgp* hx_new_bgp ( int size, hx_triple** triples );
hx_bgp* hx_new_bgp1 ( hx_triple* t1 );
hx_bgp* hx_new_bgp2 ( hx_triple* t1, hx_triple* t2 );
int hx_free_bgp ( hx_bgp* b );

int _hx_bgp_selectivity_cmp ( const void* a, const void* b );
int _hx_bgp_triple_joins_with_seen ( hx_bgp* b, hx_triple* t, int* seen, int size );

int hx_bgp_size ( hx_bgp* b );
int hx_bgp_variables ( hx_bgp* b, hx_node*** vars );
hx_triple* hx_bgp_triple ( hx_bgp* b, int index );
int hx_bgp_reorder ( hx_bgp* , hx_hexastore* );
hx_variablebindings_iter* hx_bgp_execute ( hx_bgp*, hx_hexastore* );

hx_bgp* hx_bgp_substitute_variables ( hx_bgp* orig, hx_variablebindings* b, hx_nodemap* map );

int hx_bgp_debug ( hx_bgp* b );
int hx_bgp_string ( hx_bgp* b, char** string );
int hx_bgp_sse ( hx_bgp* b, char** string, char* indent, int level );

hx_node_id* hx_bgp_thaw_ids ( char* ptr, int* len );
char* hx_bgp_freeze( hx_bgp* b, int* len, hx_nodemap* map );

#ifdef __cplusplus
}
#endif

#endif
