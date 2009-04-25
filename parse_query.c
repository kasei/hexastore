#include "SPARQLParser.h"
#include "bgp.h"

extern hx_bgp* parse_bgp_query ( void );

int main( int argc, char** argv ) {
	char* filename	= argv[1];
	FILE* f	= fopen( filename, "r" );
	if (f == NULL) {
		perror( "Failed to open hexastore file for reading: " );
		return 1;
	}
	
	hx_storage_manager* st	= hx_new_memory_storage_manager();
	hx_hexastore* hx	= hx_read( st, f, 0 );
	hx_nodemap* map		= hx_get_nodemap( hx );;
	
	
	
	
	hx_bgp* b	= parse_bgp_query();
	if (b == NULL) {
		fprintf( stderr, "Failed to parse query\n" );
		return 1;
	}

	uint64_t count	= 0;
	hx_variablebindings_iter* iter	= hx_bgp_execute( b, hx, st );
	int size		= hx_variablebindings_iter_size( iter );
	char** names	= hx_variablebindings_iter_names( iter );
	for (int i = 0; i < size; i++) {
		fprintf( stderr, "column: %s\n", names[i] );
	}
	
	if (iter != NULL) {
		while (!hx_variablebindings_iter_finished( iter )) {
			count++;
			hx_variablebindings* b;
			hx_variablebindings_iter_current( iter, &b );
			
			fprintf( stdout, "Row %d:\n", (int) count );
			for (int i = 0; i < size; i++) {
				char* string;
				hx_node* node	= hx_variablebindings_node_for_binding( b, map, i );
				hx_node_string( node, &string );
				fprintf( stdout, "\t%s: %s\n", names[i], string );
				free( string );
			}
			
			hx_free_variablebindings( b, 0 );
			hx_variablebindings_iter_next( iter );
		}
	}
	
	fprintf( stderr, "%d total results\n", (int) count );
	hx_free_hexastore( hx, st );
	hx_free_storage_manager( st );
	return 0;
}

