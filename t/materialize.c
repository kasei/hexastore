#include <unistd.h>
#include "hexastore.h"
#include "nodemap.h"
#include "node.h"
#include "storage.h"
#include "tap.h"

void _add_data ( hx_hexastore* hx, hx_storage_manager* s );
hx_variablebindings* _new_vb ( int size, char** names, hx_node_id* _nodes );
hx_variablebindings_iter* _get_triples ( hx_hexastore* hx, hx_storage_manager* s, int sort );
void _test_iter_expected_values ( hx_variablebindings_iter* iter, hx_nodemap* map );

hx_node* p1;
hx_node* p2;
hx_node* r1;
hx_node* r2;
hx_node* l1;
hx_node* l2;

void materialize_iter_test ( void );
void materialize_data_test ( void );
void materialize_reset_test ( void );

int main ( void ) {
	plan_tests(2*13+3);
	p1	= hx_new_node_resource( "p1" );
	p2	= hx_new_node_resource( "p2" );
	r1	= hx_new_node_resource( "r1" );
	r2	= hx_new_node_resource( "r2" );
	l1	= hx_new_node_literal( "l1" );
	l2	= hx_new_node_literal( "l2" );
	
	materialize_iter_test();
	materialize_data_test();
	materialize_reset_test();
	
	hx_free_node( p1 );
	hx_free_node( p2 );
	hx_free_node( r1 );
	hx_free_node( r2 );
	hx_free_node( l1 );
	hx_free_node( l2 );

	return exit_status();
}


void materialize_iter_test ( void ) {
	fprintf( stdout, "# materialize_iter_test\n" );
	hx_storage_manager* s	= hx_new_memory_storage_manager();
	hx_hexastore* hx	= hx_new_hexastore( s );
	hx_nodemap* map		= hx_get_nodemap( hx );
	_add_data( hx, s );
// <r1> :p1 <r2>
// <r2> :p1 <r1>
// <r2> :p2 "l2"
// <r1> :p2 "l1"
		
	// get ?subj ?pred ?obj ordered by object
	hx_variablebindings_iter* _iter	= _get_triples( hx, s, HX_OBJECT );
	hx_variablebindings_iter* iter	= hx_new_materialize_iter( _iter );
	
// 	while (!hx_variablebindings_iter_finished( iter )) {
// 		hx_variablebindings* b;
// 		hx_node_id s, p, o;
// 		hx_variablebindings_iter_current( iter, &b );
// 		char* string;
// 		hx_variablebindings_string( b, map, &string );
// 		fprintf( stdout, "**************** %s\n", string );
// 		free( string );
// 		
// //		hx_free_variablebindings( b );
// 		hx_variablebindings_iter_next( iter );
// 		hx_free_variablebindings( b );
// 	}
// 	return;
	
	_test_iter_expected_values( iter, map );
	
	hx_free_variablebindings_iter( iter );
	hx_free_hexastore( hx, s );
	hx_free_storage_manager( s );
}

void materialize_reset_test ( void ) {
	fprintf( stdout, "# materialize_reset_test\n" );
	hx_storage_manager* s	= hx_new_memory_storage_manager();
	hx_hexastore* hx	= hx_new_hexastore( s );
	hx_nodemap* map		= hx_get_nodemap( hx );
	_add_data( hx, s );
	hx_variablebindings_iter* _iter	= _get_triples( hx, s, HX_OBJECT );
	hx_variablebindings_iter* iter	= hx_new_materialize_iter( _iter );
	
	int counter	 = 0;
 	while (!hx_variablebindings_iter_finished( iter )) {
 		counter++;
		hx_variablebindings_iter_next( iter );
 	}
	ok1( counter == 4 );
	hx_materialize_reset_iter( iter );
	ok1( !hx_variablebindings_iter_finished(iter) );
 	while (!hx_variablebindings_iter_finished( iter )) {
 		counter++;
		hx_variablebindings_iter_next( iter );
 	}
	ok1( counter == 8 );
	
	hx_free_variablebindings_iter( iter );
	hx_free_hexastore( hx, s );
	hx_free_storage_manager( s );
}

void materialize_data_test ( void ) {
	fprintf( stdout, "# materialize_data_test\n" );
	hx_nodemap* map			= hx_new_nodemap();
	hx_node_id p1_id		= hx_nodemap_add_node( map, p1 );
	hx_node_id r1_id		= hx_nodemap_add_node( map, r1 );
	hx_node_id l1_id		= hx_nodemap_add_node( map, l1 );
	hx_node_id p2_id		= hx_nodemap_add_node( map, p2 );
	hx_node_id r2_id		= hx_nodemap_add_node( map, r2 );
	hx_node_id l2_id		= hx_nodemap_add_node( map, l2 );
	
	char* names[]	= { "subj", "pred", "obj" };
	
	// the ordering of t1-t4 is important here -- it's the same order that
	// we expect the triples to come out of the hexastore iterator above
	// in materialize_iter_test()
	hx_node_id t1[]	= { r2_id, p1_id, r1_id };
	hx_node_id t2[]	= { r1_id, p1_id, r2_id };
	hx_node_id t3[]	= { r2_id, p2_id, l2_id };
	hx_node_id t4[]	= { r1_id, p2_id, l1_id };
	
	hx_variablebindings** bindings	= (hx_variablebindings**) calloc( 4, sizeof( hx_variablebindings* ) );
	bindings[0]	= _new_vb( 3, names, t1 );
	bindings[1]	= _new_vb( 3, names, t2 );
	bindings[2]	= _new_vb( 3, names, t3 );
	bindings[3]	= _new_vb( 3, names, t4 );
	hx_variablebindings_iter* iter	= hx_new_materialize_iter_with_data( 3, names, 4, bindings );
	
	_test_iter_expected_values( iter, map );
	
	hx_free_variablebindings_iter( iter );
	hx_free_nodemap( map );
}

void _test_iter_expected_values ( hx_variablebindings_iter* iter, hx_nodemap* map ) {
	int size;
	char* name;
	char* string;
	hx_node_id nid;
	hx_variablebindings* b;
	
	_hx_materialize_iter_vb_info* info	= (_hx_materialize_iter_vb_info*) iter->ptr;
	ok1( !hx_variablebindings_iter_finished( iter ) );
	hx_variablebindings_iter_current( iter, &b );
	
	// expect 3 variable bindings for the three triple nodes
	size	= hx_variablebindings_size( b );
	ok1( size == 3 );

	{
		// expect the first variable binding to be "subj"
		name	= hx_variablebindings_name_for_binding( b, 0 );
		hx_variablebindings_string( b, map, &string );
		free( string );
		ok1( strcmp( name, "subj" ) == 0);
	}
	{
		// expect the third variable binding to be "obj"
		name	= hx_variablebindings_name_for_binding( b, 2 );
		ok1( strcmp( name, "obj" ) == 0);
	}
	
	
	{
		hx_node_id nid	= hx_variablebindings_node_id_for_binding( b, 2 );
		hx_node* node	= hx_nodemap_get_node( map, nid );
		
		// expect the FIRST result has "obj" of r1
		ok1( hx_node_cmp( node, r2 ) != 0 );
		ok1( hx_node_cmp( node, r1 ) == 0 );
	}
	
	hx_free_variablebindings( b );
	b	= NULL;
	hx_variablebindings_iter_next( iter );
	
	{
		// expect that the iterator isn't finished
		ok1( !hx_variablebindings_iter_finished( iter ) );
		
		hx_variablebindings_iter_current( iter, &b );
		hx_variablebindings_string( b, map, &string );
//		fprintf( stdout, "[2] bindings: %s\n", string );
		free( string );

		hx_node_id nid	= hx_variablebindings_node_id_for_binding( b, 2 );
		hx_node* node	= hx_nodemap_get_node( map, nid );
		
		// expect the SECOND result has "obj" of r2
		ok1( hx_node_cmp( node, r2 ) == 0 );
//		hx_free_variablebindings( b );
	}
	
	hx_free_variablebindings( b );
	b	= NULL;
	hx_variablebindings_iter_next( iter );
	
	{
		// expect that the iterator isn't finished
		ok1( !hx_variablebindings_iter_finished( iter ) );
		
		hx_variablebindings_iter_current( iter, &b );
		hx_variablebindings_string( b, map, &string );
//		fprintf( stdout, "[3] bindings: %s\n", string );
		free( string );

		hx_node_id nid	= hx_variablebindings_node_id_for_binding( b, 2 );
		hx_node* node	= hx_nodemap_get_node( map, nid );
		
		// expect the THIRD result has "obj" of l2
		ok1( hx_node_cmp( node, l2 ) == 0 );
//		hx_free_variablebindings( b );
	}
	
	hx_free_variablebindings( b );
	b	= NULL;
	hx_variablebindings_iter_next( iter );
	
	{
		// expect that the iterator isn't finished
		ok1( !hx_variablebindings_iter_finished( iter ) );
		
		hx_variablebindings_iter_current( iter, &b );
		hx_variablebindings_string( b, map, &string );
//		fprintf( stdout, "[3] bindings: %s\n", string );
		free( string );

		hx_node_id nid	= hx_variablebindings_node_id_for_binding( b, 2 );
		hx_node* node	= hx_nodemap_get_node( map, nid );
		
		// expect the FOURTH result has "obj" of l1
		ok1( hx_node_cmp( node, l1 ) == 0 );
//		hx_free_variablebindings( b );
	}
	
	hx_free_variablebindings( b );
	b	= NULL;
	
	hx_variablebindings_iter_next( iter );
	ok1( hx_variablebindings_iter_finished( iter ) );
}

hx_variablebindings_iter* _get_triples ( hx_hexastore* hx, hx_storage_manager* s, int sort ) {
	hx_node* v1	= hx_new_node_variable( -1 );
	hx_node* v2	= hx_new_node_variable( -2 );
	hx_node* v3	= hx_new_node_variable( -3 );
	
	hx_index_iter* titer	= hx_get_statements( hx, s, v1, v2, v3, HX_OBJECT );
	hx_variablebindings_iter* iter	= hx_new_iter_variablebindings( titer, s, "subj", "pred", "obj" );
	return iter;
}

hx_variablebindings* _new_vb ( int size, char** names, hx_node_id* _nodes ) {
	hx_node_id* nodes	= (hx_node_id*) calloc( size, sizeof( hx_node_id ) );
	int i;
	for (i = 0; i < size; i++) {
		nodes[i]	= _nodes[i];
	}
	return hx_new_variablebindings ( size, names, nodes );
}

void _add_data ( hx_hexastore* hx, hx_storage_manager* s ) {
	hx_add_triple( hx, s, r1, p1, r2 );
	hx_add_triple( hx, s, r2, p1, r1 );
	hx_add_triple( hx, s, r2, p2, l2 );
	hx_add_triple( hx, s, r1, p2, l1 );
}
