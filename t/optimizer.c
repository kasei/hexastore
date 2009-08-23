#include "hexastore.h"
#include "misc/nodemap.h"
#include "rdf/node.h"
#include "parser/parser.h"
#include "test/tap.h"

void _add_data ( hx_hexastore* hx );

int main ( void ) {
	plan_tests(1);
	
	hx_hexastore* hx	= hx_new_hexastore( NULL );
	_add_data( hx );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );

	hx_node* v1	= hx_new_node_named_variable( -1, "a" );
	hx_node* v2	= hx_new_node_named_variable( -1, "b" );
	hx_node* v3	= hx_new_node_named_variable( -1, "c" );
	hx_node* l1	= hx_new_node_literal("a");
	hx_node* type	= (hx_node*) hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	hx_node* resultset	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet");
	hx_node* r1	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/XMLSchema#integer");
	hx_node* r2	= (hx_node*) hx_new_node_resource("http://xmlns.com/foaf/0.1/name");
	hx_node* b1	= (hx_node*) hx_new_node_blank("r1");
	hx_triple* t	= hx_new_triple( v1, type, v2 );
	
	hx_optimizer_access_plans( ctx, t );
	
	hx_free_triple( t );
	hx_free_node(v1);
	hx_free_node(v2);
	hx_free_node(v3);
	hx_free_node(l1);
	hx_free_node(r1);
	hx_free_node(r2);
	hx_free_node(b1);
	hx_free_node(type);
	hx_free_node(resultset);
	return exit_status();
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
