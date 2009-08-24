#ifndef _PLAN_ACCESS_H
#define _PLAN_ACCESS_H

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
#include "engine/variablebindings_iter.h"
#include "rdf/triple.h"
#include "rdf/node.h"
#include "index.h"

typedef enum {
	HX_OPTIMIZER_PLAN_INDEX	= 0,	// access to triple pattern matching via hx_index
	HX_OPTIMIZER_PLAN_JOIN	= 1,	// join of two hx_optimizer_plan*s
	HX_OPTIMIZER_PLAN_LAST	= HX_OPTIMIZER_PLAN_JOIN
} hx_optimizer_plan_type;

typedef enum {
	HX_OPTIMIZER_PLAN_HASHJOIN			= 0,
	HX_OPTIMIZER_PLAN_MERGEJOIN			= 1,
	HX_OPTIMIZER_PLAN_NESTEDLOOPJOIN	= 2,
	HX_OPTIMIZER_PLAN_JOIN_LAST	= HX_OPTIMIZER_PLAN_NESTEDLOOPJOIN
} hx_optimizer_plan_join_type;

static char* HX_OPTIMIZER_PLAN_JOIN_NAMES[HX_OPTIMIZER_PLAN_JOIN_LAST+1]	= {
	"hash-join",
	"merge-join",
	"nestedloop-join"
};


typedef struct {
	hx_optimizer_plan_type type;
	hx_optimizer_plan_join_type join_type;
	union {
		hx_index* source;	// access
		void* lhs_plan;		// join
	};
	union {
		hx_triple* triple;	// access
		void* rhs_plan;		// join
	};
	int leftjoin;			// join;
	int order_count;
	hx_variablebindings_iter_sorting** order;
} hx_optimizer_plan;

hx_optimizer_plan* hx_copy_optimizer_plan ( hx_optimizer_plan* plan );
hx_optimizer_plan* hx_new_optimizer_access_plan ( hx_index* source, hx_triple* t, int order_count, hx_variablebindings_iter_sorting** order );
hx_optimizer_plan* hx_new_optimizer_join_plan ( hx_optimizer_plan_join_type type, hx_optimizer_plan* lhs, hx_optimizer_plan* rhs, int order_count, hx_variablebindings_iter_sorting** order, int leftjoin );
int hx_free_optimizer_plan ( hx_optimizer_plan* plan );

int hx_optimizer_plan_string ( hx_optimizer_plan* p, char** string );

#ifdef __cplusplus
}
#endif

#endif
