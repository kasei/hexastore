#include <unistd.h>
#include "rdf/triple.h"
#include "tap.h"

void test_hash ( void );

int main ( void ) {
	plan_tests(3);
	
	test_hash();
	
	return exit_status();
}

void test_hash ( void ) {
	hx_node* l1	= hx_new_node_literal("a");
	hx_node* r1	= (hx_node*) hx_new_node_resource("http://www.w3.org/2001/XMLSchema#integer");
	hx_node* r2	= (hx_node*) hx_new_node_resource("http://xmlns.com/foaf/0.1/name");
	hx_node* b1	= (hx_node*) hx_new_node_blank("r1");
	
	hx_triple* t1	= hx_new_triple( b1, r1, l1 );
	hx_triple* t2	= hx_new_triple( b1, r2, l1 );
	
	{
		uint64_t h1	= hx_triple_hash( t1, NULL );
		uint64_t h2	= hx_triple_hash( t2, NULL );
		ok1( h1 != h2 );
	}
	
	{
		uint64_t h1	= hx_triple_hash_on_node( t1, HX_SUBJECT, NULL );
		uint64_t h2	= hx_triple_hash_on_node( t2, HX_SUBJECT, NULL );
		ok1( h1 == h2 );
	}
	
	{
		uint64_t h1	= hx_triple_hash_on_node( t1, HX_PREDICATE, NULL );
		uint64_t h2	= hx_triple_hash_on_node( t2, HX_PREDICATE, NULL );
		ok1( h1 != h2 );
	}
	
	hx_free_triple( t1 );
	hx_free_triple( t2 );
	hx_free_node(l1);
	hx_free_node(r1);
	hx_free_node(r2);
	hx_free_node(b1);
}

