#include <time.h>
#include <sys/time.h>
#include "mentok/parser/SPARQLParser.h"
#include "mentok/algebra/bgp.h"
#include "mentok/algebra/graphpattern.h"
#include "mentok/engine/bgp.h"
#include "mentok/engine/hashjoin.h"
#include "mentok/engine/union.h"
#include "mentok/engine/delay.h"
#include "mentok/engine/graphpattern.h"
#include "mentok/engine/nestedloopjoin.h"
#include "mentok/store/hexastore/hexastore.h"
#include "mentok/store/tokyocabinet/tokyocabinet.h"
#include "mentok/optimizer/optimizer.h"
#include "mentok/optimizer/plan.h"
#include "mentok/optimizer/optimizer-federated.h"

#define DIFFTIME(a,b) ((b-a)/(double)CLOCKS_PER_SEC)

extern hx_bgp* parse_bgp_query ( void );
extern hx_graphpattern* parse_query_string ( char* );

void help (int argc, char** argv) {
	fprintf( stderr, "Usage: %s [-d] data1 data2 ...\n", argv[0] );
	fprintf( stderr, "\n\n" );
}

int _set_location_store ( hx_execution_context* ctx, hx_optimizer_plan* plan, void* store ) {
	if (plan->type == HX_OPTIMIZER_PLAN_INDEX) {
		plan->data.access.store	= store;
// 	} else if (plan->type == HX_OPTIMIZER_PLAN_JOIN) {
// 		plan->data.join.join_type	= HX_OPTIMIZER_PLAN_MERGEJOIN;
	}
	return 0;
}

hx_optimizer_plan* set_plan_location ( hx_execution_context* ctx, hx_optimizer_plan* plan, int location, hx_model** models ) {
	plan->location	= location;
	hx_optimizer_plan_visit_postfix( ctx, plan, _set_location_store, models[location-1]->store );
	
	return plan;
}

hx_variablebindings_iter* q2_plan_t12 ( hx_store* store, hx_triple* t1, hx_triple* t2, hx_node* q ) {
	hx_variablebindings_iter* t1i1		= hx_store_hexastore_get_statements(store, t1, q);
	hx_variablebindings_iter* t2i1		= hx_store_hexastore_get_statements(store, t2, q);
	hx_variablebindings_iter* t12i1		= hx_new_mergejoin_iter( t1i1, t2i1 );
	return t12i1;
}

hx_variablebindings_iter* q2_plan_t123 ( hx_store* store, hx_triple* t1, hx_triple* t2, hx_triple* t3, hx_node* p, hx_node* q ) {
	hx_variablebindings_iter* t12i		= q2_plan_t12( store, t1, t2, q );
	hx_variablebindings_iter* t3i		= hx_store_hexastore_get_statements(store, t3, p);
	hx_variablebindings_iter* t123i1	= hx_new_mergejoin_iter( t12i, t3i );
	return t123i1;
}

void debug_iter ( hx_variablebindings_iter* iter ) {
	int count	= 0;
	fprintf( stderr, "---------------------------\n" );
	fprintf( stderr, "trying to execute union sub-plan...\n" );
	while (!hx_variablebindings_iter_finished( iter )) {
		count++;
		hx_variablebindings* b;
		hx_variablebindings_iter_current( iter, &b );
		fprintf( stderr, "Result %d: ", (int) count );
		hx_variablebindings_debug( b );
		hx_free_variablebindings(b);
		hx_variablebindings_iter_next( iter );
	}
	hx_free_variablebindings_iter( iter );
	fprintf( stderr, "---------------------------\n" );
}

hx_variablebindings_iter* plan_full_optimized ( hx_execution_context* ctx, hx_model** models, int querynum, int debug ) {
	hx_triple *t1, *t2, *t3;
	hx_node* paper	= hx_new_node_named_variable( -1, "paper" );
	hx_node* person	= hx_new_node_named_variable( -2, "person" );
	hx_node* p		= hx_new_node_named_variable( -1, "p" );
	hx_node* q		= hx_new_node_named_variable( -2, "q" );
	hx_node* r		= hx_new_node_named_variable( -3, "r" );
	if (querynum == 2) {
		hx_node* knows	= (hx_node*) hx_new_node_resource("http://xmlns.com/foaf/0.1/knows");
		t1	= hx_new_triple( p, knows, q );
		t2	= hx_new_triple( q, knows, r );
		t3	= hx_new_triple( r, knows, p );
		hx_free_node(knows);
		hx_container_t* iters				= hx_new_container( 'I', 8 );

		hx_container_t* sources	= ctx->remote_sources;
		hx_remote_service* s	= hx_container_item( sources, 1 );
		long ilatency			= s->latency1;
		long jlatency			= s->latency2;
		double irate			= s->results_per_second1;
		double jrate			= s->results_per_second2;

		hx_variablebindings_iter* t123i1	= hx_new_delay_iter( q2_plan_t123( models[0]->store, t1, t2, t3, p, q ), jlatency, jrate );
		hx_container_push_item( iters, t123i1 );
		
		hx_variablebindings_iter* t123i2	= hx_new_delay_iter( q2_plan_t123( models[1]->store, t1, t2, t3, p, q ), jlatency, jrate );
		hx_container_push_item( iters, t123i2 );
		
		{
			hx_variablebindings_iter* t12i1		= hx_new_delay_iter( q2_plan_t12( models[0]->store, t1, t2, q ), jlatency, jrate );
			hx_variablebindings_iter* t3i2		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[1]->store, t3, p), ilatency, irate );
			hx_variablebindings_iter* i3		= hx_new_hashjoin_iter( t12i1, t3i2 );
			hx_container_push_item( iters, i3 );
		}
		
		{
			hx_variablebindings_iter* t12i2		= hx_new_delay_iter( q2_plan_t12( models[1]->store, t1, t2, q ), jlatency, jrate );
			hx_variablebindings_iter* t3i1		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[0]->store, t3, p), ilatency, irate );
			hx_variablebindings_iter* i4		= hx_new_hashjoin_iter( t12i2, t3i1 );
			hx_container_push_item( iters, i4 );
		}
		
		{
			hx_variablebindings_iter* t1i1		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[0]->store, t1, q), ilatency, irate );
			hx_variablebindings_iter* t2i2		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[1]->store, t2, q), ilatency, irate );
			hx_variablebindings_iter* t3i1		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[0]->store, t3, p), ilatency, irate );
			hx_variablebindings_iter* t12i		= hx_new_hashjoin_iter( t1i1, t2i2 );
			hx_variablebindings_iter* i5		= hx_new_hashjoin_iter( t12i, t3i1 );
			
			debug_iter( i5 );
			exit(0);
			
			hx_container_push_item( iters, i5 );
		}
		
		{
			hx_variablebindings_iter* t1i1		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[0]->store, t1, q), ilatency, irate );
			hx_variablebindings_iter* t2i2		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[1]->store, t2, q), ilatency, irate );
			hx_variablebindings_iter* t3i2		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[1]->store, t3, p), ilatency, irate );
			hx_variablebindings_iter* t12i		= hx_new_hashjoin_iter( t1i1, t2i2 );
			hx_variablebindings_iter* i6		= hx_new_hashjoin_iter( t12i, t3i2 );
			hx_container_push_item( iters, i6 );
		}
		
		{
			hx_variablebindings_iter* t1i2		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[1]->store, t1, q), ilatency, irate );
			hx_variablebindings_iter* t2i1		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[0]->store, t2, q), ilatency, irate );
			hx_variablebindings_iter* t3i1		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[0]->store, t3, p), ilatency, irate );
			hx_variablebindings_iter* t12i		= hx_new_hashjoin_iter( t1i2, t2i1 );
			hx_variablebindings_iter* i7		= hx_new_hashjoin_iter( t12i, t3i1 );
			hx_container_push_item( iters, i7 );
		}
		
		{
			hx_variablebindings_iter* t1i2		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[1]->store, t1, q), ilatency, irate );
			hx_variablebindings_iter* t2i1		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[0]->store, t2, q), ilatency, irate );
			hx_variablebindings_iter* t3i2		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[1]->store, t3, p), ilatency, irate );
			hx_variablebindings_iter* t12i		= hx_new_hashjoin_iter( t1i2, t2i1 );
			hx_variablebindings_iter* i8		= hx_new_hashjoin_iter( t12i, t3i2 );
			hx_container_push_item( iters, i8 );
		}
		
		hx_variablebindings_iter* iter		= hx_new_union_iter( ctx, iters );
		
// 		if (debug) {
// 			int count	= 0;
// 			fprintf( stderr, "---------------------------\n" );
// 			fprintf( stderr, "trying to execute topic union sub-plan...\n" );
// 			while (!hx_variablebindings_iter_finished( iter )) {
// 				count++;
// 				hx_variablebindings* b;
// 				hx_variablebindings_iter_current( iter, &b );
// 				fprintf( stderr, "Result %d: ", (int) count );
// 				hx_variablebindings_debug( b );
// 				hx_free_variablebindings(b);
// 				hx_variablebindings_iter_next( iter );
// 			}
// 			hx_free_variablebindings_iter( iter );
// 			fprintf( stderr, "---------------------------\n" );
// 		}
		
		return iter;
	} else {
		hx_node* type	= (hx_node*) hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
		hx_node* maker	= (hx_node*) hx_new_node_resource("http://xmlns.com/foaf/0.1/maker");
		hx_node* topic	= (hx_node*) hx_new_node_resource("http://xmlns.com/foaf/0.1/topic");
		hx_node* inproc	= (hx_node*) hx_new_node_resource("http://swrc.ontoware.org/ontology#InProceedings");
		hx_node* semweb	= (hx_node*) hx_new_node_resource("http://dbpedia.org/resource/Semantic_Web");
		t1	= hx_new_triple( paper, type, inproc );
		t2	= hx_new_triple( paper, topic, semweb );
		t3	= hx_new_triple( paper, maker, person );
		hx_free_node(type);
		hx_free_node(maker);
		hx_free_node(topic);
		hx_free_node(inproc);
		hx_free_node(semweb);
		
		hx_container_t* typeplans	= hx_optimizer_access_plans( ctx, t1 );
		hx_container_t* topicplans	= hx_optimizer_access_plans( ctx, t2 );
		hx_container_t* makerplans	= hx_optimizer_access_plans( ctx, t3 );
		
		hx_optimizer_plan* typep	= hx_container_item( typeplans, 0 );
		hx_optimizer_plan* topicp	= hx_container_item( topicplans, 0 );
		hx_optimizer_plan* makerp	= hx_container_item( makerplans, 0 );
		
		hx_optimizer_plan* tt		= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, topicp, typep, topicp->order, 0 );
		hx_optimizer_plan* ttm;
		ttm		= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, tt, makerp, tt->order, 0 );
		
		hx_optimizer_plan* topic1	= set_plan_location( ctx, hx_copy_optimizer_plan( topicp ), 1, models );
		hx_optimizer_plan* topic2	= set_plan_location( ctx, hx_copy_optimizer_plan( topicp ), 2, models );
		hx_optimizer_plan* type1	= set_plan_location( ctx, hx_copy_optimizer_plan( typep ), 1, models );
		hx_optimizer_plan* type2	= set_plan_location( ctx, hx_copy_optimizer_plan( typep ), 2, models );
		hx_optimizer_plan* maker1	= set_plan_location( ctx, hx_copy_optimizer_plan( makerp ), 1, models );
		hx_optimizer_plan* maker2	= set_plan_location( ctx, hx_copy_optimizer_plan( makerp ), 2, models );
		
		hx_optimizer_plan* t1t2		= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, topic1, type2, topic1->order, 0 );
		hx_optimizer_plan* t2t1		= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, topic2, type1, topic2->order, 0 );
		
		hx_optimizer_plan* tt1		= set_plan_location( ctx, hx_copy_optimizer_plan( tt ), 1, models );
		hx_optimizer_plan* tt2		= set_plan_location( ctx, hx_copy_optimizer_plan( tt ), 2, models );
		
		
		// final sub plans:
		hx_optimizer_plan* ttm1		= set_plan_location( ctx, hx_copy_optimizer_plan( ttm ), 1, models );
		hx_optimizer_plan* ttm2		= set_plan_location( ctx, hx_copy_optimizer_plan( ttm ), 2, models );
		hx_optimizer_plan* tt1m2	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, tt1, maker2, tt1->order, 0 );
		hx_optimizer_plan* tt2m1	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, tt2, maker1, tt2->order, 0 );
	
		hx_optimizer_plan* t1t2m1	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, t1t2, maker1, t1t2->order, 0 );
		hx_optimizer_plan* t1t2m2	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, t1t2, maker2, t1t2->order, 0 );
	
		hx_optimizer_plan* t2t1m1	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, t2t1, maker1, t2t1->order, 0 );
		hx_optimizer_plan* t2t1m2	= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, t2t1, maker2, t2t1->order, 0 );
		
		hx_container_t* plans		= hx_new_container( 'P', 8 );
		hx_container_push_item( plans, ttm1 );
		hx_container_push_item( plans, ttm2 );
		hx_container_push_item( plans, tt1m2 );
		hx_container_push_item( plans, tt2m1 );
		hx_container_push_item( plans, t1t2m1 );
		hx_container_push_item( plans, t1t2m2 );
		hx_container_push_item( plans, t2t1m1 );
		hx_container_push_item( plans, t2t1m2 );
		hx_optimizer_plan* plan		= hx_new_optimizer_union_plan( plans );
	
		fprintf( stderr, ">>> " );
		hx_optimizer_plan_debug( ctx, plan );
		
		int j;
		for (j = 0; j < hx_container_size(typeplans); j++)
			hx_free_optimizer_plan( hx_container_item(typeplans, j) );
		for (j = 0; j < hx_container_size(topicplans); j++)
			hx_free_optimizer_plan( hx_container_item(topicplans, j) );
		for (j = 0; j < hx_container_size(makerplans); j++)
			hx_free_optimizer_plan( hx_container_item(makerplans, j) );
		hx_free_container( typeplans );
		hx_free_container( topicplans );
		hx_free_container( makerplans );	
		
		hx_free_optimizer_plan( tt );
		hx_free_optimizer_plan( ttm );
		hx_free_optimizer_plan( topic1 );
		hx_free_optimizer_plan( topic2 );
		hx_free_optimizer_plan( type1 );
		hx_free_optimizer_plan( type2 );
		hx_free_optimizer_plan( maker1 );
		hx_free_optimizer_plan( maker2 );
		hx_free_optimizer_plan( t1t2 );
		hx_free_optimizer_plan( t2t1 );
		hx_free_optimizer_plan( tt1 );
		hx_free_optimizer_plan( tt2 );
	
		hx_free_triple(t1);
		hx_free_triple(t2);
		hx_free_triple(t3);
		
		hx_free_node(paper);
		hx_free_node(person);
		hx_free_node(p);
		hx_free_node(q);
		hx_free_node(r);
	
		if (debug) {
			char* string;
			hx_optimizer_plan_string( ctx, plan, &string );
			fprintf( stdout, "QEP: %s\n", string );
			free(string);
		}
		
		return hx_optimizer_plan_execute( ctx, plan );
	}
}

hx_variablebindings_iter* plan_naive ( hx_execution_context* ctx, hx_model** models, int querynum, int debug ) {
	if (querynum == 1) {
		hx_node* paper	= hx_new_node_named_variable( -1, "paper" );
		hx_node* person	= hx_new_node_named_variable( -2, "person" );
		hx_node* type	= (hx_node*) hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
		hx_node* maker	= (hx_node*) hx_new_node_resource("http://xmlns.com/foaf/0.1/maker");
		hx_node* topic	= (hx_node*) hx_new_node_resource("http://xmlns.com/foaf/0.1/topic");
		hx_node* inproc	= (hx_node*) hx_new_node_resource("http://swrc.ontoware.org/ontology#InProceedings");
		hx_node* semweb	= (hx_node*) hx_new_node_resource("http://dbpedia.org/resource/Semantic_Web");
		hx_triple* t1	= hx_new_triple( paper, type, inproc );
		hx_triple* t2	= hx_new_triple( paper, topic, semweb );
		hx_triple* t3	= hx_new_triple( paper, maker, person );
		
		hx_container_t* typeplans	= hx_optimizer_access_plans( ctx, t1 );
		hx_container_t* topicplans	= hx_optimizer_access_plans( ctx, t2 );
		hx_container_t* makerplans	= hx_optimizer_access_plans( ctx, t3 );
		
		hx_optimizer_plan* typep	= hx_container_item( typeplans, 0 );
		hx_optimizer_plan* topicp	= hx_container_item( topicplans, 0 );
		hx_optimizer_plan* makerp	= hx_container_item( makerplans, 0 );
		
		hx_optimizer_plan* topic1	= set_plan_location( ctx, hx_copy_optimizer_plan( topicp ), 1, models );
		hx_optimizer_plan* topic2	= set_plan_location( ctx, hx_copy_optimizer_plan( topicp ), 2, models );
		hx_optimizer_plan* type1	= set_plan_location( ctx, hx_copy_optimizer_plan( typep ), 1, models );
		hx_optimizer_plan* type2	= set_plan_location( ctx, hx_copy_optimizer_plan( typep ), 2, models );
		hx_optimizer_plan* maker1	= set_plan_location( ctx, hx_copy_optimizer_plan( makerp ), 1, models );
		hx_optimizer_plan* maker2	= set_plan_location( ctx, hx_copy_optimizer_plan( makerp ), 2, models );
		
		hx_container_t* topic_plans		= hx_new_container( 'P', 2 );
		hx_container_push_item( topic_plans, topic1 );
		hx_container_push_item( topic_plans, topic2 );
		hx_optimizer_plan* topic_union		= hx_new_optimizer_union_plan( topic_plans );
		
		
		if (0) {
			hx_optimizer_plan_debug( ctx, topic_union );
			hx_variablebindings_iter* iter	= hx_optimizer_plan_execute( ctx, topic_union );
			int count	= 0;
			fprintf( stderr, "trying to execute topic union sub-plan...\n" );
			while (!hx_variablebindings_iter_finished( iter )) {
				count++;
				hx_variablebindings* b;
				hx_variablebindings_iter_current( iter, &b );
				fprintf( stdout, "Result %d: ", (int) count );
				hx_variablebindings_debug( b );
				hx_free_variablebindings(b);
				hx_variablebindings_iter_next( iter );
			}
			
			hx_free_variablebindings_iter( iter );
			exit(-1);
		}
		
		hx_container_t* type_plans		= hx_new_container( 'P', 2 );
		hx_container_push_item( type_plans, type1 );
		hx_container_push_item( type_plans, type2 );
		hx_optimizer_plan* type_union		= hx_new_optimizer_union_plan( type_plans );
		
		hx_container_t* maker_plans		= hx_new_container( 'P', 2 );
		hx_container_push_item( maker_plans, maker1 );
		hx_container_push_item( maker_plans, maker2 );
		hx_optimizer_plan* maker_union		= hx_new_optimizer_union_plan( maker_plans );
		
		hx_optimizer_plan* tt		= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, topic_union, type_union, topic_union->order, 0 );
		hx_optimizer_plan* ttm		= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, tt, maker_union, tt->order, 0 );
		
		hx_free_triple(t1);
		hx_free_triple(t2);
		hx_free_triple(t3);
		hx_free_node(paper);
		hx_free_node(person);
		hx_free_node(type);
		hx_free_node(maker);
		hx_free_node(topic);
		hx_free_node(inproc);
		hx_free_node(semweb);
		
		if (debug) {
			char* string;
			hx_optimizer_plan_string( ctx, ttm, &string );
			fprintf( stdout, "QEP: %s\n", string );
			free(string);
		}
		
		return hx_optimizer_plan_execute( ctx, ttm );
	} else {
		hx_container_t* sources	= ctx->remote_sources;
		hx_remote_service* s	= hx_container_item( sources, 1 );
		long ilatency			= s->latency1;
		long jlatency			= s->latency2;
		double irate			= s->results_per_second1;
		double jrate			= s->results_per_second2;
		
		hx_triple *t1, *t2, *t3;
		hx_node* p		= hx_new_node_named_variable( -1, "p" );
		hx_node* q		= hx_new_node_named_variable( -2, "q" );
		hx_node* r		= hx_new_node_named_variable( -3, "r" );
		hx_node* knows	= (hx_node*) hx_new_node_resource("http://xmlns.com/foaf/0.1/knows");
		t1	= hx_new_triple( p, knows, q );
		t2	= hx_new_triple( q, knows, r );
		t3	= hx_new_triple( r, knows, p );
		hx_free_node(knows);
		
		
		hx_variablebindings_iter* t1i1		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[0]->store, t1, q), ilatency, irate );
		hx_variablebindings_iter* t1i2		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[1]->store, t1, q), ilatency, irate );
		hx_variablebindings_iter* t1i		= hx_new_union_iter2( ctx, t1i1, t1i2 );
		
		hx_variablebindings_iter* t2i1		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[0]->store, t2, q), ilatency, irate );
		hx_variablebindings_iter* t2i2		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[1]->store, t2, q), ilatency, irate );
		hx_variablebindings_iter* t2i		= hx_new_union_iter2( ctx, t2i1, t2i2 );

		hx_variablebindings_iter* t3i1		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[0]->store, t3, r), ilatency, irate );
		hx_variablebindings_iter* t3i2		= hx_new_delay_iter( hx_store_hexastore_get_statements(models[1]->store, t3, r), ilatency, irate );
		hx_variablebindings_iter* t3i		= hx_new_union_iter2( ctx, t3i1, t3i2 );
		
		{
			int size		= hx_variablebindings_iter_size( t1i1 );
			char** names1	= hx_variablebindings_iter_names( t1i1 );
			char** names2	= hx_variablebindings_iter_names( t1i2 );
			int i;
			for (i = 0; i < size; i++) {
				fprintf( stderr, "name[%d]: '%s', '%s'\n", i, names1[i], names2[i] );
			}
		}
		
		hx_variablebindings_iter* t12i		= hx_new_hashjoin_iter( t1i, t2i );
		hx_variablebindings_iter* t123i		= hx_new_hashjoin_iter( t3i, t12i );
		if (1) {
			debug_iter( t123i );
			exit(0);
		}
		return t123i;
	}
}

void print_count ( long count, hx_variablebindings* b, int comment ) {
// 	fprintf( stderr, "\r" );
// 	fflush(stderr);
	
	struct timeval t;
	gettimeofday (&t, NULL);
	if (comment) fprintf( stdout, "# " );
	fprintf( stdout, "%ld\tresults at\t%ld.%06ld\t", count, (long) t.tv_sec, (long) t.tv_usec );
	if (b != NULL) {
		hx_variablebindings_print( b );
	} else {
		fprintf( stdout, "\n" );
	}
}

int main( int argc, char** argv ) {
	int argi		= 1;
	int dryrun		= 0;
	int debug		= 0;
	int querynum	= 1;
	
	if (argc < 3) {
		help( argc, argv );
		exit(1);
	}
	
	char store_type	= 'H';
	const char* plan_name	= "naive";
	if (argc > 3) {
		while (argi < argc && *(argv[argi]) == '-') {
			if (strcmp(argv[argi], "-naive") == 0) {
				plan_name	= "naive";
			} else if (strcmp(argv[argi], "-optimized") == 0) {
				plan_name	= "optimized";
			} else if (strcmp(argv[argi], "-optimal") == 0) {
				plan_name	= "optimal";
			} else if (strcmp(argv[argi], "-query1") == 0) {
				querynum	= 1;
			} else if (strcmp(argv[argi], "-query2") == 0) {
				querynum	= 2;
			} else if (strcmp(argv[argi], "-n") == 0) {
				dryrun	= 1;
			} else if (strcmp(argv[argi], "-d") == 0) {
				debug	= 1;
			}
			argi++;
		}
	}
	
// 	const char* qf	= argv[ argi++ ];
	fprintf( stderr, "Reading triplestore data...\n" );
	int source_files	= argc - argi;
	char** filenames	= (char**) calloc( source_files, sizeof(char*) );
	hx_model** models	= (hx_model**) calloc( source_files, sizeof(hx_model*) );
	int i;
	for (i = 0; i < source_files; i++) {
		filenames[i]	= argv[ argi++ ];
		FILE* f	= fopen( filenames[i], "r" );
		if (f == NULL) {
			perror( "Failed to open hexastore file for reading: " );
			return 1;
		}
		
		hx_store* store	= hx_store_hexastore_read( NULL, f, 0 );
		if (debug) {
			fprintf( stderr, "%s store (%p) has %ld triples\n", filenames[i], (void*) store, (long) hx_store_size( store ) );
		}
		models[i]	= hx_new_model_with_store( NULL, store );
// 		fprintf( stderr, "model store: %p\n", models[i]->store );
	}
	
	hx_model* hx	= models[0];
	
	
// 	hx_bgp* b;
// 	struct stat st;
// 	int fd	= open( qf, O_RDONLY );
// 	if (fd < 0) {
// 		perror( "Failed to open query file for reading: " );
// 		return 1;
// 	}
// 	
// 	fstat( fd, &st );
// 	fprintf( stderr, "query is %d bytes\n", (int) st.st_size );
// 	FILE* f	= fdopen( fd, "r" );
// 	if (f == NULL) {
// 		perror( "Failed to open query file for reading: " );
// 		return 1;
// 	}
// 	
// 	char* query	= malloc( st.st_size + 1 );
// 	if (query == NULL) {
// 		fprintf( stderr, "*** malloc failed in parse_query.c:main\n" );
// 	}
// 	fread(query, st.st_size, 1, f);
// 	query[ st.st_size ]	= 0;
// 	b	= hx_bgp_parse_string( query );
// //	b	= hx_bgp_parse_string("PREFIX foaf: <http://xmlns.com/foaf/0.1/> { ?x a foaf:Person ; foaf:name ?name . }");
// 	free( query );
// 	
// 	if (b == NULL) {
// 		fprintf( stderr, "Failed to parse query\n" );
// 		return 1;
// 	}
// 	
// 	fprintf( stderr, "basic graph pattern: " );
// 	char* sse;
// 	hx_bgp_sse( b, &sse, "  ", 0 );
// 	fprintf( stdout, "%s\n", sse );
// 	free( sse );

	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );
	ctx->optimizer_access_plans	= (optimizer_access_plans_t) hx_optimizer_federated_access_plans;
	ctx->optimizer_join_plans	= (optimizer_join_plans_t) hx_optimizer_federated_join_plans;
	for (i = 0; i < source_files; i++) {
		hx_remote_service* s	= hx_new_remote_service(filenames[i]);
		if (debug) {
			s->latency1				= 0;
			s->latency2				= 0;
			s->results_per_second1	= 0.0;
			s->results_per_second2	= 0.0;
		} else {
			s->latency1				= 1310;
			s->latency2				= 1516;
			s->results_per_second1	= 1245.727;
			s->results_per_second2	= 728.044;
		}
		hx_execution_context_add_service( ctx, s );
	}
	
// 	fprintf( stderr, "Optimizing query plan...\n" );
	hx_variablebindings_iter* iter;
	if (strcmp(plan_name, "optimal") == 0) {
		fprintf( stderr, "optimal query plan not implemented yet\n" );
		exit(-1);
	} else if (strcmp(plan_name, "optimized") == 0) {
		fprintf( stderr, "Testing OPTIMIZED query plan\n" );
		iter	= plan_full_optimized( ctx, models, querynum, debug );
	} else {
		fprintf( stderr, "Testing NAIVE query plan\n" );
		iter	= plan_naive( ctx, models, querynum, debug );
	}
	
	fprintf( stderr, "Executing query...\n" );
	print_count( 0, NULL, 0 );
	clock_t st_time	= clock();
	uint64_t count	= 0;
	
	if (iter != NULL) {
		while (!hx_variablebindings_iter_finished( iter )) {
			count++;
			hx_variablebindings* b;
			hx_variablebindings_iter_current( iter, &b );
			if (debug) {
				int size		= hx_variablebindings_size( b );
				char** names	= hx_variablebindings_names( b );
				
				fprintf( stdout, "Row %d:\n", (int) count );
				int i;
				for (i = 0; i < size; i++) {
					char* string;
					hx_node* node	= hx_variablebindings_node_for_binding( b, hx->store, i );
					hx_node_string( node, &string );
					fprintf( stdout, "\t%s: %s\n", names[i], string );
					free( string );
				}
				
				hx_free_variablebindings(b);
			} else {
				print_count( count, b, 0 );
// 					fprintf( stderr, "\r%d results", (int) count );
// 					fflush(stderr);
			}
			hx_variablebindings_iter_next( iter );
		}
		
		fprintf( stderr, "Cleaning up iterator...\n" );
		hx_free_variablebindings_iter( iter );
	}
	clock_t end_time	= clock();
	
	struct timeval t;
	gettimeofday (&t, NULL);
	fprintf( stdout, "# finished with %ld total results at %ld.%06ld\n", (long) count, (long) t.tv_sec, (long) t.tv_usec );
	
	fprintf( stderr, "%d total results\n", (int) count );
	fprintf( stderr, "query execution time: %lfs\n", DIFFTIME(st_time, end_time) );
	
	fprintf( stderr, "Cleaning up execution context object...\n" );
	hx_free_execution_context( ctx );
	
//	hx_free_optimizer_plan( plan );
	
// 	fprintf( stderr, "Cleaning up graph pattern object...\n" );
// 	hx_free_bgp(b);
	fprintf( stderr, "Cleaning up triplestore...\n" );
	hx_free_model( hx );
	free( filenames );
	free( models );
	return 0;
}

