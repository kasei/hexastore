#include <unistd.h>
#include "hexastore.h"
#include "nodemap.h"
#include "mergejoin.h"
#include "nestedloopjoin.h"
#include "node.h"
#include "storage.h"
#include "tap.h"

void _add_data ( hx_hexastore* hx, hx_storage_manager* s );
void _debug_node ( char* h, hx_node* node );
hx_variablebindings_iter* _get_triples ( hx_hexastore* hx, hx_storage_manager* s, int sort );

hx_node* p1;
hx_node* p2;
hx_node* r1;
hx_node* r2;
hx_node* l1;
hx_node* l2;

void test_path_join ( hx_variablebindings_iter* join_constructor( hx_variablebindings_iter*, hx_variablebindings_iter* ) );
void test_cartesian_join ( hx_variablebindings_iter* join_constructor( hx_variablebindings_iter*, hx_variablebindings_iter* ), int expect );

int main ( void ) {
	plan_tests(20 + 7 + 1);
	p1	= hx_new_node_resource( "p1" );
	p2	= hx_new_node_resource( "p2" );
	r1	= hx_new_node_resource( "r1" );
	r2	= hx_new_node_resource( "r2" );
	l1	= hx_new_node_literal( "l1" );
	l2	= hx_new_node_literal( "l2" );
	
	test_path_join( hx_new_mergejoin_iter );
	test_path_join( hx_new_nestedloopjoin_iter );
	
	test_cartesian_join( hx_new_nestedloopjoin_iter, 1 );	// the 1 signifies that we expect the join to work and produce 1 result
	test_cartesian_join( hx_new_mergejoin_iter, 0 );		// the 0 signifies that we don't expect the join to work, since mergejoin isn't implemented for the cartesian join case (with no shared variables)
	
	return exit_status();
}

void test_cartesian_join ( hx_variablebindings_iter* join_constructor( hx_variablebindings_iter*, hx_variablebindings_iter* ), int expect ) {
	fprintf( stdout, "# test_cartesian_join\n" );
	hx_storage_manager* s	= hx_new_memory_storage_manager();
	hx_hexastore* hx	= hx_new_hexastore( s );
	hx_nodemap* map		= hx_get_nodemap( hx );
	_add_data( hx, s );
// <r1> :p1 <r2>
// <r2> :p1 <r1>
// <r2> :p2 "l2"
// <r1> :p2 "l1"
	
	int size;
	char* name;
	char* string;
	hx_node_id nid;
	hx_variablebindings* b;
	hx_node* v1		= hx_new_variable( hx );
	hx_node* v2		= hx_new_variable( hx );
	hx_node* v3		= hx_new_variable( hx );
	hx_node* v4		= hx_new_variable( hx );
	
	{
		hx_index_iter* titer_a	= hx_get_statements( hx, s, r1, p1, v1, HX_OBJECT );
		hx_variablebindings_iter* iter_a	= hx_new_iter_variablebindings( titer_a, s, NULL, NULL, "x", 0 );
		
		hx_index_iter* titer_b	= hx_get_statements( hx, s, r2, p1, v2, HX_SUBJECT );
		hx_variablebindings_iter* iter_b	= hx_new_iter_variablebindings( titer_b, s, NULL, NULL, "y", 0 );
		
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
				hx_variablebindings_string( b, map, &string );
				free( string );
				ok1( strcmp( name, "x" ) == 0);
			}
			{
				// expect the second variable binding to be "y"
				name	= hx_variablebindings_name_for_binding( b, 1 );
				hx_variablebindings_string( b, map, &string );
				free( string );
				ok1( strcmp( name, "y" ) == 0);
			}
			
			{
				hx_node_id fid	= hx_variablebindings_node_id_for_binding( b, 0 );
				hx_node* x		= hx_nodemap_get_node( map, fid );
				hx_node_id tid	= hx_variablebindings_node_id_for_binding( b, 1 );
				hx_node* y		= hx_nodemap_get_node( map, tid );
		
				ok1( hx_node_cmp( x, r2 ) == 0 );
				ok1( hx_node_cmp( y, r1 ) == 0 );
			}
			hx_variablebindings_iter_next( iter );
			ok1( hx_variablebindings_iter_finished( iter ) );
			
			hx_free_variablebindings_iter( iter, 1 );
		} else {
			// expect the join constructor to fail and return NULL
			ok1( iter == NULL );
		}
	}
	
	{
		hx_index_iter* titer_a	= hx_get_statements( hx, s, v1, p1, v2, HX_OBJECT );
		hx_variablebindings_iter* iter_a	= hx_new_iter_variablebindings( titer_a, s, "a", NULL, "b", 0 );
		
		hx_index_iter* titer_b	= hx_get_statements( hx, s, v3, p2, v4, HX_SUBJECT );
		hx_variablebindings_iter* iter_b	= hx_new_iter_variablebindings( titer_b, s, "c", NULL, "d", 0 );
		
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
				hx_variablebindings_string( b, map, &string );
				free( string );
				ok1( strcmp( name, "a" ) == 0);
			}
			{
				// expect the second variable binding to be "b"
				name	= hx_variablebindings_name_for_binding( b, 1 );
				hx_variablebindings_string( b, map, &string );
				free( string );
				ok1( strcmp( name, "b" ) == 0);
			}
			
			{
				// expect the third variable binding to be "c"
				name	= hx_variablebindings_name_for_binding( b, 2 );
				hx_variablebindings_string( b, map, &string );
				free( string );
				ok1( strcmp( name, "c" ) == 0);
			}
			{
				// expect the fourth variable binding to be "d"
				name	= hx_variablebindings_name_for_binding( b, 3 );
				hx_variablebindings_string( b, map, &string );
				free( string );
				ok1( strcmp( name, "d" ) == 0);
			}
			
			int counter	= 0;
			while (!hx_variablebindings_iter_finished( iter )) {
				counter++;
				hx_variablebindings_iter_next( iter );
			}
			ok1( counter == 4 );
			hx_free_variablebindings_iter( iter, 1 );
		} else {
			// expect the join constructor to fail and return NULL
			ok1( iter == NULL );
		}
	}
	
	hx_free_hexastore( hx, s );
	hx_free_storage_manager( s );
}

void test_path_join ( hx_variablebindings_iter* join_constructor( hx_variablebindings_iter*, hx_variablebindings_iter* ) ) {
	fprintf( stdout, "# test_path_join\n" );
	hx_storage_manager* s	= hx_new_memory_storage_manager();
	hx_hexastore* hx	= hx_new_hexastore( s );
	hx_nodemap* map		= hx_get_nodemap( hx );
	_add_data( hx, s );
// <r1> :p1 <r2>
// <r2> :p1 <r1>
// <r2> :p2 "l2"
// <r1> :p2 "l1"
	
	int size;
	char* name;
	char* string;
	hx_node_id nid;
	hx_variablebindings* b;
	hx_node* v1		= hx_new_variable( hx );
	hx_node* v2		= hx_new_variable( hx );
	hx_node* v3		= hx_new_variable( hx );
	
	hx_index_iter* titer_a	= hx_get_statements( hx, s, v1, p1, v2, HX_OBJECT );
	hx_variablebindings_iter* iter_a	= hx_new_iter_variablebindings( titer_a, s, "from", NULL, "neighbor", 0 );
	
	hx_index_iter* titer_b	= hx_get_statements( hx, s, v2, p1, v3, HX_SUBJECT );
	hx_variablebindings_iter* iter_b	= hx_new_iter_variablebindings( titer_b, s, "neighbor", NULL, "to", 0 );
	
	hx_variablebindings_iter* iter	= join_constructor( iter_a, iter_b );
	
	ok1( !hx_variablebindings_iter_finished( iter ) );
	hx_variablebindings_iter_current( iter, &b );
	
	// expect 3 variable bindings for the three triple nodes
	size	= hx_variablebindings_size( b );
	ok1( size == 3 );

	{
		// expect the first variable binding to be "from"
		name	= hx_variablebindings_name_for_binding( b, 0 );
		hx_variablebindings_string( b, map, &string );
		free( string );
		ok1( strcmp( name, "from" ) == 0);
	}
	{
		// expect the first variable binding to be "from"
		name	= hx_variablebindings_name_for_binding( b, 2 );
		hx_variablebindings_string( b, map, &string );
		free( string );
		ok1( strcmp( name, "to" ) == 0);
	}
	
	{
		hx_node_id fid	= hx_variablebindings_node_id_for_binding( b, 0 );
		hx_node* from	= hx_nodemap_get_node( map, fid );
		hx_node_id tid	= hx_variablebindings_node_id_for_binding( b, 2 );
		hx_node* to		= hx_nodemap_get_node( map, tid );

		ok1( hx_node_cmp( from, r2 ) == 0 );
		ok1( hx_node_cmp( to, r2 ) == 0 );
	}
	hx_variablebindings_iter_next( iter );
	ok1( !hx_variablebindings_iter_finished( iter ) );
	hx_variablebindings_iter_current( iter, &b );
	{
		hx_node_id fid	= hx_variablebindings_node_id_for_binding( b, 0 );
		hx_node* from	= hx_nodemap_get_node( map, fid );
		hx_node_id tid	= hx_variablebindings_node_id_for_binding( b, 2 );
		hx_node* to		= hx_nodemap_get_node( map, tid );
		
// 		_debug_node( "from: ", from );
// 		_debug_node( "to: ", to );
		
		ok1( hx_node_cmp( from, r1 ) == 0 );
		ok1( hx_node_cmp( to, r1 ) == 0 );
	}
	
	hx_variablebindings_iter_next( iter );
	ok1( hx_variablebindings_iter_finished( iter ) );
	
	hx_free_variablebindings_iter( iter, 1 );
	hx_free_hexastore( hx, s );
	hx_free_storage_manager( s );
}

hx_variablebindings_iter* _get_triples ( hx_hexastore* hx, hx_storage_manager* s, int sort ) {
	hx_node* v1	= hx_new_node_variable( -1 );
	hx_node* v2	= hx_new_node_variable( -2 );
	hx_node* v3	= hx_new_node_variable( -3 );
	
	hx_index_iter* titer	= hx_get_statements( hx, s, v1, v2, v3, HX_OBJECT );
	hx_variablebindings_iter* iter	= hx_new_iter_variablebindings( titer, s, "subj", "pred", "obj", 0 );
	return iter;
}

void _add_data ( hx_hexastore* hx, hx_storage_manager* s ) {
	hx_add_triple( hx, s, r1, p1, r2 );
	hx_add_triple( hx, s, r2, p1, r1 );
	hx_add_triple( hx, s, r2, p2, l2 );
	hx_add_triple( hx, s, r1, p2, l1 );
}

void _debug_node ( char* h, hx_node* node ) {
	char* string;
	hx_node_string( node, &string );
	fprintf( stderr, "%s %s\n", h, string );
}

