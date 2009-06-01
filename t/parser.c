#include "hexastore.h"
#include "graphpattern.h"
#include "tap.h"

extern hx_bgp* parse_bgp_query ( void );
extern hx_bgp* parse_bgp_query_string ( char* );

void serialization_test ( void );

hx_node *p1, *p2, *r1, *r2, *l1, *l2, *l3;
int main ( void ) {
	plan_tests(5);
	
	p1	= hx_new_node_resource( "p1" );
	p2	= hx_new_node_resource( "p2" );
	r1	= hx_new_node_resource( "r1" );
	r2	= hx_new_node_resource( "r2" );
	l1	= hx_new_node_literal( "l1" );
	l2	= hx_new_node_literal( "l2" );
	l3	= hx_new_node_literal( "l3" );
	
	serialization_test();
	
	hx_free_node( p1 );
	hx_free_node( p2 );
	hx_free_node( r1 );
	hx_free_node( r2 );
	hx_free_node( l1 );
	hx_free_node( l2 );
	
	return exit_status();
}


void serialization_test ( void ) {
	{
		hx_bgp* b	= parse_bgp_query_string( "{ <r1> <p1> \"l1\" . <r2> <p1> \"l2\" }" );
		{
			char* string;
			hx_bgp_sse( b, &string, "\t", 0 );
			ok( strcmp( string, "(bgp\n\t(triple <r1> <p1> \"l1\")\n\t(triple <r2> <p1> \"l2\")\n)\n" ) == 0, "expected 2-bgp sse, tab indent" );
			free( string );
		}
		hx_free_bgp( b );
	}

	{
		hx_bgp* b	= parse_bgp_query_string( "PREFIX foaf: <http://xmlns.com/foaf/0.1/> { ?p foaf:name ?name }" );
		{
			char* string;
			hx_bgp_sse( b, &string, "  ", 0 );
			ok( strcmp( string, "(bgp\n  (triple ?p <http://xmlns.com/foaf/0.1/name> ?name)\n)\n" ) == 0, "expected 1-bgp sse, space indent" );
			free( string );
		}
		hx_free_bgp( b );
	}

	{
		hx_graphpattern* g	= parse_query_string( "PREFIX foaf: <http://xmlns.com/foaf/0.1/> { ?p foaf:name ?name }" );
		{
			char* string;
			hx_graphpattern_sse( g, &string, "  ", 0 );
			ok( strcmp( string, "(ggp\n  (bgp\n    (triple ?p <http://xmlns.com/foaf/0.1/name> ?name)\n  )\n)\n" ) == 0, "expected ggp sse" );
			free( string );
		}
		hx_free_graphpattern( g );
	}

	{
		hx_graphpattern* g	= parse_query_string( "PREFIX foaf: <http://xmlns.com/foaf/0.1/> { ?p foaf:name ?name }" );
		{
			char* string;
			hx_graphpattern_sse( g, &string, "  ", 2 );
			ok( strcmp( string, "(ggp\n      (bgp\n        (triple ?p <http://xmlns.com/foaf/0.1/name> ?name)\n      )\n    )\n" ) == 0, "expected ggp sse" );
			free( string );
		}
		hx_free_graphpattern( g );
	}

	{
		hx_graphpattern* g	= parse_query_string( "PREFIX foaf: <http://xmlns.com/foaf/0.1/> { ?p a foaf:Person ; foaf:name ?name . FILTER isBLANK(?p) }" );
		{
			char* string;
			hx_graphpattern_sse( g, &string, "  ", 0 );
			ok( strcmp( string, "(filter\n  (sparql:isblank ?p)\n  (ggp\n    (bgp\n      (triple ?p <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://xmlns.com/foaf/0.1/Person>)\n      (triple ?p <http://xmlns.com/foaf/0.1/name> ?name)\n    )\n  )\n)\n" ) == 0, "expected ggp with filter sse" );
			free( string );
		}
		hx_free_graphpattern( g );
	}
}
