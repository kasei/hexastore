#include <unistd.h>
#include "mentok.h"
#include "store/hexastore/hexastore.h"
#include "algebra/expr.h"
#include "engine/expr.h"
#include "misc/nodemap.h"
#include "test/tap.h"

void test_serialization ( void );
void test_constructors ( void );
void test_eval ( void );
void expr_varsub_test1 ( void );
void test_expr_copy ( void );

int main ( void ) {
	plan_tests(21);
	
	test_constructors();
	test_serialization();
	test_eval();
	expr_varsub_test1();
	test_expr_copy();
	
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
		
		hx_free_node(l);
		hx_free_node(m);
	}
	
	{
		// variable substitution expr
		hx_node* value	= NULL;
		hx_model* hx		= hx_new_model( NULL );
		hx_execution_context* ctx		= hx_new_execution_context( NULL, hx );
		hx_nodemap* map			= hx_store_hexastore_get_nodemap( hx->store );
		hx_node* v		= hx_new_node_named_variable( -1, "x" );
		hx_node* l		= (hx_node*) hx_new_node_dt_literal( "1", "http://www.w3.org/2001/XMLSchema#integer" );
		hx_node* m		= hx_new_node_literal( "ja" );
		
		hx_node_id lid		= hx_nodemap_add_node( map, l );
		hx_node_id mid		= hx_nodemap_add_node( map, m );
		char* names[2]		= { "x", "y" };
		hx_node_id ids[2]	= { lid, mid };
		hx_variablebindings* b	= hx_new_variablebindings ( 2, names, ids );
		
		hx_expr* e	= hx_new_node_expr( v );
		int r		= hx_expr_eval( e, b, ctx, &value );
		ok1( r == 0 );
		
		ok1( value != l );
		ok1( hx_node_cmp( l, value ) == 0 );
		ok1( hx_node_cmp( m, value ) != 0 );
		ok1( hx_node_ebv(l) == 1 );
		
		hx_free_node( v );
		hx_free_node( l );
		hx_free_node( m );
		hx_free_execution_context( ctx );
	}
	
	{
		// built-in function expr ISLITERAL(literal)
		hx_node* value	= NULL;
		hx_model* hx		= hx_new_model( NULL );
		hx_execution_context* ctx		= hx_new_execution_context( NULL, hx );
		hx_nodemap* map			= hx_store_hexastore_get_nodemap( hx->store );
		hx_node* x		= hx_new_node_named_variable( -1, "x" );
		
		hx_node* iri	= hx_new_node_resource( "http://example.org/" );
		hx_node* lit	= hx_new_node_literal( "en" );
		
		hx_node_id lid		= hx_nodemap_add_node( map, lit );
		hx_node_id iid		= hx_nodemap_add_node( map, iri );
		char* names[2]		= { "y", "x" };
		hx_node_id ids[2]	= { iid, lid };
		hx_variablebindings* b	= hx_new_variablebindings ( 2, names, ids );
		
		hx_expr* x_e	= hx_new_node_expr( x );
		hx_expr* e		= hx_new_builtin_expr1( HX_EXPR_BUILTIN_ISLITERAL, x_e );
		
		int r		= hx_expr_eval( e, b, ctx, &value );
		ok1( r == 0 );
		
		ok1( hx_node_ebv(value) == 1 );
		
		hx_free_node(x);
		hx_free_node(iri);
		hx_free_node(lit);
		hx_free_execution_context( ctx );
	}
}

void expr_varsub_test1 ( void ) {
	{
		hx_node* p1	= hx_new_node_resource( "http://xmlns.com/foaf/0.1/name" );
		hx_expr* v1	= hx_new_node_expr( hx_new_node_named_variable( -1, "v" ) );
		hx_expr* e	= hx_new_builtin_expr1( HX_EXPR_BUILTIN_STR, v1 );

		hx_model* hx		= hx_new_model( NULL );
		hx_nodemap* map			= hx_store_hexastore_get_nodemap( hx->store );
		hx_node_id p1_id		= hx_nodemap_add_node( map, p1 );
		
		char* names[1]			= { "v" };
		hx_node_id* nodes		= (hx_node_id*) calloc( 1, sizeof( hx_node_id ) );
		nodes[0]				= p1_id;
		hx_variablebindings* b	= hx_new_variablebindings( 1, names, nodes );

		hx_expr* f				= hx_expr_substitute_variables( e, b, hx->store );
		
		char* string;
		hx_expr_sse( f, &string, "  ", 0 );
		ok1( strcmp( string, "(sparql:str <http://xmlns.com/foaf/0.1/name>)" ) == 0 );
		free( string );
		hx_free_expr(e);
		hx_free_expr(f);
		
		hx_free_variablebindings(b);
		hx_free_nodemap( map );
	}
}

void test_expr_copy ( void ) {
	hx_expr* copy;
	{
		hx_node* p1	= hx_new_node_resource( "http://xmlns.com/foaf/0.1/name" );
		hx_expr* v1	= hx_new_node_expr( hx_new_node_named_variable( -1, "v" ) );
		hx_expr* e	= hx_new_builtin_expr1( HX_EXPR_BUILTIN_STR, v1 );
		copy	= hx_copy_expr( e );
		hx_free_expr(e);
	}
	
	char* string;
	hx_expr_sse( copy, &string, "  ", 0 );
	ok1( strcmp( string, "(sparql:str ?v)" ) == 0 );
	free( string );
	
	hx_free_expr(copy);
}
