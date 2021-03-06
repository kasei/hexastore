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

#include "mentok/mentok.h"
#include "mentok/mentok_types.h"
#include "mentok/algebra/expr.h"
#include "mentok/algebra/bgp.h"
#include "mentok/algebra/variablebindings.h"
#include "mentok/store/store.h"

typedef enum {
	HX_GRAPHPATTERN_BGP			= 'B',
	HX_GRAPHPATTERN_GRAPH		= 'N',
	HX_GRAPHPATTERN_OPTIONAL	= 'O',
	HX_GRAPHPATTERN_UNION		= 'U',
	HX_GRAPHPATTERN_GROUP		= 'G',
	HX_GRAPHPATTERN_FILTER		= 'F',
	HX_GRAPHPATTERN_SERVICE		= 'S',
} hx_graphpattern_type_t;

typedef struct {
	hx_graphpattern_type_t type;
	int arity;
	void* data;
} hx_graphpattern;

hx_graphpattern* hx_new_graphpattern ( hx_graphpattern_type_t type, ... );
hx_graphpattern* hx_new_graphpattern_ptr ( hx_graphpattern_type_t type, int size, void* ptr );
hx_graphpattern* hx_graphpattern_parse_string ( const char* string );

int hx_free_graphpattern ( hx_graphpattern* p );

hx_graphpattern_type_t hx_graphpattern_type ( hx_graphpattern* p );
int hx_graphpattern_variables ( hx_graphpattern* p, hx_node*** vars );
int hx_graphpattern_sse ( hx_graphpattern* e, char** string, char* indent, int level );
int hx_graphpattern_debug ( hx_graphpattern* p );

#ifdef __cplusplus
}
#endif

#endif
