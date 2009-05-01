#include "nodemap.h"

int _hx_nodemap_cmp_nodes ( const void* _a, const void* _b );
// 
// 
// // int _sparql_sort_cmp (const void * a, const void * b);
// int _hx_node_cmp_id ( const void* a, const void* b, void* param ) {
// 	hx_nodemap_item* ia	= (hx_nodemap_item*) a;
// 	hx_nodemap_item* ib	= (hx_nodemap_item*) b;
// // 	fprintf( stderr, "hx_node_cmp_id( %d, %d )\n", (int) ia->id, (int) ib->id );
// 	return (ia->id - ib->id);
// }
// 
// int _hx_node_cmp_str ( const void* a, const void* b, void* param ) {
// 	hx_nodemap_item* ia	= (hx_nodemap_item*) a;
// 	hx_nodemap_item* ib	= (hx_nodemap_item*) b;
// 	return hx_node_cmp(ia->node, ib->node);
// }
// 
// void _hx_free_node_item (void *avl_item, void *avl_param) {
// 	hx_nodemap_item* i	= (hx_nodemap_item*) avl_item;
// 	if (i->node != NULL) {
// 		hx_free_node( i->node );
// 	}
// 	free( i );
// }

hx_nodemap* hx_new_nodemap( void ) {
	hx_nodemap* m	= (hx_nodemap*) calloc( 1, sizeof( hx_nodemap ) );
	m->type			= 'M';
	m->id2node		= tcmdbnew();
	m->node2id		= tcmdbnew();
	m->next_id		= (hx_node_id) 1;
	return m;
}

int hx_free_nodemap ( hx_nodemap* m ) {
	tcmdbvanish( m->id2node );
	tcmdbvanish( m->node2id );
	free( m );
	return 0;
}

hx_node_id hx_nodemap_add_node ( hx_nodemap* m, hx_node* n ) {
	hx_node* node	= hx_node_copy( n );
// 	hx_nodemap_item i;
// 	i.node	= node;
	
	int size;
	hx_node_id* p	= tcmdbget(m->node2id, n, sizeof( hx_node ), &size);
	if (p == NULL) {
		if (0) {
			char* nodestr;
			hx_node_string( node, &nodestr );
			fprintf( stderr, "nodemap adding key '%s'\n", nodestr );
			free(nodestr);
		}
		
		hx_node_id id	= m->next_id++;
		tcmdbputkeep(m->node2id, n, sizeof( hx_node ), &id, sizeof( hx_node_id ));
		tcmdbputkeep(m->id2node, &id, sizeof( hx_node_id ), n, sizeof( hx_node ));
		
		if (0) {
			hx_node_id id	= hx_nodemap_get_node_id( m, node );
			fprintf( stderr, "*** After adding: %d\n", (int) id );
		}
		
		return id;
	} else {
		hx_free_node( node );
		return *p;
	}
}

int hx_nodemap_remove_node_id ( hx_nodemap* m, hx_node_id id ) {
	int size;
	hx_node* p	= tcmdbget(m->id2node, &id, sizeof( hx_node_id ), &size);
	if (p != NULL) {
		tcmdbout( m->node2id, p, sizeof( hx_node ) );
		tcmdbout( m->id2node, &id, size );
		return 0;
	} else {
		return 1;
	}
}

int hx_nodemap_remove_node ( hx_nodemap* m, hx_node* n ) {
	int size;
	hx_node_id* p	= tcmdbget(m->node2id, n, sizeof( hx_node ), &size);
	if (p != NULL) {
		tcmdbout( m->node2id, n, sizeof( hx_node ) );
		tcmdbout( m->id2node, p, size );
		return 0;
	} else {
		return 1;
	}
}

hx_node_id hx_nodemap_get_node_id ( hx_nodemap* m, hx_node* n ) {
	int size;
	hx_node_id* p	= tcmdbget(m->node2id, n, sizeof( hx_node ), &size);
	if (p == NULL) {
		return (hx_node_id) 0;
	} else {
		return *p;
	}
}

hx_node* hx_nodemap_get_node ( hx_nodemap* m, hx_node_id id ) {
	int size;
	hx_node* p	= tcmdbget(m->id2node, &id, sizeof( hx_node_id ), &size);
	if (p == NULL) {
		return NULL;
	} else {
		return p;
	}
}

int hx_nodemap_debug ( hx_nodemap* m ) {
	tcmdbiterinit(m->id2node);
	
	int size;
	hx_node_id* id;
	fprintf( stderr, "Nodemap:\n" );
	while ((id = tcmdbiternext(m->id2node, &size)) != NULL) {
		hx_node* node	= hx_nodemap_get_node( m, *id );
		char* string;
		hx_node_string( node, &string );
		fprintf( stderr, "\t%d -> %s\n", (int) id, string );
		free( string );
	}
	return 0;
}

int hx_nodemap_write( hx_nodemap* m, FILE* f ) {
	fputc( 'M', f );
	size_t used	= tcmdbrnum( m->id2node );
	fwrite( &used, sizeof( size_t ), 1, f );
	fwrite( &( m->next_id ), sizeof( hx_node_id ), 1, f );
	
	int size;
	hx_node_id* id;
	tcmdbiterinit(m->id2node);
	while ((id = tcmdbiternext(m->id2node, &size)) != NULL) {
		hx_node* node	= hx_nodemap_get_node( m, *id );
		fwrite( id, sizeof( hx_node_id ), 1, f );
		hx_node_write( node, f );
	}
	
	return 0;
}

hx_nodemap* hx_nodemap_read( hx_storage_manager* s, FILE* f, int buffer ) {
	size_t used, read;
	hx_node_id next_id;
	int c	= fgetc( f );
	if (c != 'M') {
		fprintf( stderr, "*** Bad header cookie trying to read nodemap from file.\n" );
		return NULL;
	}
	
	hx_nodemap* m	= hx_new_nodemap();
	read	= fread( &used, sizeof( size_t ), 1, f );
	read	= fread( &next_id, sizeof( hx_node_id ), 1, f );
	m->next_id	= next_id;
	for (int i = 0; i < used; i++) {
		hx_node_id id;
		if ((read = fread( &id, sizeof( hx_node_id ), 1, f )) == 0) {
			fprintf( stderr, "*** Failed to read item hx_node_id\n" );
		}
		hx_node* n	= hx_node_read( f, 0 );
		tcmdbputkeep(m->node2id, n, sizeof( hx_node ), &id, sizeof( hx_node_id ));
		tcmdbputkeep(m->id2node, &id, sizeof( hx_node_id ), n, sizeof( hx_node ));
	}
	return m;
}

hx_nodemap* hx_nodemap_sparql_order_nodes ( hx_nodemap* map ) {
	size_t count	= tcmdbrnum( map->id2node );
	hx_node** node_handles	= (hx_node**) calloc( count, sizeof( hx_node* ) );
	int i	= 0;
	
	
	int size;
	hx_node_id* id;
	tcmdbiterinit(map->id2node);
	while ((id = tcmdbiternext(map->id2node, &size)) != NULL) {
		hx_node* node	= hx_nodemap_get_node( map, *id );
		node_handles[ i++ ]	= node;
	}
	qsort( node_handles, i, sizeof( hx_node* ), _hx_nodemap_cmp_nodes );
	hx_nodemap* sorted	= hx_new_nodemap();
	for (int j = 0; j < i; j++) {
		hx_nodemap_add_node( sorted, node_handles[ j ] );
	}
	free( node_handles );
	return sorted;
}

int _hx_nodemap_cmp_nodes ( const void* _a, const void* _b ) {
	hx_node** a	= (hx_node**) _a;
	hx_node** b	= (hx_node**) _b;
	return hx_node_cmp( *a, *b );
}
