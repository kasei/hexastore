#include "engine/hashjoin.h"

// prototypes
int _hx_hashjoin_join_vb_names ( hx_variablebindings* lhs, hx_variablebindings* rhs, char*** merged_names, int* size );
int _hx_hashjoin_join_iter_names ( hx_variablebindings_iter* lhs, hx_variablebindings_iter* rhs, char*** merged_names, int* size );
int _hx_hashjoin_join_names ( char** lhs_names, int lhs_size, char** rhs_names, int rhs_size, char*** merged_names, int* size );
int _hx_hashjoin_debug ( void* info, char* header, int indent );

// implementations

void ___hash_debug_1 ( void* key, void* value ) {
	hx_node_id id	= *( (hx_node_id*) key );
	hx_variablebindings* b	= (hx_variablebindings*) value;
	
	char* string;
	hx_variablebindings_string( b, NULL, &string );
	fprintf( stderr, "\t(%llu => %s)\n", (unsigned long long) id, string );
	free(string);
}

void _hx_hashjoin_free_hash_cb ( void* key, void* value ) {
	hx_variablebindings* b	= (hx_variablebindings*) value;
	hx_free_variablebindings(b);
}

int _hx_hashjoin_get_matching_hash_items ( void* key, void* value, void* thunk ) {
	hx_container_t* c	= (hx_container_t*) thunk;
	hx_container_push_item( c, value );
	return 0;
}

int _hx_hashjoin_prime_results ( _hx_hashjoin_iter_vb_info* info ) {
	info->started	= 1;
	
	// fill up the hash for the RHS
	hx_variablebindings_iter* iter	= info->rhs;
	while (!hx_variablebindings_iter_finished( iter )) {
		hx_variablebindings* b;
		hx_variablebindings_iter_current( iter, &b );
		hx_node_id id	= hx_variablebindings_node_id_for_binding ( b, info->rhs_shared_column );
		
		fprintf( stderr, "hashing on shared column %d\n", info->rhs_shared_column );
		hx_hash_add( info->hash, &id, sizeof(hx_node_id), b );
		
		hx_variablebindings_iter_next( iter );
	}
	
	hx_hash_debug( info->hash, ___hash_debug_1 );
	
	// advance to the first result
	while (!hx_variablebindings_iter_finished( info->lhs )) {
		hx_variablebindings* b;
		hx_variablebindings_iter_current( info->lhs, &b );

		char* string;
		hx_variablebindings_string( b, NULL, &string );
		fprintf( stderr, "Looking for results that join with: %s\n", string );
		free(string);

		hx_node_id id	= hx_variablebindings_node_id_for_binding ( b, info->lhs_shared_column );
		hx_container_t* matching	= hx_new_container( 'C', 10 );
		hx_hash_apply( info->hash, &id, sizeof(hx_node_id), _hx_hashjoin_get_matching_hash_items, matching );
		
		int i;
		int size	= hx_container_size(matching);
		for (i = 0; i < size; i++) {
			hx_variablebindings* b	= (hx_variablebindings*) hx_container_item( matching, i );
			char* string;
			hx_variablebindings_string( b, NULL, &string );
			fprintf( stderr, "- matching RHS result: %s\n", string );
			free(string);
		}

		if (size > 0) {
			hx_variablebindings* rhs	= hx_container_item( matching, 0 );
			hx_variablebindings* joined	= hx_variablebindings_natural_join( b, rhs );
			if (joined != NULL) {
				info->current	= joined;
				return 0;
			} else if (info->leftjoin) {
				info->current	= hx_copy_variablebindings( b );
				return 0;
			}
		}
		
		// XXX need to stash the $matching container into the info struct, and
		// XXX also keep around our current index into it. then, on next calls,
		// XXX increment the index, and pull from the LHS when the index goes
		// XXX beyond the size of the $matching container.
		
		hx_variablebindings_iter_next( iter );
	}
	
	
	
	info->finished	= 1;
	return 1;
}

int _hx_hashjoin_iter_vb_finished ( void* data ) {
//	fprintf( stderr, "*** _hx_hashjoin_iter_vb_finished\n" );
	_hx_hashjoin_iter_vb_info* info	= (_hx_hashjoin_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_hashjoin_prime_results( info );
	}
//	fprintf( stderr, "- finished == %d\n", info->finished );
	return info->finished;
}

int _hx_hashjoin_iter_vb_current ( void* data, void* results ) {
//	fprintf( stderr, "*** _hx_hashjoin_iter_vb_current\n" );
	_hx_hashjoin_iter_vb_info* info	= (_hx_hashjoin_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_hashjoin_prime_results( info );
	}
	
	hx_variablebindings** b	= (hx_variablebindings**) results;
	*b	= hx_copy_variablebindings( info->current );
	return 0;
}

int _hx_hashjoin_iter_vb_next ( void* data ) {
// 	fprintf( stderr, "*** _hx_hashjoin_iter_vb_next\n" );
	_hx_hashjoin_iter_vb_info* info	= (_hx_hashjoin_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_hashjoin_prime_results( info );
	}
	
	if (info->current != NULL) {
		hx_free_variablebindings(info->current);
		info->current	= NULL;
	}
	
	// XXX move to the next result
	
// 	fprintf( stderr, "- exhausted iterator in _hx_hashjoin_iter_vb_next\n" );
	info->finished	= 1;
	return 1;
}

int _hx_hashjoin_iter_vb_free ( void* data ) {
	_hx_hashjoin_iter_vb_info* info	= (_hx_hashjoin_iter_vb_info*) data;
	
	if (info->current != NULL) {
		hx_free_variablebindings(info->current);
		info->current	= NULL;
	}

	int i;
	hx_free_variablebindings_iter( info->lhs );	
	hx_free_variablebindings_iter( info->rhs );
	hx_free_hash( info->hash, _hx_hashjoin_free_hash_cb	);
	for (i = 0; i < info->size; i++) {
		free( info->names[i] );
	}
	free( info->names );
	free( info );
	return 0;
}

int _hx_hashjoin_iter_vb_size ( void* data ) {
	_hx_hashjoin_iter_vb_info* info	= (_hx_hashjoin_iter_vb_info*) data;
	return info->size;
}

char** _hx_hashjoin_iter_vb_names ( void* data ) {
	_hx_hashjoin_iter_vb_info* info	= (_hx_hashjoin_iter_vb_info*) data;
	return info->names;
}

int _hx_hashjoin_iter_sorted_by ( void* data, int index ) {
	_hx_hashjoin_iter_vb_info* info	= (_hx_hashjoin_iter_vb_info*) data;
	// XXX return the sort order of the lhs iterator
	return 0;
}

int _hx_hashjoin_debug ( void* data, char* header, int indent ) {
	_hx_hashjoin_iter_vb_info* info	= (_hx_hashjoin_iter_vb_info*) data;
	int i;
	for (i = 0; i < indent; i++) fwrite( " ", sizeof( char ), 1, stderr );
	fprintf( stderr, "%s hashjoin iterator\n", header );
	hx_variablebindings_iter_debug( info->lhs, header, indent + 4 );
	hx_variablebindings_iter_debug( info->rhs, header, indent + 4 );
	return 0;
}

hx_variablebindings_iter* hx_new_hashjoin_iter ( hx_variablebindings_iter* lhs, hx_variablebindings_iter* rhs ) {
	return hx_new_hashjoin_iter2( lhs, rhs, 0 );
}

hx_variablebindings_iter* hx_new_hashjoin_iter2 ( hx_variablebindings_iter* lhs, hx_variablebindings_iter* rhs, int leftjoin ) {
	int asize		= hx_variablebindings_iter_size( lhs );
	char** anames	= hx_variablebindings_iter_names( lhs );
	int bsize		= hx_variablebindings_iter_size( rhs );
	char** bnames	= hx_variablebindings_iter_names( rhs );
	
	hx_variablebindings_iter_vtable* vtable	= (hx_variablebindings_iter_vtable*) malloc( sizeof( hx_variablebindings_iter_vtable ) );
	if (vtable == NULL) {
		fprintf( stderr, "*** malloc failed in hx_new_hashjoin_iter\n" );
	}
	vtable->finished	= _hx_hashjoin_iter_vb_finished;
	vtable->current		= _hx_hashjoin_iter_vb_current;
	vtable->next		= _hx_hashjoin_iter_vb_next;
	vtable->free		= _hx_hashjoin_iter_vb_free;
	vtable->names		= _hx_hashjoin_iter_vb_names;
	vtable->size		= _hx_hashjoin_iter_vb_size;
	vtable->sorted_by	= _hx_hashjoin_iter_sorted_by;
	vtable->debug		= _hx_hashjoin_debug;
	
	int size;
	char** merged_names;
	_hx_hashjoin_join_iter_names( lhs, rhs, &merged_names, &size );
	_hx_hashjoin_iter_vb_info* info	= (_hx_hashjoin_iter_vb_info*) calloc( 1, sizeof( _hx_hashjoin_iter_vb_info ) );
	
	info->current			= NULL;
	info->lhs				= lhs;
	info->rhs				= rhs;
	info->size				= size;
	info->names				= merged_names;
	info->finished			= 0;
	info->started			= 0;
	info->leftjoin			= leftjoin;
	info->hash				= hx_new_hash( 10 ); // XXX the number of buckets (10) probably shouldn't be hard-coded.
	info->rhs_shared_column	= -1;
	info->lhs_shared_column	= -1;
	
	int i, j;
	int set	= 0;
	for (i = 0; i < asize; i++) {
		int j;
		for (j = 0; j < bsize; j++) {
			if (strcmp( anames[i], bnames[j] ) == 0) {
				fprintf( stderr, "shared column '%s' has column indexes (%d <=> %d)\n", anames[i], i, j );
				info->lhs_shared_column	= i;
				info->rhs_shared_column	= j;
				set					= 1;
				break;
			}
		}
		if (set == 1)
			break;
	}
	
	hx_variablebindings_iter* iter	= hx_variablebindings_new_iter( vtable, (void*) info );
	return iter;
}

int _hx_hashjoin_join_vb_names ( hx_variablebindings* lhs, hx_variablebindings* rhs, char*** merged_names, int* size ) {
	int lhs_size		= hx_variablebindings_size( lhs );
	char** lhs_names	= hx_variablebindings_names( lhs );
	int rhs_size		= hx_variablebindings_size( rhs );
	char** rhs_names	= hx_variablebindings_names( rhs );
	return _hx_hashjoin_join_names( lhs_names, lhs_size, rhs_names, rhs_size, merged_names, size );
}
int _hx_hashjoin_join_iter_names ( hx_variablebindings_iter* lhs, hx_variablebindings_iter* rhs, char*** merged_names, int* size ) {
	int lhs_size		= hx_variablebindings_iter_size( lhs );
	char** lhs_names	= hx_variablebindings_iter_names( lhs );
	int rhs_size		= hx_variablebindings_iter_size( rhs );
	char** rhs_names	= hx_variablebindings_iter_names( rhs );
	return _hx_hashjoin_join_names( lhs_names, lhs_size, rhs_names, rhs_size, merged_names, size );
}
int _hx_hashjoin_join_names ( char** lhs_names, int lhs_size, char** rhs_names, int rhs_size, char*** merged_names, int* size ) {
	int seen_names	= 0;
	char** names	= (char**) calloc( lhs_size + rhs_size, sizeof( char* ) );
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
			names[ seen_names++ ]	= name;
		}
	}
	
	*merged_names	= (char**) calloc( seen_names, sizeof( char* ) );
	for (i = 0; i < seen_names; i++) {
		char* n		= names[i];
		int len		= strlen(n);
//		fprintf( stderr, "copying variable name string : '%s'\n", n );
		char* nn	= (char*) calloc( len + 1, sizeof(char) );
		strcpy( nn, n );
		(*merged_names)[i]	= nn;
	}
	*size	= seen_names;
	free( names );
	return 0;
}
