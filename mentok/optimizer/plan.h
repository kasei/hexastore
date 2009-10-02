#ifndef _PLAN_ACCESS_H
#define _PLAN_ACCESS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include "mentok/mentok_types.h"
#include "mentok/mentok.h"
#include "mentok/engine/variablebindings_iter.h"
#include "mentok/engine/variablebindings_iter_sorting.h"
#include "mentok/rdf/triple.h"
#include "mentok/rdf/node.h"
#include "mentok/store/store.h"

typedef enum {
	HX_OPTIMIZER_PLAN_INDEX	= 0,	// access to triple pattern matching via hx_store thunks
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
	hx_store* store;
	void* source;
	hx_triple* triple;
} _hx_optimizer_access_plan;

typedef struct {
	hx_optimizer_plan_join_type join_type;
	void* lhs_plan;
	void* rhs_plan;
	int leftjoin;
} _hx_optimizer_join_plan;

typedef struct {
	hx_optimizer_plan_type type;
	char* string;
	hx_container_t* order;
	union {
		_hx_optimizer_access_plan access;
		_hx_optimizer_join_plan join;
	} data;
} hx_optimizer_plan;

typedef struct {
	int64_t cost;
} hx_optimizer_plan_cost_t;

hx_optimizer_plan_cost_t* hx_new_optimizer_plan_cost ( int64_t cost );
int64_t hx_optimizer_plan_cost_value ( hx_execution_context* ctx, hx_optimizer_plan_cost_t* c );
int hx_free_optimizer_plan_cost ( hx_optimizer_plan_cost_t* c );

hx_optimizer_plan* hx_copy_optimizer_plan ( hx_optimizer_plan* plan );
hx_optimizer_plan* hx_new_optimizer_access_plan ( hx_store* store, void* source, hx_triple* t, hx_container_t* order );
hx_optimizer_plan* hx_new_optimizer_join_plan ( hx_optimizer_plan_join_type type, hx_optimizer_plan* lhs, hx_optimizer_plan* rhs, hx_container_t* order, int leftjoin );
int hx_optimizer_plan_sorting ( hx_optimizer_plan* plan, hx_variablebindings_iter_sorting*** sorting );
int hx_free_optimizer_plan ( hx_optimizer_plan* plan );

hx_optimizer_plan_cost_t* hx_optimizer_plan_cost ( hx_execution_context* ctx, hx_optimizer_plan* plan );

int hx_optimizer_plan_string ( hx_optimizer_plan* p, char** string );

hx_variablebindings_iter* hx_optimizer_plan_execute ( hx_execution_context* ctx, hx_optimizer_plan* p );


#ifdef __cplusplus
}
#endif

#endif
