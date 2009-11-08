#ifndef _FILTER_H
#define _FILTER_H

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
#include "mentok/mentok.h"
#include "mentok/algebra/variablebindings.h"
#include "mentok/algebra/expr.h"
#include "mentok/store/store.h"

typedef struct {
	int started;
	int finished;
	hx_expr* expr;
	hx_variablebindings* current;
	hx_variablebindings_iter* iter;
	hx_store* store;
	hx_execution_context* ctx;
} _hx_filter_iter_vb_info;

int _hx_filter_iter_vb_finished ( void* iter );
int _hx_filter_iter_vb_current ( void* iter, void* results );
int _hx_filter_iter_vb_next ( void* iter );	
int _hx_filter_iter_vb_free ( void* iter );
int _hx_filter_iter_vb_size ( void* iter );
char** _hx_filter_iter_vb_names ( void* iter );
int _hx_filter_iter_sorted_by ( void* data, int index );
int _hx_filter_debug ( void* data, char* header, int _indent );

hx_variablebindings_iter* hx_new_filter_iter ( hx_variablebindings_iter* iter, hx_expr* e, hx_execution_context* ctx );


#ifdef __cplusplus
}
#endif

#endif
