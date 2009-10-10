#include <stdlib.h>
#include "mentok/mentok.h"
#include "mentok/misc/nodemap.h"
#include "mentok/rdf/node.h"
#include "mentok/parser/parser.h"
#include "mentok/optimizer/optimizer.h"
#include "mentok/optimizer/plan.h"
#include "mentok/algebra/bgp.h"
#include "mentok/store/hexastore/hexastore.h"
#include "test/tap.h"

void _add_data ( hx_model* hx );

void federated_plans_test1 ( hx_model* hx );

int _strcmp (const void *a, const void *b) {
	return strcmp( *((const char**) a), *((const char**) b) );
}

int main ( void ) {
	plan_tests(1);

	hx_model* hx	= hx_new_model( NULL );
//	_add_data( hx );
	
	federated_plans_test1( hx );
	
	hx_free_model(hx);
	return exit_status();
}

void federated_plans_test1 ( hx_model* hx ) {
	fprintf( stdout, "# federated_plans_test1\n" );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );
	ctx->optimizer_access_plans	= hx_optimizer_access_plans_federated;
	hx_execution_context_add_service( ctx, hx_new_remote_service("http://A/sparql") );
	hx_execution_context_add_service( ctx, hx_new_remote_service("http://B/sparql") );
	
	hx_bgp* b	= hx_bgp_parse_string("PREFIX foaf: <http://xmlns.com/foaf/0.1/> { ?x a foaf:Person ; foaf:name ?name . }");
	hx_optimizer_plan* plan	= hx_optimizer_optimize_bgp( ctx, b );
	char* string;
	hx_optimizer_plan_string( ctx, plan, &string );
	ok( strcmp(string, "hash-join(union(PSO[http://A/sparql]({?x <http://xmlns.com/foaf/0.1/name> ?name}), PSO[http://B/sparql]({?x <http://xmlns.com/foaf/0.1/name> ?name})), union(POS[http://A/sparql]({?x <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://xmlns.com/foaf/0.1/Person>}), POS[http://B/sparql]({?x <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://xmlns.com/foaf/0.1/Person>})))") == 0, "expected best federated 2-bgp plan" );
//	fprintf( stderr, "GOT BEST PLAN: %s\n", string );
	free(string);
	
	hx_free_optimizer_plan(plan);
	hx_free_bgp( b );
	hx_free_execution_context(ctx);
}

// void _add_data ( hx_model* hx ) {
// 	const char* rdf	= "@prefix :        <http://example/> . \
// @prefix rs:      <http://www.w3.org/2001/sw/DataAccess/tests/result-set#> . \
// @prefix rdf:     <http://www.w3.org/1999/02/22-rdf-syntax-ns#> . \
// @prefix xsd:        <http://www.w3.org/2001/XMLSchema#> . \
//  \
// <http://resultset/>    rdf:type      rs:ResultSet ; \
//       rs:resultVariable  \"p\" ; \
//       rs:resultVariable  \"g\" ; \
//       rs:resultVariable  \"s\" ; \
//       rs:resultVariable  \"o\" ; \
//       rs:solution   [ rs:binding    [ rs:value      :x ; \
//                                       rs:variable   \"s\" \
//                                     ] ; \
//                       rs:binding    [ rs:value      :p ; \
//                                       rs:variable   \"p\" \
//                                     ] ; \
//                       rs:binding    [ rs:value      <data-g1.ttl> ; \
//                                       rs:variable   \"g\" \
//                                     ] ; \
//                       rs:binding    [ rs:value      \"1\"^^xsd:integer ; \
//                                       rs:variable   \"o\" \
//                                     ] \
//                     ] ; \
//       rs:solution   [ rs:binding    [ rs:value      :a ; \
//                                       rs:variable   \"s\" \
//                                     ] ; \
//                       rs:binding    [ rs:value      :p ; \
//                                       rs:variable   \"p\" \
//                                     ] ; \
//                       rs:binding    [ rs:value      \"9\"^^xsd:integer ; \
//                                       rs:variable   \"o\" \
//                                     ] ; \
//                       rs:binding    [ rs:value      <data-g1.ttl> ; \
//                                       rs:variable   \"g\" \
//                                     ] \
//                     ] . \
// <http://resultset2/>    rdf:type      rs:ResultSet . \
// ";
// 	hx_parser* parser	= hx_new_parser();
// 	hx_parser_parse_string_into_model( parser, hx, rdf, "http://example.org/", "turtle" );
// 	hx_free_parser(parser);
// }
