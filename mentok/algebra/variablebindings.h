#ifndef _VARIABLEBINDINGS_H
#define _VARIABLEBINDINGS_H

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
#include "mentok/misc/nodemap.h"
#include "mentok/misc/util.h"
#include "mentok/rdf/node.h"

typedef struct {
	int size;
	char** names;
	hx_node_id* nodes;
} hx_variablebindings;

typedef struct {
	int size;
	char** names;
	hx_node** nodes;
} hx_variablebindings_nodes;

hx_variablebindings* hx_model_new_variablebindings ( int size, char** names, hx_node_id* nodes );
hx_variablebindings_nodes* hx_model_new_variablebindings_nodes ( int size, char** names, hx_node** nodes );
hx_variablebindings* hx_copy_variablebindings ( hx_variablebindings* b );
int hx_free_variablebindings ( hx_variablebindings* b );
int hx_free_variablebindings_nodes ( hx_variablebindings_nodes* b );

hx_variablebindings* hx_variablebindings_project ( hx_variablebindings* b, int newsize, int* columns );
hx_variablebindings* hx_variablebindings_project_names ( hx_variablebindings* b, int newsize, char** names );

int hx_variablebindings_string_with_nodemap ( hx_variablebindings* b, hx_nodemap* map, char** string );
int hx_variablebindings_string ( hx_variablebindings* b, char** string );
void hx_variablebindings_debug ( hx_variablebindings* b );
void hx_variablebindings_print ( hx_variablebindings* b );

int hx_variablebindings_set_names ( hx_variablebindings* b, char** names );
int hx_variablebindings_size ( hx_variablebindings* b );
char* hx_variablebindings_name_for_binding ( hx_variablebindings* b, int column );
hx_node_id hx_variablebindings_node_id_for_binding ( hx_variablebindings* b, int column );
hx_node_id hx_variablebindings_node_id_for_binding_name ( hx_variablebindings* b, char* name );
char** hx_variablebindings_names ( hx_variablebindings* b );
int hx_variablebindings_cmp ( void* a, void* b );

hx_variablebindings* hx_variablebindings_thaw ( char* ptr, int len, hx_nodemap* map );
hx_variablebindings* hx_variablebindings_thaw_noadd ( char* ptr, int len, hx_nodemap* map, int join_vars_count, char** join_vars );
char* hx_variablebindings_freeze( hx_variablebindings* b, hx_nodemap* map, int* len );

hx_variablebindings* hx_variablebindings_natural_join( hx_variablebindings* left, hx_variablebindings* right );

int hx_variablebindings_nodes_string ( hx_variablebindings_nodes* b, char** string );


#ifdef __cplusplus
}
#endif

#endif
