#include <unistd.h>
#include "mentok.h"
#include "misc/nodemap.h"
#include "rdf/node.h"
#include "parser/parser.h"
#include "test/tap.h"

void _add_data ( hx_model* hx );
void _debug_node ( char* h, hx_node* node );
hx_variablebindings_iter* _get_triples ( hx_model* hx, int sort );

void test_small_iter ( void );

int main ( void ) {
	plan_tests(10);
	hx_model* hx	= hx_new_model( NULL );
	_add_data( hx );
	
	hx_node* x			= hx_new_named_variable( hx, "x" );
	hx_node* y			= hx_new_named_variable( hx, "y" );
	hx_node* z			= hx_new_named_variable( hx, "z" );
	hx_node* binding	= hx_new_node_resource( "http://www.w3.org/2001/sw/DataAccess/tests/result-set#binding" );
	hx_node* variable	= hx_new_node_resource( "http://www.w3.org/2001/sw/DataAccess/tests/result-set#variable" );
	hx_node* resvar		= hx_new_node_resource( "http://www.w3.org/2001/sw/DataAccess/tests/result-set#resultVariable" );
	hx_node* rs			= hx_new_node_resource( "http://resultset/" );
	hx_node* rstype		= hx_new_node_resource( "http://www.w3.org/2001/sw/DataAccess/tests/result-set#ResultSet" );
	hx_node* sl			= hx_new_node_literal( "s" );

	{	// ALL TRIPLES
		uint64_t total	= hx_triples_count( hx );
		ok1( total == 31 );
	}
	
	{	// fff
		uint64_t total	= hx_count_statements( hx, x, y, z );
		ok1( total == 31 );
	}
	
	{	// fbf
		uint64_t total	= hx_count_statements( hx, x, binding, z );
		ok1( total == 8 );
	}
	
	{	// bff
		uint64_t total	= hx_count_statements( hx, rs, x, y );
		ok1( total == 7 );
	}
	
	{	// ffb
		uint64_t total	= hx_count_statements( hx, x, y, sl );
		ok1( total == 3 );
	}
	
	{	// fbb
		uint64_t total	= hx_count_statements( hx, x, variable, sl );
		ok1( total == 2 );
	}
	
	{	// bfb
		uint64_t total	= hx_count_statements( hx, rs, x, rstype );
		ok1( total == 1 );
	}

	{	// bbf
		uint64_t total	= hx_count_statements( hx, rs, resvar, y );
		ok1( total == 4 );
	}
	
	{	// bbb
		uint64_t total	= hx_count_statements( hx, rs, resvar, sl );
		ok1( total == 1 );
	}
	
	{	// bbb
		uint64_t total	= hx_count_statements( hx, rs, resvar, rstype );
		ok1( total == 0 );
	}
	
	hx_free_model( hx );
	return exit_status();
}

void _add_data ( hx_model* hx ) {
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

void _debug_node ( char* h, hx_node* node ) {
	char* string;
	hx_node_string( node, &string );
	fprintf( stderr, "%s %s\n", h, string );
}

