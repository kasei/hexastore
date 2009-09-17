#include "mentok/misc/idmap.h"
typedef struct avl_table avl;

// int _sparql_sort_cmp (const void * a, const void * b);
int _hx_idmap_cmp_id ( const void* a, const void* b, void* param ) {
	hx_idmap_item* ia	= (hx_idmap_item*) a;
	hx_idmap_item* ib	= (hx_idmap_item*) b;
// 	fprintf( stderr, "hx_idmap_cmp_id( %d, %d )\n", (int) ia->id, (int) ib->id );
	return (ia->id - ib->id);
}

void _hx_free_idmap_item (void *avl_item, void *avl_param) {
	hx_idmap_item* i	= (hx_idmap_item*) avl_item;
	if (i->string != NULL) {
		free( i->string );
	}
	free( i );
}


hx_idmap* hx_new_idmap( void ) {
	hx_idmap* m		= (hx_idmap*) calloc( 1, sizeof( hx_idmap ) );
	m->id2string	= avl_create( _hx_idmap_cmp_id, NULL, &avl_allocator_default );
	m->string2id	= avl_create( (avl_comparison_func*) strcmp, NULL, &avl_allocator_default );
	m->next_id		= (uint64_t) 1;
	return m;
}

int hx_free_idmap ( hx_idmap* m ) {
	avl_destroy( m->id2string, NULL );
	avl_destroy( m->string2id, _hx_free_idmap_item );
	free( m );
	return 0;
}

uint64_t hx_idmap_add_string ( hx_idmap* m, char* n ) {
	char* string	= calloc( strlen(n) + 1, sizeof(char) );
	strcpy( string, n );
	
	hx_idmap_item i;
	i.string	= string;
	hx_idmap_item* item	= (hx_idmap_item*) avl_find( m->string2id, &i );
	if (item == NULL) {
		if (0) {
			fprintf( stderr, "idmap adding key '%s'\n", string );
		}
		
		item	= (hx_idmap_item*) calloc( 1, sizeof( hx_idmap_item ) );
		item->string	= string;
		item->id	= m->next_id++;
		avl_insert( m->string2id, item );
		avl_insert( m->id2string, item );
// 		fprintf( stderr, "*** new item %d -> %p\n", (int) item->id, (void*) item->string );
		
		if (0) {
			uint64_t id	= hx_idmap_get_id( m, string );
			fprintf( stderr, "*** After adding: %d\n", (int) id );
		}
		
		return item->id;
	} else {
		free( string );
		return item->id;
	}
}

int hx_idmap_remove_id ( hx_idmap* m, uint64_t id ) {
	hx_idmap_item i;
	i.id	= id;
	hx_idmap_item* item	= (hx_idmap_item*) avl_delete( m->id2string, &i );
	if (item != NULL) {
		avl_delete( m->string2id, item );
		_hx_free_idmap_item( item, NULL );
		return 0;
	} else {
		return 1;
	}
}

int hx_idmap_remove_string ( hx_idmap* m, char* n ) {
	hx_idmap_item i;
	i.string	= n;
	hx_idmap_item* item	= (hx_idmap_item*) avl_delete( m->string2id, &i );
	if (item != NULL) {
		avl_delete( m->id2string, item );
		_hx_free_idmap_item( item, NULL );
		return 0;
	} else {
		return 1;
	}
}

uint64_t hx_idmap_get_id ( hx_idmap* m, char* n ) {
	hx_idmap_item i;
	i.string	= n;
	hx_idmap_item* item	= (hx_idmap_item*) avl_find( m->string2id, &i );
	if (item == NULL) {
//		fprintf( stderr, "hx_idmap_get_id: did not find string in idmap\n" );
		return (uint64_t) 0;
	} else {
		return item->id;
	}
}

char* hx_idmap_get_string ( hx_idmap* m, uint64_t id ) {
	hx_idmap_item i;
	i.id	= id;
	i.string	= NULL;
// 	fprintf( stderr, "hx_idmap_get_string( %p, %d )\n", (void*) m, (int) id );
	hx_idmap_item* item	= (hx_idmap_item*) avl_find( m->id2string, &i );
	if (item == NULL) {
// 		fprintf( stderr, "*** string %d string not found\n", (int) id );
		return NULL;
	} else {
// 		fprintf( stderr, "*** string %d string: '%s'\n", (int) id, item->string );
		return item->string;
	}
}

int hx_idmap_debug ( hx_idmap* map ) {
	struct avl_traverser iter;
	avl_t_init( &iter, map->id2string );
	hx_idmap_item* item;
	fprintf( stderr, "Nodemap:\n" );
	while ((item = (hx_idmap_item*) avl_t_next( &iter )) != NULL) {
		char* string	= item->string;
		fprintf( stderr, "\t%"PRIu64" -> %s\n", item->id, string );
	}
	return 0;
}

hx_container_t* hx_idmap_strings ( hx_idmap* map ) {
	size_t size	= avl_count( map->id2string );
	hx_container_t* c	= hx_new_container( 'S', size );
	
	struct avl_traverser iter;
	avl_t_init( &iter, map->id2string );
	hx_idmap_item* item;
	while ((item = (hx_idmap_item*) avl_t_next( &iter )) != NULL) {
		int len			= strlen( item->string ) + 1;
		char* string	= calloc( len, sizeof(char) );
		strcpy(string, item->string );
		hx_container_push_item( c, string );
	}
	
	return c;
}
