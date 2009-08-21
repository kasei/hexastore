#ifndef _PROJECT_H
#define _PROJECT_H

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
#include "engine/variablebindings.h"
#include "algebra/expr.h"

typedef struct {
	int started;
	int finished;
	int size;
	hx_variablebindings* current;
	hx_variablebindings_iter* iter;
	char** names;
	int* columns;
} _hx_project_iter_vb_info;

int _hx_project_iter_vb_finished ( void* iter );
int _hx_project_iter_vb_current ( void* iter, void* results );
int _hx_project_iter_vb_next ( void* iter );	
int _hx_project_iter_vb_free ( void* iter );
int _hx_project_iter_vb_size ( void* iter );
char** _hx_project_iter_vb_names ( void* iter );
int _hx_project_iter_sorted_by ( void* data, int index );
int _hx_project_debug ( void* data, char* header, int _indent );

hx_variablebindings_iter* hx_new_project_iter ( hx_variablebindings_iter* iter, int size, char** names );


#ifdef __cplusplus
}
#endif

#endif
