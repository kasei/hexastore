#include "optimizer/plan.h"
#include "algebra/bgp.h"

hx_optimizer_plan* hx_new_optimizer_access_plan ( hx_index* source, hx_triple* t, int order_count, hx_variablebindings_iter_sorting** order ) {
	hx_optimizer_plan* plan	= (hx_optimizer_plan*) calloc( 1, sizeof(hx_optimizer_plan) );
	plan->type			= HX_OPTIMIZER_PLAN_INDEX;
	plan->triple		= hx_copy_triple(t);
	plan->source		= source;
	plan->order_count	= order_count;
	plan->order			= (hx_variablebindings_iter_sorting**) calloc( order_count, sizeof(hx_variablebindings_iter_sorting*) );
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
		return 1;
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
	return 0;
}
