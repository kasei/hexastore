#ifndef _RENDEZVOUSJOIN_H
#define _RENDEZVOUSJOIN_H

#ifdef __cplusplus
extern "C" {
#endif

#define NODE_LIST_ALLOC_SIZE	10

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
#include "variablebindings.h"

typedef struct {
	int size;
	char** names;
	hx_variablebindings_iter* lhs;
	hx_variablebindings_iter* rhs;
	int finished;
	int started;
	hx_variablebindings* current;
	int lhs_size;
	int rhs_size;
	hx_variablebindings** lhs_batch;
	hx_variablebindings** rhs_batch;
	int lhs_batch_alloc_size;
	int rhs_batch_alloc_size;
	int lhs_batch_index;
	int rhs_batch_index;
} _hx_rendezvousjoin_iter_vb_info;

int _hx_rendezvousjoin_iter_vb_finished ( void* iter );
int _hx_rendezvousjoin_iter_vb_current ( void* iter, void* results );
int _hx_rendezvousjoin_iter_vb_next ( void* iter );	
int _hx_rendezvousjoin_iter_vb_free ( void* iter );
int _hx_rendezvousjoin_iter_vb_size ( void* iter );
char** _hx_rendezvousjoin_iter_vb_names ( void* iter );

hx_variablebindings_iter* hx_new_rendezvousjoin_iter ( hx_variablebindings_iter* lhs, hx_variablebindings_iter* rhs );
hx_variablebindings* hx_rendezvousjoin_join_variablebindings( hx_variablebindings* left, hx_variablebindings* right );


#ifdef __cplusplus
}
#endif

#endif
