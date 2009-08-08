#include "variablebindings.h"

/* nodes must not be stack-allocated; it must be heap-allocated, and control for freeing
   that memory is now handed off to the variablebindings code (hx_free_variablebindings) */
hx_variablebindings* hx_new_variablebindings ( int size, char** names, hx_node_id* nodes ) {
	hx_variablebindings* b	= (hx_variablebindings*) calloc( 1, sizeof( hx_variablebindings ) );
	b->size			= size;
	b->names		= (char**) calloc( size, sizeof( char* ) );
	b->nodes		= nodes;
	
	int i;
	for (i = 0; i < size; i++) {
		b->names[i]	= (char*) calloc( 1, strlen(names[i]) + 1 );
		strcpy( b->names[i], names[i] );
	}
	
// 	if (free_names) {
// 		free(names);
// 	}
	
	return b;
}

hx_variablebindings* hx_copy_variablebindings ( hx_variablebindings* b ) {
	hx_variablebindings* c	= (hx_variablebindings*) calloc( 1, sizeof( hx_variablebindings ) );
	c->size		= b->size;
	c->names	= (char**) calloc( c->size, sizeof( char* ) );
	c->nodes	= (hx_node_id*) calloc( c->size, sizeof( hx_node_id ) );
	int i;
	for (i = 0; i < c->size; i++) {
		char* _new	= (char*) calloc( strlen(b->names[i]) + 1, sizeof( char ) );
		strcpy( _new, b->names[i] );
		c->names[i]	= _new;
		c->nodes[i]	= b->nodes[i];
	}
	return c;
}

hx_variablebindings_nodes* hx_new_variablebindings_nodes ( int size, char** names, hx_node** nodes ) {
	hx_variablebindings_nodes* b	= (hx_variablebindings_nodes*) calloc( 1, sizeof( hx_variablebindings_nodes ) );
	b->size			= size;
	b->names		= (char**) calloc( size, sizeof( char* ) );
	b->nodes		= nodes;
	
	int i;
	for (i = 0; i < size; i++) {
		b->names[i]	= (char*) calloc( 1, strlen(names[i]) + 1 );
		strcpy( b->names[i], names[i] );
	}
	
	return b;
}

int hx_free_variablebindings ( hx_variablebindings* b ) {
	int i;
	for (i = 0; i < b->size; i++) {
		if (b->names[i] != NULL) {
			free( b->names[i] );
		}
		b->names[i]	= NULL;
	}
	
	free( b->names );
	b->names	= NULL;
	free( b->nodes );
	b->nodes	= NULL;
	
	free( b );
	return 0;
}

int hx_free_variablebindings_nodes ( hx_variablebindings_nodes* b ) {
	int i;
	for (i = 0; i < b->size; i++) {
		free( b->names[i] );
		b->names[i]	= NULL;
		hx_free_node( b->nodes[i] );
	}
	
	free( b->names );
	b->names	= NULL;
	free( b->nodes );
	b->nodes	= NULL;
	
	free( b );
	return 0;
}

hx_variablebindings* hx_variablebindings_project ( hx_variablebindings* b, int newsize, int* columns ) {
	hx_variablebindings* c	= (hx_variablebindings*) calloc( 1, sizeof( hx_variablebindings ) );
	c->size		= newsize;
	c->names	= (char**) calloc( c->size, sizeof( char* ) );
	c->nodes	= (hx_node_id*) calloc( c->size, sizeof( hx_node_id ) );
	int i;
	for (i = 0; i < c->size; i++) {
		int col	= columns[i];
		if (col < 0) {
			c->names[i]	= NULL;
			c->nodes[i]	= 0;
		} else {
			char* name	= b->names[ col ];
			char* _new	= (char*) calloc( strlen(name) + 1, sizeof( char ) );
			strcpy( _new, name );
			c->names[i]	= _new;
			c->nodes[i]	= b->nodes[ col ];
		}
	}
	return c;
}

hx_variablebindings* hx_variablebindings_project_names ( hx_variablebindings* b, int newsize, char** names ) {
	hx_variablebindings* c	= (hx_variablebindings*) calloc( 1, sizeof( hx_variablebindings ) );
	c->size		= newsize;
	c->names	= (char**) calloc( c->size, sizeof( char* ) );
	c->nodes	= (hx_node_id*) calloc( c->size, sizeof( hx_node_id ) );
	int i;
	for (i = 0; i < c->size; i++) {
		char* name	= names[i];
		int col		= -1;
		int j;
		for (j = 0; j < b->size; j++) {
			if (strcmp(b->names[j], name) == 0) {
				col	= j;
				break;
			}
		}
		if (col < 0 || col >= c->size) {
			c->names[i]	= NULL;
			c->nodes[i]	= 0;
		} else {
			char* _new	= (char*) calloc( strlen(name) + 1, sizeof( char ) );
			strcpy( _new, name );
			c->names[i]	= _new;
			c->nodes[i]	= b->nodes[ col ];
		}
	}
	return c;
}

int hx_variablebindings_string ( hx_variablebindings* b, hx_nodemap* map, char** string ) {
	int size	= b->size;
	hx_node_id* id	= (hx_node_id*) calloc( size, sizeof( hx_node_id ) );
	char** nodestrs	= (char**) calloc( size, sizeof( char* ) );
	size_t len	= 5;
	int i;
	for (i = 0; i < size; i++) {
		hx_node_id id	= b->nodes[ i ];
		if (map == NULL) {
			char* number	= (char*) malloc( 20 );
			if (number == NULL) {
				fprintf( stderr, "*** malloc failed in hx_variablebindings_string\n" );
			}
			sprintf( number, "%d", (int) id );
			nodestrs[i]	= number;
		} else {
			hx_node* node	= hx_nodemap_get_node( map, id );
			hx_node_string( node, &( nodestrs[i] ) );
		}
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
		free( nodestrs[i] );
		p	+= strlen( nodestrs[i] );
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

void hx_variablebindings_debug ( hx_variablebindings* b, hx_nodemap* m ) {
	char* string;
	if (hx_variablebindings_string( b, m, &string ) != 0) {
		return;
	}

	fprintf( stderr, "%s\n", string );
	
	free( string );
}

int hx_variablebindings_nodes_string ( hx_variablebindings_nodes* b, char** string ) {
	int size		= b->size;
	char** nodestrs	= (char**) calloc( size, sizeof( char* ) );
	size_t len	= 5;
	int i;
	for (i = 0; i < size; i++) {
		hx_node* node	= b->nodes[i];
		hx_node_string( node, &( nodestrs[i] ) );
		len	+= strlen( nodestrs[i] ) + 2 + strlen(b->names[i]) + 1;
	}
	*string	= (char*) malloc( len );
	if (*string == NULL) {
		fprintf( stderr, "*** malloc failed in hx_variablebindings_string\n" );
	}
	char* p			= *string;
	if (*string == NULL) {
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
		if (i == size-1) {
			strcpy( p, " }" );
		} else {
			strcpy( p, ", " );
		}
		p	+= 2;
	}
	free( nodestrs );
	return 0;
}

char** hx_variablebindings_names ( hx_variablebindings* b ) {
	return b->names;
}

int hx_variablebindings_size ( hx_variablebindings* b ) {
	return b->size;
}

char* hx_variablebindings_name_for_binding ( hx_variablebindings* b, int column ) {
	return b->names[ column ];
}

hx_node_id hx_variablebindings_node_id_for_binding ( hx_variablebindings* b, int column ) {
	if (column >= b->size) {
		return 0;
	}
	return b->nodes[ column ];
}

hx_node_id hx_variablebindings_node_id_for_binding_name ( hx_variablebindings* b, char* name ) {
	int column	= -1;
	int i;
	for (i = 0; i < b->size; i++) {
		if (b->names[i] == NULL) {
			continue;
		}
		if (strcmp(b->names[i], name) == 0) {
			column	= i;
			break;
		}
	}
	if (column >= b->size) {
		return 0;
	}
	if (column >= 0) {
		hx_node_id id	= b->nodes[ column ];
		return id;
	} else {
		return 0;
	}
}

hx_node* hx_variablebindings_node_for_binding ( hx_variablebindings* b, hx_nodemap* map, int column ) {
	hx_node_id id	= b->nodes[ column ];
	hx_node* node	= hx_nodemap_get_node( map, id );
	return node;
}

hx_node* hx_variablebindings_node_for_binding_name ( hx_variablebindings* b, hx_nodemap* map, char* name ) {
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
		hx_node* node	= hx_nodemap_get_node( map, id );
		return node;
	} else {
		return NULL;
	}
}

int hx_variablebindings_cmp ( void* _a, void* _b ) {
	hx_variablebindings* a	= (hx_variablebindings*) _a;
	hx_variablebindings* b	= (hx_variablebindings*) _b;
	int asize	= a->size;
	int bsize	= b->size;
	if (asize < bsize) {
		return -1;
	} else if (bsize < asize) {
		return 1;
	}
	
	int i;
	
	for (i = 0; i < asize; i++) {
		hx_node_id av	= a->nodes[i];
		hx_node_id bv	= b->nodes[i];
		if (av < bv) {
			return -1;
		} else if (bv < av) {
			return 1;
		}
	}
	return 0;
}

int _hx_variablebindings_join_names ( hx_variablebindings* lhs, hx_variablebindings* rhs, char*** merged_names, int* size ) {
	int lhs_size		= hx_variablebindings_size( lhs );
	char** lhs_names	= hx_variablebindings_names( lhs );
	int rhs_size		= hx_variablebindings_size( rhs );
	char** rhs_names	= hx_variablebindings_names( rhs );
	int seen_names	= 0;
	char** names		= (char**) calloc( lhs_size + rhs_size, sizeof( char* ) );
	int i;
	for (i = 0; i < lhs_size; i++) {
		char* name	= lhs_names[ i ];
		int seen	= 0;
		int j;
		for (j = 0; j < seen_names; j++) {
			if (strcmp( name, names[ j ] ) == 0) {
				seen	= 1;
			}
		}
		if (!seen) {
// 			fprintf( stderr, "lhs adding name '%s'\n", name );
			names[ seen_names++ ]	= name;
		}
	}
	for (i = 0; i < rhs_size; i++) {
		char* name	= rhs_names[ i ];
		int seen	= 0;
		int j;
		for (j = 0; j < seen_names; j++) {
			if (strcmp( name, names[ j ] ) == 0) {
				seen	= 1;
			}
		}
		if (!seen) {
// 			fprintf( stderr, "rhs adding name '%s'\n", name );
			names[ seen_names++ ]	= name;
		}
	}
	
	*merged_names	= (char**) calloc( seen_names, sizeof( char* ) );
	for (i = 0; i < seen_names; i++) {
		(*merged_names)[ i ]	= names[ i ];
	}
	*size	= seen_names;
	free( names );
	return 0;
}
hx_variablebindings* hx_variablebindings_natural_join( hx_variablebindings* left, hx_variablebindings* right ) {
	int lhs_size		= hx_variablebindings_size( left );
	char** lhs_names	= hx_variablebindings_names( left );
	int rhs_size		= hx_variablebindings_size( right );
	char** rhs_names	= hx_variablebindings_names( right );
	int max_size		= (lhs_size > rhs_size) ? lhs_size : rhs_size;
	
// 	fprintf( stderr, "natural join...\n" );
	int shared_count	= 0;
	int* shared_lhs_index	= (int*) calloc( max_size, sizeof(int) );
	char** shared_names	= (char**) calloc( max_size, sizeof( char* ) );
	int i;
	for (i = 0; i < lhs_size; i++) {
		char* lhs_name	= lhs_names[ i ];
		int j;
		for (j = 0; j < rhs_size; j++) {
			char* rhs_name	= rhs_names[ j ];
			if (strcmp( lhs_name, rhs_name ) == 0) {
				int k	= shared_count++;
				shared_lhs_index[ k ]	= i;
				shared_names[ k ]	= lhs_name;
				break;
			}
		}
	}
	
	for (i = 0; i < shared_count; i++) {
		char* name		= shared_names[i];
// 		fprintf( stderr, "*** shared key in natural join: %s\n", name );
		hx_node_id node	= hx_variablebindings_node_id_for_binding( left, shared_lhs_index[i] );
		int j;
		for (j = 0; j < rhs_size; j++) {
			char* rhs_name	= rhs_names[ j ];
			if (strcmp( name, rhs_name ) == 0) {
// 				fprintf( stderr, "rhs_name: %s\n", rhs_name );
// 				fprintf( stderr, "\tindex in lhs is %d\n", shared_lhs_index[j] );
				char* lhs_name	= lhs_names[ shared_lhs_index[j] ];
// 				fprintf( stderr, "lhs_name: %s\n", lhs_name );
				hx_node_id rnode	= hx_variablebindings_node_id_for_binding( right, j );
// 				fprintf( stderr, "\tcomparing nodes %d <=> %d\n", node, rnode );
				if (node != rnode) {
					free( shared_names );
					free( shared_lhs_index );
					return NULL;
				}
			}
		}
		
	}
	
	free( shared_lhs_index );
	free( shared_names );
	
	int size;
	char** names;
	_hx_variablebindings_join_names( left, right, &names, &size );
	hx_variablebindings* b;
	
	hx_node_id* values	= (hx_node_id*) calloc( size, sizeof( hx_node_id ) );
	for (i = 0; i < size; i++) {
		char* name	= names[ i ];
		int j;
		for (j = 0; j < lhs_size; j++) {
			if (strcmp( name, lhs_names[j] ) == 0) {
				values[i]	= hx_variablebindings_node_id_for_binding( left, j );
			}
		}
		if (!values[i]) {
			int j;
			for (j = 0; j < rhs_size; j++) {
				if (strcmp( name, rhs_names[j] ) == 0) {
					values[i]	= hx_variablebindings_node_id_for_binding( right, j );
				}
			}
		}
	}
	
	b	= hx_new_variablebindings( size, names, values );
	free(names);
	return b;
}

hx_variablebindings* hx_variablebindings_thaw ( char* ptr, int len, hx_nodemap* map ) {
	int i;
	int size;
	char* p	= ptr;
	memcpy( &size, p, sizeof( int ) );
	p	+= sizeof( int );
	char** names		= (char**) calloc( size, sizeof( char* ) );
	hx_node_id* nodes	= (hx_node_id*) calloc( size, sizeof( hx_node_id ) );
	
	for (i = 0; i < size; i++) {
		int name_len	= strlen(p);
		char* name		= (char*) calloc( name_len + 1, sizeof( char ) );
		strcpy( name, p );
		names[i]		= name;
		p				+= name_len + 1;
	}
	
	for (i = 0; i < size; i++) {
		int node_len	= strlen(p);
		hx_node* n		= hx_node_parse( p );
		nodes[i]		= hx_nodemap_add_node( map, n );
		hx_free_node(n);
		p				+= node_len + 1;
	}
	
	hx_variablebindings* b	= hx_new_variablebindings( size, names, nodes );
	for (i = 0; i < size; i++) {
		free(names[i]);
	}
	free(names);
	return b;
}

char* hx_variablebindings_freeze( hx_variablebindings* b, hx_nodemap* map, int* len ) {
	int i;
	int names_length	= 0;
	int* name_lengths	= calloc( b->size, sizeof( int ) );
	int node_length		= 0;
	int* node_lengths	= calloc( b->size, sizeof( int ) );
	char** node_strings	= (char**) calloc( b->size, sizeof( char* ) );
	for (i = 0; i < b->size; i++) {
		name_lengths[i]	= strlen( b->names[i] );
		names_length	+= name_lengths[i] + 1;
		hx_node* n		= hx_nodemap_get_node( map, b->nodes[i] );
		hx_node_string( n, &( node_strings[i] ) );
		node_lengths[i]	= strlen( node_strings[i] );
		node_length		+= node_lengths[i] + 1;
	}
	
	*len		= sizeof(int) + (names_length * sizeof(char)) + (node_length * sizeof(char));
	char* ptr	= (char*) calloc( 1, *len );
	char* p		= ptr;
	memcpy( p, &( b->size ), sizeof( int ) );
	p			+= sizeof( int );
	for (i = 0; i < b->size; i++) {
		memcpy( p, b->names[i], name_lengths[i] + 1 );
		p		+= name_lengths[i] + 1;
	}
	for (i = 0; i < b->size; i++) {
		memcpy( p, node_strings[i], node_lengths[i] + 1 );
		p		+= node_lengths[i] + 1;
		free( node_strings[i] );
	}
	free( node_lengths );
	free( node_strings );
	free( name_lengths );
	return ptr;
}

hx_variablebindings_iter* hx_variablebindings_new_empty_iter ( void ) {
	hx_variablebindings_iter* iter	= (hx_variablebindings_iter*) malloc( sizeof( hx_variablebindings_iter ) );
	if (iter == NULL) {
		fprintf( stderr, "*** malloc failed in hx_variablebindings_new_empty_iter\n" );
	}
	iter->vtable		= NULL;
	iter->ptr			= NULL;
	return iter;
}

hx_variablebindings_iter* hx_variablebindings_new_empty_iter_with_names ( int size, char** names ) {
	return hx_new_materialize_iter_with_data( size, names, 0, NULL );
}

hx_variablebindings_iter* hx_variablebindings_new_iter ( hx_variablebindings_iter_vtable* vtable, void* ptr ) {
	hx_variablebindings_iter* iter	= (hx_variablebindings_iter*) malloc( sizeof( hx_variablebindings_iter ) );
	if (iter == NULL) {
		fprintf( stderr, "*** malloc failed in hx_variablebindings_new_iter\n" );
	}
	iter->vtable		= vtable;
	iter->ptr			= ptr;
	return iter;
}

int hx_free_variablebindings_iter ( hx_variablebindings_iter* iter ) {
	if (iter->vtable != NULL) {
		iter->vtable->free( iter->ptr );
		free( iter->vtable );
		iter->vtable	= NULL;
		iter->ptr		= NULL;
	}
	free( iter );
	return 0;
}

int hx_variablebindings_iter_finished ( hx_variablebindings_iter* iter ) {
	if (iter->vtable != NULL) {
		return iter->vtable->finished( iter->ptr );
	} else {
		return 1;
	}
}

int hx_variablebindings_iter_current ( hx_variablebindings_iter* iter, hx_variablebindings** b ) {
	if (iter->vtable != NULL) {
		return iter->vtable->current( iter->ptr, b );
	} else {
		return 1;
	}
}

int hx_variablebindings_iter_next ( hx_variablebindings_iter* iter ) {
	if (iter->vtable != NULL) {
		return iter->vtable->next( iter->ptr );
	} else {
		return 1;
	}
}

int hx_variablebindings_set_names ( hx_variablebindings* b, char** names ) {
	int i;
	for (i = 0; i < b->size; i++) {
		if (b->names[i] != NULL) {
			free( b->names[i] );
		}
	}
	
	b->names		= (char**) calloc( b->size, sizeof( char* ) );
	
	for (i = 0; i < b->size; i++) {
		b->names[i]	= (char*) calloc( 1, strlen(names[i]) + 1 );
		strcpy( b->names[i], names[i] );
	}
	
	return 0;
}

int hx_variablebindings_iter_size ( hx_variablebindings_iter* iter ) {
	if (iter->vtable != NULL) {
		return iter->vtable->size( iter->ptr );
	} else {
		return -1;
	}
}

char** hx_variablebindings_iter_names ( hx_variablebindings_iter* iter ) {
	if (iter->vtable != NULL) {
		return iter->vtable->names( iter->ptr );
	} else {
		return NULL;
	}
}

int hx_variablebindings_column_index ( hx_variablebindings_iter* iter, char* column ) {
	int size		= hx_variablebindings_iter_size( iter );
	char** names	= hx_variablebindings_iter_names( iter );
	int i;
	for (i = 0; i < size; i++) {
		if (strcmp(column, names[i]) == 0) {
			return i;
		}
	}
	return -1;
}

int hx_variablebindings_iter_is_sorted_by_index ( hx_variablebindings_iter* iter, int index ) {
	if (iter->vtable != NULL) {
		return iter->vtable->sorted_by( iter->ptr, index );
	} else {
		return 0;
	}
}

int hx_variablebindings_iter_debug ( hx_variablebindings_iter* iter, char* header, int indent ) {
	if (iter->vtable != NULL) {
		return iter->vtable->debug( iter->ptr, header, indent );
	} else {
		return 1;
	}
}

hx_variablebindings_iter* hx_variablebindings_sort_iter( hx_variablebindings_iter* iter, int index ) {
	int size		= hx_variablebindings_iter_size( iter );
	char** names	= hx_variablebindings_iter_names( iter );
// 	fprintf( stderr, "requested sorting of iterator on '%s'\n", names[index] );
	
	if (hx_variablebindings_iter_is_sorted_by_index(iter, index)) {
		return iter;
	} else {
		// iterator isn't sorted on the requested column...
		
		// so, materialize the iterator
		hx_variablebindings_iter* sorted	= hx_new_materialize_iter( iter );
		if (sorted == NULL) {
			hx_free_variablebindings_iter( iter );
			return NULL;
		}
		
// 		hx_materialize_iter_debug( iter );
		
		// and sort the materialized bindings by the requested column...
		int r	= hx_materialize_sort_iter_by_column( sorted, index );
		if (r == 0) {
			return sorted;
		} else {
			hx_free_variablebindings_iter( sorted );
			return NULL;
		}
	}
}

