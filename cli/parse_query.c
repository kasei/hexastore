#include "parser/SPARQLParser.h"
#include <time.h>
#include "algebra/bgp.h"
#include "algebra/graphpattern.h"
#include "engine/bgp.h"
#include "engine/graphpattern.h"
#include "store/hexastore/hexastore.h"
#include "store/tokyocabinet/tokyocabinet.h"

#define DIFFTIME(a,b) ((b-a)/(double)CLOCKS_PER_SEC)

extern hx_bgp* parse_bgp_query ( void );
extern hx_graphpattern* parse_query_string ( char* );

void help (int argc, char** argv) {
	fprintf( stderr, "Usage:\n" );
	fprintf( stderr, "\t%s -store=S [-n] hexastore.dat [query.rq]\n", argv[0] );
	fprintf( stderr, "\t\tReads a SPARQL query from query.rq or on standard input.\n" );
	fprintf( stderr, "\n\n" );
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
	
	char* filename	= argv[ argi++ ];
	
	hx_hexastore* hx;
	if (store_type == 'T') {
		hx_store* store		= hx_new_store_tokyocabinet( NULL, filename );
		hx		= hx_new_hexastore_with_store( NULL, store );
	} else {
		FILE* f	= fopen( filename, "r" );
		if (f == NULL) {
			perror( "Failed to open hexastore file for reading: " );
			return 1;
		}
		
		hx_store* store			= hx_store_hexastore_read( NULL, f, 0 );
		hx		= hx_new_hexastore_with_store( NULL, store );
	}
	
	hx_bgp* b;
	hx_graphpattern* g;
	if (argi >= argc) {
		b	= parse_bgp_query();
		if (b == NULL) {
			fprintf( stderr, "Failed to parse query\n" );
			return 1;
		}
	} else {
		struct stat st;
		const char* qf	= argv[ argi++ ];
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
		g	= parse_query_string( query );
		free( query );
		
		if (b == NULL) {
			fprintf( stderr, "Failed to parse query\n" );
			return 1;
		}
	}
	
	if (g) {
		hx_graphpattern_debug( g );
	} else {
		char* sse;
		hx_bgp_sse( b, &sse, "  ", 0 );
		fprintf( stdout, "%s\n", sse );
		free( sse );
	}
	
	if (!dryrun) {
		clock_t st_time	= clock();
		uint64_t count	= 0;
		
		hx_variablebindings_iter* iter;
		if (g) {
			iter	= hx_graphpattern_execute( g, hx );
		} else {
			iter	= hx_bgp_execute( b, hx );
		}
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
	}
	
	if (g) {
		hx_free_graphpattern(g);
	}
	
	hx_free_bgp(b);
	hx_free_hexastore( hx );
	return 0;
}

