#include "hexastore.h"
#include "filter.h"
#include "tap.h"

void _add_data ( hx_hexastore* hx, hx_storage_manager* s );
hx_variablebindings_iter* _get_triples ( hx_hexastore* hx, hx_storage_manager* s, int sort );

void filter_test1 ( void );
void filter_test2 ( void );
void serialization_test ( void );

hx_node* p1;
hx_node* p2;
hx_node* r1;
hx_node* r2;
hx_node* l1;
hx_node* l2;

int main ( void ) {
	plan_tests(3);
	p1	= hx_new_node_resource( "p1" );
	p2	= hx_new_node_resource( "p2" );
	r1	= hx_new_node_resource( "r1" );
	r2	= hx_new_node_resource( "r2" );
	l1	= hx_new_node_literal( "l1" );
	l2	= hx_new_node_literal( "l2" );
	
	filter_test1();
	filter_test2();
	serialization_test();
	
	hx_free_node( p1 );
	hx_free_node( p2 );
	hx_free_node( r1 );
	hx_free_node( r2 );
	hx_free_node( l1 );
	hx_free_node( l2 );
	
	return exit_status();
}


void filter_test1 ( void ) {
	fprintf( stdout, "# isliteral filter test\n" );
	hx_storage_manager* s	= hx_new_memory_storage_manager();
	hx_hexastore* hx	= hx_new_hexastore( s );
	hx_nodemap* map		= hx_get_nodemap( hx );
	_add_data( hx, s );
	
	hx_node* x						= hx_new_named_variable( hx, "obj" );
	hx_expr* x_e					= hx_new_node_expr( x );
	hx_expr* e						= hx_new_builtin_expr1( HX_EXPR_BUILTIN_ISLITERAL, x_e );
	hx_variablebindings_iter* _iter	= _get_triples( hx, s, HX_OBJECT );
	hx_variablebindings_iter* iter	= hx_new_filter_iter( _iter, e, map );
	
	int counter	= 0;
	while (!hx_variablebindings_iter_finished( iter )) {
		counter++;
//		fprintf( stderr, "- loop iteration %d\n", counter );
		hx_variablebindings* b;
		hx_variablebindings_iter_current( iter, &b );
		hx_node* obj	= hx_variablebindings_node_for_binding_name( b, map, "obj" );
		ok1( hx_node_is_literal(obj) == 1 );
		
// 		char* string;
// 		hx_variablebindings_string( b, map, &string );
// 		fprintf( stderr, "<< %s >>\n", string );
// 		free(string);
		
		hx_variablebindings_iter_next( iter );
	}
	ok1( counter == 2 );
	hx_free_variablebindings_iter( iter, 1 );
	hx_free_hexastore( hx, s );
	hx_free_storage_manager( s );
}

void filter_test2 ( void ) {
	fprintf( stdout, "# term equal filter test\n" );
	hx_storage_manager* s	= hx_new_memory_storage_manager();
	hx_hexastore* hx	= hx_new_hexastore( s );
	hx_nodemap* map		= hx_get_nodemap( hx );
	_add_data( hx, s );
	
	hx_node* v						= hx_new_named_variable( hx, "obj" );
	hx_expr* v_e					= hx_new_node_expr( v );
	hx_expr* lit_e					= hx_new_node_expr( r1 );
	hx_expr* e						= hx_new_builtin_expr2( HX_EXPR_OP_EQUAL, v_e, lit_e );
	hx_variablebindings_iter* _iter	= _get_triples( hx, s, HX_OBJECT );
	hx_variablebindings_iter* iter	= hx_new_filter_iter( _iter, e, map );
	
	int counter	= 0;
	while (!hx_variablebindings_iter_finished( iter )) {
		counter++;
//		fprintf( stderr, "- loop iteration %d\n", counter );
		hx_variablebindings* b;
		hx_variablebindings_iter_current( iter, &b );
		hx_node* obj	= hx_variablebindings_node_for_binding_name( b, map, "subj" );
		ok1( hx_node_is_literal(obj) == 1 );
		
		char* string;
		hx_variablebindings_string( b, map, &string );
		fprintf( stderr, "<< %s >>\n", string );
		free(string);
		
		hx_variablebindings_iter_next( iter );
	}
	ok1( counter == 1 );
	hx_free_variablebindings_iter( iter, 1 );
	hx_free_hexastore( hx, s );
	hx_free_storage_manager( s );
}

void serialization_test ( void ) {
}

void _add_data ( hx_hexastore* hx, hx_storage_manager* s ) {
	hx_add_triple( hx, s, r1, p1, r2 );
	hx_add_triple( hx, s, r2, p1, r1 );
	hx_add_triple( hx, s, r2, p2, l2 );
	hx_add_triple( hx, s, r1, p2, l1 );
}

hx_variablebindings_iter* _get_triples ( hx_hexastore* hx, hx_storage_manager* s, int sort ) {
	hx_node* v1	= hx_new_node_variable( -1 );
	hx_node* v2	= hx_new_node_variable( -2 );
	hx_node* v3	= hx_new_node_variable( -3 );
	
	hx_index_iter* titer	= hx_get_statements( hx, s, v1, v2, v3, HX_OBJECT );
	hx_variablebindings_iter* iter	= hx_new_iter_variablebindings( titer, s, "subj", "pred", "obj", 0 );
	return iter;
}

