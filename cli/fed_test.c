#include "mentok/parser/SPARQLParser.h"
#include <time.h>
#include "mentok/algebra/bgp.h"
#include "mentok/algebra/graphpattern.h"
#include "mentok/engine/bgp.h"
#include "mentok/engine/graphpattern.h"
#include "mentok/store/hexastore/hexastore.h"
#include "mentok/store/tokyocabinet/tokyocabinet.h"
#include "mentok/optimizer/optimizer.h"
#include "mentok/optimizer/optimizer-federated.h"

#define DIFFTIME(a,b) ((b-a)/(double)CLOCKS_PER_SEC)

extern hx_bgp* parse_bgp_query ( void );
extern hx_graphpattern* parse_query_string ( char* );

void help (int argc, char** argv) {
	fprintf( stderr, "Usage: %s -store=S [-n] query.rq hexastore.dat\n", argv[0] );
	fprintf( stderr, "    Reads a SPARQL query from query.rq or on standard input.\n\n" );
	fprintf( stderr, "    S must be one of the following:\n" );
	fprintf( stderr, "        'T' - Use the tokyocabinet backend with files stored in the directory data/\n" );
	fprintf( stderr, "        'H' - Use the hexastore memory backend serialized to the file data.\n\n" );
	fprintf( stderr, "\n\n" );
}

hx_optimizer_plan* set_plan_location ( hx_optimizer_plan* plan, int location ) {
	plan->location	= location;
	return plan;
}

int main( int argc, char** argv ) {
	int argi		= 1;
	int dryrun		= 0;
	
	if (argc < 3) {
		help( argc, argv );
		exit(1);
	}
	
	char store_type	= 'T';
	if (strncmp(argv[argi], "-store=", 7) == 0) {
		switch (argv[argi][7]) {
			case 'T':
				store_type	= 'T';
				break;
			case 'H':
				store_type	= 'H';
				break;
			default:
				fprintf( stderr, "Unrecognized store type.\n\n" );
				exit(1);
		};
		argi++;
	} else {
		fprintf( stderr, "No store type specified.\n" );
		exit(1);
	}
	
	
	if (argc > 3) {
		while (argi < argc && *(argv[argi]) == '-') {
			if (strncmp(argv[argi], "-n", 2) == 0) {
				dryrun	= 1;
			}
			argi++;
		}
	}
	
// 	const char* qf	= argv[ argi++ ];
	int source_files	= argc - argi;
	char** filenames	= (char**) calloc( source_files, sizeof(char*) );
	int i;
	for (i = 0; i < source_files; i++) {
		filenames[i]	= argv[ argi++ ];
	}
	
	char* filename	= filenames[0];
	
	fprintf( stderr, "Reading triplestore data...\n" );
	hx_model* hx;
	if (store_type == 'T') {
		hx_store* store		= hx_new_store_tokyocabinet( NULL, filename );
		hx		= hx_new_model_with_store( NULL, store );
	} else {
		FILE* f	= fopen( filename, "r" );
		if (f == NULL) {
			perror( "Failed to open hexastore file for reading: " );
			return 1;
		}
		
		hx_store* store			= hx_store_hexastore_read( NULL, f, 0 );
		fprintf( stderr, "store: %p\n", store );
		hx		= hx_new_model_with_store( NULL, store );
		fprintf( stderr, "model store: %p\n", hx->store );
	}
	
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
		hx_execution_context_add_service( ctx, hx_new_remote_service(filenames[i]) );
	}
	
	fprintf( stderr, "Optimizing query plan...\n" );
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
	
	hx_optimizer_plan* tt		= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, topicp, typep, topicp->order, 0 );
	hx_optimizer_plan* ttm		= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, tt, makerp, tt->order, 0 );
	
	
	
	hx_optimizer_plan* topic1	= set_plan_location( hx_copy_optimizer_plan( topicp ), 1 );
	hx_optimizer_plan* topic2	= set_plan_location( hx_copy_optimizer_plan( topicp ), 2 );
	hx_optimizer_plan* type1	= set_plan_location( hx_copy_optimizer_plan( typep ), 1 );
	hx_optimizer_plan* type2	= set_plan_location( hx_copy_optimizer_plan( typep ), 2 );
	hx_optimizer_plan* maker1	= set_plan_location( hx_copy_optimizer_plan( makerp ), 1 );
	hx_optimizer_plan* maker2	= set_plan_location( hx_copy_optimizer_plan( makerp ), 2 );
	
	hx_optimizer_plan* t1t2		= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, topic1, type2, topic1->order, 0 );
	hx_optimizer_plan* t2t1		= hx_new_optimizer_join_plan( HX_OPTIMIZER_PLAN_HASHJOIN, topic2, type1, topic2->order, 0 );
	
	hx_optimizer_plan* tt1		= set_plan_location( hx_copy_optimizer_plan( tt ), 1 );
	hx_optimizer_plan* tt2		= set_plan_location( hx_copy_optimizer_plan( tt ), 2 );
	
	
	// final sub plans:
	hx_optimizer_plan* ttm1		= set_plan_location( hx_copy_optimizer_plan( ttm ), 1 );
	hx_optimizer_plan* ttm2		= set_plan_location( hx_copy_optimizer_plan( ttm ), 2 );
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
	
	char* string;
	hx_optimizer_plan_string( ctx, plan, &string );
	fprintf( stdout, "QEP: %s\n", string );
	free(string);
	
	
	if (!dryrun) {
		fprintf( stderr, "Executing query...\n" );
		clock_t st_time	= clock();
		uint64_t count	= 0;
		
// 		hx_variablebindings_iter* iter;
// 		iter	= hx_bgp_execute( ctx, b );
		
		hx_variablebindings_iter* iter	= hx_optimizer_plan_execute( ctx, plan );
		
		if (0) {
			hx_variablebindings_iter_debug( iter, "> ", 0 );
		} else {
			if (iter != NULL) {
				while (!hx_variablebindings_iter_finished( iter )) {
					count++;
					hx_variablebindings* b;
					hx_variablebindings_iter_current( iter, &b );
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
					hx_variablebindings_iter_next( iter );
				}
				
				fprintf( stderr, "Cleaning up iterator...\n" );
				hx_free_variablebindings_iter( iter );
			}
		}
		clock_t end_time	= clock();
		
		fprintf( stderr, "%d total results\n", (int) count );
		fprintf( stderr, "query execution time: %lfs\n", DIFFTIME(st_time, end_time) );
		
		fprintf( stderr, "Cleaning up execution context object...\n" );
		hx_free_execution_context( ctx );
	}
	
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
	hx_free_optimizer_plan( plan );

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
	
// 	fprintf( stderr, "Cleaning up graph pattern object...\n" );
// 	hx_free_bgp(b);
	fprintf( stderr, "Cleaning up triplestore...\n" );
	hx_free_model( hx );
	free( filenames );
	return 0;
}

