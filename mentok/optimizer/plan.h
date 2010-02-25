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
	HX_OPTIMIZER_PLAN_UNION	= 2,	// union of two or more hx_optimizer_plan*s
	HX_OPTIMIZER_PLAN_LAST	= HX_OPTIMIZER_PLAN_UNION
} hx_optimizer_plan_type;

const static char* hx_optimizer_plan_name[]	= { "access", "join", "union" };

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
	hx_container_t* plans;
} _hx_optimizer_union_plan;

typedef struct {
	hx_optimizer_plan_type type;
	int location;	// location for the plan to be executed (this integer should match up with some useful information in the execution context object, like a remote SPARQL endpoint URI).
	char* string;
	hx_container_t* order;
	union {
		_hx_optimizer_access_plan access;
		_hx_optimizer_join_plan join;
		_hx_optimizer_union_plan _union;
	} data;
} hx_optimizer_plan;

typedef struct {
	int64_t cost;
} hx_optimizer_plan_cost_t;

// a visitor function for all the sub-plans of a plan.
// return value dictates whether the children of a particular plan will be visited:
//	a return value of 0 indicates that the visitor function should be applied recursively to all sub-plans
//	any other return value indicates that no recursion on the current node should occur
typedef int hx_optimizer_plan_visitor( hx_execution_context* ctx, hx_optimizer_plan* plan, void* thunk );
typedef int hx_optimizer_plan_rewriter( hx_execution_context* ctx, hx_optimizer_plan* plan, hx_optimizer_plan** thunk );

hx_optimizer_plan_cost_t* hx_new_optimizer_plan_cost ( int64_t cost );
int64_t hx_optimizer_plan_cost_value ( hx_execution_context* ctx, hx_optimizer_plan_cost_t* c );
int hx_free_optimizer_plan_cost ( hx_optimizer_plan_cost_t* c );

hx_optimizer_plan* hx_copy_optimizer_plan ( hx_optimizer_plan* plan );
hx_optimizer_plan* hx_new_optimizer_access_plan ( hx_store* store, void* source, hx_triple* t, hx_container_t* order );
hx_optimizer_plan* hx_new_optimizer_join_plan ( hx_optimizer_plan_join_type type, hx_optimizer_plan* lhs, hx_optimizer_plan* rhs, hx_container_t* order, int leftjoin );
hx_optimizer_plan* hx_new_optimizer_union_plan ( hx_container_t* plans );
int hx_optimizer_plan_sorting ( hx_optimizer_plan* plan, hx_variablebindings_iter_sorting*** sorting );
int hx_free_optimizer_plan ( hx_optimizer_plan* plan );

int hx_optimizer_plan_service_calls ( hx_execution_context* ctx, hx_optimizer_plan* plan );
hx_optimizer_plan_cost_t* hx_optimizer_plan_cost ( hx_execution_context* ctx, hx_optimizer_plan* plan );

int hx_optimizer_plan_string ( hx_execution_context* ctx, hx_optimizer_plan* p, char** string );

hx_variablebindings_iter* hx_optimizer_plan_execute ( hx_execution_context* ctx, hx_optimizer_plan* p );

int hx_optimizer_plan_visit ( hx_execution_context* ctx, hx_optimizer_plan* plan, hx_optimizer_plan_visitor* v, void* thunk );
int hx_optimizer_plan_visit_postfix ( hx_execution_context* ctx, hx_optimizer_plan* plan, hx_optimizer_plan_visitor* v, void* thunk );

int hx_optimizer_plan_rewrite ( hx_execution_context* ctx, hx_optimizer_plan** plan, hx_optimizer_plan_rewriter* v );

int hx_optimizer_plan_cmp_service_calls (void* thunk, const void *a, const void *b);
int hx_optimizer_plan_debug( hx_execution_context* ctx, hx_optimizer_plan* plan );

#ifdef __cplusplus
}
#endif

#endif
