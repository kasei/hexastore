#include <stdio.h>
#include <stdlib.h>
#include "hexastore.h"
#include "nodemap.h"
#include "node.h"
#include "mergejoin.h"
#include "avl.h"

typedef struct {
	int next;
	struct avl_table* name2id;
	struct avl_table* id2name;
} varmap_t;

typedef struct {
	char* name;
	int id;
} varmap_item_t;

int _varmap_name2id_cmp ( const void* a, const void* b, void* param ) {
	return strcmp( (const char*) a, (const char*) b );
}

int _varmap_id2name_cmp ( const void* a, const void* b, void* param ) {
	varmap_item_t* _a	= (varmap_item_t*) a;
	varmap_item_t* _b	= (varmap_item_t*) b;
	return (_a->id - _b->id);
}

hx_node* node_for_string ( char* string, hx_hexastore* hx );
hx_node_id node_id_for_string ( char* string, hx_hexastore* hx );
hx_node* node_for_string_with_varmap ( char* string, hx_hexastore* hx, varmap_t* varmap );
void print_triple ( hx_nodemap* map, hx_node_id s, hx_node_id p, hx_node_id o, int count );
char* variable_name ( varmap_t* varmap, int id );

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
		fprintf( stdout, "%-10"PRIuHXID"\t%s\n", item->id, string );
		free( string );
	}
	
	hx_free_nodemap( map );
	return 0;
}

void print_triple ( hx_nodemap* map, hx_node_id s, hx_node_id p, hx_node_id o, int count ) {
// 	fprintf( stderr, "[%d] %d, %d, %d\n", count++, (int) s, (int) p, (int) o );
	hx_node* sn	= hx_nodemap_get_node( map, s );
	hx_node* pn	= hx_nodemap_get_node( map, p );
	hx_node* on	= hx_nodemap_get_node( map, o );
	char *ss, *sp, *so;
	hx_node_string( sn, &ss );
	hx_node_string( pn, &sp );
	hx_node_string( on, &so );
	if (count > 0) {
		fprintf( stdout, "[%d] ", count );
	}
	fprintf( stdout, "%s %s %s\n", ss, sp, so );
	free( ss );
	free( sp );
	free( so );
}

hx_node_id node_id_for_string ( char* string, hx_hexastore* hx ) {
	hx_nodemap* map	= hx_get_nodemap( hx );
	static int var_id	= -100;
	hx_node_id id;
	hx_node* node;
	if (strcmp( string, "-" ) == 0) {
		id	= (hx_node_id) var_id--;
	} else if (strcmp( string, "0" ) == 0) {
		id	= (hx_node_id) 0;
	} else if (*string == '-') {
		id	= 0 - atoi( string+1 );
	} else {
		node	= hx_new_node_resource( string );
		id		= hx_nodemap_get_node_id( map, node );
		hx_free_node( node );
		if (id <= 0) {
			fprintf( stderr, "No such subject found: '%s'.\n", string );
		}
	}
	return id;
}

hx_node* node_for_string ( char* string, hx_hexastore* hx ) {
	if (strcmp( string, "-" ) == 0) {
		return hx_new_variable( hx );
	} else if (strcmp( string, "0" ) == 0) {
		return NULL;
	} else if (*string == '-') {
		return hx_new_node_variable( 0 - atoi( string+1 ) );
	} else {
		return hx_new_node_resource( string );
	}
}

hx_node* node_for_string_with_varmap ( char* string, hx_hexastore* hx, varmap_t* varmap ) {
	if (strcmp( string, "-" ) == 0) {
		return hx_new_variable( hx );
	} else if (string[0] == '?') {
		varmap_item_t* item	= (varmap_item_t*) avl_find( varmap->name2id, &( string[1] ) );
		if (item == NULL) {
			item	= (varmap_item_t*) calloc( 1, sizeof(varmap_item_t) );
			item->name	= &( string[1] );
			item->id		= --( varmap->next );
			avl_insert( varmap->name2id, item );
			avl_insert( varmap->id2name, item );
		}
		return hx_new_node_variable( item->id );
	} else if (strcmp( string, "0" ) == 0) {
		return NULL;
	} else if (*string == '-') {
		return hx_new_node_variable( 0 - atoi( string+1 ) );
	} else {
		return hx_new_node_resource( string );
	}
}

char* variable_name ( varmap_t* varmap, int id ) {
	varmap_item_t i;
	i.id	= id;
	varmap_item_t* item	= (varmap_item_t*) avl_find( varmap->id2name, &i );
	if (item != NULL) {
		return item->name;
	} else {
		return NULL;
	}
}
