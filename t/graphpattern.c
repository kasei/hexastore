#include "hexastore.h"
#include "graphpattern.h"
#include "tap.h"

void _fill_triple ( hx_triple* t, hx_node* s, hx_node* p, hx_node* o );
hx_bgp* _test_bgp1 ( void );
hx_bgp* _test_bgp2 ( void );

void serialization_test ( void );

hx_node *p1, *p2, *r1, *r2, *l1, *l2, *l3, *v1, *v2;
int main ( void ) {
	plan_tests(8);
	
	p1	= hx_new_node_resource( "p1" );
	p2	= hx_new_node_resource( "p2" );
	r1	= hx_new_node_resource( "r1" );
	r2	= hx_new_node_resource( "r2" );
	l1	= hx_new_node_literal( "l1" );
	l2	= hx_new_node_literal( "l2" );
	l3	= hx_new_node_literal( "l3" );
	v1	= hx_new_node_named_variable( -1, "x" );
	v2	= hx_new_node_named_variable( -2, "y" );
	
	serialization_test();
	
	hx_free_node( p1 );
	hx_free_node( p2 );
	hx_free_node( r1 );
	hx_free_node( r2 );
	hx_free_node( l1 );
	hx_free_node( l2 );
	hx_free_node( v1 );
	hx_free_node( v2 );
	
	return exit_status();
}


void serialization_test ( void ) {
	{
		hx_bgp* b1	= _test_bgp1();
		hx_graphpattern* p	= hx_new_graphpattern( HX_GRAPHPATTERN_BGP, b1 );
		ok1( p != NULL );
		
		char* string;
		hx_graphpattern_sse( p, &string, "  ", 0 );
		char* expect	= "(bgp\n  (triple <r1> <p1> \"l1\")\n  (triple <r1> <p2> ?x)\n)\n";
		ok1( strcmp(string, expect) == 0 );
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
		char* expect	= "(ggp\n  (bgp\n    (triple <r1> <p1> \"l1\")\n    (triple <r1> <p2> ?x)\n  )\n  (bgp\n    (triple <r2> <p2> ?x)\n    (triple <r2> <p2> ?y)\n  )\n)\n";
		ok1( strcmp(string, expect) == 0 );
		free( string );
		hx_free_graphpattern( p );
	}
	
	{
		hx_graphpattern* b	= hx_new_graphpattern( HX_GRAPHPATTERN_BGP, _test_bgp1() );
		hx_graphpattern* p	= hx_new_graphpattern( HX_GRAPHPATTERN_GRAPH, hx_node_copy(v2), b );
		ok1( p != NULL );
		
		char* string;
		hx_graphpattern_sse( p, &string, "  ", 0 );
		char* expect	= "(named-graph ?y\n  (bgp\n    (triple <r1> <p1> \"l1\")\n    (triple <r1> <p2> ?x)\n  )\n)\n";
		ok1( strcmp(string, expect) == 0 );
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
		char* expect	= "(filter\n  (sparql:str ?x)\n  (bgp\n    (triple <r1> <p1> \"l1\")\n    (triple <r1> <p2> ?x)\n  )\n)\n";
		ok1( strcmp(string, expect) == 0 );
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
	hx_triple* t1	= hx_new_triple( r1, p1, l1 );
	hx_triple* t2	= hx_new_triple( r1, p2, v1 );
	hx_bgp* b	= hx_new_bgp2( t1, t2 );
	return b;
}

hx_bgp* _test_bgp2 ( void ) {
	hx_triple* t1	= hx_new_triple( r2, p2, v1 );
	hx_triple* t2	= hx_new_triple( r2, p2, v2 );
	hx_bgp* b		= hx_new_bgp2( t1, t2 );
	return b;
}

