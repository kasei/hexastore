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

hx_nodemap* hx_new_file_nodemap( const char* directory ) {
	hx_nodemap* m	= (hx_nodemap*) calloc( 1, sizeof( hx_nodemap ) );
	m->type			= 'B';
	m->id2node		= tcbdbnew();
	m->node2id		= tcbdbnew();
	m->directory	= directory;
	
	int len			= strlen( directory ) + 13;
	char* i2n		= (char*) malloc( len );
	char* n2i		= (char*) malloc( len );
	sprintf( n2i, "%s/node2id.tcb", directory );
	sprintf( i2n, "%s/id2node.tcb", directory );
	
	if(!tcbdbopen(m->id2node, i2n, BDBOWRITER | BDBOCREAT)){
		int ecode = tcbdbecode(m->id2node);
		fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
		free(m);
		return NULL;
	}
	
	if(!tcbdbopen(m->node2id, n2i, BDBOWRITER | BDBOCREAT)){
		int ecode = tcbdbecode(m->node2id);
		fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
		tcbdbdel(m->id2node);
		free(m);
		return NULL;
	}
	
	m->next_id		= (hx_node_id) 1;
	return m;
}

int hx_remove_nodemap ( hx_nodemap* m ) {
	hx_free_nodemap( m );
	char* directory	= m->directory;
	int len			= strlen( directory ) + 13;
	char* i2n		= (char*) malloc( len );
	char* n2i		= (char*) malloc( len );
	sprintf( n2i, "%s/node2id.tcb", directory );
	sprintf( i2n, "%s/id2node.tcb", directory );
	unlink( n2i );
	unlink( i2n );
	return 0;
}

int hx_free_nodemap ( hx_nodemap* m ) {
	if (m->type == 'M') {
		tcmdbvanish( m->id2node );
		tcmdbvanish( m->node2id );
	} else {
		tcbdbdel( m->id2node );
		tcbdbdel( m->node2id );
	}
	free( m );
	return 0;
}

hx_node_id hx_nodemap_add_node ( hx_nodemap* m, hx_node* n ) {
	int size;
	if (m->type == 'M') {
		char* string;
		hx_node_nodestr( n, &string );
		int len	= strlen(string);
		hx_node_id* p	= tcmdbget(m->node2id, string, len, &size);
		if (p == NULL) {
			hx_node_id id	= m->next_id++;
			tcmdbputkeep(m->node2id, string, len, &id, sizeof( hx_node_id ));
			tcmdbputkeep(m->id2node, &id, sizeof( hx_node_id ), string, len);
			free(string);
			return id;
		} else {
			free(string);
			return *p;
		}
	} else {
		char* string;
		hx_node_nodestr( n, &string );
		int len	= strlen(string);
		hx_node_id* p	= tcbdbget(m->node2id, string, len, &size);
		if (p == NULL) {
			hx_node_id id	= m->next_id++;
			tcbdbputkeep(m->node2id, string, len, &id, sizeof( hx_node_id ));
			tcbdbputkeep(m->id2node, &id, sizeof( hx_node_id ), string, len);
			free(string);
			return id;
		} else {
			free(string);
			return *p;
		}
	}
}

int hx_nodemap_remove_node_id ( hx_nodemap* m, hx_node_id id ) {
	int size;
	if (m->type == 'M') {
		hx_node* p	= tcmdbget(m->id2node, &id, sizeof( hx_node_id ), &size);
		if (p != NULL) {
			tcmdbout( m->node2id, p, size );
			tcmdbout( m->id2node, &id, sizeof( hx_node_id ) );
			return 0;
		} else {
			return 1;
		}
	} else {
		hx_node* p	= tcbdbget(m->id2node, &id, sizeof( hx_node_id ), &size);
		if (p != NULL) {
			tcbdbout( m->node2id, p, size );
			tcbdbout( m->id2node, &id, sizeof( hx_node_id ) );
			return 0;
		} else {
			return 1;
		}
	}
}

int hx_nodemap_remove_node ( hx_nodemap* m, hx_node* n ) {
	int size;
	char* string;
	hx_node_nodestr( n, &string );
	int len	= strlen(string);
	if (m->type == 'M') {
		hx_node_id* p	= tcmdbget(m->node2id, string, len, &size);
		if (p != NULL) {
			tcmdbout( m->node2id, string, len );
			tcmdbout( m->id2node, p, size );
			free(string);
			return 0;
		} else {
			free(string);
			return 1;
		}
	} else {
		hx_node_id* p	= tcbdbget(m->node2id, string, len, &size);
		if (p != NULL) {
			tcbdbout( m->node2id, string, len );
			tcbdbout( m->id2node, p, size );
			free(string);
			return 0;
		} else {
			free(string);
			return 1;
		}
	}
}

hx_node_id hx_nodemap_get_node_id ( hx_nodemap* m, hx_node* n ) {
	int size;
	char* string;
	hx_node_nodestr( n, &string );
	int len	= strlen(string);
	if (m->type == 'M') {
		hx_node_id* p	= tcmdbget(m->node2id, string, len, &size);
		free(string);
		if (p == NULL) {
			return (hx_node_id) 0;
		} else {
			return *p;
		}
	} else {
		hx_node_id* p	= tcbdbget(m->node2id, string, len, &size);
		free(string);
		if (p == NULL) {
			return (hx_node_id) 0;
		} else {
			return *p;
		}
	}
}

hx_node* hx_nodemap_get_node ( hx_nodemap* m, hx_node_id id ) {
	int size;
	if (m->type == 'M') {
		hx_node* p	= tcmdbget(m->id2node, &id, sizeof( hx_node_id ), &size);
		if (p == NULL) {
			return NULL;
		} else {
			char* string	= (char*) malloc( size + 1 );
			memcpy( string, p, size );
			string[size]	= (char) 0;
			hx_node* node	= hx_node_strnode( string );
			free( string );
			return node;
		}
	} else {
		char* p	= tcbdbget(m->id2node, &id, sizeof( hx_node_id ), &size);
		if (p == NULL) {
			return NULL;
		} else {
			char* string	= (char*) malloc( size + 1 );
			memcpy( string, p, size );
			string[size]	= (char) 0;
			hx_node* node	= hx_node_strnode( string );
			free( string );
			return node;
		}
	}
}

int hx_nodemap_debug ( hx_nodemap* m ) {
	int size;
	if (m->type == 'M') {
		tcmdbiterinit(m->id2node);
		
		hx_node_id* key;
		void* value;
		fprintf( stderr, "Nodemap:\n" );
		while ((key = tcmdbiternext(m->id2node, &size)) != NULL) {
			hx_node* node	= hx_nodemap_get_node( m, *key );
			char* string;
			hx_node_string( node, &string );
			fprintf( stderr, "\t%d -> %s\n", (int) *key, string );
			free( string );
			hx_free_node( node );
		}
		return 0;
	} else {
		hx_node_id* key;
		void* value;
		BDBCUR *cur	= tcbdbcurnew(m->id2node);
		tcbdbcurfirst(cur);
		while((key = tcbdbcurkey(cur, &size)) != NULL) {
			value	= tcbdbcurval(cur, &size);
			char* nodestr	= (char*) malloc( size + 1 );
			memcpy( nodestr, value, size );
			nodestr[size]	= (char) 0;
			hx_node* node	= hx_node_strnode( nodestr );
			char* string;
			hx_node_string( node, &string );
			fprintf( stderr, "\t%d -> %s\n", (int) *key, string );
			free( string );
			free( nodestr );
			hx_free_node( node );
		}
		tcbdbcurdel(cur);
	}
}

int hx_nodemap_write( hx_nodemap* m, FILE* f ) {
	if (m->type == 'M') {
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
	} else {
		fprintf( stderr, "*** Cannot call hx_nodemap_write on an already disk-based nodemap\n" );
		return 1;
	}
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
	int size;
	size_t count;
	if (map->type == 'M') {
		count	= tcmdbrnum( map->id2node );
	} else {
		count	= tcbdbrnum( map->id2node );
	}
	hx_node** node_handles	= (hx_node**) calloc( count, sizeof( hx_node* ) );
	int i	= 0;
	
	hx_node_id* id;
	if (map->type == 'M') {
		tcmdbiterinit(map->id2node);
		while ((id = tcmdbiternext(map->id2node, &size)) != NULL) {
			hx_node* node	= hx_nodemap_get_node( map, *id );
			node_handles[ i++ ]	= node;
		}
	} else {
		hx_node_id* key;
		void* value;
		BDBCUR *cur	= tcbdbcurnew(map->id2node);
		tcbdbcurfirst(cur);
		while((key = tcbdbcurkey(cur, &size)) != NULL) {
			value	= tcbdbcurval(cur, &size);
			char* nodestr	= (char*) malloc( size + 1 );
			memcpy( nodestr, value, size );
			nodestr[size]	= (char) 0;
			hx_node* node	= hx_node_strnode( nodestr );
			node_handles[ i++ ]	= node;
			free( nodestr );
		}
		tcbdbcurdel(cur);
	
	}
	qsort( node_handles, i, sizeof( hx_node* ), _hx_nodemap_cmp_nodes );
	hx_nodemap* sorted	= hx_new_nodemap();
	for (int j = 0; j < i; j++) {
		hx_nodemap_add_node( sorted, node_handles[ j ] );
		if (map->type != 'M') {
			hx_free_node( node_handles[j] );
		}
	}
	free( node_handles );
	return sorted;
}

int _hx_nodemap_cmp_nodes ( const void* _a, const void* _b ) {
	hx_node** a	= (hx_node**) _a;
	hx_node** b	= (hx_node**) _b;
	return hx_node_cmp( *a, *b );
}
