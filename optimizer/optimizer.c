#include "optimizer/optimizer.h"
#include "misc/idmap.h"

typedef struct {
	hx_execution_context* ctx;
	hx_optimizer_plan* plan;
	char* string;
	int64_t cost;
} _hx_optimizer_plan_with_ctx;

int _hx_optimizer_plan_is_interesting ( hx_execution_context* ctx, hx_idmap* map, hx_optimizer_plan* plan );
int _hx_optimizer_cmp_plan_by_cost (const void *, const void *);
hx_container_t* _hx_optimizer_plan_subsets_of_size ( hx_execution_context* ctx, int total, int size );
hx_container_t* _hx_optimizer_plan_subsets ( hx_execution_context* ctx, int total );

void _hx_optimizer_debug_key ( char* name, char* key, int size ) {
	int i;
	if (size < 10) {
		fprintf( stderr, "%s: ", name );
		for (i = 0; i < size; i++) {
			if (key[i]) {
				fprintf( stderr, "%d", i );
			} else {
				fprintf( stderr, "_" );
			}
		}
		fprintf( stderr, "\n" );
	} else {
		fprintf( stderr, "**************************\n" );
	}
}

void _hx_optimizer_set_opt_plan_access_key( char* key, int triple ) {
	key[triple]	= 'b';
}

char* _hx_optimizer_final_opt_plan_access_key ( int size ) {
	char* key	= (char*) calloc( size, sizeof(char) );
	int i;
	for (i = 0; i < size; i++) {
		key[i]	= 'b';
	}
	return key;
}

char* _hx_optimizer_new_opt_plan_access_key ( int size ) {
	char* key	= (char*) calloc( size, sizeof(char) );
	return key;
}

char* _hx_optimizer_new_opt_plan_access_key_set ( int size, int triple ) {
	char* key	= _hx_optimizer_new_opt_plan_access_key( size );
	key[triple]	= 'b';
	return key;
}

char* _hx_optimizer_opt_plan_access_key_inverse ( int size, char* key ) {
	char* ikey	= _hx_optimizer_new_opt_plan_access_key( size );
	int i;
	for (i = 0; i < size; i++) {
		if (key[i] != 'b') {
			_hx_optimizer_set_opt_plan_access_key( ikey, i );
		}
	}
	return ikey;
}

void _hx_optimizer_optplans_debug_cb ( int klen, void* key, void* value ) {
	int i;
	int j	= 0;
	char* k	= (char*) key;
	
	hx_container_t* plans	= (hx_container_t*) value;
	int size	= hx_container_size(plans);
	
	fprintf( stderr, "%d optPlan for subquery with triples: ", size );
	for (i = 0; i < klen; i++) {
		if (k[i] == 'b') {
			if (j++ > 0) fprintf( stderr, ", " );
			fprintf( stderr, "%d", i );
		}
	}
	fprintf( stderr, "\n" );
}

void _hx_optimizer_debug_plans ( hx_execution_context* ctx, char* message, hx_container_t* plans ) {
	int i;
	int psize	= hx_container_size(plans);
	if (message) {
		fprintf( stderr, "*** %s\n", message );
	}
	for (i = 0; i < psize; i++) {
		hx_optimizer_plan* plan	= hx_container_item( plans, i );
		int64_t cost	= hx_optimizer_plan_cost( ctx, plan );
		char* string;
		hx_optimizer_plan_string( plan, &string );
		fprintf( stderr, "- (plan %d, cost %lld) %s\n", i, cost, string );
		free(string);
	}
}

void _hx_optimizer_free_hash_cb ( void* key, size_t klen, void* value ) {
	hx_container_t* plans	= (hx_container_t*) value;
	int size	= hx_container_size(plans);
	int i;
	for (i = 0; i < size; i++) {
		hx_optimizer_plan* plan	= hx_container_item(plans,i);
		hx_free_optimizer_plan( plan );
	}
	hx_free_container(plans);
}

hx_optimizer_plan* hx_optimizer_optimize_bgp ( hx_execution_context* ctx, hx_bgp* b ) {
	int i;
	int bgpsize	= hx_bgp_size( b );
	hx_hash_t* optPlans	= hx_new_hash( bgpsize * bgpsize );
	for (i = 0; i < bgpsize; i++) {
		hx_triple* t	= hx_bgp_triple( b, i );
		hx_container_t* plans	= hx_optimizer_access_plans( ctx, t );
		char* key				= _hx_optimizer_new_opt_plan_access_key_set( bgpsize, i );
// 		_hx_optimizer_debug_plans( ctx, "Access plans (before pruning):", plans );
		hx_container_t* pruned	= hx_optimizer_prune_plans( ctx, plans );
		hx_hash_add( optPlans, key, bgpsize, pruned );
		free(key);
	}
//	hx_hash_debug( optPlans, _hx_optimizer_optplans_debug_cb );
	
	
	for (i = 2; i <= bgpsize; i++) {
// 		fprintf( stderr, "---------------------------------------------------------------------\n" );
// 		fprintf( stderr, "*** GENERATING OPTIMAL PLANS OF SIZE %d (max %d)\n", i, bgpsize );
		hx_container_t* si	= _hx_optimizer_plan_subsets_of_size( ctx, bgpsize, i );
		int sisize			= hx_container_size(si);
		hx_container_t* oi	= _hx_optimizer_plan_subsets( ctx, i );
		int oisize			= hx_container_size(oi);
		
// 		fprintf( stderr, "*** oisize = %d\n", oisize );
		
		int j;
		// for each S (subset of the BGP)
		for (j = 0; j < sisize; j++) {
// 			fprintf( stderr, "--------------------------------------\n" );
			hx_container_t* s	= hx_container_item( si, j );
			int ssize		= hx_container_size(s);
			char* s_key		= _hx_optimizer_new_opt_plan_access_key( bgpsize );
			
			
			int j;
			for (j = 0; j < ssize; j++) {
				int k	= (int) hx_container_item( s, j );
				_hx_optimizer_set_opt_plan_access_key( s_key, k );
			}

// 			fprintf( stderr, "S (%d subset of BGP) has triples", i );
// 			_hx_optimizer_debug_key( "", s_key, bgpsize );
			
			
			
			hx_container_t* optPlan	= hx_new_container( 'P', bgpsize );
			
			int k;
			// for each proper subset O of S
			for (k = 0; k < oisize; k++) {
				hx_container_t* oindexes	= hx_container_item( oi, k );
				int oindexessize			= hx_container_size( oindexes );
				int j;
				
				char* o_key		= _hx_optimizer_new_opt_plan_access_key( bgpsize );
// 				fprintf( stderr, "O (subset of S) has triples: ", i );
				for (j = 0; j < oindexessize; j++) {
					int idx	= (int) hx_container_item(oindexes,j);
					int triple_number	= (int) hx_container_item(s,idx);
					_hx_optimizer_set_opt_plan_access_key( o_key, triple_number );
// 					fprintf( stderr, "%d ", triple_number );
				}
// 				fprintf( stderr, "\n" );
				hx_container_t* optPlanO	= hx_hash_get( optPlans, o_key, bgpsize );
				int z;
// 				fprintf( stderr, "LHS (%p):\n", (void*) optPlanO );
// 				for (z = 0; z < bgpsize; z++) {
// 					if (o_key[z]) {
// 						fprintf( stderr, "- includes triple (%d)\n", z );
// 					}
// 				}
				
				
				// create a key for all the triples in S not in O (S-O).
				char* s_not_o_key	= _hx_optimizer_new_opt_plan_access_key( bgpsize );
				for (j = 0; j < bgpsize; j++) {
					if (s_key[j] && !(o_key[j])) {
						_hx_optimizer_set_opt_plan_access_key( s_not_o_key, j );
					}
				}
				hx_container_t* optPlanSO	= hx_hash_get( optPlans, s_not_o_key, bgpsize );
				
// 				fprintf( stderr, "RHS (%p):\n", (void*) optPlanSO );
// 				for (z = 0; z < bgpsize; z++) {
// 					if (s_not_o_key[z]) {
// 						fprintf( stderr, "- includes triple (%d)\n", z );
// 					}
// 				}
				
//				hx_hash_debug( optPlans, _hx_optimizer_optplans_debug_cb );
				
				
				hx_container_t* joins		= hx_optimizer_join_plans( ctx, optPlanO, optPlanSO, 0 );
				int joins_size	= hx_container_size(joins);
				for (j = 0; j < joins_size; j++) {
					hx_container_push_item( optPlan, hx_container_item(joins,j) );
				}
				hx_free_container(joins);
				free(o_key);
				free(s_not_o_key);
			}
			
			hx_container_t* pruned	= hx_optimizer_prune_plans( ctx, optPlan );
			optPlan	= pruned;
			
			
// 			_hx_optimizer_debug_key( "SETTING optPlans for subquery with triples", s_key, bgpsize );
// 			_hx_optimizer_debug_plans( ctx, NULL, optPlan );
			hx_hash_add( optPlans, s_key, bgpsize, optPlan );
			free(s_key);
		}
		
		for (i = 0; i < oisize; i++) {
			hx_container_t* oindexes	= hx_container_item( oi, i );
			hx_free_container(oindexes);
		}
		hx_free_container( oi );
		for (j = 0; j < sisize; j++) {
			hx_container_t* s	= hx_container_item( si, j );
			hx_free_container(s);
		}
		hx_free_container( si );
	}
	
	
		
	char* finalkey	= _hx_optimizer_final_opt_plan_access_key( bgpsize );
	hx_container_t* final	= hx_hash_get( optPlans, finalkey, bgpsize );
	if (final == NULL) {
		fprintf( stderr, "*** something went wrong in hx_optimizer_optimize_bgp - failed to generate full query plan\n" );
		return NULL;
	}
	free(finalkey);
	
	hx_container_t* opt	= hx_optimizer_prune_plans( ctx, final );
// 	_hx_optimizer_debug_plans( ctx, "optimizer ended with plans>", opt );
	
	// finalize(opt)
	// prune(opt)
	hx_optimizer_plan* plan	= hx_copy_optimizer_plan( hx_container_item( opt, 0 ) );
	
	// XXX lots of memory cleanup needs to happen here
	hx_free_hash( optPlans, _hx_optimizer_free_hash_cb );
	
	return plan;
}

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
			
			if (1) {
				{					// LHS nestedloop-join RHS
					hx_optimizer_plan* nl_lr	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_NESTEDLOOPJOIN, lhsp, rhsp, lhs_order_count, lhs_order, leftjoin );
					hx_container_push_item( join_plans, nl_lr );
				}
				
				if (!leftjoin) {	// RHS nestedloop-join LHS
					hx_optimizer_plan* nl_rl	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_NESTEDLOOPJOIN, rhsp, lhsp, rhs_order_count, rhs_order, leftjoin );
					hx_container_push_item( join_plans, nl_rl );
				}
			}
			
			if (1) {
				{					// LHS hash-join RHS
					hx_optimizer_plan* hj_lr	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, lhsp, rhsp, lhs_order_count, lhs_order, leftjoin );
					hx_container_push_item( join_plans, hj_lr );
				}
				
				if (!leftjoin) {	// RHS hash-join LHS
					hx_optimizer_plan* hj_rl	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, rhsp, lhsp, rhs_order_count, rhs_order, leftjoin );
					hx_container_push_item( join_plans, hj_rl );
				}
			}
			
			if (1) {
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
	}


	
	return join_plans;
}

// - finalizePlans (add projection, ordering, filters)
hx_container_t* hx_optimizer_finalize_plans ( hx_execution_context* ctx, hx_container_t* plans, hx_container_t* requested_order, hx_container_t* project_variables, hx_container_t* filters ) {
	return plans;
}

// - prunePlans
hx_container_t* hx_optimizer_prune_plans ( hx_execution_context* ctx, hx_container_t* plans ) {
	int size	= hx_container_size( plans );
	if (size < 2) {
		return plans;
	}
	
	hx_container_t* pruned	= hx_new_container( hx_container_type(plans), size );
	hx_idmap* map			= hx_new_idmap();
	
	int i;
	int sortarray_size	= 0;
	_hx_optimizer_plan_with_ctx* sortarray	= (_hx_optimizer_plan_with_ctx*) calloc( size, sizeof(_hx_optimizer_plan_with_ctx) );
// 	fprintf( stderr, "Before sort: -----------------------------\n" );
	for (i = 0; i < size; i++) {
		hx_optimizer_plan* p	= hx_container_item(plans, i);
		if (_hx_optimizer_plan_is_interesting(ctx, map, p)) {
			hx_container_push_item(pruned,p);
		} else {
			char* string;
			hx_optimizer_plan_string( p, &string );
			int64_t cost	= hx_optimizer_plan_cost( ctx, p );
// 			fprintf( stderr, "%d: (cost %lld) %s\n", i, cost, string );
			
			int idx	= sortarray_size++;
			sortarray[ idx ].ctx	= ctx;
			sortarray[ idx ].plan	= p;
			sortarray[ idx ].cost	= cost;
			sortarray[ idx ].string	= string;
		}
	}
	
	qsort( sortarray, sortarray_size, sizeof(_hx_optimizer_plan_with_ctx), _hx_optimizer_cmp_plan_by_cost );

// 	fprintf( stderr, "After sort: -----------------------------\n" );
	for (i = 0; i < sortarray_size; i++) {
		hx_optimizer_plan* p	= sortarray[i].plan;
// 		fprintf( stderr, "%d: (cost %lld) %s\n", i, sortarray[i].cost, sortarray[i].string );

		free( sortarray[i].string );
		
		if (i == 0) {
			hx_container_push_item( pruned, p );
		} else {
			hx_free_optimizer_plan( p );
		}
	}
	free(sortarray);
	hx_free_idmap( map );
	hx_free_container( plans );
	return pruned;
}

int _hx_optimizer_plan_is_interesting ( hx_execution_context* ctx, hx_idmap* map, hx_optimizer_plan* plan ) {
	return 0;
	
// 	int interesting	= 0;
// 	
// 	hx_variablebindings_iter_sorting** sorting;
// 	int count	= hx_optimizer_plan_sorting( plan, &sorting );
// 	hx_variablebindings_iter_sorting* s	= sorting[0];
// 	
// 	char* sort_string;
// 	hx_variablebindings_iter_sorting_string( s, &sort_string );
// 	fprintf( stderr, "- %s\n", sort_string );
// 	
// 	if (hx_idmap_get_id( map, sort_string ) == 0) {
// 		
// 	}
// 	
// 	
// 	int j;
// 	for (j = 0; j < count; j++) {
// 		hx_free_variablebindings_iter_sorting( sorting[j] );
// 	}
// 	free(sorting);
// 	free(sort_string);
// 	
// 	return 0; // XXX not implemented yet. will be required when we get ORDER BY clauses working
}


int _hx_optimizer_cmp_plan_by_cost (const void *a, const void *b) {
	_hx_optimizer_plan_with_ctx* _a	= (_hx_optimizer_plan_with_ctx*) a;
	_hx_optimizer_plan_with_ctx* _b	= (_hx_optimizer_plan_with_ctx*) b;
	int64_t cost_a	= _a->cost;
	int64_t cost_b	= _b->cost;
	if (cost_a < cost_b) {
		return -1;
	} else if (cost_a > cost_b) {
		return 1;
	} else {
		// try to imbue this process with some predictability (helpful for testing)
		return -1 * strcmp(_a->string, _b->string);
	}
}

hx_container_t* _hx_optimizer_plan_subsets_of_size ( hx_execution_context* ctx, int total, int size ) {
	int i,j,k;
	hx_container_t* subsets	= hx_new_container( 'S', size );
	if (size == 0) {
		return subsets;
	} else if (size == 1) {
		for (i = 0; i < total; i++) {
			hx_container_t* subset	= hx_new_container( 's', 1 );
			hx_container_push_item( subset, (void*) i );
			hx_container_push_item( subsets, subset );
		}
	} else {
		// subsets of size size-1
		hx_container_t* bigsubsets	= _hx_optimizer_plan_subsets_of_size( ctx, total, size-1 );
		
		// for each subset of size size-1
		int bigsize					= hx_container_size( bigsubsets );
		for (i = 0; i < bigsize; i++) {
			hx_container_t* subset	= hx_container_item( bigsubsets, i );
			int ssize	= hx_container_size( subset );
			
			// for each j in [0 .. size-1]
			for (j = 0; j < total; j++) {
				// only add j to a subset if the last element in the subset is less than j (preserves order and makes sure we don't add duplicates)
				if ((int) hx_container_item( subset, ssize-1 ) < j) {
					// add j to the subset
					hx_container_t* copy	= hx_copy_container( subset );
					hx_container_push_item( copy, (void*) j );
					if (hx_container_size(copy) != size) {
						fprintf( stderr, "*** subset isn't the correct size\n" );
					}
					hx_container_push_item( subsets, copy );
				}
			}
			hx_free_container( subset );
		}
		hx_free_container( bigsubsets );
	}
	return subsets;
}

hx_container_t* _hx_optimizer_plan_subsets ( hx_execution_context* ctx, int total ) {
	int i;
	hx_container_t* subsets	= hx_new_container( 'S', total );
	for (i = 1; i < total; i++) {
		hx_container_t* subsets_slice	= _hx_optimizer_plan_subsets_of_size( ctx, total, i );
		int size	= hx_container_size( subsets_slice );
		int j;
		for (j = 0; j < size; j++) {
			hx_container_push_item( subsets, hx_container_item( subsets_slice, j ) );
		}
		hx_free_container(subsets_slice);
	}
	
	return subsets;
}
