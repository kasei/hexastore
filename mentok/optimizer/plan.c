#include "mentok/optimizer/plan.h"
#include "mentok/algebra/bgp.h"
#include "mentok/engine/nestedloopjoin.h"
#include "mentok/engine/hashjoin.h"
#include "mentok/engine/mergejoin.h"

int _hx_optimizer_plan_mergeable_sorting ( hx_execution_context* ctx, hx_optimizer_plan* p );

hx_optimizer_plan_cost_t* hx_new_optimizer_plan_cost ( int64_t cost ) {
	hx_optimizer_plan_cost_t* c	= (hx_optimizer_plan_cost_t*) calloc( 1, sizeof(hx_optimizer_plan_cost_t) );
	c->cost	= cost;
	return c;
}

int hx_free_optimizer_plan_cost ( hx_optimizer_plan_cost_t* c ) {
	free(c);
	return 0;
}

int64_t hx_optimizer_plan_cost_value ( hx_execution_context* ctx, hx_optimizer_plan_cost_t* c ) {
	return c->cost;
}

int hx_optimizer_plan_service_calls ( hx_execution_context* ctx, hx_optimizer_plan* p ) {
	if (p->location > 0) {
		if (0) {
			char* string;
			hx_optimizer_plan_string( ctx, p, &string );
			fprintf( stderr, "counting service call: %s\n", string );
			free(string);
		}
		return 1;
	} else {
		int count	= 0;
		if (p->type == HX_OPTIMIZER_PLAN_INDEX) {
		} else if (p->type == HX_OPTIMIZER_PLAN_JOIN) {
			count	+= hx_optimizer_plan_service_calls( ctx, p->data.join.lhs_plan );
			count	+= hx_optimizer_plan_service_calls( ctx, p->data.join.rhs_plan );
		} else if (p->type == HX_OPTIMIZER_PLAN_UNION) {
			int i;
			hx_container_t* src		= p->data._union.plans;
			int size				= hx_container_size(src);
			hx_container_t* plans	= hx_new_container( 'P', size );
			for (i = 0; i < size; i++) {
				count	+= hx_optimizer_plan_service_calls( ctx, hx_container_item(src,i) );
			}
		} else {
			fprintf( stderr, "*** unrecognized plan type in hx_optimizer_plan_service_calls\n" );
			return -1;
		}
		return count;
	}
}

hx_optimizer_plan* hx_new_optimizer_access_plan ( hx_store* store, void* source, hx_triple* t, hx_container_t* order ) {
	int order_count				= hx_container_size( order );
	hx_optimizer_plan* plan		= (hx_optimizer_plan*) calloc( 1, sizeof(hx_optimizer_plan) );
	plan->type					= HX_OPTIMIZER_PLAN_INDEX;
	plan->data.access.triple	= hx_copy_triple(t);
	plan->data.access.source	= source;
	plan->data.access.store		= store;
	plan->order					= hx_new_container( 'S', order_count );
	plan->string				= NULL;
	plan->location				= 0;
	int i;
	for (i = 0; i < order_count; i++) {
		hx_variablebindings_iter_sorting* s	= hx_container_item( order, i );
		hx_container_push_item( plan->order, hx_copy_variablebindings_iter_sorting( s ) );
	}
	return plan;
}

hx_optimizer_plan* hx_new_optimizer_join_plan ( hx_optimizer_plan_join_type type, hx_optimizer_plan* lhs, hx_optimizer_plan* rhs, hx_container_t* order, int leftjoin ) {
	int order_count				= hx_container_size( order );
	hx_optimizer_plan* plan		= (hx_optimizer_plan*) calloc( 1, sizeof(hx_optimizer_plan) );
	plan->type					= HX_OPTIMIZER_PLAN_JOIN;
	plan->data.join.join_type	= type;
	plan->data.join.lhs_plan	= hx_copy_optimizer_plan(lhs);
	plan->data.join.rhs_plan	= hx_copy_optimizer_plan(rhs);
	plan->order					= hx_new_container( 'S', order_count );
	plan->string				= NULL;
	plan->location				= 0;
	int i;
	for (i = 0; i < order_count; i++) {
		hx_variablebindings_iter_sorting* s	= hx_container_item( order, i );
		hx_container_push_item( plan->order, hx_copy_variablebindings_iter_sorting( s ) );
	}
	return plan;
}

hx_optimizer_plan* hx_new_optimizer_union_plan ( hx_container_t* plans ) {
	hx_optimizer_plan* plan		= (hx_optimizer_plan*) calloc( 1, sizeof(hx_optimizer_plan) );
	plan->type					= HX_OPTIMIZER_PLAN_UNION;
	plan->data._union.plans		= plans;
	plan->order					= hx_new_container( 'S', 1 );	// unions have no predictable ordering (might have different variables, or execution might be parallelized)
	plan->string				= NULL;
	plan->location				= 0;
	
	if (0) {
		int i;
		int size	= hx_container_size( plans );
		fprintf( stderr, "*** Creating new UNION plan with %d subplans...\n", size );
		for (i = 0; i < size; i++) {
			hx_optimizer_plan* p	= hx_container_item( plans, i );
			char* string;
			hx_optimizer_plan_string( NULL, p, &string );
			fprintf( stderr, "- (%d) %s\n", i, string );
			free(string);
		}
		if (size > 3) {
			fprintf( stderr, "breakpoint\n" );
		}
	}
	
	return plan;
}

hx_optimizer_plan* hx_copy_optimizer_plan ( hx_optimizer_plan* p ) {
	hx_optimizer_plan* copy;
	if (p->type == HX_OPTIMIZER_PLAN_INDEX) {
		copy	= hx_new_optimizer_access_plan( p->data.access.store, p->data.access.source, p->data.access.triple, p->order );
	} else if (p->type == HX_OPTIMIZER_PLAN_JOIN) {
		copy	= hx_new_optimizer_join_plan( p->data.join.join_type, p->data.join.lhs_plan, p->data.join.rhs_plan, p->order, p->data.join.leftjoin );
	} else if (p->type == HX_OPTIMIZER_PLAN_UNION) {
		int i;
		hx_container_t* src		= p->data._union.plans;
		int size				= hx_container_size(src);
		hx_container_t* plans	= hx_new_container( 'P', size );
		for (i = 0; i < size; i++) {
			hx_container_push_item( plans, hx_copy_optimizer_plan(hx_container_item(src,i)) );
		}
		copy	= hx_new_optimizer_union_plan( plans );
	} else {
		fprintf( stderr, "*** unrecognized plan type in hx_copy_optimizer_plan\n" );
		return NULL;
	}
	copy->location	= p->location;
	return copy;
}

int hx_free_optimizer_plan ( hx_optimizer_plan* p ) {
	int i;
	int size	= hx_container_size( p->order );
	for (i = 0; i < size; i++) {
		hx_variablebindings_iter_sorting* s	= hx_container_item( p->order, i );
		hx_free_variablebindings_iter_sorting( s );
	}
	hx_free_container( p->order );
	
	if (p->string) {
		free(p->string);
	}
	
	if (p->type == HX_OPTIMIZER_PLAN_INDEX) {
		hx_free_triple( p->data.access.triple );
	} else if (p->type == HX_OPTIMIZER_PLAN_UNION) {
		hx_container_t* plans	= p->data._union.plans;
		int size	= hx_container_size(plans);
		for (i = 0; i < size; i++) {
			hx_optimizer_plan* subplan	= hx_container_item(plans,i);
			hx_free_optimizer_plan(subplan);
		}
		hx_free_container(plans);
	} else if (p->type == HX_OPTIMIZER_PLAN_JOIN) {
		hx_free_optimizer_plan( p->data.join.lhs_plan );
		hx_free_optimizer_plan( p->data.join.rhs_plan );
	} else {
		fprintf( stderr, "*** unrecognized plan type in hx_free_optimizer_plan\n" );
		return 1;
	}
	
	free( p );
	return 0;
}

int hx_optimizer_plan_string ( hx_execution_context* ctx, hx_optimizer_plan* p, char** string ) {
	if (p->string) {
		*string	= calloc( strlen(p->string) + 1, sizeof(char) );
		strcpy( *string, p->string );
		return 0;
	}
	
	int len		= 0;
	char* loc	= "";
	if (p->location > 0) {
		if (ctx == NULL) {
			len		= 7;
			loc		= (char*) calloc( len, sizeof(char) );
			snprintf( loc, len, "[%d]", p->location );
		} else {
			hx_remote_service* s	= hx_container_item( ctx->remote_sources, p->location );
			char* service	= hx_remote_service_name( s );
			len		= strlen(service) + 3;
			loc		= (char*) calloc( len, sizeof(char) );
			snprintf( loc, len, "[%s]", service );
		}
	}
	
	if (p->type == HX_OPTIMIZER_PLAN_INDEX) {
		char *tstring;
		hx_triple* t	= p->data.access.triple;
		
		hx_bgp* b		= hx_new_bgp1( hx_copy_triple(t) );
		hx_bgp_string( b, &tstring );
		hx_free_bgp(b);
		
		hx_store* store	= p->data.access.store;
		void* i			= p->data.access.source;
		char* iname		= hx_store_ordering_name( store, i );
		
		len	+= strlen(iname) + strlen(tstring) + 3;
		*string	= (char*) calloc( len, sizeof(char) );
		if (*string == NULL) {
			fprintf( stderr, "*** failed to allocate memory in hx_optimizer_plan_string\n" );
			return 1;
		}
		
		snprintf( *string, len, "%s%s(%s)", iname, loc, tstring );
		
		free(iname);
		free(tstring);
	} else if (p->type == HX_OPTIMIZER_PLAN_JOIN) {
		char *lhs_string, *rhs_string;
		const char* jname	= HX_OPTIMIZER_PLAN_JOIN_NAMES[ p->data.join.join_type ];
		
		hx_optimizer_plan* lhs	= p->data.join.lhs_plan;
		hx_optimizer_plan* rhs	= p->data.join.rhs_plan;
		hx_optimizer_plan_string( ctx, lhs, &lhs_string );
		hx_optimizer_plan_string( ctx, rhs, &rhs_string );
		
		len	+= strlen(jname) + strlen(lhs_string) + strlen(rhs_string) + 5;
		*string	= (char*) calloc( len, sizeof(char) );
		if (*string == NULL) {
			fprintf( stderr, "*** failed to allocate memory in hx_optimizer_plan_string\n" );
			return 1;
		}
		
		snprintf( *string, len, "%s%s(%s, %s)", jname, loc, lhs_string, rhs_string );
		free(lhs_string);
		free(rhs_string);
	} else if (p->type == HX_OPTIMIZER_PLAN_UNION) {
		int i;
		int length	= 0;
		int size	= hx_container_size( p->data._union.plans );
		char** strings	= (char**) calloc( size, sizeof(char*) );
		for (i = 0; i < size; i++) {
			hx_optimizer_plan* subplan	= hx_container_item(p->data._union.plans,i);
			hx_optimizer_plan_string( ctx, subplan, &(strings[i]) );
			length	+= strlen(strings[i]) + 2;
		}
		
		char* subplans	= (char*) calloc( length, sizeof(char) );
		for (i = 0; i < size; i++) {
			if (i > 0) {
				strncat(subplans, ", ", 2);
			}
			strcat(subplans, strings[i]);
		}
		
		len	+= length + 8;
		*string	= (char*) calloc( len, sizeof(char) );
		sprintf( *string, "union%s(%s)", loc, subplans );
		
		free(subplans);
		for (i = 0; i < size; i++) {
			free(strings[i]);
		}
		free(strings);
	} else {
		*string	= NULL;
		fprintf( stderr, "*** unrecognized plan type in hx_optimizer_plan_string\n" );
		return 1;
	}
	
	p->string	= calloc( strlen(*string) + 1, sizeof(char) );
	strcpy( p->string, *string );
	
	if (strlen(loc) > 0) {
		free(loc);
	}
	
	return 0;
}

int hx_optimizer_plan_sorting ( hx_optimizer_plan* plan, hx_variablebindings_iter_sorting*** sorting ) {
	int count	= hx_container_size( plan->order );
	*sorting	= (hx_variablebindings_iter_sorting**) calloc( count, sizeof(hx_variablebindings_iter_sorting*) );
	int i;
	for (i = 0; i < count; i++) {
		hx_variablebindings_iter_sorting* s	= hx_container_item( plan->order, i );
		(*sorting)[i]	= hx_copy_variablebindings_iter_sorting( s );
	}
	return count;
}

int64_t _hx_optimizer_plan_cost ( hx_execution_context* ctx, hx_optimizer_plan* p, int64_t* accumulator, int level ) {
	hx_model* hx	= ctx->hx;
	
	if (p->type == HX_OPTIMIZER_PLAN_INDEX) {
		hx_triple* t	= p->data.access.triple;
		uint64_t count;
		if (hx) {
			count	= hx_model_count_statements( hx, t->subject, t->predicate, t->object );
		} else {
			count	= 1;
		}
		return (int64_t) count;
	} else if (p->type == HX_OPTIMIZER_PLAN_UNION) {
		int i;
		uint64_t cost	= 0;
		hx_container_t* plans	= p->data._union.plans;
		int size	= hx_container_size( plans );
		for (i = 0; i < size; i++) {
			// XXX we're estimating the cost of a union as a sum(), but if unions are parallelized, this might be better as max()
			hx_optimizer_plan* subplan	= hx_container_item( plans, i );
			cost	+= _hx_optimizer_plan_cost( ctx, subplan, accumulator, level+1 );
		}
		return (int64_t) cost;
	} else if (p->type == HX_OPTIMIZER_PLAN_JOIN) {
		int64_t lhsc	= _hx_optimizer_plan_cost( ctx, p->data.join.lhs_plan, accumulator, level+1 );
		int64_t rhsc	= _hx_optimizer_plan_cost( ctx, p->data.join.rhs_plan, accumulator, level+1 );
		int64_t cost	= (lhsc * rhsc);
		if (p->data.join.join_type == HX_OPTIMIZER_PLAN_NESTEDLOOPJOIN) {
			cost	+= ctx->nestedloopjoin_penalty;
		} else if (p->data.join.join_type == HX_OPTIMIZER_PLAN_HASHJOIN) {
			cost	+= ctx->hashjoin_penalty;
		} else if (p->data.join.join_type == HX_OPTIMIZER_PLAN_MERGEJOIN) {
			if (_hx_optimizer_plan_mergeable_sorting( ctx, p )) {
				// if the iterators are sorted appropriately for a merge-join
			} else {
				cost	+= ctx->unsorted_mergejoin_penalty;
			}
		} else {
			fprintf( stderr, "*** unrecognized plan join type in hx_optimizer_plan_cost\n" );
		}
		
		// The accumulator value is added to the total plan cost.
		// By adding to it here, we increase the total cost of plans that have
		// large expected intermediate results.
		if (p->location == 0) {
			// execution is local, so take into account intermediate values
			*accumulator	+= (cost*level);
		} else {
			// exectuion is remote, so take into account latency overhead
			*accumulator	+= ctx->remote_latency_cost;
		}
		return cost;
	} else {
		fprintf( stderr, "*** unrecognized plan type in hx_free_optimizer_plan\n" );
		return -1;
	}
}

hx_optimizer_plan_cost_t* hx_optimizer_plan_cost ( hx_execution_context* ctx, hx_optimizer_plan* p ) {
	int64_t accumulator	= 0;
	int64_t cost	= _hx_optimizer_plan_cost( ctx, p, &accumulator, 1 );
	return hx_new_optimizer_plan_cost( cost + accumulator );
}

int _hx_optimizer_plan_mergeable_sorting ( hx_execution_context* ctx, hx_optimizer_plan* p ) {
	hx_variablebindings_iter_sorting **lhs_sorting, **rhs_sorting;
	int lhs_count	= hx_optimizer_plan_sorting( p->data.join.lhs_plan, &lhs_sorting );
	int rhs_count	= hx_optimizer_plan_sorting( p->data.join.rhs_plan, &rhs_sorting );
	int i;
	
	if (lhs_count == 0 || rhs_count == 0) {
		return 0;
	}
	
	char *lhs_sort_string, *rhs_sort_string;
	hx_variablebindings_iter_sorting_string( lhs_sorting[0], &lhs_sort_string );
	hx_variablebindings_iter_sorting_string( rhs_sorting[0], &rhs_sort_string );
	
// 	fprintf( stderr, "Are these two sorting orders mergeable?\n%s\n%s\n???\n\n", lhs_sort_string, rhs_sort_string );
	
	int mergeable	= 0;
	if (strcmp(lhs_sort_string, rhs_sort_string) == 0) {
		mergeable	= 1;
	}
	
// 	for (i = 0; i < lhs_count; i++) {
// 		char* sort_string;
// 		hx_variablebindings_iter_sorting_string( lhs_sorting[i], &sort_string );
// 		fprintf( stderr, "LHS sorting: %s\n", sort_string );
// 		free(sort_string);
// 	}
	
	for (i = 0; i < lhs_count; i++) hx_free_variablebindings_iter_sorting( lhs_sorting[i] );
	for (i = 0; i < rhs_count; i++) hx_free_variablebindings_iter_sorting( rhs_sorting[i] );
	free(lhs_sorting);
	free(rhs_sorting);
	free(lhs_sort_string);
	free(rhs_sort_string);
	
	return mergeable;
}

hx_variablebindings_iter* hx_optimizer_plan_execute ( hx_execution_context* ctx, hx_optimizer_plan* plan ) {
	hx_variablebindings_iter* iter	= NULL;
	
	if (plan->type == HX_OPTIMIZER_PLAN_INDEX) {
		hx_triple* t		= plan->data.access.triple;
		iter	= hx_store_get_statements_with_index( plan->data.access.store, t, plan->data.access.source );
fprintf( stderr, "    turning index plan into iterator\n" );
	} else if (plan->type == HX_OPTIMIZER_PLAN_JOIN) {
		hx_variablebindings_iter* lhs	= hx_optimizer_plan_execute( ctx, plan->data.join.lhs_plan );
		hx_variablebindings_iter* rhs	= hx_optimizer_plan_execute( ctx, plan->data.join.rhs_plan );
		if (plan->data.join.join_type == HX_OPTIMIZER_PLAN_NESTEDLOOPJOIN) {
			iter	= hx_new_nestedloopjoin_iter2( lhs, rhs, plan->data.join.leftjoin );
		} else if (plan->data.join.join_type == HX_OPTIMIZER_PLAN_HASHJOIN) {
			iter	= hx_new_hashjoin_iter2( lhs, rhs, plan->data.join.leftjoin );
		} else if (plan->data.join.join_type == HX_OPTIMIZER_PLAN_MERGEJOIN) {
			iter	= hx_new_mergejoin_iter( lhs, rhs );
		} else {
			fprintf( stderr, "*** unrecognized plan join type in hx_optimizer_plan_execute\n" );
		}
fprintf( stderr, "  turning join plan into iterator\n" );
	} else if (plan->type == HX_OPTIMIZER_PLAN_UNION) {
		int i;
		hx_container_t* plans	= plan->data._union.plans;
		int size	= hx_container_size( plans );
		hx_container_t* iters	= hx_new_container( 'I', size );
		for (i = 0; i < size; i++) {
			hx_variablebindings_iter* _iter	= hx_optimizer_plan_execute( ctx, hx_container_item(plans, i) );
			hx_container_push_item( iters, _iter );
		}
		iter	= hx_new_union_iter( ctx, iters );
fprintf( stderr, "turning union plan into iterator (%p)\n", iter );
fprintf( stderr, "- vtable: %p\n", iter->vtable );
	} else {
		fprintf( stderr, "*** unrecognized plan type in hx_optimizer_plan_execute\n" );
	}
	
	return iter;
}

int hx_optimizer_plan_visit ( hx_execution_context* ctx, hx_optimizer_plan* plan, hx_optimizer_plan_visitor* v, void* thunk ) {
	int r	= v( ctx, plan, thunk );
	if (r == 0) {
		if (plan->type == HX_OPTIMIZER_PLAN_UNION) {
			int i;
			hx_container_t* plans	= plan->data._union.plans;
			int size	= hx_container_size( plans );
			for (i = 0; i < size; i++) {
				hx_optimizer_plan_visit( ctx, hx_container_item(plans,i), v, thunk );
			}
		} else if (plan->type == HX_OPTIMIZER_PLAN_JOIN) {
			hx_optimizer_plan_visit( ctx, plan->data.join.lhs_plan, v, thunk );
			hx_optimizer_plan_visit( ctx, plan->data.join.rhs_plan, v, thunk );
		}
	}
	return 0;
}

int hx_optimizer_plan_visit_postfix ( hx_execution_context* ctx, hx_optimizer_plan* plan, hx_optimizer_plan_visitor* v, void* thunk ) {
	if (plan->type == HX_OPTIMIZER_PLAN_UNION) {
		int i;
		hx_container_t* plans	= plan->data._union.plans;
		int size	= hx_container_size( plans );
		for (i = 0; i < size; i++) {
			hx_optimizer_plan_visit( ctx, hx_container_item(plans,i), v, thunk );
		}
	} else if (plan->type == HX_OPTIMIZER_PLAN_JOIN) {
		hx_optimizer_plan_visit( ctx, plan->data.join.lhs_plan, v, thunk );
		hx_optimizer_plan_visit( ctx, plan->data.join.rhs_plan, v, thunk );
	}
	return v( ctx, plan, thunk );
}

int hx_optimizer_plan_rewrite ( hx_execution_context* ctx, hx_optimizer_plan** plan, hx_optimizer_plan_rewriter* v ) {
	hx_optimizer_plan* new	= NULL;
	int r	= v( ctx, *plan, &new );
	if (new) {
		*plan	= new;
	}
	
	if (r == 0) {
		if ((*plan)->type == HX_OPTIMIZER_PLAN_UNION) {
			int i;
			hx_container_t* plans	= (*plan)->data._union.plans;
			int size	= hx_container_size( plans );
			for (i = 0; i < size; i++) {
				hx_optimizer_plan* p	= hx_container_item(plans,i);
				hx_optimizer_plan* q	= p;
				hx_optimizer_plan_rewrite( ctx, &q, v );
				if (q != p) {
					hx_container_set_item( plans, i, q );
				}
			}
		} else if ((*plan)->type == HX_OPTIMIZER_PLAN_JOIN) {
			hx_optimizer_plan *p, *q;
			
			p = q	= (*plan)->data.join.lhs_plan;
			hx_optimizer_plan_rewrite( ctx, &q, v );
			if (q != p) {
				(*plan)->data.join.lhs_plan	= q;
			}
			
			p = q	= (*plan)->data.join.rhs_plan;
			hx_optimizer_plan_rewrite( ctx, &q, v );
			if (q != p) {
				(*plan)->data.join.rhs_plan	= q;
			}
		}
	}
	
	return 0;
}

int hx_optimizer_plan_debug( hx_execution_context* ctx, hx_optimizer_plan* plan ) {
	char* string;
	hx_optimizer_plan_string( ctx, plan, &string );
	fprintf( stderr, "%s\n", string );
	free(string);
	return 0;
}

int hx_optimizer_plan_cmp_service_calls (void* thunk, const void *a, const void *b) {
	hx_execution_context* ctx	= (hx_execution_context*) thunk;
	hx_optimizer_plan* plana	= *((hx_optimizer_plan**) a);
	hx_optimizer_plan* planb	= *((hx_optimizer_plan**) b);
	
	int ca	= hx_optimizer_plan_service_calls(ctx, plana);
	int cb	= hx_optimizer_plan_service_calls(ctx, planb);
	return (ca - cb);
}
