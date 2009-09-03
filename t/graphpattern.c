#include "hexastore.h"
#include "algebra/graphpattern.h"
#include "test/tap.h"

void _add_data ( hx_hexastore* hx );
hx_bgp* _test_bgp1 ( void );
hx_bgp* _test_bgp2 ( void );
hx_bgp* _test_bgp3 ( void );

void eval_test1 ( void );
void eval_test2 ( void );
void serialization_test ( void );
void variable_test1 ( void );
void variable_test2 ( void );
void gp_varsub_test1 ( void );

hx_node *p1, *p2, *r1, *r2, *l1, *l2, *l3, *l4, *l5, *l6, *v1, *v2, *v3;
int main ( void ) {
	plan_tests(33);
	
	p1	= hx_new_node_resource( "p1" );
	p2	= hx_new_node_resource( "p2" );
	r1	= hx_new_node_resource( "r1" );
	r2	= hx_new_node_resource( "r2" );
	l1	= hx_new_node_literal( "l1" );
	l2	= hx_new_node_literal( "l2" );
	l3	= hx_new_node_literal( "l3" );
	l4	= hx_new_node_literal( "l4" );
	l5	= hx_new_node_literal( "l5" );
	l6	= hx_new_node_literal( "l6" );
	v1	= hx_new_node_named_variable( -1, "x" );
	v2	= hx_new_node_named_variable( -2, "y" );
	v3	= hx_new_node_named_variable( -3, "z" );
	
	serialization_test();
	eval_test1();
	eval_test2();
	variable_test1();
	variable_test2();
	gp_varsub_test1();
	
	hx_free_node( p1 );
	hx_free_node( p2 );
	hx_free_node( r1 );
	hx_free_node( r2 );
	hx_free_node( l1 );
	hx_free_node( l2 );
	hx_free_node( l3 );
	hx_free_node( l4 );
	hx_free_node( l5 );
	hx_free_node( l6 );
	hx_free_node( v1 );
	hx_free_node( v2 );
	hx_free_node( v3 );
	
	return exit_status();
}

void eval_test1 ( void ) {
	fprintf( stdout, "# eval test 1\n" );
	hx_expr_debug	= 1;
	hx_hexastore* hx		= hx_new_hexastore( NULL );
	hx_nodemap* map			= hx_store_hexastore_get_nodemap( hx->store );
	_add_data( hx );
	
	hx_bgp* b				= _test_bgp1();
	hx_graphpattern* p		= hx_new_graphpattern( HX_GRAPHPATTERN_BGP, b );
	hx_variablebindings_iter* iter	= hx_graphpattern_execute( p, hx );
	
	int counter	= 0;
	while (!hx_variablebindings_iter_finished( iter )) {
		counter++;
		hx_variablebindings* vb;
		hx_variablebindings_iter_current( iter, &vb );
		
		hx_node* obj	= hx_variablebindings_node_for_binding_name( vb, hx->store, "x" );
		ok1( hx_node_is_literal(obj) == 1 );
		char* v	= hx_node_value(obj);
		ok1( *v == 'l' );
		ok1( v[1] == '2' || v[1] == '5' );
		
		hx_node* subj	= hx_variablebindings_node_for_binding_name( vb, hx->store, "y" );
		ok1( hx_node_is_resource(subj) == 1 );
		ok1( hx_node_cmp(subj, r1) == 0 );
		
		hx_free_variablebindings(vb);
		hx_variablebindings_iter_next( iter );
	}
	ok1( counter == 2 );
	hx_free_variablebindings_iter( iter );
	hx_free_graphpattern( p );
	hx_free_hexastore( hx );
}

void eval_test2 ( void ) {
	fprintf( stdout, "# eval test 2\n" );
	hx_expr_debug	= 1;
	hx_hexastore* hx		= hx_new_hexastore( NULL );
	hx_nodemap* map			= hx_store_hexastore_get_nodemap( hx->store );
	_add_data( hx );
	
	hx_expr* e				= hx_new_builtin_expr2( HX_EXPR_OP_EQUAL, hx_new_node_expr(v1), hx_new_node_expr(l5) );
	hx_graphpattern* b		= hx_new_graphpattern( HX_GRAPHPATTERN_BGP, _test_bgp1() );
	hx_graphpattern* p		= hx_new_graphpattern( HX_GRAPHPATTERN_FILTER, e, b );
	hx_variablebindings_iter* iter	= hx_graphpattern_execute( p, hx );
	
	int counter	= 0;
	while (!hx_variablebindings_iter_finished( iter )) {
		counter++;
		hx_variablebindings* vb;
		hx_variablebindings_iter_current( iter, &vb );
		
		hx_node* obj	= hx_variablebindings_node_for_binding_name( vb, hx->store, "x" );
		ok1( hx_node_is_literal(obj) == 1 );
		char* v	= hx_node_value(obj);
		ok1( *v == 'l' );
		ok1( v[1] == '5' );
		
		hx_node* subj	= hx_variablebindings_node_for_binding_name( vb, hx->store, "y" );
		ok1( hx_node_is_resource(subj) == 1 );
		ok1( hx_node_cmp(subj, r1) == 0 );
		
		hx_free_variablebindings(vb);
		hx_variablebindings_iter_next( iter );
	}
	ok1( counter == 1 );
	hx_free_variablebindings_iter( iter );
	hx_free_graphpattern( p );
	hx_free_hexastore( hx );
}

void serialization_test ( void ) {
	{
		hx_bgp* b1	= _test_bgp1();
		hx_graphpattern* p	= hx_new_graphpattern( HX_GRAPHPATTERN_BGP, b1 );
		ok1( p != NULL );
		
		char* string;
		hx_graphpattern_sse( p, &string, "  ", 0 );
		ok1( strcmp(string, "(bgp\n  (triple ?y <p1> \"l1\")\n  (triple ?y <p2> ?x)\n)\n") == 0 );
		free( string );
		hx_free_graphpattern( p );
	}

	{
		hx_graphpattern* b1	= hx_new_graphpattern( HX_GRAPHPATTERN_BGP, _test_bgp1() );
		hx_graphpattern* b2	= hx_new_graphpattern( HX_GRAPHPATTERN_BGP, _test_bgp2() );
		hx_graphpattern* p	= hx_new_graphpattern( HX_GRAPHPATTERN_GROUP, 2, b1, b2 );
		ok1( p != NULL );
		
		char* string;
		hx_graphpattern_sse( p, &string, "  ", 0 );
		ok1( strcmp(string, "(ggp\n  (bgp\n    (triple ?y <p1> \"l1\")\n    (triple ?y <p2> ?x)\n  )\n  (bgp\n    (triple <r2> <p2> ?x)\n    (triple <r2> <p2> ?y)\n  )\n)\n") == 0 );
		free( string );
		hx_free_graphpattern( p );
	}
	
	{
		hx_graphpattern* b	= hx_new_graphpattern( HX_GRAPHPATTERN_BGP, _test_bgp1() );
		hx_graphpattern* p	= hx_new_graphpattern( HX_GRAPHPATTERN_GRAPH, hx_node_copy(v2), b );
		ok1( p != NULL );
		
		char* string;
		hx_graphpattern_sse( p, &string, "  ", 0 );
		ok1( strcmp(string, "(named-graph ?y\n  (bgp\n    (triple ?y <p1> \"l1\")\n    (triple ?y <p2> ?x)\n  )\n)\n") == 0 );
		free( string );
		hx_free_graphpattern( p );
	}

	{
		hx_expr* e	= hx_new_builtin_expr1( HX_EXPR_BUILTIN_STR, hx_new_node_expr(v1) );
		hx_graphpattern* b	= hx_new_graphpattern( HX_GRAPHPATTERN_BGP, _test_bgp1() );
		hx_graphpattern* p	= hx_new_graphpattern( HX_GRAPHPATTERN_FILTER, e, b );
		ok1( p != NULL );
		
		char* string;
		hx_graphpattern_sse( p, &string, "  ", 0 );
		ok1( strcmp(string, "(filter\n  (sparql:str ?x)\n  (bgp\n    (triple ?y <p1> \"l1\")\n    (triple ?y <p2> ?x)\n  )\n)\n") == 0 );
		free( string );
		hx_free_graphpattern( p );
	}
	
	
	
}

void variable_test1 ( void ) {
	fprintf( stdout, "# variable test 1\n" );
	hx_bgp* b				= _test_bgp1();
	hx_graphpattern* p		= hx_new_graphpattern( HX_GRAPHPATTERN_BGP, b );
	
// 	{
// 		char* string;
// 		hx_graphpattern_sse( p, &string, "  ", 0 );
// 		fprintf( stderr, "%s\n", string );
// 		free(string);
// 	}
	
	hx_node** v;
	int var_count	= hx_graphpattern_variables( p, &v );
	ok1( var_count == 2 );
	
// 	fprintf( stderr, "var count: %d\n", var_count );
// 	for (int i = 0; i < var_count; i++) {
// 		char* string;
// 		hx_node_string( v[i], &string );
// 		fprintf( stderr, "- %s\n", string );
// 		free(string);
// 	}
	
	hx_node* x	= v[0];
	ok1( hx_node_cmp(x,v1) == 0 );

	hx_node* y	= v[1];
	ok1( hx_node_cmp(y,v2) == 0 );
	
	hx_free_node(x);
	hx_free_node(y);
	free(v);
	
	hx_free_graphpattern( p );
}

void variable_test2 ( void ) {
	fprintf( stdout, "# variable test 2\n" );
	hx_graphpattern* b1	= hx_new_graphpattern( HX_GRAPHPATTERN_BGP, _test_bgp1() );
	hx_graphpattern* b2	= hx_new_graphpattern( HX_GRAPHPATTERN_BGP, _test_bgp3() );
	hx_graphpattern* p	= hx_new_graphpattern( HX_GRAPHPATTERN_GROUP, 2, b1, b2 );
	
// 	{
// 		char* string;
// 		hx_graphpattern_sse( p, &string, "  ", 0 );
// 		fprintf( stderr, "%s\n", string );
// 		free(string);
// 	}
	
	hx_node** v;
	int var_count	= hx_graphpattern_variables( p, &v );
	ok1( var_count == 3 );
	
// 	fprintf( stderr, "var count: %d\n", var_count );
// 	for (int i = 0; i < var_count; i++) {
// 		char* string;
// 		hx_node_string( v[i], &string );
// 		fprintf( stderr, "- %s\n", string );
// 		free(string);
// 	}
	
	hx_node* x	= v[0];
	ok1( hx_node_cmp(x,v1) == 0 );

	hx_node* y	= v[1];
	ok1( hx_node_cmp(y,v2) == 0 );
	
	hx_node* z	= v[2];
	ok1( hx_node_cmp(z,v3) == 0 );
	
	hx_free_node(x);
	hx_free_node(y);
	hx_free_node(z);
	free(v);
	
	hx_free_graphpattern( p );
}

void gp_varsub_test1 ( void ) {
	fprintf( stdout, "# variable substitution test\n" );
	{
		hx_hexastore* hx		= hx_new_hexastore( NULL );
		hx_nodemap* map			= hx_store_hexastore_get_nodemap( hx->store );
		hx_node_id l4_id		= hx_nodemap_add_node( map, l4 );
		
		hx_expr* e			= hx_new_builtin_expr1( HX_EXPR_BUILTIN_STR, hx_new_node_expr(v1) );
		hx_graphpattern* be	= hx_new_graphpattern( HX_GRAPHPATTERN_BGP, _test_bgp1() );
		hx_graphpattern* p	= hx_new_graphpattern( HX_GRAPHPATTERN_FILTER, e, be );
		
		{
			char* names[1]			= { "x" };
			hx_node_id* nodes		= (hx_node_id*) calloc( 1, sizeof( hx_node_id ) );
			nodes[0]				= l4_id;
			hx_variablebindings* b	= hx_new_variablebindings( 1, names, nodes );
			
			hx_graphpattern* q	= hx_graphpattern_substitute_variables( p, b, hx->store );
			char* string;
			hx_graphpattern_sse( q, &string, "  ", 0 );
			ok( strcmp( string, "(filter\n  (sparql:str \"l4\")\n  (bgp\n    (triple ?y <p1> \"l1\")\n    (triple ?y <p2> \"l4\")\n  )\n)\n" ) == 0, "expected graphpattern after varsub" );
			free( string );
			hx_free_graphpattern( q );
			hx_free_variablebindings(b);
		}
		
		hx_free_graphpattern( p );
		hx_free_nodemap( map );
	}
}

hx_bgp* _test_bgp1 ( void ) {
	hx_triple* t1	= hx_new_triple( v2, p1, l1 );
	hx_triple* t2	= hx_new_triple( v2, p2, v1 );
	hx_bgp* b	= hx_new_bgp2( t1, t2 );
	return b;
}

hx_bgp* _test_bgp2 ( void ) {
	hx_triple* t1	= hx_new_triple( r2, p2, v1 );
	hx_triple* t2	= hx_new_triple( r2, p2, v2 );
	hx_bgp* b		= hx_new_bgp2( t1, t2 );
	return b;
}

hx_bgp* _test_bgp3 ( void ) {
	hx_triple* t1	= hx_new_triple( v3, p2, v2 );
	hx_bgp* b		= hx_new_bgp1( t1 );
	return b;
}

void _add_data ( hx_hexastore* hx ) {
	hx_add_triple( hx, r2, p2, r1 );
	hx_add_triple( hx, r1, p1, l1 );
	hx_add_triple( hx, r1, p2, l2 );
	hx_add_triple( hx, r1, p2, l5 );
	hx_add_triple( hx, r1, p1, l6 );
	hx_add_triple( hx, r1, p1, l4 );
	hx_add_triple( hx, r1, p1, l3 );
}
