#include "mentok/optimizer/optimizer-federated.h"
#include "mentok/misc/idmap.h"

void qsort_r(void *base, size_t nel, size_t width, void *thunk, int (*compar)(void *, const void *, const void *)) {}

int _hx_optimizer_federated_plan_merge( hx_execution_context* ctx, hx_optimizer_plan* plan, void* thunk );

hx_container_t* hx_optimizer_federated_access_plans ( hx_execution_context* ctx, hx_triple* t ) {
// 	fprintf( stderr, "*** FEDERATED ACCESS PLANS\n" );
	hx_container_t* sources	= ctx->remote_sources;
	int ssize				= hx_container_size( sources );
	hx_container_t* plans	= hx_optimizer_access_plans( ctx, t );
	int psize				= hx_container_size( plans );
	
	int i,j;
	hx_container_t* fedplans	= hx_new_container( 'A', psize );
	for (j = 0; j < psize; j++) {
		hx_optimizer_plan* p	= hx_container_item( plans, j );
		hx_container_t* up		= hx_new_container( 'U', ssize );
		for (i = 1; i < ssize; i++) {
			hx_optimizer_plan* c	= hx_copy_optimizer_plan( p );
			c->location				= i;
			hx_container_push_item( up, c );
		}
		hx_container_push_item( fedplans, hx_new_optimizer_union_plan(up) );
		hx_free_optimizer_plan( p );
	}
	hx_free_container(plans);
	
	return fedplans;
}

hx_container_t* hx_optimizer_federated_join_plans ( hx_execution_context* ctx, hx_container_t* lhs, hx_container_t* rhs, int leftjoin ) {
// 	fprintf( stderr, "*** FEDERATED JOIN PLANS\n" );
	int i;
	hx_container_t* plans	= hx_optimizer_join_plans( ctx, lhs, rhs, leftjoin );
	int size	= hx_container_size( plans );
	for (i = 0; i < size; i++) {
		hx_optimizer_plan_visit_postfix( ctx, hx_container_item( plans, i ), _hx_optimizer_federated_plan_merge, NULL );
	}
	
	return plans;
}

int _hx_optimizer_federated_plan_merge( hx_execution_context* ctx, hx_optimizer_plan* plan, void* thunk ) {
	if (plan->type == HX_OPTIMIZER_PLAN_JOIN) {
		hx_optimizer_plan* lhs	= plan->data.join.lhs_plan;
		hx_optimizer_plan* rhs	= plan->data.join.rhs_plan;
		if (lhs->location == rhs->location) {
			plan->location	= lhs->location;
			lhs->location	= 0;
			rhs->location	= 0;
		}
	} else if (plan->type == HX_OPTIMIZER_PLAN_UNION) {
		// now that SERVICE joins have been merged, we should re-order unions to prefer fewer SERVICE calls
		hx_container_t* plans	= plan->data._union.plans;
		qsort_r( plans->items, hx_container_size(plans), sizeof(void*), ctx, hx_optimizer_plan_cmp_service_calls );
	}
	return 0;
}
