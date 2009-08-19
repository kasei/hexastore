#include "hexastore.h"
#include "filter.h"
#include "expr.h"
#include "tap.h"

void _add_data ( hx_hexastore* hx );
hx_variablebindings_iter* _get_triples ( hx_hexastore* hx, int sort );

void filter_test1 ( void );
void filter_test2 ( void );

hx_node *p1;
hx_node *r1, *r2;
hx_node *l1, *l2, *l3, *l4, *l5, *l6;

int main ( void ) {
	plan_tests(14);
	p1	= hx_new_node_resource( "p1" );
	r1	= hx_new_node_resource( "r1" );
	r2	= hx_new_node_resource( "r2" );
	l1	= hx_new_node_literal( "l1" );
	l2	= hx_new_node_literal( "l2" );
	l3	= hx_new_node_literal( "l3" );
	l4	= hx_new_node_literal( "l4" );
	l5	= hx_new_node_literal( "l5" );
	l6	= hx_new_node_literal( "l6" );
	
	filter_test1();
	filter_test2();
	
	hx_free_node( p1 );
	hx_free_node( r1 );
	hx_free_node( r2 );
	hx_free_node( l1 );
	hx_free_node( l2 );
	hx_free_node( l3 );
	hx_free_node( l4 );
	hx_free_node( l5 );
	hx_free_node( l6 );
	
	return exit_status();
}


void filter_test1 ( void ) {
	fprintf( stdout, "# isliteral filter test\n" );
	hx_hexastore* hx	= hx_new_hexastore( NULL );
	hx_nodemap* map		= hx_get_nodemap( hx );
	_add_data( hx );
	
	hx_node* x						= hx_new_named_variable( hx, "obj" );
	hx_expr* x_e					= hx_new_node_expr( x );
	hx_expr* e						= hx_new_builtin_expr1( HX_EXPR_BUILTIN_ISLITERAL, x_e );
	hx_variablebindings_iter* _iter	= _get_triples( hx, HX_OBJECT );
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
	ok1( counter == 6 );
	hx_free_variablebindings_iter( iter );
	hx_free_hexastore( hx );
}

void filter_test2 ( void ) {
	hx_expr_debug	= 1;
	fprintf( stdout, "# term equal filter test\n" );
	hx_hexastore* hx		= hx_new_hexastore( NULL );
	hx_nodemap* map			= hx_get_nodemap( hx );
	_add_data( hx );
	
	hx_node* v						= hx_new_named_variable( hx, "obj" );
	hx_expr* v_e					= hx_new_node_expr( v );
	hx_expr* lit_e					= hx_new_node_expr( r1 );
	hx_expr* e						= hx_new_builtin_expr2( HX_EXPR_OP_EQUAL, v_e, lit_e );
	hx_variablebindings_iter* _iter	= _get_triples( hx, HX_OBJECT );
	int counter	= 0;
	
	hx_variablebindings_iter* iter	= hx_new_filter_iter( _iter, e, map );
	
	while (!hx_variablebindings_iter_finished( iter )) {
		counter++;
		hx_variablebindings* b;
		hx_variablebindings_iter_current( iter, &b );
		hx_node* obj	= hx_variablebindings_node_for_binding_name( b, map, "obj" );
		ok1( hx_node_is_resource(obj) == 1 );
		ok1( hx_node_cmp(obj, r1) == 0 );
		ok1( obj != r1 );

		hx_node* subj	= hx_variablebindings_node_for_binding_name( b, map, "subj" );
		ok1( hx_node_is_resource(subj) == 1 );
		ok1( hx_node_cmp(subj, r2) == 0 );
		ok1( subj != r2 );
		
		hx_variablebindings_iter_next( iter );
	}
	ok1( counter == 1 );
	hx_free_variablebindings_iter( iter );
	hx_free_hexastore( hx );
}

void _add_data ( hx_hexastore* hx ) {
	hx_add_triple( hx, r2, p1, r1 );
	hx_add_triple( hx, r1, p1, l2 );
	hx_add_triple( hx, r1, p1, l1 );
	hx_add_triple( hx, r1, p1, l5 );
	hx_add_triple( hx, r1, p1, l6 );
	hx_add_triple( hx, r1, p1, l4 );
	hx_add_triple( hx, r1, p1, l3 );
}

hx_variablebindings_iter* _get_triples ( hx_hexastore* hx, int sort ) {
	hx_node* v1	= hx_new_node_variable( -1 );
	hx_node* v2	= hx_new_node_variable( -2 );
	hx_node* v3	= hx_new_node_variable( -3 );
	
	hx_index_iter* titer	= hx_get_statements( hx, v1, v2, v3, HX_OBJECT );
	hx_variablebindings_iter* iter	= hx_new_iter_variablebindings( titer, "subj", "pred", "obj" );
	return iter;
}

