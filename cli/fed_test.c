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

int main( int argc, char** argv ) {
	int argi		= 1;
	int dryrun		= 0;
	int optimize	= 0;
	
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
			} else if (strncmp(argv[argi], "-o",2) == 0) {
				optimize	= 1;
			}
			argi++;
		}
	}
	
	const char* qf	= argv[ argi++ ];
	int source_files	= argc - argi;
	char** filenames	= (char**) calloc( source_files, sizeof(char*) );
	int i;
	for (i = 0; i < source_files; i++) {
		filenames[i]	= argv[ argi++ ];
	}
	
	char* filename	= filenames[0];
	
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
		hx		= hx_new_model_with_store( NULL, store );
	}
	
	hx_bgp* b;
	struct stat st;
	int fd	= open( qf, O_RDONLY );
	if (fd < 0) {
		perror( "Failed to open query file for reading: " );
		return 1;
	}
	
	fstat( fd, &st );
	fprintf( stderr, "query is %d bytes\n", (int) st.st_size );
	FILE* f	= fdopen( fd, "r" );
	if (f == NULL) {
		perror( "Failed to open query file for reading: " );
		return 1;
	}
	
	char* query	= malloc( st.st_size + 1 );
	if (query == NULL) {
		fprintf( stderr, "*** malloc failed in parse_query.c:main\n" );
	}
	fread(query, st.st_size, 1, f);
	query[ st.st_size ]	= 0;
	b	= hx_bgp_parse_string( query );
//	b	= hx_bgp_parse_string("PREFIX foaf: <http://xmlns.com/foaf/0.1/> { ?x a foaf:Person ; foaf:name ?name . }");
	free( query );
	
	if (b == NULL) {
		fprintf( stderr, "Failed to parse query\n" );
		return 1;
	}
	
	fprintf( stderr, "basic graph pattern: " );
	char* sse;
	hx_bgp_sse( b, &sse, "  ", 0 );
	fprintf( stdout, "%s\n", sse );
	free( sse );

	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );
	ctx->optimizer_access_plans	= (optimizer_access_plans_t) hx_optimizer_federated_access_plans;
	ctx->optimizer_join_plans	= (optimizer_join_plans_t) hx_optimizer_federated_join_plans;
	for (i = 0; i < source_files; i++) {
		hx_execution_context_add_service( ctx, hx_new_remote_service(filenames[i]) );
	}
	
	if (optimize) {
		char* string;
		hx_optimizer_plan* plan	= hx_optimizer_optimize_bgp( ctx, b );
		hx_optimizer_plan_string( ctx, plan, &string );
		fprintf( stdout, "QEP: %s\n", string );
		free(string);
	}
	
	
	if (!dryrun) {
		clock_t st_time	= clock();
		uint64_t count	= 0;
		
		hx_variablebindings_iter* iter;
		iter	= hx_bgp_execute( ctx, b );
	// 	for (int i = 0; i < size; i++) {
	// 		fprintf( stderr, "column: %s\n", names[i] );
	// 	}
		
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
			hx_free_variablebindings_iter( iter );
		}
		clock_t end_time	= clock();
		
		fprintf( stderr, "%d total results\n", (int) count );
		fprintf( stderr, "query execution time: %lfs\n", DIFFTIME(st_time, end_time) );
		
		hx_free_execution_context( ctx );
	}
	
	hx_free_bgp(b);
	hx_free_model( hx );
	return 0;
}

