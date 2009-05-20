#include "hexastore.h"
#include "graphpattern.h"
#include "tap.h"

void _add_data ( hx_hexastore* hx, hx_storage_manager* s );
void _fill_triple ( hx_triple* t, hx_node* s, hx_node* p, hx_node* o );
hx_bgp* _test_bgp1 ( void );
hx_bgp* _test_bgp2 ( void );

void eval_test1 ( void );
void eval_test2 ( void );
void serialization_test ( void );

hx_node *p1, *p2, *r1, *r2, *l1, *l2, *l3, *l4, *l5, *l6, *v1, *v2;
int main ( void ) {
	plan_tests(25);
	
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
	
	serialization_test();
	eval_test1();
	eval_test2();
	
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
	
	return exit_status();
}

void eval_test1 ( void ) {
	fprintf( stdout, "# eval test 1\n" );
	hx_expr_debug	= 1;
	hx_storage_manager* s	= hx_new_memory_storage_manager();
	hx_hexastore* hx		= hx_new_hexastore( s );
	hx_nodemap* map			= hx_get_nodemap( hx );
	_add_data( hx, s );
	
	hx_bgp* b				= _test_bgp1();
	hx_graphpattern* p		= hx_new_graphpattern( HX_GRAPHPATTERN_BGP, b );
	hx_variablebindings_iter* iter	= hx_graphpattern_execute( p, hx, s );
	
	int counter	= 0;
	while (!hx_variablebindings_iter_finished( iter )) {
		counter++;
		hx_variablebindings* vb;
		hx_variablebindings_iter_current( iter, &vb );
		
		hx_node* obj	= hx_variablebindings_node_for_binding_name( vb, map, "x" );
		ok1( hx_node_is_literal(obj) == 1 );
		char* v	= hx_node_value(obj);
		ok1( *v == 'l' );
		ok1( v[1] == '2' || v[1] == '5' );
		
		hx_node* subj	= hx_variablebindings_node_for_binding_name( vb, map, "y" );
		ok1( hx_node_is_resource(subj) == 1 );
		ok1( hx_node_cmp(subj, r1) == 0 );
		
		hx_free_variablebindings( vb, 1 );
		hx_variablebindings_iter_next( iter );
	}
	ok1( counter == 2 );
	hx_free_variablebindings_iter( iter, 1 );
	hx_free_graphpattern( p );
	hx_free_hexastore( hx, s );
	hx_free_storage_manager( s );
}

void eval_test2 ( void ) {
	fprintf( stdout, "# eval test 2\n" );
	hx_expr_debug	= 1;
	hx_storage_manager* s	= hx_new_memory_storage_manager();
	hx_hexastore* hx		= hx_new_hexastore( s );
	hx_nodemap* map			= hx_get_nodemap( hx );
	_add_data( hx, s );
	
	hx_expr* e				= hx_new_builtin_expr2( HX_EXPR_OP_EQUAL, hx_new_node_expr(v1), hx_new_node_expr(l5) );
	hx_graphpattern* b		= hx_new_graphpattern( HX_GRAPHPATTERN_BGP, _test_bgp1() );
	hx_graphpattern* p		= hx_new_graphpattern( HX_GRAPHPATTERN_FILTER, e, b );
	hx_variablebindings_iter* iter	= hx_graphpattern_execute( p, hx, s );
	
	int counter	= 0;
	while (!hx_variablebindings_iter_finished( iter )) {
		counter++;
		hx_variablebindings* vb;
		hx_variablebindings_iter_current( iter, &vb );
		
		hx_node* obj	= hx_variablebindings_node_for_binding_name( vb, map, "x" );
		ok1( hx_node_is_literal(obj) == 1 );
		char* v	= hx_node_value(obj);
		ok1( *v == 'l' );
		ok1( v[1] == '5' );
		
		hx_node* subj	= hx_variablebindings_node_for_binding_name( vb, map, "y" );
		ok1( hx_node_is_resource(subj) == 1 );
		ok1( hx_node_cmp(subj, r1) == 0 );
		
		hx_free_variablebindings( vb, 1 );
		hx_variablebindings_iter_next( iter );
	}
	ok1( counter == 1 );
	hx_free_variablebindings_iter( iter, 1 );
	hx_free_graphpattern( p );
	hx_free_hexastore( hx, s );
	hx_free_storage_manager( s );
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

void _fill_triple ( hx_triple* t, hx_node* s, hx_node* p, hx_node* o ) {
	t->subject		= s;
	t->predicate	= p;
	t->object		= o;
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

void _add_data ( hx_hexastore* hx, hx_storage_manager* s ) {
	hx_add_triple( hx, s, r2, p2, r1 );
	hx_add_triple( hx, s, r1, p1, l1 );
	hx_add_triple( hx, s, r1, p2, l2 );
	hx_add_triple( hx, s, r1, p2, l5 );
	hx_add_triple( hx, s, r1, p1, l6 );
	hx_add_triple( hx, s, r1, p1, l4 );
	hx_add_triple( hx, s, r1, p1, l3 );
}
