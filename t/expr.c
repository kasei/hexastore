#include <unistd.h>
#include "expr.h"
#include "nodemap.h"
#include "tap.h"

void test_serialization ( void );
void test_constructors ( void );
void test_eval ( void );

int main ( void ) {
	plan_tests(19);
	
	test_constructors();
	test_serialization();
	test_eval();
	
	return exit_status();
}

void test_constructors ( void ) {
	hx_expr* v1	= hx_new_node_expr( hx_new_node_named_variable( -1, "v" ) );
	hx_expr* l1	= hx_new_node_expr( hx_new_node_literal( "en" ) );
	hx_expr* l2	= hx_new_node_expr( hx_new_node_literal( "foo" ) );
	
	// testing for NULL return values when we try to use the wrong number
	// of arguments for a specific built-in
	ok1( hx_new_builtin_expr1( HX_EXPR_BUILTIN_LANGMATCHES, v1 ) == NULL );
	ok1( hx_new_builtin_expr1( HX_EXPR_BUILTIN_REGEX, v1 ) == NULL );

	ok1( hx_new_builtin_expr2( HX_EXPR_BUILTIN_STR, v1, l1 ) == NULL );
	ok1( hx_new_builtin_expr2( HX_EXPR_BUILTIN_REGEX, v1, l1 ) == NULL );

	ok1( hx_new_builtin_expr3( HX_EXPR_BUILTIN_STR, v1, l1, l2 ) == NULL );
	ok1( hx_new_builtin_expr3( HX_EXPR_BUILTIN_LANGMATCHES, v1, l1, l2 ) == NULL );
	
	hx_free_expr( v1 );
	hx_free_expr( l1 );
	hx_free_expr( l2 );
}

void test_serialization ( void ) {
	{
		hx_expr* v1	= hx_new_node_expr( hx_new_node_named_variable( -1, "v" ) );
		hx_expr* e	= hx_new_builtin_expr1( HX_EXPR_BUILTIN_STR, v1 );
		
		char* string;
		hx_expr_sse( e, &string, "  ", 0 );
		ok1( strcmp( string, "(sparql:str ?v)" ) == 0 );
		free( string );
		hx_free_expr(e);
	}

	{
		hx_expr* v1	= hx_new_node_expr( hx_new_node_named_variable( -1, "v" ) );
		hx_expr* l1	= hx_new_node_expr( hx_new_node_literal( "en" ) );
		hx_expr* e	= hx_new_builtin_expr2( HX_EXPR_BUILTIN_LANGMATCHES, v1, l1 );
		
		char* string;
		hx_expr_sse( e, &string, "  ", 0 );
		ok1( strcmp( string, "(sparql:langmatches ?v \"en\")" ) == 0 );
		free( string );
		hx_free_expr(e);
	}
}

void test_eval ( void ) {
	{
		// no-op expr
		hx_node* value;
		hx_node* l	= hx_new_node_literal( "en" );
		hx_node* m	= hx_new_node_literal( "ja" );
		hx_expr* e	= hx_new_node_expr( l );
		int r		= hx_expr_eval( e, NULL, NULL, &value );
		ok1( r == 0 );
		ok1( value != l );
		ok1( hx_node_cmp( l, value ) == 0 );
		ok1( hx_node_cmp( m, value ) != 0 );
	}
	
	{
		// variable substitution expr
		hx_node* value	= NULL;
		hx_nodemap* map	= hx_new_nodemap();
		hx_node* v		= hx_new_node_named_variable( -1, "x" );
		hx_node* l		= (hx_node*) hx_new_node_dt_literal( "1", "http://www.w3.org/2001/XMLSchema#integer" );
		hx_node* m		= hx_new_node_literal( "ja" );
		
		hx_node_id lid		= hx_nodemap_add_node( map, l );
		hx_node_id mid		= hx_nodemap_add_node( map, m );
		char* names[2]		= { "x", "y" };
		hx_node_id ids[2]	= { lid, mid };
		hx_variablebindings* b	= hx_new_variablebindings ( 2, names, ids, 0 );
		
		hx_expr* e	= hx_new_node_expr( v );
		int r		= hx_expr_eval( e, b, map, &value );
		ok1( r == 0 );
		
		ok1( value != l );
		ok1( hx_node_cmp( l, value ) == 0 );
		ok1( hx_node_cmp( m, value ) != 0 );
		ok1( hx_node_ebv(l) == 1 );
	}
	
	{
		// built-in function expr ISLITERAL(literal)
		hx_node* value	= NULL;
		hx_nodemap* map	= hx_new_nodemap();
		hx_node* x		= hx_new_node_named_variable( -1, "x" );
		
		hx_node* iri	= hx_new_node_resource( "http://example.org/" );
		hx_node* lit	= hx_new_node_literal( "en" );
		
		hx_node_id lid		= hx_nodemap_add_node( map, lit );
		hx_node_id iid		= hx_nodemap_add_node( map, iri );
		char* names[2]		= { "y", "x" };
		hx_node_id ids[2]	= { iid, lid };
		hx_variablebindings* b	= hx_new_variablebindings ( 2, names, ids, 0 );
		
		hx_expr* x_e	= hx_new_node_expr( x );
		hx_expr* e		= hx_new_builtin_expr1( HX_EXPR_BUILTIN_ISLITERAL, x_e );
		
		int r		= hx_expr_eval( e, b, map, &value );
		ok1( r == 0 );
		
		ok1( hx_node_ebv(value) == 1 );
	}
}
