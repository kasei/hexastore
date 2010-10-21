#include "mentok/optimizer/optimizer.h"
#include "mentok/misc/idmap.h"

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

hx_optimizer_plan* hx_optimizer_optimize_plans_dp ( hx_execution_context* ctx, hx_hash_t* optPlans, int size );

int _hx_optimizer_plan_normalize_unions( hx_execution_context* ctx, hx_optimizer_plan* plan, hx_optimizer_plan** thunk );
int _hx_optimizer_plan_merge_unions( hx_execution_context* ctx, hx_optimizer_plan* plan, hx_optimizer_plan** thunk );


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

char* _hx_optimizer_key_triples_list( void* key, int klen ) {
	char* k	= (char*) key;
	char* s	= (char*) calloc( 1, 5 * klen );
	int i;
	for (i = 0; i < klen; i++) {
		if (k[i] == 'b') {
			char* num	= calloc(1,10);
			sprintf(num,"%d ",i);
			strcat(s,num);
			free(num);
		}
	}
	strcat(s,"\n");
	return s;
}

void _hx_optimizer_optplans_debug_cb ( void* key, int klen, void* value ) {
	int i;
	int j	= 0;
//	char* k	= (char*) key;
	
	hx_container_t* plans	= (hx_container_t*) value;
	int size	= hx_container_size(plans);
	
	char* triples	= _hx_optimizer_key_triples_list( key, klen );
	fprintf( stderr, "%d optPlan for subquery with triples: %s", size, triples );
// 	for (i = 0; i < klen; i++) {
// 		if (k[i] == 'b') {
// 			if (j++ > 0) fprintf( stderr, ", " );
// 			fprintf( stderr, "%d", i );
// 		}
// 	}
// 	fprintf( stderr, "\n" );
}

void _hx_optimizer_debug_plans ( hx_execution_context* ctx, char* message, hx_container_t* plans ) {
	int i;
	int psize	= hx_container_size(plans);
	if (message) {
		fprintf( stderr, "*** %s\n", message );
	}
	for (i = 0; i < psize; i++) {
		hx_optimizer_plan* plan	= hx_container_item( plans, i );
		hx_optimizer_plan_cost_t* c	= hx_optimizer_plan_cost( ctx, plan );
		int64_t cost				= hx_optimizer_plan_cost_value( ctx, c );
		char* string;
		hx_optimizer_plan_string( ctx, plan, &string );
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
		hx_container_t* plans	= ctx->optimizer_access_plans( ctx, t );
		
		char* key				= _hx_optimizer_new_opt_plan_access_key_set( bgpsize, i );
// 		_hx_optimizer_debug_plans( ctx, "Access plans (before pruning):", plans );
		hx_container_t* pruned	= hx_optimizer_prune_plans( ctx, plans );
		hx_hash_add( optPlans, key, bgpsize, pruned );
		free(key);
	}
// 	hx_hash_debug( optPlans, _hx_optimizer_optplans_debug_cb );
	
	return hx_optimizer_optimize_optplans( ctx, optPlans, bgpsize );
}

hx_optimizer_plan* hx_optimizer_optimize_plans ( hx_execution_context* ctx, hx_container_t* p ) {
	int i;
	int size	= hx_container_size( p );
	hx_hash_t* optPlans	= hx_new_hash( size * size );
	for (i = 0; i < size; i++) {
		hx_optimizer_plan* t	= hx_container_item( p, i );
		hx_container_t* plans	= hx_new_container( 'P', 1 );
		hx_container_push_item( plans, t );
		char* key				= _hx_optimizer_new_opt_plan_access_key_set( size, i );
		hx_hash_add( optPlans, key, size, plans );
		free(key);
	}
// 	hx_hash_debug( optPlans, _hx_optimizer_optplans_debug_cb );
	hx_free_container( p );
	return hx_optimizer_optimize_optplans( ctx, optPlans, size );
}

hx_optimizer_plan* hx_optimizer_optimize_plans_dp ( hx_execution_context* ctx, hx_hash_t* optPlans, int size ) {
	return hx_optimizer_optimize_optplans( ctx, optPlans, size );
}

hx_optimizer_plan* hx_optimizer_optimize_optplans ( hx_execution_context* ctx, hx_hash_t* optPlans, int size ) {
	int i;
	for (i = 2; i <= size; i++) {
// 		fprintf( stderr, "---------------------------------------------------------------------\n" );
// 		fprintf( stderr, "*** GENERATING OPTIMAL PLANS OF SIZE %d (out of %d)\n", i, size );
		hx_container_t* si	= _hx_optimizer_plan_subsets_of_size( ctx, size, i );
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
			char* s_key		= _hx_optimizer_new_opt_plan_access_key( size );
			
			
			int j;
			for (j = 0; j < ssize; j++) {
				int k	= (int) hx_container_item( s, j );
				_hx_optimizer_set_opt_plan_access_key( s_key, k );
			}

// 			fprintf( stderr, "S (%d subset of BGP) has triples", i );
// 			_hx_optimizer_debug_key( "", s_key, size );
			
			hx_container_t* optPlan	= hx_new_container( 'P', size );
			
			int k;
			// for each proper subset O of S
			for (k = 0; k < oisize; k++) {
				hx_container_t* oindexes	= hx_container_item( oi, k );
				int oindexessize			= hx_container_size( oindexes );
				int j;
				
				char* o_key		= _hx_optimizer_new_opt_plan_access_key( size );
// 				fprintf( stderr, "O (subset of S) has triples: ", i );
				for (j = 0; j < oindexessize; j++) {
					int idx	= (int) hx_container_item(oindexes,j);
					int triple_number	= (int) hx_container_item(s,idx);
					_hx_optimizer_set_opt_plan_access_key( o_key, triple_number );
// 					fprintf( stderr, "%d ", triple_number );
				}
// 				fprintf( stderr, "\n" );

// 				if (1) {
// 					char* keystr	= _hx_optimizer_key_triples_list( o_key, size );
// 					fprintf(stderr, "*** %s\n", keystr);
// 					free(keystr);
// 					hx_hash_debug( optPlans, _hx_optimizer_optplans_debug_cb );
// 				}

				hx_container_t* optPlanO	= hx_hash_get( optPlans, o_key, size );
				int z;
// 				fprintf( stderr, "LHS (%p size %d):\n", (void*) optPlanO, hx_container_size(optPlanO) );
// 				for (z = 0; z < size; z++) {
// 					if (o_key[z]) {
// 						fprintf( stderr, "- includes triple (%d)\n", z );
// 					}
// 				}
				
				
				// create a key for all the triples in S not in O (S-O).
				char* s_not_o_key	= _hx_optimizer_new_opt_plan_access_key( size );
				for (j = 0; j < size; j++) {
					if (s_key[j] && !(o_key[j])) {
						_hx_optimizer_set_opt_plan_access_key( s_not_o_key, j );
					}
				}
				hx_container_t* optPlanSO	= hx_hash_get( optPlans, s_not_o_key, size );
				
// 				fprintf( stderr, "RHS (%p size %d):\n", (void*) optPlanSO, hx_container_size(optPlanSO) );
// 				for (z = 0; z < size; z++) {
// 					if (s_not_o_key[z]) {
// 						fprintf( stderr, "- includes triple (%d)\n", z );
// 					}
// 				}
				
// 				hx_hash_debug( optPlans, _hx_optimizer_optplans_debug_cb );
				
				hx_container_t* joins		= ctx->optimizer_join_plans( ctx, hx_copy_container(optPlanO), hx_copy_container(optPlanSO), 0 );
// 				fprintf( stderr, "got join plan %p\n", joins );
				
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
			
			
// 			_hx_optimizer_debug_key( "SETTING optPlans for subquery with triples", s_key, size );
// 			_hx_optimizer_debug_plans( ctx, NULL, optPlan );
			hx_hash_add( optPlans, s_key, size, optPlan );
			free(s_key);
		}
		
		for (j = 0; j < oisize; j++) {
			hx_container_t* oindexes	= hx_container_item( oi, j );
			hx_free_container(oindexes);
		}
		hx_free_container( oi );
		for (j = 0; j < sisize; j++) {
			hx_container_t* s	= hx_container_item( si, j );
			hx_free_container(s);
		}
		hx_free_container( si );
	}
	
	
		
	char* finalkey	= _hx_optimizer_final_opt_plan_access_key( size );
	hx_container_t* final	= hx_hash_get( optPlans, finalkey, size );
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
	hx_model* hx		= ctx->hx;
	hx_container_t* indexes	= hx_store_triple_orderings( hx->store, t );
	int size				= hx_container_size(indexes);
	
	int i,j;
// 	char* string;
// 	hx_triple_string( t, &string );
// 	fprintf( stderr, "triple: %s\n", string );
// 	free(string);
	
//	int bound	= hx_triple_bound_count(t);
// 	fprintf( stderr, "triple has %d bound terms\n", bound );
	
	hx_container_t* access_plans	= hx_new_container( 'A', 6 );
	for (i = 0; i < size; i++) {
		void* idx	= hx_container_item( indexes, i );
		char* name	= hx_store_ordering_name( hx->store, idx );
//		fprintf( stderr, "store has index %s (%p)\n", name, (void*) idx );
		
// 		fprintf( stderr, "%s index can be used\n", name );
		
		hx_container_t* order	= hx_store_iter_sorting( hx->store, t, idx );
		int order_count	= hx_container_size(order);
		hx_optimizer_plan* plan	= hx_new_optimizer_access_plan( hx->store, idx, t, order );
		hx_container_push_item( access_plans, plan );
		
		int k;
		for (k = 0; k < order_count; k++) {
			hx_variablebindings_iter_sorting* s	= hx_container_item( order, k );
			hx_free_variablebindings_iter_sorting( s );
		}
		hx_free_container(order);
		free(name);
	}
	
	size	= hx_container_size(access_plans);
	if (size == 0) {
		// none of the indexes we've got can cover the requested triple pattern optimally.
		// we can fall back to using a fulls can of one of the other indexes and filter for what we want
		fprintf( stderr, "*** no appropriate access plan found. scan+filter isn't implemented yet.\n" );
	}
	
	hx_free_container( indexes );
	return access_plans;
}

// - joinPlans (which join algorithm to use? is sorting required?)
hx_container_t* hx_optimizer_join_plans ( hx_execution_context* ctx, hx_container_t* lhs, hx_container_t* rhs, int leftjoin ) {
	int i,j;
	hx_container_t* join_plans	= (hx_container_t*) hx_new_container( 'J', 6 );
	
	for (i = 0; i < hx_container_size(lhs); i++) {
		hx_optimizer_plan* lhsp	= hx_container_item( lhs, i );
//		int lhs_order_count	= hx_container_size( lhsp->order );
		hx_container_t* lhs_order	= lhsp->order;
		for (j = 0; j < hx_container_size(rhs); j++) {
			hx_optimizer_plan* rhsp	= hx_container_item( rhs, j );
//			int rhs_order_count	= hx_container_size( rhsp->order );
			hx_container_t* rhs_order	= rhsp->order;
			
			if (1) {
				{					// LHS nestedloop-join RHS
					hx_optimizer_plan* nl_lr	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_NESTEDLOOPJOIN, lhsp, rhsp, lhs_order, leftjoin );
					hx_container_push_item( join_plans, nl_lr );
				}
				
				if (!leftjoin) {	// RHS nestedloop-join LHS
					hx_optimizer_plan* nl_rl	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_NESTEDLOOPJOIN, rhsp, lhsp, rhs_order, leftjoin );
					hx_container_push_item( join_plans, nl_rl );
				}
			}
			
			if (1) {
				{					// LHS hash-join RHS
					hx_optimizer_plan* hj_lr	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, lhsp, rhsp, lhs_order, leftjoin );
					hx_container_push_item( join_plans, hj_lr );
				}
				
				if (!leftjoin) {	// RHS hash-join LHS
					hx_optimizer_plan* hj_rl	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, rhsp, lhsp, rhs_order, leftjoin );
					hx_container_push_item( join_plans, hj_rl );
				}
			}
			
			if (1) {
				{					// LHS merge-join RHS
					hx_optimizer_plan* nl_lr	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_MERGEJOIN, lhsp, rhsp, lhs_order, leftjoin );
					hx_container_push_item( join_plans, nl_lr );
				}
				
				if (!leftjoin) {	// RHS merge-join LHS
					hx_optimizer_plan* nl_rl	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_MERGEJOIN, rhsp, lhsp, rhs_order, leftjoin );
					hx_container_push_item( join_plans, nl_rl );
				}
			}
		}
	}
	
	int size	= hx_container_size( join_plans );
	for (i = 0; i < size; i++) {
		hx_optimizer_plan* p	= hx_container_item( join_plans, i );
		hx_optimizer_plan* q	= p;
		
// 		fprintf( stderr, "*** Trying to rewrite plan: (%p)\n", p );
// 		hx_optimizer_plan_debug( ctx, p );
		hx_optimizer_plan_rewrite( ctx, &q, _hx_optimizer_plan_normalize_unions );
		if (q != p) {
			// the root of the QEP was re-written, so we need to stash it back in the container
// 			fprintf( stderr, "*** rewritten to: (%p)\n", q );
// 			hx_optimizer_plan_debug( ctx, q );
// 			fprintf( stderr, "---------------\n" );
			
			hx_container_set_item( join_plans, i, q );
		}
		hx_optimizer_plan_visit( ctx, q, _hx_optimizer_plan_merge_unions, NULL );
	}
	
	hx_free_container( lhs );
	hx_free_container( rhs );
	
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
			hx_optimizer_plan_string( ctx, p, &string );
			hx_optimizer_plan_cost_t* c	= hx_optimizer_plan_cost( ctx, p );
			int64_t cost				= hx_optimizer_plan_cost_value( ctx, c );
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

int _hx_optimizer_plan_merge_unions( hx_execution_context* ctx, hx_optimizer_plan* plan, hx_optimizer_plan** thunk ) {
	if (plan->type != HX_OPTIMIZER_PLAN_UNION) return 1;
	hx_container_t* plans	= plan->data._union.plans;
	int size	= hx_container_size( plans );
	int i;
	int merged	= 0;
	hx_container_t* newplans	= hx_new_container('P', size);
	for (i = 0; i < size; i++) {
		hx_optimizer_plan* p	= hx_container_item( plans, i );
		if (p->type == HX_OPTIMIZER_PLAN_UNION) {
			merged	= 1;
			hx_container_t* children	= p->data._union.plans;
			int childsize	= hx_container_size( children );
			int j;
			for (j = 0; j < childsize; j++) {
				hx_container_push_item( newplans, hx_container_item( children, j ) );
			}
			hx_free_container( children );
		} else {
			hx_container_push_item( newplans, p );
		}
	}
	
	if (merged) {
		hx_free_container( plans );
// don't try to compile this if HXMPI is defined. qsort_r isn't available on
// the clusters, but we're not using this optimizer code on the clusters anyway.
#ifndef HXMPI
		qsort_r( newplans->items, hx_container_size(newplans), sizeof(void*), ctx, hx_optimizer_plan_cmp_service_calls );
#endif
		plan->data._union.plans	= newplans;
	} else {
		hx_free_container( newplans );
	}
	return 0;
}

int _hx_optimizer_plan_normalize_unions( hx_execution_context* ctx, hx_optimizer_plan* plan, hx_optimizer_plan** thunk ) {
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

