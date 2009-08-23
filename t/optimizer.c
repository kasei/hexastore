#include <stdlib.h>
#include "hexastore.h"
#include "misc/nodemap.h"
#include "rdf/node.h"
#include "parser/parser.h"
#include "optimizer/optimizer.h"
#include "optimizer/plan.h"
#include "test/tap.h"

void _add_data ( hx_hexastore* hx );

void access_plans_test1 ( void );
void access_plans_test2 ( void );
void access_plans_test3 ( void );

int main ( void ) {
	plan_tests(20);
	
	access_plans_test1();
	access_plans_test2();
	access_plans_test3();
	
	return exit_status();
}

void access_plans_test1 ( void ) {
	hx_hexastore* hx	= hx_new_hexastore( NULL );
	_add_data( hx );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );

	hx_node* v1	= hx_new_node_named_variable( -1, "a" );
	hx_node* v2	= hx_new_node_named_variable( -2, "b" );
	hx_node* type	= (hx_node*) hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	hx_triple* t	= hx_new_triple( v1, type, v2 );
	
	int i;
	hx_container_t* plans	= hx_optimizer_access_plans( ctx, t );
	int size				= hx_container_size(plans);
	ok1( size == 2 );
	
	int sorting_ok[2]	= {0,0};
//	fprintf( stderr, "%d plans (container %p)\n", size, (void*) plans );
	for (i = 0; i < size; i++) {
		hx_optimizer_plan* plan	= hx_container_item( plans, i );
		hx_index* i	= plan->source;
		char* name	= hx_index_name( i );
		ok1( *name == 'P' );
		ok1( plan->order_count == 2 );
		
		
//		fprintf( stderr, "- %s\n", name );
		char* string[2];
		hx_variablebindings_iter_sorting_string( plan->order[0], &(string[0]) );
		hx_variablebindings_iter_sorting_string( plan->order[1], &(string[1]) );
		
		int cmp	= (strcmp( string[0], string[1] ) > 0) ? 1 : 0;
		sorting_ok[ cmp ]	= 1;
		
		qsort( string, 2, sizeof(char*), strcmp );
		
		ok1( strcmp(string[0], "ASC(?a)") == 0 );
		ok1( strcmp(string[1], "ASC(?b)") == 0 );
		
		free(string[0]);
		free(string[1]);
		free(name);
	}
	
	ok1( sorting_ok[0] );
	ok1( sorting_ok[1] );
	
	hx_free_triple( t );
	hx_free_node(v1);
	hx_free_node(v2);
	hx_free_node(type);
}

void access_plans_test2 ( void ) {
	hx_hexastore* hx	= hx_new_hexastore( NULL );
	_add_data( hx );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );

	hx_node* v1	= hx_new_node_named_variable( -1, "x" );
	hx_node* type	= (hx_node*) hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	hx_node* resultset	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet");
	hx_triple* t	= hx_new_triple( v1, type, resultset );
	
	int i;
	hx_container_t* plans	= hx_optimizer_access_plans( ctx, t );
	int size				= hx_container_size(plans);
	ok1( size == 2 );
	
//	fprintf( stderr, "%d plans (container %p)\n", size, (void*) plans );
	for (i = 0; i < size; i++) {
		hx_optimizer_plan* plan	= hx_container_item( plans, i );
		hx_index* i	= plan->source;
		char* name	= hx_index_name( i );
		ok1( name[2] == 'S' );
		ok1( plan->order_count == 1 );
		
		char* string;
		hx_variablebindings_iter_sorting_string( plan->order[0], &string );
		ok1( strcmp(string, "ASC(?x)") == 0 );
		
		free(string);
		free(name);
	}
	
	hx_free_triple( t );
	hx_free_node(v1);
	hx_free_node(resultset);
	hx_free_node(type);
}

void access_plans_test3 ( void ) {
	hx_hexastore* hx	= hx_new_hexastore( NULL );
	_add_data( hx );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );

	hx_node* v1	= hx_new_node_named_variable( -1, "x" );
	hx_node* v2	= hx_new_node_named_variable( -1, "y" );
	hx_node* v3	= hx_new_node_named_variable( -1, "z" );
	hx_triple* t	= hx_new_triple( v1, v2, v3 );
	
	int i;
	hx_container_t* plans	= hx_optimizer_access_plans( ctx, t );
	int size				= hx_container_size(plans);
	ok1( size == 6 );
	hx_optimizer_plan* plan	= hx_container_item( plans, 0 );
	ok1( plan->order_count == 3 );
	
	
	hx_free_triple( t );
	hx_free_node(v1);
	hx_free_node(v2);
	hx_free_node(v3);
}

void _add_data ( hx_hexastore* hx ) {
	const char* rdf	= "@prefix :        <http://example/> . \
@prefix rs:      <http://www.w3.org/2001/sw/DataAccess/tests/result-set#> . \
@prefix rdf:     <http://www.w3.org/1999/02/22-rdf-syntax-ns#> . \
@prefix xsd:        <http://www.w3.org/2001/XMLSchema#> . \
 \
<http://resultset/>    rdf:type      rs:ResultSet ; \
      rs:resultVariable  \"p\" ; \
      rs:resultVariable  \"g\" ; \
      rs:resultVariable  \"s\" ; \
      rs:resultVariable  \"o\" ; \
      rs:solution   [ rs:binding    [ rs:value      :x ; \
                                      rs:variable   \"s\" \
                                    ] ; \
                      rs:binding    [ rs:value      :p ; \
                                      rs:variable   \"p\" \
                                    ] ; \
                      rs:binding    [ rs:value      <data-g1.ttl> ; \
                                      rs:variable   \"g\" \
                                    ] ; \
                      rs:binding    [ rs:value      \"1\"^^xsd:integer ; \
                                      rs:variable   \"o\" \
                                    ] \
                    ] ; \
      rs:solution   [ rs:binding    [ rs:value      :a ; \
                                      rs:variable   \"s\" \
                                    ] ; \
                      rs:binding    [ rs:value      :p ; \
                                      rs:variable   \"p\" \
                                    ] ; \
                      rs:binding    [ rs:value      \"9\"^^xsd:integer ; \
                                      rs:variable   \"o\" \
                                    ] ; \
                      rs:binding    [ rs:value      <data-g1.ttl> ; \
                                      rs:variable   \"g\" \
                                    ] \
                    ] . \
";
	hx_parser* parser	= hx_new_parser();
	hx_parser_parse_string_into_hexastore( parser, hx, rdf, "http://example.org/", "turtle" );
	hx_free_parser(parser);
}




// 	hx_node* v1	= hx_new_node_named_variable( -1, "a" );
// 	hx_node* v2	= hx_new_node_named_variable( -2, "b" );
// 	hx_node* v3	= hx_new_node_named_variable( -3, "c" );
// 	hx_node* l1	= hx_new_node_literal("a");
// 	hx_node* type	= (hx_node*) hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
// 	hx_node* resultset	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet");
// 	hx_node* r1	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/XMLSchema#integer");
// 	hx_node* r2	= (hx_node*) hx_new_node_resource("http://xmlns.com/foaf/0.1/name");
// 	hx_node* b1	= (hx_node*) hx_new_node_blank("r1");
// 	hx_triple* t	= hx_new_triple( v1, type, v2 );
// 	
// 	hx_free_triple( t );
// 	hx_free_node(v1);
// 	hx_free_node(v2);
// 	hx_free_node(v3);
// 	hx_free_node(l1);
// 	hx_free_node(r1);
// 	hx_free_node(r2);
// 	hx_free_node(b1);
// 	hx_free_node(type);
// 	hx_free_node(resultset);
