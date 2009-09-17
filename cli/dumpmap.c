#include <stdio.h>
#include <stdlib.h>
#include "mentok.h"
#include "misc/nodemap.h"
#include "rdf/node.h"
#include "engine/mergejoin.h"
#include "misc/avl.h"

void help (int argc, char** argv) {
	fprintf( stderr, "Usage:\n" );
	fprintf( stderr, "\t%s nodemap.data\n", argv[0] );
	fprintf( stderr, "\n\n" );
}

int main (int argc, char** argv) {
	const char* filename	= NULL;
	char* arg				= NULL;
	
	if (argc < 2) {
		help(argc, argv);
		exit(1);
	}

	filename	= argv[1];
	if (argc > 2)
		arg		= argv[2];
	
	FILE* f	= fopen( filename, "r" );
	if (f == NULL) {
		perror( "Failed to open nodemap file for reading: " );
		return 1;
	}
	
	hx_nodemap* map			= hx_nodemap_read( f, 0 );
	
	// print out the nodemap
	size_t used	= avl_count( map->id2node );
	struct avl_traverser iter;
	avl_t_init( &iter, map->id2node );
	hx_nodemap_item* item;
	while ((item = (hx_nodemap_item*) avl_t_next( &iter )) != NULL) {
		char* string;
		hx_node_string( item->node, &string );
		fprintf( stdout, "%-10"PRIdHXID"\t%s\n", item->id, string );
		free( string );
	}
	
	hx_free_nodemap( map );
	return 0;
}
