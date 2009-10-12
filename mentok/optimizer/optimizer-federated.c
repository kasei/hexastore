#include "mentok/optimizer/optimizer-federated.h"
#include "mentok/misc/idmap.h"


int _hx_optimizer_federated_plan_merge( hx_execution_context* ctx, hx_optimizer_plan* plan, void* thunk );
int _hx_optimizer_federated_plan_normalize( hx_execution_context* ctx, hx_optimizer_plan* plan, hx_optimizer_plan** thunk );

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
		hx_optimizer_plan* p	= hx_container_item( plans, i );
		hx_optimizer_plan* q	= p;
		
// 		fprintf( stderr, "*** Trying to rewrite plan: (%p)\n", p );
// 		hx_optimizer_plan_debug( ctx, p );
		
		hx_optimizer_plan_rewrite( ctx, &q, _hx_optimizer_federated_plan_normalize );
		if (q != p) {
			// the root of the QEP was re-written, so we need to stash it back in the container
// 			fprintf( stderr, "*** rewritten to: (%p)\n", q );
// 			hx_optimizer_plan_debug( ctx, q );
// 			fprintf( stderr, "---------------\n" );
			
			hx_container_set_item( plans, i, q );
		}
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
	}
	return 0;
}

int _hx_optimizer_federated_plan_normalize( hx_execution_context* ctx, hx_optimizer_plan* plan, hx_optimizer_plan** thunk ) {
	int i;
	if (plan->type != HX_OPTIMIZER_PLAN_JOIN) return 1;
	if (plan->data.join.leftjoin != 0) return 1;
	if (plan->location != 0) return 1;
	hx_optimizer_plan* lhs	= plan->data.join.lhs_plan;
	hx_optimizer_plan* rhs	= plan->data.join.rhs_plan;
	if (lhs->type == HX_OPTIMIZER_PLAN_UNION) {
		if (lhs->location != 0) return 1;
		hx_container_t* plans	= lhs->data._union.plans;
		int size	= hx_container_size( plans );
// 		fprintf( stderr, "LHS is a union with %d children\n", size );
		hx_container_t* newplans	= hx_new_container( 'P', size );
		for (i = 0; i < size; i++) {
// 			hx_container_t* c			= hx_new_container('P',2);
// 			hx_container_push_item( c, hx_container_item( plans, i ) );
// 			hx_container_push_item( c, hx_copy_optimizer_plan( rhs ) );
// 			hx_optimizer_plan* jplan	= hx_optimizer_optimize_plans( ctx, c );
			
			hx_optimizer_plan* p	= hx_container_item( plans, i );
			hx_optimizer_plan* q	= hx_copy_optimizer_plan( rhs );
			hx_container_t* order	= hx_new_container( 'O', 1 );
			hx_optimizer_plan* jplan	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, p, q, order, 0 );
			
			hx_container_push_item( newplans, jplan );
		}
		hx_optimizer_plan* new	= hx_new_optimizer_union_plan( newplans );
		*thunk	= new;
		return 0;
	} else if (rhs->type == HX_OPTIMIZER_PLAN_UNION) {
		if (rhs->location != 0) return 1;
		hx_container_t* plans	= rhs->data._union.plans;
		int size	= hx_container_size( plans );
// 		fprintf( stderr, "RHS is a union with %d children\n", size );
		hx_container_t* newplans	= hx_new_container( 'P', size );
		for (i = 0; i < size; i++) {
// 			hx_container_t* c			= hx_new_container('P',2);
// 			hx_container_push_item( c, hx_container_item( plans, i ) );
// 			hx_container_push_item( c, hx_copy_optimizer_plan( lhs ) );
// 			hx_optimizer_plan* jplan	= hx_optimizer_optimize_plans( ctx, c );

			hx_optimizer_plan* q	= hx_container_item( plans, i );
			hx_optimizer_plan* p	= hx_copy_optimizer_plan( lhs );
			hx_container_t* order	= hx_new_container( 'O', 1 );
			hx_optimizer_plan* jplan	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, p, q, order, 0 );

			hx_container_push_item( newplans, jplan );
		}
		hx_optimizer_plan* new	= hx_new_optimizer_union_plan( newplans );
		*thunk	= new;
		return 0;
	}
	return 1;
}
