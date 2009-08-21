#ifndef _GRAPHPATTERN_H
#define _GRAPHPATTERN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
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
#include "engine/mergejoin.h"
#include "algebra/expr.h"
#include "algebra/bgp.h"
#include "engine/variablebindings.h"

typedef enum {
	HX_GRAPHPATTERN_BGP			= 'B',
	HX_GRAPHPATTERN_GRAPH		= 'N',
	HX_GRAPHPATTERN_OPTIONAL	= 'O',
	HX_GRAPHPATTERN_UNION		= 'U',
	HX_GRAPHPATTERN_GROUP		= 'G',
	HX_GRAPHPATTERN_FILTER		= 'F'
} hx_graphpattern_type_t;

typedef struct {
	hx_graphpattern_type_t type;
	int arity;
	void* data;
} hx_graphpattern;

hx_graphpattern* hx_new_graphpattern ( hx_graphpattern_type_t type, ... );
hx_graphpattern* hx_new_graphpattern_ptr ( hx_graphpattern_type_t type, int size, void* ptr );
int hx_free_graphpattern ( hx_graphpattern* p );

hx_graphpattern* hx_graphpattern_substitute_variables ( hx_graphpattern* pat, hx_variablebindings* b, hx_nodemap* map );

hx_variablebindings_iter* hx_graphpattern_execute ( hx_graphpattern*, hx_hexastore* );

int hx_graphpattern_variables ( hx_graphpattern* p, hx_node*** vars );
int hx_graphpattern_sse ( hx_graphpattern* e, char** string, char* indent, int level );
int hx_graphpattern_debug ( hx_graphpattern* p );

#ifdef __cplusplus
}
#endif

#endif
