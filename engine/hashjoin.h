#ifndef _HASHJOIN_H
#define _HASHJOIN_H

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
#include "engine/variablebindings.h"

typedef struct {
	int size;
	char** names;
	hx_variablebindings_iter* lhs;
	hx_variablebindings_iter* rhs;
	int finished;
	int started;
	hx_variablebindings* current;
	int leftjoin;
} _hx_hashjoin_iter_vb_info;

int _hx_hashjoin_iter_vb_finished ( void* iter );
int _hx_hashjoin_iter_vb_current ( void* iter, void* results );
int _hx_hashjoin_iter_vb_next ( void* iter );	
int _hx_hashjoin_iter_vb_free ( void* iter );
int _hx_hashjoin_iter_vb_size ( void* iter );
char** _hx_hashjoin_iter_vb_names ( void* iter );

hx_variablebindings_iter* hx_new_hashjoin_iter ( hx_variablebindings_iter* lhs, hx_variablebindings_iter* rhs );
hx_variablebindings_iter* hx_new_hashjoin_iter2 ( hx_variablebindings_iter* lhs, hx_variablebindings_iter* rhs, int leftjoin );

#ifdef __cplusplus
}
#endif

#endif
