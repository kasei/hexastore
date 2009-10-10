#include <unistd.h>
#include "mentok/mentok.h"
#include "mentok/misc/nodemap.h"
#include "mentok/engine/union.h"
#include "mentok/rdf/node.h"
#include "test/tap.h"
#include "mentok/algebra/bgp.h"
#include "mentok/engine/bgp.h"
#include "mentok/store/hexastore/hexastore.h"

void _add_data ( hx_model* hx );
void _debug_node ( char* h, hx_node* node );
hx_variablebindings_iter* _get_triples ( hx_model* hx, int sort );

hx_node* p1;
hx_node* p2;
hx_node* r1;
hx_node* r2;
hx_node* l1;
hx_node* l2;

void test_union (void);

int main ( void ) {
	plan_tests(10);
	p1	= hx_new_node_resource( "p1" );
	p2	= hx_new_node_resource( "p2" );
	r1	= hx_new_node_resource( "r1" );
	r2	= hx_new_node_resource( "r2" );
	l1	= hx_new_node_literal( "l1" );
	l2	= hx_new_node_literal( "l2" );
	
	test_union();
	
	return exit_status();
}

void test_union (void) {
	fprintf( stdout, "# test_union\n" );
	hx_model* hx		= hx_new_model( NULL );
	hx_nodemap* map		= hx_store_hexastore_get_nodemap( hx->store );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );
	_add_data( hx );
// <r1> :p1 <r2>
// <r2> :p1 <r1>
// <r2> :p2 "l2"
// <r1> :p2 "l1"
	
	int size;
	char* name;
	char* string;
	hx_variablebindings* b;
	
	{
		hx_node* v1		= hx_model_new_named_variable( hx, "x" );
		hx_node* v2		= hx_model_new_named_variable( hx, "y" );
		
		hx_triple* ta	= hx_new_triple( r1, v1, r2 );
		hx_variablebindings_iter* iter_a	= hx_model_new_variablebindings_iter_for_triple( hx, ta, HX_OBJECT );
		
		hx_triple* tb	= hx_new_triple( r2, v2, l2 );
		hx_variablebindings_iter* iter_b	= hx_model_new_variablebindings_iter_for_triple( hx, tb, HX_SUBJECT );
		
		hx_variablebindings_iter* iter	= hx_new_union_iter2( ctx, iter_a, iter_b );
		ok1( iter != NULL );
		
		{
			ok1( !hx_variablebindings_iter_finished( iter ) );
			hx_variablebindings_iter_current( iter, &b );
			
			// expect 1 variable binding
			size	= hx_variablebindings_size( b );
			ok1( size == 1 );
			
			char* v	= hx_variablebindings_name_for_binding( b, 0 );
			ok1( strcmp( v, "x" ) == 0 );
			
			hx_node_id id	= hx_variablebindings_node_id_for_binding_name( b, "x" );
			hx_node* x		= hx_nodemap_get_node( map, id );
			ok1( hx_node_cmp( x, p1 ) == 0 );
		}
		
		hx_variablebindings_iter_next(iter);
		
		{
			ok1( !hx_variablebindings_iter_finished( iter ) );
			hx_variablebindings_iter_current( iter, &b );
			
			// expect 1 variable binding
			size	= hx_variablebindings_size( b );
			ok1( size == 1 );
			
			char* v	= hx_variablebindings_name_for_binding( b, 0 );
			ok1( strcmp( v, "y" ) == 0 );
			
			hx_node_id id	= hx_variablebindings_node_id_for_binding_name( b, "y" );
			hx_node* y		= hx_nodemap_get_node( map, id );
			ok1( hx_node_cmp( y, p2 ) == 0 );
		}
		
		hx_variablebindings_iter_next(iter);
		ok1( hx_variablebindings_iter_finished( iter ) );
			
// 			{
// 				// expect the first variable binding to be "x"
// 				name	= hx_variablebindings_name_for_binding( b, 0 );
// 				hx_store_variablebindings_string( hx->store, b, &string );
// 				free( string );
// 				ok1( strcmp( name, "x" ) == 0);
// 			}
// 			{
// 				// expect the second variable binding to be "y"
// 				name	= hx_variablebindings_name_for_binding( b, 1 );
// 				hx_store_variablebindings_string( hx->store, b, &string );
// 				free( string );
// 				ok1( strcmp( name, "y" ) == 0);
// 			}
// 			
// 			{
// 				hx_node_id fid	= hx_variablebindings_node_id_for_binding_name( b, "x" );
// 				hx_node* x		= hx_nodemap_get_node( map, fid );
// 				hx_node_id tid	= hx_variablebindings_node_id_for_binding_name( b, "y" );
// 				hx_node* y		= hx_nodemap_get_node( map, tid );
// 		
// 				ok1( hx_node_cmp( x, r2 ) == 0 );
// 				ok1( hx_node_cmp( y, r1 ) == 0 );
// 			}
// 			hx_variablebindings_iter_next( iter );
// 			ok1( hx_variablebindings_iter_finished( iter ) );
// 			
// 			hx_free_variablebindings_iter( iter );
// 		} else {
// 			// expect the join constructor to fail and return NULL
// 			ok1( iter == NULL );
// 		}
		
		hx_free_node(v1);
		hx_free_node(v2);
	}
	
	hx_free_model( hx );
}

hx_variablebindings_iter* _get_triples ( hx_model* hx, int sort ) {
	hx_node* v1	= hx_new_node_named_variable( -1, "subj" );
	hx_node* v2	= hx_new_node_named_variable( -2, "pred" );
	hx_node* v3	= hx_new_node_named_variable( -3, "obj" );
	hx_triple* t	= hx_new_triple( v1, v2, v3 );
	hx_variablebindings_iter* iter	=  hx_model_new_variablebindings_iter_for_triple( hx, t, HX_OBJECT );
	hx_free_triple(t);
	hx_free_node(v1);
	hx_free_node(v2);
	hx_free_node(v3);
	return iter;
}

void _add_data ( hx_model* hx ) {
	hx_model_add_triple( hx, r1, p1, r2 );
	hx_model_add_triple( hx, r2, p1, r1 );
	hx_model_add_triple( hx, r2, p2, l2 );
	hx_model_add_triple( hx, r1, p2, l1 );
}

void _debug_node ( char* h, hx_node* node ) {
	char* string;
	hx_node_string( node, &string );
	fprintf( stderr, "%s %s\n", h, string );
}
