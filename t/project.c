#include <unistd.h>
#include "mentok.h"
#include "misc/nodemap.h"
#include "rdf/node.h"
#include "engine/project.h"
#include "test/tap.h"
#include "store/hexastore/hexastore.h"

void _add_data ( hx_model* hx );
void _debug_node ( char* h, hx_node* node );
hx_variablebindings_iter* _get_triples ( hx_model* hx, int sort );

hx_node* p1;
hx_node* p2;
hx_node* r1;
hx_node* r2;
hx_node* l1;
hx_node* l2;

void project_test1 ( void );

int main ( void ) {
	plan_tests(13);
	p1	= hx_new_node_resource( "p1" );
	p2	= hx_new_node_resource( "p2" );
	r1	= hx_new_node_resource( "r1" );
	r2	= hx_new_node_resource( "r2" );
	l1	= hx_new_node_literal( "l1" );
	l2	= hx_new_node_literal( "l2" );
	
	project_test1();
	
	return exit_status();
}

void project_test1 ( void ) {
	hx_model* hx		= hx_new_model( NULL );
	hx_nodemap* map			= hx_store_hexastore_get_nodemap( hx->store );
	_add_data( hx );
// <r1> :p1 <r2>
// <r2> :p1 <r1>
// <r2> :p2 "l2"
// <r1> :p2 "l1"
	
	{
		char* name;
		char* string;
		hx_node_id nid;
		hx_variablebindings* b;
		char* pnames[]	= { "pred" };
		// get ?subj ?pred ?obj ordered by object
		hx_variablebindings_iter* iter	= hx_new_project_iter(
											_get_triples( hx, HX_OBJECT ),
											1,
											pnames
										);
		{
			int size	= hx_variablebindings_iter_size( iter );
			ok1( size == 1 );
		}
		
		ok1( !hx_variablebindings_iter_finished( iter ) );
		
		hx_variablebindings_iter_current( iter, &b );
		{
			// expect 1 variable binding for the projected result
			int size	= hx_variablebindings_size( b );
			ok1( size == 1 );
		}
		
		{
			// expect the first variable binding to be "subj"
			name	= hx_variablebindings_name_for_binding( b, 0 );
			ok1( strcmp( name, "pred" ) == 0);
		}
	
	
		{
			hx_node_id nid	= hx_variablebindings_node_id_for_binding( b, 0 );
			hx_node* node	= hx_nodemap_get_node( map, nid );
	//		_debug_node( "[1] node: ", node );
			
			// expect the first result has "pred" of p1
			ok1( hx_node_cmp( node, p2 ) != 0 );
			ok1( hx_node_cmp( node, p1 ) == 0 );
		}
		
		hx_free_variablebindings(b);
		hx_variablebindings_iter_next( iter );
		hx_variablebindings_iter_current( iter, &b );
		{
			// expect that the iterator isn't finished
			ok1( !hx_variablebindings_iter_finished( iter ) );
			hx_node* node	= hx_variablebindings_node_for_binding( b, hx->store, 0 );
//			_debug_node( "[2] node: ", node );
			
			// expect the second result has "obj" of r2
			ok1( hx_node_cmp( node, p1 ) == 0 );
		}
		
		hx_free_variablebindings(b);
		hx_variablebindings_iter_next( iter );
		hx_variablebindings_iter_current( iter, &b );
		{
			// expect that the iterator isn't finished
			ok1( !hx_variablebindings_iter_finished( iter ) );
			
			hx_variablebindings_iter_current( iter, &b );
			hx_node* node	= hx_variablebindings_node_for_binding( b, hx->store, 0 );
//			_debug_node( "[3] node: ", node );
			
			// expect the second result has "obj" of l2
			ok1( hx_node_cmp( node, p2 ) == 0 );
		}
		
		hx_free_variablebindings(b);
		hx_variablebindings_iter_next( iter );
		hx_variablebindings_iter_current( iter, &b );
		{
			// expect that the iterator isn't finished
			ok1( !hx_variablebindings_iter_finished( iter ) );
			hx_node* node	= hx_variablebindings_node_for_binding( b, hx->store, 0 );
//			_debug_node( "[4] node: ", node );
			
			// expect the second result has "obj" of r2
			ok1( hx_node_cmp( node, p2 ) == 0 );
		}
	
		hx_free_variablebindings(b);
		hx_variablebindings_iter_next( iter );
		ok1( hx_variablebindings_iter_finished( iter ) );
		
		hx_free_variablebindings_iter( iter );
	}
	
	hx_free_model( hx );
}

hx_variablebindings_iter* _get_triples ( hx_model* hx, int sort ) {
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

void _add_data ( hx_model* hx ) {
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

