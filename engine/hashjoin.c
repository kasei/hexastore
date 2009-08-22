#include "engine/hashjoin.h"

// prototypes
int _hx_hashjoin_join_vb_names ( hx_variablebindings* lhs, hx_variablebindings* rhs, char*** merged_names, int* size );
int _hx_hashjoin_join_iter_names ( hx_variablebindings_iter* lhs, hx_variablebindings_iter* rhs, char*** merged_names, int* size );
int _hx_hashjoin_join_names ( char** lhs_names, int lhs_size, char** rhs_names, int rhs_size, char*** merged_names, int* size );
int _hx_hashjoin_debug ( void* info, char* header, int indent );

// implementations

int _hx_hashjoin_prime_results ( _hx_hashjoin_iter_vb_info* info ) {
	info->started	= 1;
	
	// fill up the hash for the RHS
	hx_variablebindings_iter* iter	= info->rhs;
	while (!hx_variablebindings_iter_finished( iter )) {
		hx_variablebindings* b;
		hx_variablebindings_iter_current( iter, &b );
		
		char* string;
		hx_variablebindings_string( b, NULL, &string );
		fprintf( stderr, "hashing hashjoin rhs: %s\n", string );
		free(string);
		
		hx_free_variablebindings( b );
		hx_variablebindings_iter_next( iter );
	}
	
	// XXX advance to the first result
	
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
		// (*merged_names)[ i ]    = names[ i ];
		// XXXXXXXXXXXXX experimentally put in place while seeking a memory leak... maybe want to roll this back at some point in favor of the preceeding line
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
