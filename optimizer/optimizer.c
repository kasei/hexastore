#include "optimizer/optimizer.h"
#include "optimizer/plan.h"

// - accessPlans (get vb iter construct info from a triple pattern, which index to use?)
hx_container_t* hx_optimizer_access_plans ( hx_execution_context* ctx, hx_triple* t ) {
	hx_hexastore* hx		= ctx->hx;
	hx_container_t* indexes	= hx_get_indexes( hx );
	int size				= hx_container_size(indexes);
	
	int i,j;
// 	char* string;
// 	hx_triple_string( t, &string );
// 	fprintf( stderr, "triple: %s\n", string );
// 	free(string);
	
	int bound	= hx_triple_bound_count(t);
// 	fprintf( stderr, "triple has %d bound terms\n", bound );
	
	int repeated_variable	= 0;
	int bound_nodes[3]	= {1,1,1};
	if (bound == 3) {
	} else {
		for (i = 0; i < 3; i++) {
			hx_node* n	= hx_triple_node( t, i );
			if (hx_node_is_variable(n)) {
// 				fprintf( stderr, "- %s is unbound\n", HX_POSITION_NAMES[i] );
				bound_nodes[i]	= 0;
				// scan through the other node positions, checking for repeated variables
				for (j = i+1; j < 3; j++) {
					hx_node* m	= hx_triple_node( t, j );
					if (hx_node_is_variable(m)) {
						if (hx_node_iv(n) == hx_node_iv(m)) {
							repeated_variable	= hx_node_iv(n);
						}
					}
				}
			}
		}
	}
	
// 	if (repeated_variable) {
// 		fprintf( stderr, "triple has a shared variable\n" );
// 	}
// 	
// 	for (i = 0; i < 3; i++) {
// 		if (bound_nodes[i]) {
// 			fprintf( stderr, "prefix of index must have %s\n", HX_POSITION_NAMES[i] );
// 		}
// 	}
	
	hx_container_t* access_plans	= hx_new_container( 'A', 6 );
	for (i = 0; i < size; i++) {
		hx_index* idx	= (hx_index*) hx_container_item( indexes, i );
		int isize	= idx->size;
		char* name	= hx_index_name( idx );
//		fprintf( stderr, "hexastore has index %s (%p)\n", name, (void*) idx );
		
		int index_is_ok	= 1;
		for (j = 0; j < bound; j++) {
			if (!bound_nodes[ idx->order[j] ]) {
				index_is_ok	= 0;
//				fprintf( stderr, "- won't work because %s comes before some bound terms\n", HX_POSITION_NAMES[ idx->order[j] ] );
			}
		}
		
		if (bound == 0) {
			// there are 3 variables. if we have shared variables, they must appear sequentially, and as a prefix of the index order
			// (the nodes in index->order[0] and index->order[1] have to be the shared variable)
			if (repeated_variable) {
				hx_node* first	= hx_triple_node( t, idx->order[0] );
				hx_node* middle	= hx_triple_node( t, idx->order[1] );
				
// 				char* string;
// 				hx_node_string( middle, &string );
// 				fprintf( stderr, "middle-of-index node: %s (%d)\n", string, hx_node_iv(middle) );
// 				free(string);
//				fprintf( stderr, "repeated variable: %d\n", repeated_variable );
				
				if (hx_node_iv(first) != repeated_variable) {
// 					fprintf( stderr, "- %s won't work because the %s isn't the shared variable\n", name, HX_POSITION_NAMES[ idx->order[0] ] );
					index_is_ok	= 0;
				}
				if (hx_node_iv(middle) != repeated_variable) {
// 					fprintf( stderr, "- %s won't work because the %s isn't the shared variable\n", name, HX_POSITION_NAMES[ idx->order[1] ] );
					index_is_ok	= 0;
				}
			}
		}
		
		if (index_is_ok) {
// 			fprintf( stderr, "%s index can be used\n", name );
			
			int order_count	= 3 - bound;
			hx_variablebindings_iter_sorting** order	= (hx_variablebindings_iter_sorting**) calloc( order_count, sizeof(hx_variablebindings_iter_sorting*) );
			
			int k;
			int l	= 0;
			for (k = bound; k < 3; k++) {
				int ordered_by	= idx->order[ k ];
				hx_node* n	= hx_triple_node( t, ordered_by );
				char* string;
				hx_node_string( n, &string );
// 				fprintf( stderr, "\tSorted by %s (%s)\n", HX_POSITION_NAMES[ ordered_by ], string );
				free(string);
				
				hx_variablebindings_iter_sorting* sorting	= hx_variablebindings_iter_new_node_sorting( HX_VARIABLEBINDINGS_ITER_SORT_ASCENDING, 0, n );
				order[ l++ ]	= sorting;
			}
			
			hx_optimizer_plan* plan	= hx_new_optimizer_access_plan( idx, t, order_count, order );
			hx_container_push_item( access_plans, plan );
			
			for (k = 0; k < order_count; k++) {
				hx_free_variablebindings_iter_sorting( order[k] );
			}
			free(order);
		}
		free(name);
	}
	
	size	= hx_container_size(access_plans);
	if (size == 0) {
		// none of the indexes we've got can cover the requested triple pattern optimally.
		// we can fall back to using a fulls can of one of the other indexes and filter for what we want
		fprintf( stderr, "*** no appropriate access plan found. scan+filter isn't implemented yet.\n" );
	}
	
	return access_plans;
}

// - joinPlans (which join algorithm to use? is sorting required?)
hx_container_t* hx_optimizer_join_plans ( hx_execution_context* ctx, hx_container_t* lhs, hx_container_t* rhs, int leftjoin ) {
	int i,j;
	hx_container_t* join_plans	= (hx_container_t*) hx_new_container( 'J', 6 );
	
	for (i = 0; i < hx_container_size(lhs); i++) {
		hx_optimizer_plan* lhsp	= hx_container_item( lhs, i );
		int lhs_order_count	= lhsp->order_count;
		hx_variablebindings_iter_sorting** lhs_order	= lhsp->order;
		for (j = 0; j < hx_container_size(rhs); j++) {
			hx_optimizer_plan* rhsp	= hx_container_item( rhs, j );
			int rhs_order_count	= rhsp->order_count;
			hx_variablebindings_iter_sorting** rhs_order	= rhsp->order;
			
			{					// LHS hash-join RHS
				hx_optimizer_plan* hj_lr	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, lhsp, rhsp, lhs_order_count, lhs_order, leftjoin );
				hx_container_push_item( join_plans, hj_lr );
			}
			
			if (!leftjoin) {	// RHS hash-join LHS
				hx_optimizer_plan* hj_rl	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, rhsp, lhsp, rhs_order_count, rhs_order, leftjoin );
				hx_container_push_item( join_plans, hj_rl );
			}
			
			{					// LHS nestedloop-join RHS
				hx_optimizer_plan* nl_lr	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_NESTEDLOOPJOIN, lhsp, rhsp, lhs_order_count, lhs_order, leftjoin );
				hx_container_push_item( join_plans, nl_lr );
			}
			
			if (!leftjoin) {	// RHS nestedloop-join LHS
				hx_optimizer_plan* nl_rl	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_NESTEDLOOPJOIN, rhsp, lhsp, rhs_order_count, rhs_order, leftjoin );
				hx_container_push_item( join_plans, nl_rl );
			}
			
			{					// LHS merge-join RHS
				hx_optimizer_plan* nl_lr	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_MERGEJOIN, lhsp, rhsp, lhs_order_count, lhs_order, leftjoin );
				hx_container_push_item( join_plans, nl_lr );
			}
			
			if (!leftjoin) {	// RHS merge-join LHS
				hx_optimizer_plan* nl_rl	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_MERGEJOIN, rhsp, lhsp, rhs_order_count, rhs_order, leftjoin );
				hx_container_push_item( join_plans, nl_rl );
			}
		}
	}


	
	return join_plans;
}

// - finalizePlans (add projection, ordering, filters)
hx_container_t* hx_optimizer_finalize_plans ( hx_execution_context* ctx, hx_container_t* plans ) {
	return NULL;
}

// - prunePlans
hx_container_t* hx_optimizer_prune_plans ( hx_execution_context* ctx, hx_container_t* plans ) {
	return NULL;
}

