#include "SPARQLParser.h"
#include <time.h>
#include "bgp.h"
#define DIFFTIME(a,b) ((b-a)/(double)CLOCKS_PER_SEC)

extern hx_bgp* parse_bgp_query ( void );
extern hx_bgp* parse_bgp_query_string ( char* );

void help (int argc, char** argv) {
	fprintf( stderr, "Usage:\n" );
	fprintf( stderr, "\t%s [-n] hexastore.dat [query.rq]\n", argv[0] );
	fprintf( stderr, "\t\tReads a SPARQL query from query.rq or on standard input.\n" );
	fprintf( stderr, "\n\n" );
}

int main( int argc, char** argv ) {
	int argi		= 1;
	int dryrun		= 0;
	
	if (argc < 2) {
		help( argc, argv );
		exit(1);
	} else if (argc > 2) {
		while (argi < argc && *(argv[argi]) == '-') {
			if (strncmp(argv[argi], "-n", 2) == 0) {
				dryrun	= 1;
			}
			argi++;
		}
	}
	
	char* filename	= argv[ argi++ ];
	FILE* f	= fopen( filename, "r" );
	
	if (f == NULL) {
		perror( "Failed to open hexastore file for reading: " );
		return 1;
	}
	
	hx_hexastore* hx	= hx_read( f, 0 );
	hx_nodemap* map		= hx_get_nodemap( hx );
	
	hx_bgp* b;
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
		b	= parse_bgp_query_string( query );
		free( query );
		if (b == NULL) {
			fprintf( stderr, "Failed to parse query\n" );
			return 1;
		}
	}
	
	char* sse;
	hx_bgp_sse( b, &sse, "  ", 0 );
	fprintf( stdout, sse );
	free( sse );
	
	if (!dryrun) {
		clock_t st_time	= clock();
		uint64_t count	= 0;
		hx_variablebindings_iter* iter	= hx_bgp_execute( b, hx );
		int size		= hx_variablebindings_iter_size( iter );
		char** names	= hx_variablebindings_iter_names( iter );
	// 	for (int i = 0; i < size; i++) {
	// 		fprintf( stderr, "column: %s\n", names[i] );
	// 	}
		
		if (iter != NULL) {
			while (!hx_variablebindings_iter_finished( iter )) {
				count++;
				hx_variablebindings* b;
				hx_variablebindings_iter_current( iter, &b );
				
				fprintf( stdout, "Row %d:\n", (int) count );
				int i;
				for (i = 0; i < size; i++) {
					char* string;
					hx_node* node	= hx_variablebindings_node_for_binding( b, map, i );
					hx_node_string( node, &string );
					fprintf( stdout, "\t%s: %s\n", names[i], string );
					free( string );
				}
				
				hx_free_variablebindings(b);
				hx_variablebindings_iter_next( iter );
			}
		}
		clock_t end_time	= clock();
		
		fprintf( stderr, "%d total results\n", (int) count );
		fprintf( stderr, "query execution time: %lfs\n", DIFFTIME(st_time, end_time) );
	}
	
	hx_free_hexastore( hx );
	return 0;
}

