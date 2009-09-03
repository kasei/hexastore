#include "engine/nestedloopjoin.h"

// prototypes
int _hx_nestedloopjoin_join_vb_names ( hx_variablebindings* lhs, hx_variablebindings* rhs, char*** merged_names, int* size );
int _hx_nestedloopjoin_join_iter_names ( hx_variablebindings_iter* lhs, hx_variablebindings_iter* rhs, char*** merged_names, int* size );
int _hx_nestedloopjoin_join_names ( char** lhs_names, int lhs_size, char** rhs_names, int rhs_size, char*** merged_names, int* size );
int _hx_nestedloopjoin_get_batch ( _hx_nestedloopjoin_iter_vb_info* info, hx_variablebindings_iter* iter, int* batch_size, int* batch_alloc_size, hx_variablebindings*** batch );
int _hx_nestedloopjoin_debug ( void* info, char* header, int indent );

// implementations

int _hx_nestedloopjoin_prime_results ( _hx_nestedloopjoin_iter_vb_info* info ) {
	info->started	= 1;
	_hx_nestedloopjoin_get_batch( info, info->lhs, &( info->lhs_size ), &( info->lhs_batch_alloc_size ), &( info->lhs_batch ) );
	_hx_nestedloopjoin_get_batch( info, info->rhs, &( info->rhs_size ), &( info->rhs_batch_alloc_size ), &( info->rhs_batch ) );
	
	if ((info->lhs_size == 0) || (info->rhs_size == 0)) {
		// one side has no results, so we can't possibly produce any results from
		// the join free the remaining sub-results, and set the iterator as finished
		if (info->lhs_size > 0) {
			int i;
			for (i = 0; i < info->lhs_size; i++) {
				hx_free_variablebindings(info->lhs_batch[i]);
				info->lhs_batch[i]	= NULL;
			}
			info->lhs_size	= 0;
		}
		if (info->rhs_size > 0) {
			int i;
			for (i = 0; i < info->rhs_size; i++) {
				hx_free_variablebindings(info->rhs_batch[i]);
				info->rhs_batch[i]	= NULL;
			}
			info->rhs_size	= 0;
		}
		info->finished	= 1;
		return 1;
	}
	
	// we've got results from both sides of the join. now let's find the first
	// set of variablebindings that are compatible, and set them as the current
	// result
	int i;
	for (i = 0; i < info->lhs_size; i++) {
		hx_variablebindings* lhs	= info->lhs_batch[i];
		int j;
		for (j = 0; j < info->rhs_size; j++) {
			hx_variablebindings* rhs	= info->rhs_batch[j];
			hx_variablebindings* joined	= hx_variablebindings_natural_join( lhs, rhs );
			if (joined != NULL) {
				info->lhs_batch_index	= i;
				info->rhs_batch_index	= j;
				info->current			= joined;
				info->leftjoin_seen_lhs_result	= 1;
				return 0;
			}
		}
		
		if (info->leftjoin) {
//			fprintf( stderr, "in leftjoin, first result is from LHS\n" );
			info->lhs_batch_index	= i;
			info->rhs_batch_index	= info->rhs_size;
			info->current			= hx_copy_variablebindings( lhs );
			info->leftjoin_seen_lhs_result	= 1;
			return 0;
		}
	}
	
	info->finished	= 1;
	return 1;
}

int _hx_nestedloopjoin_iter_vb_finished ( void* data ) {
//	fprintf( stderr, "*** _hx_nestedloopjoin_iter_vb_finished\n" );
	_hx_nestedloopjoin_iter_vb_info* info	= (_hx_nestedloopjoin_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_nestedloopjoin_prime_results( info );
	}
//	fprintf( stderr, "- finished == %d\n", info->finished );
	return info->finished;
}

int _hx_nestedloopjoin_iter_vb_current ( void* data, void* results ) {
//	fprintf( stderr, "*** _hx_nestedloopjoin_iter_vb_current\n" );
	_hx_nestedloopjoin_iter_vb_info* info	= (_hx_nestedloopjoin_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_nestedloopjoin_prime_results( info );
	}
	
	hx_variablebindings** b	= (hx_variablebindings**) results;
	*b	= hx_copy_variablebindings( info->current );
	return 0;
}

int _hx_nestedloopjoin_iter_vb_next ( void* data ) {
// 	fprintf( stderr, "*** _hx_nestedloopjoin_iter_vb_next\n" );
	_hx_nestedloopjoin_iter_vb_info* info	= (_hx_nestedloopjoin_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_nestedloopjoin_prime_results( info );
	}
	
	if (info->current != NULL) {
		hx_free_variablebindings(info->current);
		info->current	= NULL;
	}
	
	while (!(info->lhs_batch_index == info->lhs_size - 1 && info->rhs_batch_index == info->rhs_size - 1)) {
		info->rhs_batch_index++;
		if (info->rhs_batch_index >= info->rhs_size) {
// 			fprintf( stderr, "- end of RHS. incrementing LHS index and resetting RHS index to 0\n" );
			if (info->leftjoin) {
// 				fprintf( stderr, "  - in leftjoin\n" );
				if (info->leftjoin_seen_lhs_result == 0) {
// 					fprintf( stderr, "- in a leftjoin and no RHS result used in a join. using LHS as a join result\n" );
					info->leftjoin_seen_lhs_result	= 1;
					info->current	= hx_copy_variablebindings( info->lhs_batch[info->lhs_batch_index] );
					return 0;
				}
			}
			info->rhs_batch_index	= 0;
			info->lhs_batch_index++;
			
			info->leftjoin_seen_lhs_result	= 0;
			if (info->lhs_batch_index >= info->lhs_size) {
// 				fprintf( stderr, "- end of LHS. finding new matching batches...\n" );
				info->finished	= 1;
				return 1;
			}
		}
		
		hx_variablebindings* lhs	= info->lhs_batch[info->lhs_batch_index];
		hx_variablebindings* rhs	= info->rhs_batch[info->rhs_batch_index];
		
// 		char* string;
// 		hx_variablebindings_string( info->lhs_batch[ info->lhs_batch_index ], &string );
// 		fprintf( stderr, "- new lhs result in nestedloopjoin: %s\n", string );
// 		free(string);
// 		hx_variablebindings_string( info->rhs_batch[ info->rhs_batch_index ], &string );
// 		fprintf( stderr, "- new rhs result in nestedloopjoin: %s\n", string );
// 		free(string);
		
		hx_variablebindings* joined	= hx_variablebindings_natural_join( lhs, rhs );
		if (joined != NULL) {
			info->current	= joined;
			info->leftjoin_seen_lhs_result	= 1;
			return 0;
		}
	}
	
	if (info->leftjoin) {
		if (info->leftjoin_seen_lhs_result == 0) {
// 			fprintf( stderr, "- at end of iterator, in a leftjoin and no RHS result used in a join. using LHS as a join result\n" );
			info->leftjoin_seen_lhs_result	= 1;
			info->current	= hx_copy_variablebindings( info->lhs_batch[info->lhs_batch_index] );
			return 0;
		}
	}
	
// 	fprintf( stderr, "- exhausted iterator in _hx_nestedloopjoin_iter_vb_next\n" );
	info->finished	= 1;
	return 1;
}

int _hx_nestedloopjoin_iter_vb_free ( void* data ) {
	_hx_nestedloopjoin_iter_vb_info* info	= (_hx_nestedloopjoin_iter_vb_info*) data;
	
	if (info->current != NULL) {
		hx_free_variablebindings(info->current);
		info->current	= NULL;
	}

	int i;
	for (i = 0; i < info->lhs_size; i++) {
		hx_free_variablebindings(info->lhs_batch[i]);
		info->lhs_batch[i]	= NULL;
	}
	for (i = 0; i < info->rhs_size; i++) {
		hx_free_variablebindings(info->rhs_batch[i]);
		info->rhs_batch[i]	= NULL;
	}
	info->lhs_size	= 0;
	info->rhs_size	= 0;
	hx_free_variablebindings_iter( info->lhs );
	hx_free_variablebindings_iter( info->rhs );
	free( info->rhs_batch );
	free( info->lhs_batch );
	for (i = 0; i < info->size; i++) {
		free( info->names[i] );
	}
	free( info->names );
	free( info );
	return 0;
}

int _hx_nestedloopjoin_iter_vb_size ( void* data ) {
	_hx_nestedloopjoin_iter_vb_info* info	= (_hx_nestedloopjoin_iter_vb_info*) data;
	return info->size;
}

char** _hx_nestedloopjoin_iter_vb_names ( void* data ) {
	_hx_nestedloopjoin_iter_vb_info* info	= (_hx_nestedloopjoin_iter_vb_info*) data;
	return info->names;
}

int _hx_nestedloopjoin_iter_sorted_by ( void* data, int index ) {
// 	_hx_nestedloopjoin_iter_vb_info* info	= (_hx_nestedloopjoin_iter_vb_info*) data;
// 	char* name	= info->names[ index ];
// 	int lhs_size		= hx_variablebindings_iter_size( info->lhs );
// 	char** lhs_names	= hx_variablebindings_iter_names( info->lhs );
// 	int lhs_index		= hx_variablebindings_column_index( info->lhs, name );
// 	return (lhs_index == info->lhs_index);
	return 0;
}

int _hx_nestedloopjoin_debug ( void* data, char* header, int indent ) {
	_hx_nestedloopjoin_iter_vb_info* info	= (_hx_nestedloopjoin_iter_vb_info*) data;
	int i;
	for (i = 0; i < indent; i++) fwrite( " ", sizeof( char ), 1, stderr );
	fprintf( stderr, "%s nestedloopjoin iterator\n", header );
	hx_variablebindings_iter_debug( info->lhs, header, indent + 4 );
	hx_variablebindings_iter_debug( info->rhs, header, indent + 4 );
	return 0;
}

hx_variablebindings_iter* hx_new_nestedloopjoin_iter ( hx_variablebindings_iter* lhs, hx_variablebindings_iter* rhs ) {
	return hx_new_nestedloopjoin_iter2( lhs, rhs, 0 );
}

hx_variablebindings_iter* hx_new_nestedloopjoin_iter2 ( hx_variablebindings_iter* lhs, hx_variablebindings_iter* rhs, int leftjoin ) {
	hx_variablebindings_iter_vtable* vtable	= (hx_variablebindings_iter_vtable*) malloc( sizeof( hx_variablebindings_iter_vtable ) );
	if (vtable == NULL) {
		fprintf( stderr, "*** malloc failed in hx_new_nestedloopjoin_iter\n" );
		return NULL;
	}
	vtable->finished	= _hx_nestedloopjoin_iter_vb_finished;
	vtable->current		= _hx_nestedloopjoin_iter_vb_current;
	vtable->next		= _hx_nestedloopjoin_iter_vb_next;
	vtable->free		= _hx_nestedloopjoin_iter_vb_free;
	vtable->names		= _hx_nestedloopjoin_iter_vb_names;
	vtable->size		= _hx_nestedloopjoin_iter_vb_size;
	vtable->sorted_by_index	= _hx_nestedloopjoin_iter_sorted_by;
	vtable->debug		= _hx_nestedloopjoin_debug;
	
	int size;
	char** merged_names;
	_hx_nestedloopjoin_join_iter_names( lhs, rhs, &merged_names, &size );
	_hx_nestedloopjoin_iter_vb_info* info	= (_hx_nestedloopjoin_iter_vb_info*) calloc( 1, sizeof( _hx_nestedloopjoin_iter_vb_info ) );
	
	info->current			= NULL;
	info->lhs				= lhs;
	info->rhs				= rhs;
	info->size				= size;
	info->names				= merged_names;
	info->finished			= 0;
	info->started			= 0;
	info->leftjoin			= leftjoin;
	info->leftjoin_seen_lhs_result	= 0;
	
	info->lhs_size			= 0;
	info->rhs_size			= 0;
	
	info->lhs_batch_index	= 0;
	info->rhs_batch_index	= 0;
	
	info->lhs_batch			= (hx_variablebindings**) calloc( NODE_LIST_ALLOC_SIZE, sizeof( hx_variablebindings* ) );
	info->rhs_batch			= (hx_variablebindings**) calloc( NODE_LIST_ALLOC_SIZE, sizeof( hx_variablebindings* ) );
	
	info->lhs_batch_alloc_size	= NODE_LIST_ALLOC_SIZE;
	info->rhs_batch_alloc_size	= NODE_LIST_ALLOC_SIZE;
	
	hx_variablebindings_iter* iter	= hx_variablebindings_new_iter( vtable, (void*) info );
	return iter;
}

int _hx_nestedloopjoin_get_batch ( _hx_nestedloopjoin_iter_vb_info* info, hx_variablebindings_iter* iter, int* batch_size, int* batch_alloc_size, hx_variablebindings*** batch ) {
	hx_variablebindings* b;
	
	if (*batch_size > 0) {
//		fprintf( stderr, "getting new batch (batch currently has %d items)\n", *batch_size );
		int i;
		for (i = 0; i < *batch_size; i++) {
			hx_free_variablebindings((*batch)[i]);
			(*batch)[i]	= NULL;
		}
	}
	
	*batch_size	= 0;
	if (hx_variablebindings_iter_finished( iter )) {
// 		fprintf( stderr, "- iterator is finished (in get_batch)\n" );
		return 1;
	}
	
	hx_variablebindings_iter_current( iter, &b );
	(*batch)[ (*batch_size)++ ]	= b;
	hx_variablebindings_iter_next( iter );
	
	while (!hx_variablebindings_iter_finished( iter )) {
		hx_variablebindings_iter_current( iter, &b );
// 		fprintf( stderr, "*** get_batch: next id %d\n", (int) id );
		if (*batch_size >= *batch_alloc_size) {
			int size	= *batch_alloc_size * 2;
			hx_variablebindings** _new	= (hx_variablebindings**) calloc( size, sizeof( hx_variablebindings* ) );
			if (_new == NULL) {
				return -1;
			}
			int i;
			for (i = 0; i < *batch_size; i++) {
				_new[i]	= (*batch)[i];
			}
			free( *batch );
			*batch	= _new;
			*batch_alloc_size	= size;
		}
		(*batch)[ (*batch_size)++ ]	= b;
		hx_variablebindings_iter_next( iter );
	}

	if (*batch_size > 0) {
		return 0;
	} else {
		info->finished	= 1;
		*batch_size	= 0;
		return 1;
	}
}
// 
// int _hx_nestedloopjoin_get_lhs_batch ( _hx_nestedloopjoin_iter_vb_info* info ) {
// 	return _hx_nestedloopjoin_get_batch( info, info->lhs, info->lhs_index, &( info->lhs_key ), &( info->lhs_batch_size ), &( info->lhs_batch_alloc_size ), &( info->lhs_batch ) );
// }
// 
// int _hx_nestedloopjoin_get_rhs_batch ( _hx_nestedloopjoin_iter_vb_info* info ) {
// 	return _hx_nestedloopjoin_get_batch( info, info->rhs, info->rhs_index, &( info->rhs_key ), &( info->rhs_batch_size ), &( info->rhs_batch_alloc_size ), &( info->rhs_batch ) );
// }

int _hx_nestedloopjoin_join_vb_names ( hx_variablebindings* lhs, hx_variablebindings* rhs, char*** merged_names, int* size ) {
	int lhs_size		= hx_variablebindings_size( lhs );
	char** lhs_names	= hx_variablebindings_names( lhs );
	int rhs_size		= hx_variablebindings_size( rhs );
	char** rhs_names	= hx_variablebindings_names( rhs );
	return _hx_nestedloopjoin_join_names( lhs_names, lhs_size, rhs_names, rhs_size, merged_names, size );
}
int _hx_nestedloopjoin_join_iter_names ( hx_variablebindings_iter* lhs, hx_variablebindings_iter* rhs, char*** merged_names, int* size ) {
	int lhs_size		= hx_variablebindings_iter_size( lhs );
	char** lhs_names	= hx_variablebindings_iter_names( lhs );
	int rhs_size		= hx_variablebindings_iter_size( rhs );
	char** rhs_names	= hx_variablebindings_iter_names( rhs );
	return _hx_nestedloopjoin_join_names( lhs_names, lhs_size, rhs_names, rhs_size, merged_names, size );
}
int _hx_nestedloopjoin_join_names ( char** lhs_names, int lhs_size, char** rhs_names, int rhs_size, char*** merged_names, int* size ) {
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
