#include <stdlib.h>
#include "mentok/mentok.h"
#include "mentok/misc/nodemap.h"
#include "mentok/rdf/node.h"
#include "mentok/parser/parser.h"
#include "mentok/optimizer/optimizer-federated.h"
#include "mentok/optimizer/plan.h"
#include "mentok/algebra/bgp.h"
#include "mentok/store/hexastore/hexastore.h"
#include "test/tap.h"

void _add_data ( hx_model* hx );

void federated_merge_test1 ( hx_model* hx );
void federated_normalize_test1 ( hx_model* hx );

int _strcmp (const void *a, const void *b) {
	return strcmp( *((const char**) a), *((const char**) b) );
}

int main ( void ) {
	plan_tests(5);

	hx_model* hx	= hx_new_model( NULL );
	
	federated_merge_test1( hx );
	federated_normalize_test1( hx );
	
	hx_free_model(hx);
	return exit_status();
}

void federated_merge_test1 ( hx_model* hx ) {
	fprintf( stdout, "# federated_plans_test1\n" );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );
	ctx->optimizer_access_plans	= hx_optimizer_federated_access_plans;
	ctx->optimizer_join_plans	= hx_optimizer_federated_join_plans;
	hx_execution_context_add_service( ctx, hx_new_remote_service("http://A/sparql") );
	hx_execution_context_add_service( ctx, hx_new_remote_service("http://B/sparql") );
	
	hx_node* v1	= hx_new_node_named_variable( -1, "x" );
	hx_node* v2	= hx_new_node_named_variable( -2, "y" );
	hx_node* type	= (hx_node*) hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	hx_node* person	= (hx_node*) hx_new_node_resource("http://xmlns.com/foaf/0.1/Person");
	hx_node* name	= (hx_node*) hx_new_node_resource("http://xmlns.com/foaf/0.1/name");
	hx_triple* t1	= hx_new_triple( v1, type, person );
	hx_triple* t2	= hx_new_triple( v1, name, v2 );
	
	int i, size;
	{
		// the un-merged case
		hx_container_t* plans1	= hx_optimizer_access_plans( ctx, t1 );
		size					= hx_container_size(plans1);
		for (i = 0; i < size; i++) {
			hx_optimizer_plan* p	= hx_container_item( plans1, i );
			p->location				= 1;
		}
		
		hx_container_t* plans2	= hx_optimizer_access_plans( ctx, t2 );
		size					= hx_container_size(plans2);
		for (i = 0; i < size; i++) {
			hx_optimizer_plan* p	= hx_container_item( plans2, i );
			p->location				= 1;
		}
	
		hx_container_t* jplans	= hx_optimizer_join_plans( ctx, plans1, plans2, 0 );
		hx_optimizer_plan* plan	= hx_container_item( jplans, 0 );
		ok1( plan != NULL );
		char* string;
		hx_optimizer_plan_string( ctx, plan, &string );
		ok( strcmp(string, "nestedloop-join(POS[http://A/sparql]({?x <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://xmlns.com/foaf/0.1/Person>}), POS[http://A/sparql]({?x <http://xmlns.com/foaf/0.1/name> ?y}))") == 0, "expected un-merged plan" );
// 		fprintf( stderr, "GOT BEST PLAN: %s\n", string );
		free(string);
		hx_free_optimizer_plan(plan);
	}
	
	{
		// the merged case
		hx_container_t* plans1	= hx_optimizer_access_plans( ctx, t1 );
		size					= hx_container_size(plans1);
		for (i = 0; i < size; i++) {
			hx_optimizer_plan* p	= hx_container_item( plans1, i );
			p->location				= 1;
		}
		
		hx_container_t* plans2	= hx_optimizer_access_plans( ctx, t2 );
		size					= hx_container_size(plans2);
		for (i = 0; i < size; i++) {
			hx_optimizer_plan* p	= hx_container_item( plans2, i );
			p->location				= 1;
		}
		
		hx_container_t* jplans	= hx_optimizer_federated_join_plans( ctx, plans1, plans2, 0 );
		hx_optimizer_plan* plan	= hx_container_item( jplans, 0 );
		ok1( plan != NULL );
		char* string;
		hx_optimizer_plan_string( ctx, plan, &string );
		ok( strcmp(string, "nestedloop-join[http://A/sparql](POS({?x <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://xmlns.com/foaf/0.1/Person>}), POS({?x <http://xmlns.com/foaf/0.1/name> ?y}))") == 0, "expected merged plan" );
// 		fprintf( stderr, "GOT BEST PLAN: %s\n", string );
		free(string);
		hx_free_optimizer_plan(plan);
	}
	
	hx_free_execution_context(ctx);
}

void federated_normalize_test1 ( hx_model* hx ) {
	fprintf( stdout, "# federated_normalize_test1\n" );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );
	ctx->optimizer_access_plans	= hx_optimizer_federated_access_plans;
	ctx->optimizer_join_plans	= hx_optimizer_federated_join_plans;
	hx_execution_context_add_service( ctx, hx_new_remote_service("http://A/sparql") );
	hx_execution_context_add_service( ctx, hx_new_remote_service("http://B/sparql") );
	
	hx_bgp* b	= hx_bgp_parse_string("PREFIX foaf: <http://xmlns.com/foaf/0.1/> { ?x a foaf:Person ; foaf:name ?name . }");
	hx_optimizer_plan* plan	= hx_optimizer_optimize_bgp( ctx, b );
	
	char* string;
	hx_optimizer_plan_string( ctx, plan, &string );
	ok( strcmp(string, "union(union(hash-join[http://A/sparql](PSO({?x <http://xmlns.com/foaf/0.1/name> ?name}), POS({?x <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://xmlns.com/foaf/0.1/Person>})), hash-join(PSO[http://A/sparql]({?x <http://xmlns.com/foaf/0.1/name> ?name}), POS[http://B/sparql]({?x <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://xmlns.com/foaf/0.1/Person>}))), union(hash-join(PSO[http://B/sparql]({?x <http://xmlns.com/foaf/0.1/name> ?name}), POS[http://A/sparql]({?x <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://xmlns.com/foaf/0.1/Person>})), hash-join[http://B/sparql](PSO({?x <http://xmlns.com/foaf/0.1/name> ?name}), POS({?x <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://xmlns.com/foaf/0.1/Person>}))))") == 0, "expected normalized federated plan" );
// 	fprintf( stderr, "GOT BEST PLAN: %s\n", string );
	free(string);
	
	hx_free_optimizer_plan(plan);
	hx_free_bgp( b );
	hx_free_execution_context(ctx);
}
