#include "engine/variablebindings_iter.h"

hx_variablebindings_iter* hx_variablebindings_new_empty_iter ( void ) {
	hx_variablebindings_iter* iter	= (hx_variablebindings_iter*) malloc( sizeof( hx_variablebindings_iter ) );
	if (iter == NULL) {
		fprintf( stderr, "*** malloc failed in hx_variablebindings_new_empty_iter\n" );
		return NULL;
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
		return NULL;
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
		return iter->vtable->sorted_by_index( iter->ptr, index );
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

