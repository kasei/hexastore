#ifndef _TRIPLE_H
#define _TRIPLE_H

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
#include "misc/nodemap.h"
#include "rdf/node.h"
#include "misc/util.h"

typedef struct {
	hx_node* subject;
	hx_node* predicate;
	hx_node* object;
} hx_triple;

typedef struct {
	hx_node_id subject;
	hx_node_id predicate;
	hx_node_id object;
} hx_triple_id;

hx_triple* hx_new_triple( hx_node* s, hx_node* p, hx_node* o );
int hx_free_triple ( hx_triple* t );

hx_node* hx_triple_node ( hx_triple* t, int i );

int hx_triple_bound_count ( hx_triple* t );

int hx_triple_id_string ( hx_triple_id* t, hx_nodemap* map, char** string );
int hx_triple_string ( hx_triple* t, char** string );

uint64_t hx_triple_hash_on_node ( hx_triple* t, hx_node_position_t pos, hx_hash_function* func );
uint64_t hx_triple_hash ( hx_triple* t, hx_hash_function* func );

#ifdef __cplusplus
}
#endif

#endif
