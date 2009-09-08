#include "store/store.h"

/* Called by a store implementation constructor (i.e. hx_new_store_hexastore) */
hx_store* hx_new_store ( void* world, hx_store_vtable* vtable, void* ptr ) {
	hx_store* s	= (hx_store*) calloc( 1, sizeof( hx_store ) );
	s->world	= world;
	s->vtable	= vtable;
	s->ptr		= ptr;
	return s;
}

int hx_free_store ( hx_store* store ) {
	store->vtable->close( store );
	free( store->vtable );
	free( store );
	return 0;
}

uint64_t hx_store_size ( hx_store* store ) {
	return store->vtable->size( store );
}

uint64_t hx_store_count ( hx_store* store, hx_triple* triple ) {
	return store->vtable->count( store, triple );
}

int hx_store_add_triple ( hx_store* store, hx_triple* triple ) {
	return store->vtable->add_triple( store, triple );
}

int hx_store_remove_triple ( hx_store* store, hx_triple* triple ) {
	return store->vtable->remove_triple( store, triple );
}

int hx_store_contains_triple ( hx_store* store, hx_triple* triple ) {
	return store->vtable->contains_triple( store, triple );
}

hx_variablebindings_iter* hx_store_get_statements ( hx_store* store, hx_triple* triple, hx_node* sort_variable ) {
	hx_variablebindings_iter* iter	= store->vtable->get_statements( store, triple, sort_variable );
	if (iter == NULL) {
		char* names[3];
		int i;
		int j	= 0;
		for (i = 0; i < 3; i++) {
			hx_node* n	= hx_triple_node(triple,i);
			if (hx_node_is_variable(n)) {
				hx_node_variable_name(n, &(names[j++]));
			}
		}
		iter	= hx_variablebindings_new_empty_iter_with_names(j, names);
		for (i = 0; i < j; i++) {
			free(names[i]);
		}
	}
	return iter;
}

hx_variablebindings_iter* hx_store_get_statements_with_index (hx_store* store, hx_triple* triple, void* thunk) {
	hx_variablebindings_iter* iter	= store->vtable->get_statements_with_index( store, triple, thunk );
	if (iter == NULL) {
		char* names[3];
		int i;
		int j	= 0;
		for (i = 0; i < 3; i++) {
			hx_node* n	= hx_triple_node(triple,i);
			if (hx_node_is_variable(n)) {
				hx_node_variable_name(n, &(names[j++]));
			}
		}
		iter	= hx_variablebindings_new_empty_iter_with_names(j, names);
		for (i = 0; i < j; i++) {
			free(names[i]);
		}
	}
	return iter;
}

hx_container_t* hx_store_triple_orderings ( hx_store* store, hx_triple* triple ) {
	return store->vtable->triple_orderings( store, triple );
}

hx_node_id hx_store_get_node_id ( hx_store* store, hx_node* node ) {
	return store->vtable->node2id( store, node );
}

hx_node* hx_store_get_node ( hx_store* store, hx_node_id id ) {
	return store->vtable->id2node( store, id );
}

int hx_store_variablebindings_string ( hx_store* store, hx_variablebindings* b, char** string ) {
	int size	= b->size;
	hx_node_id* id	= (hx_node_id*) calloc( size, sizeof( hx_node_id ) );
	char** nodestrs	= (char**) calloc( size, sizeof( char* ) );
	size_t len	= 5;
	int i;
	for (i = 0; i < size; i++) {
		hx_node_id id	= b->nodes[i];
		hx_node* node	= hx_store_get_node( store, id );
		hx_node_string( node, &( nodestrs[i] ) );
		len	+= strlen( nodestrs[i] ) + 2 + strlen(b->names[i]) + 1;
	}
	*string	= (char*) malloc( len );
	if (*string == NULL) {
		fprintf( stderr, "*** malloc failed in hx_variablebindings_string\n" );
	}
	char* p			= *string;
	if (*string == NULL) {
		free( id );
		free( nodestrs );
		fprintf( stderr, "*** Failed to allocated memory in hx_variablebindings_string\n" );
		return 1;
	}
	
	strcpy( p, "{ " );
	p	+= 2;
	for (i = 0; i < size; i++) {
		strcpy( p, b->names[i] );
		p	+= strlen( b->names[i] );
		
		strcpy( p, "=" );
		p	+= 1;
		
		strcpy( p, nodestrs[i] );
		p	+= strlen( nodestrs[i] );
		free( nodestrs[i] );
		if (i == size-1) {
			strcpy( p, " }" );
		} else {
			strcpy( p, ", " );
		}
		p	+= 2;
	}
	free( nodestrs );
	free( id );
	return 0;
}

int hx_store_variablebindings_debug ( hx_store* store, hx_variablebindings* b ) {
	char* string;
	hx_store_variablebindings_string( store, b, &string );
	fprintf( stderr, "%s\n", string );
	free(string);
	return 0;
}


hx_node* hx_variablebindings_node_for_binding ( hx_variablebindings* b, hx_store* store, int column ) {
	hx_node_id id	= b->nodes[ column ];
	hx_node* node	= hx_store_get_node( store, id );
	return node;
}

hx_node* hx_variablebindings_node_for_binding_name ( hx_variablebindings* b, hx_store* store, char* name ) {
	int column	= -1;
	int i;
	for (i = 0; i < b->size; i++) {
		if (strcmp(b->names[i], name) == 0) {
			column	= i;
			break;
		}
	}
	if (column >= 0) {
		hx_node_id id	= b->nodes[ column ];
		hx_node* node	= hx_store_get_node( store, id );
		return node;
	} else {
		return NULL;
	}
}
