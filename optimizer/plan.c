#include "optimizer/plan.h"
#include "algebra/bgp.h"

int _hx_optimizer_plan_mergeable_sorting ( hx_execution_context* ctx, hx_optimizer_plan* p );

hx_optimizer_plan* hx_new_optimizer_access_plan ( hx_index* source, hx_triple* t, int order_count, hx_variablebindings_iter_sorting** order ) {
	hx_optimizer_plan* plan	= (hx_optimizer_plan*) calloc( 1, sizeof(hx_optimizer_plan) );
	plan->type			= HX_OPTIMIZER_PLAN_INDEX;
	plan->triple		= hx_copy_triple(t);
	plan->source		= source;
	plan->order_count	= order_count;
	plan->order			= (hx_variablebindings_iter_sorting**) calloc( order_count, sizeof(hx_variablebindings_iter_sorting*) );
	plan->string		= NULL;
	int i;
	for (i = 0; i < order_count; i++) {
		plan->order[i]	= hx_copy_variablebindings_iter_sorting( order[i] );
	}
	return plan;
}

hx_optimizer_plan* hx_new_optimizer_join_plan ( hx_optimizer_plan_join_type type, hx_optimizer_plan* lhs, hx_optimizer_plan* rhs, int order_count, hx_variablebindings_iter_sorting** order, int leftjoin ) {
	hx_optimizer_plan* plan	= (hx_optimizer_plan*) calloc( 1, sizeof(hx_optimizer_plan) );
	plan->type			= HX_OPTIMIZER_PLAN_JOIN;
	plan->join_type		= type;
	plan->lhs_plan		= hx_copy_optimizer_plan(lhs);
	plan->rhs_plan		= hx_copy_optimizer_plan(rhs);
	plan->order_count	= order_count;
	plan->order			= (hx_variablebindings_iter_sorting**) calloc( order_count, sizeof(hx_variablebindings_iter_sorting*) );
	plan->string		= NULL;
	int i;
	for (i = 0; i < order_count; i++) {
		plan->order[i]	= hx_copy_variablebindings_iter_sorting( order[i] );
	}
	return plan;
}

hx_optimizer_plan* hx_copy_optimizer_plan ( hx_optimizer_plan* p ) {
	if (p->type == HX_OPTIMIZER_PLAN_INDEX) {
		return hx_new_optimizer_access_plan( p->source, p->triple, p->order_count, p->order );
	} else if (p->type == HX_OPTIMIZER_PLAN_JOIN) {
		return hx_new_optimizer_join_plan( p->join_type, p->lhs_plan, p->rhs_plan, p->order_count, p->order, p->leftjoin );
	} else {
		fprintf( stderr, "*** unrecognized plan type in hx_copy_optimizer_plan\n" );
		return NULL;
	}
}

int hx_free_optimizer_plan ( hx_optimizer_plan* p ) {
	int i;
	int size	= p->order_count;
	for (i = 0; i < size; i++) {
		hx_variablebindings_iter_sorting* s	= p->order[i];
		hx_free_variablebindings_iter_sorting( s );
	}
	free( p->order );
	
	if (p->string) {
		free(p->string);
	}
	
	if (p->type == HX_OPTIMIZER_PLAN_INDEX) {
		hx_free_triple( p->triple );
	} else if (p->type == HX_OPTIMIZER_PLAN_JOIN) {
		hx_free_optimizer_plan( p->lhs_plan );
		hx_free_optimizer_plan( p->rhs_plan );
	} else {
		fprintf( stderr, "*** unrecognized plan type in hx_free_optimizer_plan\n" );
		return 1;
	}
	
	free( p );
	return 0;
}

int hx_optimizer_plan_string ( hx_optimizer_plan* p, char** string ) {
	if (p->string) {
		*string	= calloc( strlen(p->string) + 1, sizeof(char) );
		strcpy( *string, p->string );
		return 0;
	}
	
	if (p->type == HX_OPTIMIZER_PLAN_INDEX) {
		char *tstring;
		hx_triple* t	= p->triple;
		
		hx_bgp* b		= hx_new_bgp1( hx_copy_triple(t) );
		hx_bgp_string( b, &tstring );
		hx_free_bgp(b);
		
		hx_index* i	= p->source;
		char* iname	= hx_index_name( i );
		
		int len	= strlen(iname) + strlen(tstring) + 3;
		*string	= (char*) calloc( len, sizeof(char) );
		if (*string == NULL) {
			fprintf( stderr, "*** failed to allocate memory in hx_optimizer_plan_string\n" );
			return 1;
		}
		
		snprintf( *string, len, "%s(%s)", iname, tstring );
		
		free(iname);
		free(tstring);
	} else if (p->type == HX_OPTIMIZER_PLAN_JOIN) {
		char *lhs_string, *rhs_string;
		const char* jname	= HX_OPTIMIZER_PLAN_JOIN_NAMES[ p->join_type ];
		
		hx_optimizer_plan* lhs	= p->lhs_plan;
		hx_optimizer_plan* rhs	= p->rhs_plan;
		hx_optimizer_plan_string( lhs, &lhs_string );
		hx_optimizer_plan_string( rhs, &rhs_string );
		
		int len	= strlen(jname) + strlen(lhs_string) + strlen(rhs_string) + 5;
		*string	= (char*) calloc( len, sizeof(char) );
		if (*string == NULL) {
			fprintf( stderr, "*** failed to allocate memory in hx_optimizer_plan_string\n" );
			return 1;
		}
		
		snprintf( *string, len, "%s(%s, %s)", jname, lhs_string, rhs_string );
		free(lhs_string);
		free(rhs_string);
	} else {
		*string	= NULL;
		fprintf( stderr, "*** unrecognized plan type in hx_optimizer_plan_string\n" );
		return 1;
	}
	
	p->string	= calloc( strlen(*string) + 1, sizeof(char) );
	strcpy( p->string, *string );
	
	return 0;
}

int hx_optimizer_plan_sorting ( hx_optimizer_plan* plan, hx_variablebindings_iter_sorting*** sorting ) {
	int count	= plan->order_count;
	*sorting	= (hx_variablebindings_iter_sorting**) calloc( count, sizeof(hx_variablebindings_iter_sorting*) );
	int i;
	for (i = 0; i < count; i++) {
		(*sorting)[i]	= hx_copy_variablebindings_iter_sorting( plan->order[i] );
	}
	return count;
}

int64_t _hx_optimizer_plan_cost ( hx_execution_context* ctx, hx_optimizer_plan* p, int* accumulator, int level ) {
	hx_hexastore* hx	= ctx->hx;
	
	if (p->type == HX_OPTIMIZER_PLAN_INDEX) {
		hx_triple* t	= p->triple;
		uint64_t count;
		if (hx) {
			count	= hx_count_statements( hx, t->subject, t->predicate, t->object );
		} else {
			count	= 1;
		}
		return (int64_t) count;
	} else if (p->type == HX_OPTIMIZER_PLAN_JOIN) {
		int64_t lhsc	= _hx_optimizer_plan_cost( ctx, p->lhs_plan, accumulator, level+1 );
		int64_t rhsc	= _hx_optimizer_plan_cost( ctx, p->rhs_plan, accumulator, level+1 );
		int64_t cost	= (lhsc * rhsc);
		if (p->join_type == HX_OPTIMIZER_PLAN_NESTEDLOOPJOIN) {
			cost	+= ctx->nestedloopjoin_penalty;
		} else if (p->join_type == HX_OPTIMIZER_PLAN_HASHJOIN) {
			cost	+= ctx->hashjoin_penalty;
		} else if (p->join_type == HX_OPTIMIZER_PLAN_MERGEJOIN) {
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
		*accumulator	+= (cost*level);
		return cost;
	} else {
		fprintf( stderr, "*** unrecognized plan type in hx_free_optimizer_plan\n" );
		return -1;
	}
}

int64_t hx_optimizer_plan_cost ( hx_execution_context* ctx, hx_optimizer_plan* p ) {
	int64_t accumulator	= 0;
	int64_t cost	= _hx_optimizer_plan_cost( ctx, p, &accumulator, 1 );
	return (cost + accumulator);
}

int _hx_optimizer_plan_mergeable_sorting ( hx_execution_context* ctx, hx_optimizer_plan* p ) {
	hx_variablebindings_iter_sorting **lhs_sorting, **rhs_sorting;
	int lhs_count	= hx_optimizer_plan_sorting( p->lhs_plan, &lhs_sorting );
	int rhs_count	= hx_optimizer_plan_sorting( p->rhs_plan, &rhs_sorting );
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
