#include <stdlib.h>
#include "mentok/mentok.h"
#include "mentok/misc/nodemap.h"
#include "mentok/rdf/node.h"
#include "mentok/parser/parser.h"
#include "mentok/optimizer/optimizer.h"
#include "mentok/optimizer/plan.h"
#include "mentok/algebra/bgp.h"
#include "mentok/store/hexastore/hexastore.h"
#include "test/tap.h"

void _add_data ( hx_model* hx );

void access_plans_test1 ( hx_model* hx );
void access_plans_test2 ( hx_model* hx );
void access_plans_test3 ( hx_model* hx );
void join_plans_test1 ( hx_model* hx );
void sorting_test1 ( hx_model* hx );
void access_cost_test1 ( hx_model* hx );
void join_cost_test1 ( hx_model* hx );
void prune_plans_test1 ( hx_model* hx );
void prune_plans_test2 ( hx_model* hx );
void optimize_bgp_test1 ( hx_model* hx );
void optimize_bgp_test2 ( hx_model* hx );
void optimize_bgp_test3 ( hx_model* hx );
void execute_bgp_test1 ( hx_model* hx );

int _strcmp (const void *a, const void *b) {
	return strcmp( *((const char**) a), *((const char**) b) );
}

int main ( void ) {
	plan_tests(65);

	hx_model* hx	= hx_new_model( NULL );
	_add_data( hx );
	
	access_plans_test1( hx );
	access_plans_test2( hx );
	access_plans_test3( hx );
	
	join_plans_test1( hx );
	
	sorting_test1( hx );
	sorting_test1( hx );
	
	access_cost_test1( hx );
	join_cost_test1( hx );
	
	prune_plans_test1( hx );
	prune_plans_test2( hx );
	
	optimize_bgp_test1( hx );
	optimize_bgp_test2( hx );
	optimize_bgp_test3( hx );
	
	execute_bgp_test1( hx );
	
	hx_free_model(hx);
	return exit_status();
}

void access_plans_test1 ( hx_model* hx ) {
	fprintf( stdout, "# access_plans_test1\n" );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );

	hx_node* v1	= hx_new_node_named_variable( -1, "a" );
	hx_node* v2	= hx_new_node_named_variable( -2, "b" );
	hx_node* type	= (hx_node*) hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	hx_triple* t	= hx_new_triple( v1, type, v2 );
	
	int i;
	hx_container_t* plans	= hx_optimizer_access_plans( ctx, t );
	int size				= hx_container_size(plans);
	ok1( size == 2 );
	ok1( 'A' == hx_container_type(plans) );	// access plan
	
	int sorting_ok[2]	= {0,0};
//	fprintf( stderr, "%d plans (container %p)\n", size, (void*) plans );
	for (i = 0; i < size; i++) {
		hx_optimizer_plan* plan	= hx_container_item( plans, i );
// 		{
// 			char* string;
// 			hx_optimizer_plan_string( plan, &string );
// 			fprintf( stderr, "*** access_plans_test1 plan: %s\n", string );
// 			free(string);
// 		}
		
		void* i	= plan->data.access.source;
		char* name	= hx_store_ordering_name( hx->store, i );
		ok1( *name == 'P' );
		hx_container_t* order	= plan->order;
		ok1( hx_container_size(order) == 2 );
		
		
//		fprintf( stderr, "- %s\n", name );
		char* string[2];
		hx_variablebindings_iter_sorting_string( hx_container_item(plan->order, 0), &(string[0]) );
		hx_variablebindings_iter_sorting_string( hx_container_item(plan->order, 1), &(string[1]) );
		
		int cmp	= (strcmp( string[0], string[1] ) > 0) ? 1 : 0;
		sorting_ok[ cmp ]	= 1;
		
		qsort( string, 2, sizeof(char*), _strcmp );
		
		ok1( strcmp(string[0], "ASC(?a)") == 0 );
		ok1( strcmp(string[1], "ASC(?b)") == 0 );
		
		free(string[0]);
		free(string[1]);
		free(name);
		
		hx_free_optimizer_plan( plan );
	}
	
	hx_free_container( plans );
	
	ok1( sorting_ok[0] );
	ok1( sorting_ok[1] );
	
	hx_free_triple( t );
	hx_free_node(v1);
	hx_free_node(v2);
	hx_free_node(type);
	
	hx_free_execution_context(ctx);
}

void access_plans_test2 ( hx_model* hx ) {
	fprintf( stdout, "# access_plans_test2\n" );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );

	hx_node* v1	= hx_new_node_named_variable( -1, "x" );
	hx_node* type	= (hx_node*) hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	hx_node* resultset	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet");
	hx_triple* t	= hx_new_triple( v1, type, resultset );
	
	int i;
	hx_container_t* plans	= hx_optimizer_access_plans( ctx, t );
	int size				= hx_container_size(plans);
	ok1( size == 2 );
	
//	fprintf( stderr, "%d plans (container %p)\n", size, (void*) plans );
	for (i = 0; i < size; i++) {
		hx_optimizer_plan* plan	= hx_container_item( plans, i );
		void* i	= plan->data.access.source;
		char* name	= hx_store_ordering_name( hx->store, i );
		ok1( name[2] == 'S' );
		hx_container_t* order	= plan->order;
		ok1( hx_container_size(order) == 1 );
		
		char* string;
		hx_variablebindings_iter_sorting_string( hx_container_item(plan->order, 0), &string );
		ok1( strcmp(string, "ASC(?x)") == 0 );
		
		free(string);
		free(name);
		
		hx_free_optimizer_plan( plan );
	}
	
	hx_free_container( plans );
	
	hx_free_triple( t );
	hx_free_node(v1);
	hx_free_node(resultset);
	hx_free_node(type);

	hx_free_execution_context(ctx);
}

void access_plans_test3 ( hx_model* hx ) {
	fprintf( stdout, "# access_plans_test3\n" );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );

	hx_node* v1	= hx_new_node_named_variable( -1, "x" );
	hx_node* v2	= hx_new_node_named_variable( -1, "y" );
	hx_node* v3	= hx_new_node_named_variable( -1, "z" );
	hx_triple* t	= hx_new_triple( v1, v2, v3 );
	
	int i;
	hx_container_t* plans	= hx_optimizer_access_plans( ctx, t );
	int size				= hx_container_size(plans);
	ok1( size == 6 );
	hx_optimizer_plan* plan	= hx_container_item( plans, 0 );
	ok1( hx_container_size(plan->order) == 3 );
	
	for (i = 0; i < size; i++) {
		hx_optimizer_plan* plan	= hx_container_item( plans, i );
		hx_free_optimizer_plan( plan );
	}
	hx_free_container( plans );

	
	hx_free_triple( t );
	hx_free_node(v1);
	hx_free_node(v2);
	hx_free_node(v3);

	hx_free_execution_context(ctx);
}

void join_plans_test1 ( hx_model* hx ) {
	fprintf( stdout, "# join_plans_test1\n" );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );
	
	hx_node* v1	= hx_new_node_named_variable( -1, "x" );
	hx_node* v2	= hx_new_node_named_variable( -2, "y" );
	hx_node* type	= (hx_node*) hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	hx_node* resultset	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet");
	hx_node* resultvar	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/sw/DataAccess/tests/result-set#resultVariable");
	hx_triple* t1	= hx_new_triple( v1, type, resultset );
	hx_triple* t2	= hx_new_triple( v1, resultvar, v2 );
	
	int i;
	hx_container_t* plans1	= hx_optimizer_access_plans( ctx, t1 );
	hx_container_t* plans2	= hx_optimizer_access_plans( ctx, t2 );
	int size1				= hx_container_size(plans1);
	int size2				= hx_container_size(plans2);
	ok1( size1 == 2 );
	ok1( size2 == 2 );
	
	hx_container_t* jplans	= hx_optimizer_join_plans( ctx, plans1, plans2, 0 );
	
// 	for (i = 0; i < size1; i++) hx_free_optimizer_plan( hx_container_item( plans1, i ) );
// 	for (i = 0; i < size2; i++) hx_free_optimizer_plan( hx_container_item( plans2, i ) );
	
	int size				= hx_container_size(jplans);
	ok1( size == (size1 * size2 * 6) );	// a-join-b and b-join-a for each of the three join implementations
	ok1( 'J' == hx_container_type(jplans) );	// join plan
	
//	fprintf( stderr, "%d join plans (container %p)\n", size, (void*) jplans );
	for (i = 0; i < size; i++) {
		hx_optimizer_plan* plan	= hx_container_item( jplans, i );
//		fprintf( stderr, "join plan %d: %p\n", i, (void*) plan );
// 		char* string;
// 		hx_optimizer_plan_string( plan, &string );
// 		fprintf( stderr, "- %s\n", string );
// 		free(string);
		hx_free_optimizer_plan( plan );
	}
	hx_free_container( jplans );
	
	hx_free_triple( t1 );
	hx_free_triple( t2 );
	hx_free_node(v1);
	hx_free_node(v2);
	hx_free_node(resultvar);
	hx_free_node(resultset);
	hx_free_node(type);

	hx_free_execution_context(ctx);
}

void sorting_test1 ( hx_model* hx ) {
	fprintf( stdout, "# sorting_test1\n" );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );
	
	hx_node* v1	= hx_new_node_named_variable( -1, "a" );
	hx_node* v2	= hx_new_node_named_variable( -2, "b" );
	hx_node* type	= (hx_node*) hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	hx_triple* t	= hx_new_triple( v1, type, v2 );
	
	int i;
	hx_container_t* plans	= hx_optimizer_access_plans( ctx, t );
	int size				= hx_container_size(plans);
	
//	fprintf( stderr, "%d plans (container %p)\n", size, (void*) plans );
	for (i = 0; i < size; i++) {
		hx_optimizer_plan* plan	= hx_container_item( plans, i );
		{
			char* string;
			hx_optimizer_plan_string( plan, &string );
			
			hx_variablebindings_iter_sorting** sorting;
			int count	= hx_optimizer_plan_sorting( plan, &sorting );
			hx_variablebindings_iter_sorting* s	= sorting[0];
			ok1( count == 2 );
			
			char* sort_string;
			hx_variablebindings_iter_sorting_string( s, &sort_string );
			if (string[1] == 'S') {
				ok1( strcmp(sort_string, "ASC(?a)") == 0 );
			} else {
				ok1( strcmp(sort_string, "ASC(?b)") == 0 );
			}
			
			int j;
			for (j = 0; j < count; j++) {
				hx_free_variablebindings_iter_sorting( sorting[j] );
			}
			free(sorting);
			free(sort_string);
			free(string);
		}
		
		hx_free_optimizer_plan( plan );
	}
	
	hx_free_container( plans );
	
	hx_free_triple( t );
	hx_free_node(v1);
	hx_free_node(v2);
	hx_free_node(type);
	
	hx_free_execution_context(ctx);
}

void access_cost_test1 ( hx_model* hx ) {
	fprintf( stdout, "# access_cost_test1\n" );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );
	
	hx_node* v1	= hx_new_node_named_variable( -1, "x" );
	hx_node* v2	= hx_new_node_named_variable( -2, "y" );
	hx_node* type	= (hx_node*) hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	hx_node* resultset	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet");
	hx_node* resultvar	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/sw/DataAccess/tests/result-set#resultVariable");
	hx_triple* t1	= hx_new_triple( v1, type, resultset );
	hx_triple* t2	= hx_new_triple( v1, resultvar, v2 );
	
	int i;
	hx_container_t* plans1	= hx_optimizer_access_plans( ctx, t1 );
	hx_container_t* plans2	= hx_optimizer_access_plans( ctx, t2 );
	int size1				= hx_container_size(plans1);
	int size2				= hx_container_size(plans2);
	ok1( size1 == 2 );
	ok1( size2 == 2 );
	
	
	
	for (i = 0; i < size1; i++) {
		hx_optimizer_plan* plan	= hx_container_item( plans1, i );

		hx_optimizer_plan_cost_t* c	= hx_optimizer_plan_cost( ctx, plan );
		int64_t cost				= hx_optimizer_plan_cost_value( ctx, c );

// 		fprintf( stderr, "plan1 %d: %p\n", i, (void*) plan );
// 		char* string;
// 		hx_optimizer_plan_string( plan, &string );
// 		fprintf( stderr, "- %s\n", string );
// 		fprintf( stderr, "- cost: %lld\n", cost );
// 		free(string);
		hx_free_optimizer_plan( plan );
		
		ok1( cost == 2 );
	}
	
	for (i = 0; i < size1; i++) {
		hx_optimizer_plan* plan	= hx_container_item( plans2, i );
		hx_optimizer_plan_cost_t* c	= hx_optimizer_plan_cost( ctx, plan );
		int64_t cost				= hx_optimizer_plan_cost_value( ctx, c );
// 		fprintf( stderr, "plan2 %d: %p\n", i, (void*) plan );
// 		char* string;
// 		hx_optimizer_plan_string( plan, &string );
// 		fprintf( stderr, "- %s\n", string );
// 		fprintf( stderr, "- cost: %lld\n", cost );
// 		free(string);
		hx_free_optimizer_plan( plan );
		
		ok1( cost == 4 );
	}
	
	hx_free_container( plans1 );
	hx_free_container( plans2 );
	
	hx_free_triple( t1 );
	hx_free_triple( t2 );
	hx_free_node(v1);
	hx_free_node(v2);
	hx_free_node(resultvar);
	hx_free_node(resultset);
	hx_free_node(type);
	
	hx_free_execution_context(ctx);
}

void join_cost_test1 ( hx_model* hx ) {
	fprintf( stdout, "# join_cost_test1\n" );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );
	
	ctx->unsorted_mergejoin_penalty	= 2;
	ctx->hashjoin_penalty			= 1;
	ctx->nestedloopjoin_penalty		= 3;
	
	hx_node* v1	= hx_new_node_named_variable( -1, "x" );
	hx_node* v2	= hx_new_node_named_variable( -2, "y" );
	hx_node* type	= (hx_node*) hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	hx_node* resultset	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet");
	hx_node* resultvar	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/sw/DataAccess/tests/result-set#resultVariable");
	hx_triple* t1	= hx_new_triple( v1, type, resultset );
	hx_triple* t2	= hx_new_triple( v1, resultvar, v2 );
	
	int i;
	hx_container_t* plans1	= hx_optimizer_access_plans( ctx, t1 );
	hx_container_t* plans2	= hx_optimizer_access_plans( ctx, t2 );
	int size1				= hx_container_size(plans1);
	int size2				= hx_container_size(plans2);
	ok1( size1 == 2 );
	ok1( size2 == 2 );
	
	hx_container_t* jplans	= hx_optimizer_join_plans( ctx, plans1, plans2, 0 );
	
// 	for (i = 0; i < size1; i++) hx_free_optimizer_plan( hx_container_item( plans1, i ) );
// 	for (i = 0; i < size2; i++) hx_free_optimizer_plan( hx_container_item( plans2, i ) );
	
	int size				= hx_container_size(jplans);
	for (i = 0; i < size; i++) {
		hx_optimizer_plan* plan	= hx_container_item( jplans, i );
		hx_optimizer_plan_cost_t* c	= hx_optimizer_plan_cost( ctx, plan );
		int64_t cost				= hx_optimizer_plan_cost_value( ctx, c );
		char* string;
		hx_optimizer_plan_string( plan, &string );
// 		fprintf( stderr, "join plan %d cost %lld: %s\n", i, cost, string );
		
		if (strncmp( string, "merge-join", 10 ) == 0) {
			if (NULL != strstr(string, "POS({?x <http://www.w3.org/2001/sw/DataAccess/tests/result-set#resultVariable> ?y}")) {
				// the POS index on (?x * ?y) won't give mergeable data, since it'll be ordered by ?y, but the other iterator is ordered by ?x
				ok( cost == 20, "unsorted merge-join cost" );
			} else {
				ok( cost == 16, "merge-join cost" );
			}
		} else if (strncmp( string, "hash-join", 18 ) == 0) {
			ok( cost == 9, "hash-join cost" );
		} else if (strncmp( string, "nestedloop-join", 10 ) == 0) {
			ok( cost == 22, "nestedloop-join cost" );
		}
		
		free(string);
		hx_free_optimizer_plan( plan );
	}
	
	hx_free_container( jplans );
	
	hx_free_triple( t1 );
	hx_free_triple( t2 );
	hx_free_node(v1);
	hx_free_node(v2);
	hx_free_node(resultvar);
	hx_free_node(resultset);
	hx_free_node(type);
	
	hx_free_execution_context(ctx);
}

void prune_plans_test1 ( hx_model* hx ) {
	fprintf( stdout, "# prune_plans_test1\n" );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );

	hx_node* v1	= hx_new_node_named_variable( -1, "x" );
	hx_node* type	= (hx_node*) hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	hx_node* resultset	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet");
	hx_triple* t	= hx_new_triple( v1, type, resultset );
	
	int i;
	int size;
	hx_container_t* plans	= hx_optimizer_access_plans( ctx, t );
	size	= hx_container_size(plans);
	ok1( size == 2 );
	
	hx_container_t* pruned	= hx_optimizer_prune_plans( ctx, plans );
	size	= hx_container_size( pruned );
	ok1( size == 1 );
	
	for (i = 0; i < size; i++) {
		hx_optimizer_plan* plan	= hx_container_item( pruned, i );
		hx_free_optimizer_plan( plan );
	}
	hx_free_container( pruned );
	
	hx_free_triple( t );
	hx_free_node(v1);
	hx_free_node(resultset);
	hx_free_node(type);

	hx_free_execution_context(ctx);
}

void prune_plans_test2 ( hx_model* hx ) {
	fprintf( stdout, "# prune_plans_test2\n" );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );

	ctx->unsorted_mergejoin_penalty	= 2;
	ctx->hashjoin_penalty			= 1;
	ctx->nestedloopjoin_penalty		= 3;
	
	hx_node* v1	= hx_new_node_named_variable( -1, "x" );
	hx_node* v2	= hx_new_node_named_variable( -2, "y" );
	hx_node* type	= (hx_node*) hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	hx_node* resultset	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet");
	hx_node* resultvar	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/sw/DataAccess/tests/result-set#resultVariable");
	hx_triple* t1	= hx_new_triple( v1, type, resultset );
	hx_triple* t2	= hx_new_triple( v1, resultvar, v2 );
	
	int i;
	hx_container_t* plans1	= hx_optimizer_access_plans( ctx, t1 );
	hx_container_t* plans2	= hx_optimizer_access_plans( ctx, t2 );
	hx_container_t* jplans	= hx_optimizer_join_plans( ctx, plans1, plans2, 0 );
// 	for (i = 0; i < hx_container_size(plans1); i++) hx_free_optimizer_plan( hx_container_item( plans1, i ) );
// 	for (i = 0; i < hx_container_size(plans2); i++) hx_free_optimizer_plan( hx_container_item( plans2, i ) );
	
	hx_container_t* pruned	= hx_optimizer_prune_plans( ctx, jplans );
	int size	= hx_container_size( pruned );
	ok1( size == 1 );
	
	for (i = 0; i < size; i++) {
		hx_optimizer_plan* plan	= hx_container_item( pruned, i );
		char* string;
		hx_optimizer_plan_string( plan, &string );
		
		ok( strcmp(string, "merge-join(PSO({?x <http://www.w3.org/2001/sw/DataAccess/tests/result-set#resultVariable> ?y}), POS({?x <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet>}))") == 0, "expected pruned join plan" );
		
		free(string);
		hx_free_optimizer_plan( plan );
	}
	hx_free_container( pruned );
	
	hx_free_triple( t1 );
	hx_free_triple( t2 );
	hx_free_node(v1);
	hx_free_node(v2);
	hx_free_node(resultvar);
	hx_free_node(resultset);
	hx_free_node(type);

	hx_free_execution_context(ctx);
}

void optimize_bgp_test1 ( hx_model* hx ) {
	fprintf( stdout, "# optimize_bgp_test1\n" );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );
	
	hx_bgp* b	= hx_bgp_parse_string("{ ?x a <http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet> ; <http://www.w3.org/2001/sw/DataAccess/tests/result-set#resultVariable> ?y }");
	hx_optimizer_plan* plan	= hx_optimizer_optimize_bgp( ctx, b );
	
	char* string;
	hx_optimizer_plan_string( plan, &string );
	ok( strcmp(string, "merge-join(PSO({?x <http://www.w3.org/2001/sw/DataAccess/tests/result-set#resultVariable> ?y}), POS({?x <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet>}))") == 0, "expected best 2-bgp plan" );
//	fprintf( stderr, "GOT BEST PLAN: %s\n", string );
	free(string);
	
	hx_free_optimizer_plan(plan);
	hx_free_bgp( b );
	hx_free_execution_context(ctx);
}

void optimize_bgp_test2 ( hx_model* hx ) {
	fprintf( stdout, "# optimize_bgp_test2\n" );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );
	
	hx_bgp* b	= hx_bgp_parse_string("{ ?x <http://www.w3.org/2001/sw/DataAccess/tests/result-set#binding> ?y ; <http://www.w3.org/2001/sw/DataAccess/tests/result-set#resultVariable> ?z ; a <http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet> }");
	hx_optimizer_plan* plan	= hx_optimizer_optimize_bgp( ctx, b );
	
	char* string;
	hx_optimizer_plan_string( plan, &string );
	ok( strcmp(string, "merge-join(merge-join(PSO({?x <http://www.w3.org/2001/sw/DataAccess/tests/result-set#resultVariable> ?z}), POS({?x <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet>})), PSO({?x <http://www.w3.org/2001/sw/DataAccess/tests/result-set#binding> ?y}))") == 0, "expected best 3-bgp plan" );
// 	fprintf( stderr, "GOT BEST PLAN: %s\n", string );
	free(string);
	
	hx_free_optimizer_plan(plan);
	hx_free_bgp( b );
	hx_free_execution_context(ctx);
}

void optimize_bgp_test3 ( hx_model* hx ) {
	fprintf( stdout, "# optimize_bgp_test3\n" );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );
	
	hx_bgp* b	= hx_bgp_parse_string("PREFIX : <http://www.w3.org/2001/sw/DataAccess/tests/result-set#>\
	{\
		?x1 a :ResultSet ;\
			:solution ?s1 .\
		?s1 :binding ?b1 .\
		?b1 :value ?value1 ;\
			:variable ?variable1 .\
		?x2 a :ResultSet ;\
			:solution ?s2 .\
		?s2 :binding ?b2 .\
	}");
//	hx_bgp_debug(b);
	hx_optimizer_plan* plan	= hx_optimizer_optimize_bgp( ctx, b );
	
	char* string;
	hx_optimizer_plan_string( plan, &string );
	ok( strcmp(string, "hash-join(hash-join(hash-join(merge-join(PSO({?x2 <http://www.w3.org/2001/sw/DataAccess/tests/result-set#solution> ?s2}), POS({?x2 <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet>})), PSO({?s2 <http://www.w3.org/2001/sw/DataAccess/tests/result-set#binding> ?b2})), PSO({?s1 <http://www.w3.org/2001/sw/DataAccess/tests/result-set#binding> ?b1})), hash-join(hash-join(merge-join(PSO({?x1 <http://www.w3.org/2001/sw/DataAccess/tests/result-set#solution> ?s1}), POS({?x1 <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet>})), PSO({?b1 <http://www.w3.org/2001/sw/DataAccess/tests/result-set#variable> ?variable1})), PSO({?b1 <http://www.w3.org/2001/sw/DataAccess/tests/result-set#value> ?value1})))") == 0, "expected best 8-bgp plan" );
// 	fprintf( stderr, "GOT BEST PLAN: %s\n", string );
	free(string);
	
	hx_free_optimizer_plan(plan);
	hx_free_bgp( b );
	hx_free_execution_context(ctx);
}

void execute_bgp_test1 ( hx_model* hx ) {
	fprintf( stdout, "# execute_bgp_test1\n" );
	hx_nodemap* map	= hx_store_hexastore_get_nodemap(hx->store);
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );
	
	hx_bgp* b	= hx_bgp_parse_string("{ ?x a <http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet> ; <http://www.w3.org/2001/sw/DataAccess/tests/result-set#resultVariable> ?y }");
	hx_optimizer_plan* plan	= hx_optimizer_optimize_bgp( ctx, b );
	hx_variablebindings_iter* iter	= hx_optimizer_plan_execute( ctx, plan );
	int counter	= 0;
	while (!hx_variablebindings_iter_finished(iter)) {
		hx_variablebindings* b;
		hx_variablebindings_iter_current(iter, &b);
		
		hx_variablebindings_iter_next(iter);
		counter++;
	}
	
	ok1( counter == 4 );
}

void _add_data ( hx_model* hx ) {
	const char* rdf	= "@prefix :        <http://example/> . \
@prefix rs:      <http://www.w3.org/2001/sw/DataAccess/tests/result-set#> . \
@prefix rdf:     <http://www.w3.org/1999/02/22-rdf-syntax-ns#> . \
@prefix xsd:        <http://www.w3.org/2001/XMLSchema#> . \
 \
<http://resultset/>    rdf:type      rs:ResultSet ; \
      rs:resultVariable  \"p\" ; \
      rs:resultVariable  \"g\" ; \
      rs:resultVariable  \"s\" ; \
      rs:resultVariable  \"o\" ; \
      rs:solution   [ rs:binding    [ rs:value      :x ; \
                                      rs:variable   \"s\" \
                                    ] ; \
                      rs:binding    [ rs:value      :p ; \
                                      rs:variable   \"p\" \
                                    ] ; \
                      rs:binding    [ rs:value      <data-g1.ttl> ; \
                                      rs:variable   \"g\" \
                                    ] ; \
                      rs:binding    [ rs:value      \"1\"^^xsd:integer ; \
                                      rs:variable   \"o\" \
                                    ] \
                    ] ; \
      rs:solution   [ rs:binding    [ rs:value      :a ; \
                                      rs:variable   \"s\" \
                                    ] ; \
                      rs:binding    [ rs:value      :p ; \
                                      rs:variable   \"p\" \
                                    ] ; \
                      rs:binding    [ rs:value      \"9\"^^xsd:integer ; \
                                      rs:variable   \"o\" \
                                    ] ; \
                      rs:binding    [ rs:value      <data-g1.ttl> ; \
                                      rs:variable   \"g\" \
                                    ] \
                    ] . \
<http://resultset2/>    rdf:type      rs:ResultSet . \
";
	hx_parser* parser	= hx_new_parser();
	hx_parser_parse_string_into_model( parser, hx, rdf, "http://example.org/", "turtle" );
	hx_free_parser(parser);
}




// 	hx_node* v1	= hx_new_node_named_variable( -1, "a" );
// 	hx_node* v2	= hx_new_node_named_variable( -2, "b" );
// 	hx_node* v3	= hx_new_node_named_variable( -3, "c" );
// 	hx_node* l1	= hx_new_node_literal("a");
// 	hx_node* type	= (hx_node*) hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
// 	hx_node* resultset	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet");
// 	hx_node* r1	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/XMLSchema#integer");
// 	hx_node* r2	= (hx_node*) hx_new_node_resource("http://xmlns.com/foaf/0.1/name");
// 	hx_node* b1	= (hx_node*) hx_new_node_blank("r1");
// 	hx_triple* t	= hx_new_triple( v1, type, v2 );
// 	
// 	hx_free_triple( t );
// 	hx_free_node(v1);
// 	hx_free_node(v2);
// 	hx_free_node(v3);
// 	hx_free_node(l1);
// 	hx_free_node(r1);
// 	hx_free_node(r2);
// 	hx_free_node(b1);
// 	hx_free_node(type);
// 	hx_free_node(resultset);
