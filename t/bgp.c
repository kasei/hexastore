#include "hexastore.h"
#include "algebra/bgp.h"
#include "store/hexastore/hexastore.h"
#include "test/tap.h"

void bgp1_test ( void );
void bgp2_test ( void );
void bgp3_test ( void );
void bgp_vars_test1 ( void );
void bgp_vars_test2 ( void );
void bgp_varsub_test1 ( void );
void bgp_varsub_test2 ( void );
void serialization_test ( void );

hx_node *p1, *p2, *r1, *r2, *l1, *l2, *l3, *v1, *v2;
int main ( void ) {
	plan_tests(17);
	
	p1	= hx_new_node_resource( "p1" );
	p2	= hx_new_node_resource( "p2" );
	r1	= hx_new_node_resource( "r1" );
	r2	= hx_new_node_resource( "r2" );
	l1	= hx_new_node_literal( "l1" );
	l2	= hx_new_node_literal( "l2" );
	l3	= hx_new_node_literal( "l3" );
	v1	= hx_new_node_named_variable( -1, "x" );
	v2	= hx_new_node_named_variable( -2, "y" );
	
	bgp1_test();
	bgp2_test();
	bgp3_test();
	serialization_test();
	bgp_vars_test1();
	bgp_vars_test2();
	bgp_varsub_test1();
	bgp_varsub_test2();
	
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


void bgp1_test ( void ) {
	char* string;
	hx_triple* t1	= hx_new_triple( r1, p1, l1 );
	hx_bgp* b	= hx_new_bgp1( t1 );
	hx_bgp_string( b, &string );
	ok1( strcmp(string, "{<r1> <p1> \"l1\"}") == 0 );
	free( string );
	hx_free_bgp( b );
}

void bgp2_test ( void ) {
	char* string;
	{
		hx_triple* t1	= hx_new_triple( r1, p1, l1 );
		hx_triple* t2	= hx_new_triple( r2, p1, l2 );
		hx_bgp* b	= hx_new_bgp2( t1, t2 );
		hx_bgp_string( b, &string );
		ok1( strcmp(string, "{\n\t<r1> <p1> \"l1\" .\n\t<r2> <p1> \"l2\" .\n}\n") == 0 );
		free( string );
		hx_free_bgp( b );
	}
}

void bgp3_test ( void ) {
	char* string;
	{
		hx_triple* t1	= hx_new_triple( r1, p1, l1 );
		hx_triple* t2	= hx_new_triple( r2, p1, l2 );
		hx_triple* t3	= hx_new_triple( r2, p2, l1 );
		hx_triple* triples[3]	= { t1, t2, t3 };
		hx_bgp* b		= hx_new_bgp( 3, triples );
		hx_bgp_string( b, &string );
		ok1( strcmp(string, "{\n\t<r1> <p1> \"l1\" .\n\t<r2> <p1> \"l2\" ;\n\t\t<p2> \"l1\" .\n}\n") == 0 );
		free( string );
		hx_free_bgp( b );
	}
}

void serialization_test ( void ) {
	{
		char* string;
		hx_triple* t1	= hx_new_triple( r1, p1, l1 );
		hx_triple* t2	= hx_new_triple( r2, p1, l2 );
		hx_bgp* b		= hx_new_bgp2( t1, t2 );
		
		{
			hx_bgp_sse( b, &string, "\t", 0 );
			ok( strcmp( string, "(bgp\n\t(triple <r1> <p1> \"l1\")\n\t(triple <r2> <p1> \"l2\")\n)\n" ) == 0, "expected 2-bgp sse, tab indent" );
			free( string );
		}

		{
			hx_bgp_sse( b, &string, "  ", 2 );
			ok( strcmp( string, "(bgp\n      (triple <r1> <p1> \"l1\")\n      (triple <r2> <p1> \"l2\")\n    )\n" ) == 0, "expected 2-bgp sse, space indent" );
			free( string );
		}
		
		hx_free_bgp( b );
	}
}

void bgp_vars_test1 ( void ) {
	{
		hx_triple* t1	= hx_new_triple( r1, p1, l1 );
		hx_bgp* b	= hx_new_bgp1( t1 );
		int var_count	= hx_bgp_variables( b, NULL );
		ok1( var_count == 0 );
		hx_free_bgp( b );
	}
	
	{
		hx_triple* t1	= hx_new_triple( v1, p1, l1 );
		hx_bgp* b		= hx_new_bgp1( t1 );
		int var_count	= hx_bgp_variables( b, NULL );
		ok1( var_count == 1 );
		hx_free_bgp( b );
	}
	
	{
		hx_triple* t1	= hx_new_triple( v1, v2, l1 );
		hx_bgp* b		= hx_new_bgp1( t1 );
		int var_count	= hx_bgp_variables( b, NULL );
		ok1( var_count == 2 );
		hx_free_bgp( b );
	}
	
	{
		hx_triple* t1	= hx_new_triple( v1, v2, v1 );
		hx_bgp* b		= hx_new_bgp1( t1 );
		int var_count	= hx_bgp_variables( b, NULL );
		ok1( var_count == 2 );
		hx_free_bgp( b );
	}
}

void bgp_vars_test2 ( void ) {
	{
		hx_node** v;
		hx_triple* t1	= hx_new_triple( r1, v1, l1 );
		hx_bgp* b	= hx_new_bgp1( t1 );
		int var_count	= hx_bgp_variables( b, &v );
		ok1( var_count == 1 );
		hx_node* n	= v[0];
		ok1( hx_node_cmp(n,v1) == 0 );
		hx_free_node(n);
		free(v);
		hx_free_bgp( b );
	}

	{
		hx_node** v;
		hx_triple* t1	= hx_new_triple( v2, v1, v2 );
		hx_bgp* b	= hx_new_bgp1( t1 );
		int var_count	= hx_bgp_variables( b, &v );
		ok1( var_count == 2 );
		
		hx_node* x	= v[0];
		ok1( hx_node_cmp(x,v1) == 0 );

		hx_node* y	= v[1];
		ok1( hx_node_cmp(y,v2) == 0 );
		
		hx_free_node(x);
		hx_free_node(y);
		free(v);
		hx_free_bgp( b );
	}
}

void bgp_varsub_test1 ( void ) {
	{
		hx_hexastore* hx		= hx_new_hexastore( NULL );
		hx_nodemap* map			= hx_store_hexastore_get_nodemap( hx->store );
		hx_node_id p1_id		= hx_nodemap_add_node( map, p1 );
		hx_node_id p2_id		= hx_nodemap_add_node( map, p2 );
		
		hx_triple* t1	= hx_new_triple( r1, v1, l1 );
		hx_bgp* bgp	= hx_new_bgp1( t1 );
		
		{
			char* names[1]			= { "x" };
			hx_node_id* nodes		= (hx_node_id*) calloc( 1, sizeof( hx_node_id ) );
			nodes[0]				= p1_id;
			hx_variablebindings* b	= hx_new_variablebindings ( 1, names, nodes );
			
			hx_bgp* c	= hx_bgp_substitute_variables( bgp, b, hx->store );
			char* string;
			hx_bgp_sse( c, &string, "  ", 0 );
			ok( strcmp( string, "(bgp\n  (triple <r1> <p1> \"l1\")\n)\n" ) == 0, "expected bgp after varsub" );
			free( string );
			hx_free_bgp( c );
			hx_free_variablebindings(b);
		}
		
		{
			char* names[1]			= { "x" };
			hx_node_id* nodes		= (hx_node_id*) calloc( 1, sizeof( hx_node_id ) );
			nodes[0]				= p2_id;
			hx_variablebindings* b	= hx_new_variablebindings ( 1, names, nodes );
			
			hx_bgp* c	= hx_bgp_substitute_variables( bgp, b, hx->store );
			char* string;
			hx_bgp_sse( c, &string, "  ", 0 );
			ok( strcmp( string, "(bgp\n  (triple <r1> <p2> \"l1\")\n)\n" ) == 0, "expected bgp after varsub" );
			free( string );
			hx_free_bgp( c );
			hx_free_variablebindings(b);
		}
		
		hx_free_nodemap( map );
		hx_free_bgp( bgp );
	}
}

void bgp_varsub_test2 ( void ) {
	{
		hx_hexastore* hx		= hx_new_hexastore( NULL );
		hx_nodemap* map			= hx_store_hexastore_get_nodemap( hx->store );
		hx_node_id p1_id		= hx_nodemap_add_node( map, p1 );
		hx_node_id p2_id		= hx_nodemap_add_node( map, p2 );
		
		hx_triple* t1	= hx_new_triple( v1, v1, v2 );
		hx_bgp* bgp	= hx_new_bgp1( t1 );
		
		{
			char* names[2]			= { "y", "x" };
			hx_node_id* nodes		= (hx_node_id*) calloc( 2, sizeof( hx_node_id ) );
			nodes[0]				= p2_id;
			nodes[1]				= p1_id;
			hx_variablebindings* b	= hx_new_variablebindings( 2, names, nodes );
			
			hx_bgp* c	= hx_bgp_substitute_variables( bgp, b, hx->store );
			char* string;
			hx_bgp_sse( c, &string, "  ", 0 );
			ok( strcmp( string, "(bgp\n  (triple <p1> <p1> <p2>)\n)\n" ) == 0, "expected bgp after varsub" );
			free( string );
			hx_free_bgp( c );
			hx_free_variablebindings(b);
		}
		
		hx_free_nodemap( map );
		hx_free_bgp( bgp );
	}
}

