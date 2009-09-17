#include <unistd.h>
#include "mentok.h"
#include "misc/nodemap.h"
#include "engine/hashjoin.h"
#include "engine/mergejoin.h"
#include "engine/nestedloopjoin.h"
#include "rdf/node.h"
#include "test/tap.h"
#include "algebra/bgp.h"
#include "engine/bgp.h"
#include "store/hexastore/hexastore.h"

void _add_data ( hx_hexastore* hx );
void _debug_node ( char* h, hx_node* node );
hx_variablebindings_iter* _get_triples ( hx_hexastore* hx, int sort );

hx_node* p1;
hx_node* p2;
hx_node* r1;
hx_node* r2;
hx_node* l1;
hx_node* l2;

void test_path_join ( hx_variablebindings_iter* join_constructor( hx_variablebindings_iter*, hx_variablebindings_iter* ) );
void test_cartesian_join ( hx_variablebindings_iter* join_constructor( hx_variablebindings_iter*, hx_variablebindings_iter* ), int expect );
void test_left_join ( hx_variablebindings_iter* join_constructor( hx_variablebindings_iter*, hx_variablebindings_iter*, int ) );

int main ( void ) {
	plan_tests((3*10) + (14 + (2+2)) + 7);
	p1	= hx_new_node_resource( "p1" );
	p2	= hx_new_node_resource( "p2" );
	r1	= hx_new_node_resource( "r1" );
	r2	= hx_new_node_resource( "r2" );
	l1	= hx_new_node_literal( "l1" );
	l2	= hx_new_node_literal( "l2" );
	
	test_path_join( hx_new_mergejoin_iter );
	test_path_join( hx_new_nestedloopjoin_iter );
	test_path_join( hx_new_hashjoin_iter );
	
	test_cartesian_join( hx_new_nestedloopjoin_iter, 1 );	// the 1 signifies that we expect the join to work and produce 1 result
	test_cartesian_join( hx_new_mergejoin_iter, 0 );		// the 0 signifies that we don't expect the join to work, since mergejoin isn't implemented for the cartesian join case (with no shared variables)
	test_cartesian_join( hx_new_hashjoin_iter, 0 );
	
	test_left_join( hx_new_nestedloopjoin_iter2 );
	
	return exit_status();
}

void test_cartesian_join ( hx_variablebindings_iter* join_constructor( hx_variablebindings_iter*, hx_variablebindings_iter* ), int expect ) {
	fprintf( stdout, "# test_cartesian_join\n" );
	hx_hexastore* hx	= hx_new_hexastore( NULL );
	hx_nodemap* map		= hx_store_hexastore_get_nodemap( hx->store );
	_add_data( hx );
// <r1> :p1 <r2>
// <r2> :p1 <r1>
// <r2> :p2 "l2"
// <r1> :p2 "l1"
	
	int size;
	char* name;
	char* string;
	hx_variablebindings* b;
// 	hx_node* v1		= hx_new_named_variable( hx );
// 	hx_node* v2		= hx_new_named_variable( hx );
// 	hx_node* v3		= hx_new_named_variable( hx );
// 	hx_node* v4		= hx_new_named_variable( hx );
	
	{
		hx_node* v1		= hx_new_named_variable( hx, "x" );
		hx_node* v2		= hx_new_named_variable( hx, "y" );
		
		hx_triple* ta	= hx_new_triple( r1, p1, v1 );
		hx_variablebindings_iter* iter_a	= hx_new_variablebindings_iter_for_triple( hx, ta, HX_OBJECT );
		
		hx_triple* tb	= hx_new_triple( r2, p1, v2 );
		hx_variablebindings_iter* iter_b	= hx_new_variablebindings_iter_for_triple( hx, tb, HX_SUBJECT );
		
		hx_variablebindings_iter* iter	= join_constructor( iter_a, iter_b );
	
		if (expect) {
			ok1( !hx_variablebindings_iter_finished( iter ) );
			hx_variablebindings_iter_current( iter, &b );
			
			// expect 2 variable bindings
			size	= hx_variablebindings_size( b );
			ok1( size == 2 );
			
			{
				// expect the first variable binding to be "x"
				name	= hx_variablebindings_name_for_binding( b, 0 );
				hx_store_variablebindings_string( hx->store, b, &string );
				free( string );
				ok1( strcmp( name, "x" ) == 0);
			}
			{
				// expect the second variable binding to be "y"
				name	= hx_variablebindings_name_for_binding( b, 1 );
				hx_store_variablebindings_string( hx->store, b, &string );
				free( string );
				ok1( strcmp( name, "y" ) == 0);
			}
			
			{
				hx_node_id fid	= hx_variablebindings_node_id_for_binding_name( b, "x" );
				hx_node* x		= hx_nodemap_get_node( map, fid );
				hx_node_id tid	= hx_variablebindings_node_id_for_binding_name( b, "y" );
				hx_node* y		= hx_nodemap_get_node( map, tid );
		
				ok1( hx_node_cmp( x, r2 ) == 0 );
				ok1( hx_node_cmp( y, r1 ) == 0 );
			}
			hx_variablebindings_iter_next( iter );
			ok1( hx_variablebindings_iter_finished( iter ) );
			
			hx_free_variablebindings_iter( iter );
		} else {
			// expect the join constructor to fail and return NULL
			ok1( iter == NULL );
		}
		
		hx_free_node(v1);
		hx_free_node(v2);
	}
	
	{
		hx_node* v1		= hx_new_named_variable( hx, "a" );
		hx_node* v2		= hx_new_named_variable( hx, "b" );
		hx_node* v3		= hx_new_named_variable( hx, "c" );
		hx_node* v4		= hx_new_named_variable( hx, "d" );
		
		hx_triple* ta	= hx_new_triple( v1, p1, v2 );
		hx_variablebindings_iter* iter_a	= hx_new_variablebindings_iter_for_triple( hx, ta, HX_OBJECT );
		
		hx_triple* tb	= hx_new_triple( v3, p2, v4 );
		hx_variablebindings_iter* iter_b	= hx_new_variablebindings_iter_for_triple( hx, tb, HX_SUBJECT );
		
		hx_variablebindings_iter* iter	= join_constructor( iter_a, iter_b );
	
		if (expect) {
			ok1( !hx_variablebindings_iter_finished( iter ) );
			hx_variablebindings_iter_current( iter, &b );
			
			// expect 4 variable bindings
			size	= hx_variablebindings_size( b );
			ok1( size == 4 );
			
			{
				// expect the first variable binding to be "a"
				name	= hx_variablebindings_name_for_binding( b, 0 );
				hx_store_variablebindings_string( hx->store, b, &string );
				free( string );
				ok1( strcmp( name, "a" ) == 0);
			}
			{
				// expect the second variable binding to be "b"
				name	= hx_variablebindings_name_for_binding( b, 1 );
				hx_store_variablebindings_string( hx->store, b, &string );
				free( string );
				ok1( strcmp( name, "b" ) == 0);
			}
			
			{
				// expect the third variable binding to be "c"
				name	= hx_variablebindings_name_for_binding( b, 2 );
				hx_store_variablebindings_string( hx->store, b, &string );
				free( string );
				ok1( strcmp( name, "c" ) == 0);
			}
			{
				// expect the fourth variable binding to be "d"
				name	= hx_variablebindings_name_for_binding( b, 3 );
				hx_store_variablebindings_string( hx->store, b, &string );
				free( string );
				ok1( strcmp( name, "d" ) == 0);
			}
			
			int counter	= 0;
			while (!hx_variablebindings_iter_finished( iter )) {
				counter++;
				hx_variablebindings_iter_next( iter );
			}
			ok1( counter == 4 );
			hx_free_variablebindings_iter( iter );
		} else {
			// expect the join constructor to fail and return NULL
			ok1( iter == NULL );
		}
		
		hx_free_node(v1);
		hx_free_node(v2);
		hx_free_node(v3);
		hx_free_node(v4);
	}
	
	hx_free_hexastore( hx );
}

void test_path_join ( hx_variablebindings_iter* join_constructor( hx_variablebindings_iter*, hx_variablebindings_iter* ) ) {
	fprintf( stdout, "# test_path_join\n" );
	hx_hexastore* hx	= hx_new_hexastore( NULL );
	hx_nodemap* map		= hx_store_hexastore_get_nodemap( hx->store );
	_add_data( hx );
// <r1> :p1 <r2>
// <r2> :p1 <r1>
// <r2> :p2 "l2"
// <r1> :p2 "l1"
	
	int size;
	char* name;
	char* string;
	hx_variablebindings* b;
	hx_node* v1		= hx_new_named_variable( hx, "from" );
	hx_node* v2		= hx_new_named_variable( hx, "neighbor" );
	hx_node* v3		= hx_new_named_variable( hx, "to" );
	
	hx_triple* ta	= hx_new_triple( v1, p1, v2 );
	hx_variablebindings_iter* iter_a	= hx_new_variablebindings_iter_for_triple( hx, ta, HX_OBJECT );
	
	hx_triple* tb	= hx_new_triple( v2, p1, v3 );
	hx_variablebindings_iter* iter_b	= hx_new_variablebindings_iter_for_triple( hx, tb, HX_SUBJECT );
	
	hx_variablebindings_iter* iter	= join_constructor( iter_a, iter_b );
	
	ok1( !hx_variablebindings_iter_finished( iter ) );
	hx_variablebindings_iter_current( iter, &b );
	
	// expect 3 variable bindings for the three triple nodes
	size	= hx_variablebindings_size( b );
	ok1( size == 3 );

	{
		// expect the first variable binding to be "from"
		name	= hx_variablebindings_name_for_binding( b, 0 );
		hx_store_variablebindings_string( hx->store, b, &string );
		free( string );
		ok1( strcmp( name, "from" ) == 0);
	}
	{
		// expect the first variable binding to be "from"
		name	= hx_variablebindings_name_for_binding( b, 2 );
		hx_store_variablebindings_string( hx->store, b, &string );
		free( string );
		ok1( strcmp( name, "to" ) == 0);
	}
	
	{
		hx_node_id fid	= hx_variablebindings_node_id_for_binding_name( b, "from" );
		hx_node* from	= hx_nodemap_get_node( map, fid );
		hx_node_id tid	= hx_variablebindings_node_id_for_binding_name( b, "to" );
		hx_node* to		= hx_nodemap_get_node( map, tid );

		ok1( hx_node_cmp( from, r2 ) == 0 );
		ok1( hx_node_cmp( to, r2 ) == 0 );
	}
	hx_variablebindings_iter_next( iter );
	ok1( !hx_variablebindings_iter_finished( iter ) );
	hx_variablebindings_iter_current( iter, &b );
	{
		hx_node_id fid	= hx_variablebindings_node_id_for_binding_name( b, "from" );
		hx_node* from	= hx_nodemap_get_node( map, fid );
		hx_node_id tid	= hx_variablebindings_node_id_for_binding_name( b, "to" );
		hx_node* to		= hx_nodemap_get_node( map, tid );
		
// 		_debug_node( "from: ", from );
// 		_debug_node( "to: ", to );
		
		ok1( hx_node_cmp( from, r1 ) == 0 );
		ok1( hx_node_cmp( to, r1 ) == 0 );
	}
	
	hx_variablebindings_iter_next( iter );
	ok1( hx_variablebindings_iter_finished( iter ) );
	
	hx_free_variablebindings_iter( iter );
	hx_free_hexastore( hx );
}

void test_left_join ( hx_variablebindings_iter* join_constructor( hx_variablebindings_iter*, hx_variablebindings_iter*, int ) ) {
	fprintf( stdout, "# test_left_join\n" );
	hx_hexastore* hx	= hx_new_hexastore( NULL );
	hx_nodemap* map		= hx_store_hexastore_get_nodemap( hx->store );
	_add_data( hx );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );
	
	{
		hx_bgp* lhs	= hx_bgp_parse_string("{ ?x <p1> ?y }");
		hx_bgp* rhs	= hx_bgp_parse_string("{ ?x <p2> \"l1\" }");
		hx_variablebindings_iter* lhsi	= hx_bgp_execute( ctx, lhs );
		hx_variablebindings_iter* rhsi	= hx_bgp_execute( ctx, rhs );
		
		int counter	= 0;
		hx_variablebindings_iter* iter	= join_constructor( lhsi, rhsi, 1 );
		while (!hx_variablebindings_iter_finished(iter)) {
			hx_variablebindings* b;
			hx_variablebindings_iter_current( iter, &b );
			int size	= hx_variablebindings_size( b );
			ok1( size == 2 );
			counter++;
			
			hx_free_variablebindings( b );
			hx_variablebindings_iter_next( iter );
		}
	
		ok1( counter == 2 );
		
		hx_free_variablebindings_iter( iter );
		hx_free_bgp( lhs );
		hx_free_bgp( rhs );
	}
	
	{
		hx_bgp* lhs	= hx_bgp_parse_string("{ ?x <p1> _:a }");
		hx_bgp* rhs	= hx_bgp_parse_string("{ ?x ?p \"l1\" }");
		hx_variablebindings_iter* lhsi	= hx_bgp_execute( ctx, lhs );
		hx_variablebindings_iter* rhsi	= hx_bgp_execute( ctx, rhs );
		
		int* sizes	= calloc( 3, sizeof( int ) );
		int counter	= 0;
		hx_variablebindings_iter* iter	= join_constructor( lhsi, rhsi, 1 );
		while (!hx_variablebindings_iter_finished(iter)) {
			hx_variablebindings* b;
			hx_variablebindings_iter_current( iter, &b );
			int size	= hx_variablebindings_size( b );
			sizes[size]++;
			counter++;
			
			hx_free_variablebindings( b );
			hx_variablebindings_iter_next( iter );
		}
		
		ok1( counter == 2 );
		ok1( sizes[0] == 0 );
		ok1( sizes[1] == 1 );
		ok1( sizes[2] == 1 );
		
		hx_free_execution_context( ctx );
		hx_free_variablebindings_iter( iter );
		hx_free_bgp( lhs );
		hx_free_bgp( rhs );
	}
	
	
	
	hx_free_hexastore( hx );
}

hx_variablebindings_iter* _get_triples ( hx_hexastore* hx, int sort ) {
	hx_node* v1	= hx_new_node_named_variable( -1, "subj" );
	hx_node* v2	= hx_new_node_named_variable( -2, "pred" );
	hx_node* v3	= hx_new_node_named_variable( -3, "obj" );
	hx_triple* t	= hx_new_triple( v1, v2, v3 );
	hx_variablebindings_iter* iter	=  hx_new_variablebindings_iter_for_triple( hx, t, HX_OBJECT );
	hx_free_triple(t);
	hx_free_node(v1);
	hx_free_node(v2);
	hx_free_node(v3);
	return iter;
}

void _add_data ( hx_hexastore* hx ) {
	hx_add_triple( hx, r1, p1, r2 );
	hx_add_triple( hx, r2, p1, r1 );
	hx_add_triple( hx, r2, p2, l2 );
	hx_add_triple( hx, r1, p2, l1 );
}

void _debug_node ( char* h, hx_node* node ) {
	char* string;
	hx_node_string( node, &string );
	fprintf( stderr, "%s %s\n", h, string );
}
