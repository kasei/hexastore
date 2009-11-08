#include "mentok/engine/union.h"

int _hx_union_prime_results ( _hx_union_iter_vb_info* info );
int _hx_union_get_next_result ( _hx_union_iter_vb_info* info );

hx_variablebindings_iter* hx_new_union_iter ( hx_execution_context* ctx, hx_container_t* iters ) {
	hx_variablebindings_iter_vtable* vtable	= (hx_variablebindings_iter_vtable*) malloc( sizeof( hx_variablebindings_iter_vtable ) );
	if (vtable == NULL) {
		fprintf( stderr, "*** malloc failed in hx_new_union_iter\n" );
		return NULL;
	}
	
	if (iters == NULL) {
		fprintf( stderr, "*** NULL iterator container passed to hx_new_union_iter\n" );
		free(vtable);
		return NULL;
	}
	int size	= hx_container_size( iters );
	if (size < 1) {
		fprintf( stderr, "*** Empty iterator container passed to hx_new_union_iter\n" );
		free(vtable);
		return NULL;
	}
	
	vtable->finished	= _hx_union_iter_vb_finished;
	vtable->current		= _hx_union_iter_vb_current;
	vtable->next		= _hx_union_iter_vb_next;
	vtable->free		= _hx_union_iter_vb_free;
	vtable->names		= _hx_union_iter_vb_names;
	vtable->size		= _hx_union_iter_vb_size;
	vtable->sorted_by_index	= _hx_union_iter_sorted_by;
	vtable->debug		= _hx_union_debug;
	
	_hx_union_iter_vb_info* info	= (_hx_union_iter_vb_info*) calloc( 1, sizeof( _hx_union_iter_vb_info ) );
	info->started		= 0;
	info->finished		= 0;
	info->iter_index	= -1;
	info->iters			= iters;
	info->current		= NULL;
	info->ctx			= ctx;
	
	hx_variablebindings_iter* iter	= hx_variablebindings_new_iter( vtable, (void*) info );
	return iter;
}

hx_variablebindings_iter* hx_new_union_iter2 ( hx_execution_context* ctx, hx_variablebindings_iter* i1, hx_variablebindings_iter* i2 ) {
	hx_container_t* iters	= hx_new_container( 'I', 2 );
	hx_container_push_item( iters, i1 );
	hx_container_push_item( iters, i2 );
	return hx_new_union_iter( ctx, iters );
}

int _hx_union_iter_vb_finished ( void* data ) {
	_hx_union_iter_vb_info* info	= (_hx_union_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_union_prime_results( info );
	}
	return info->finished;
}

int _hx_union_iter_vb_current ( void* data, void* results ) {
	_hx_union_iter_vb_info* info	= (_hx_union_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_union_prime_results( info );
	}

	hx_variablebindings** b	= (hx_variablebindings**) results;
	*b	= hx_copy_variablebindings( info->current );
	return 0;
}

int _hx_union_iter_vb_next ( void* data ) {
	_hx_union_iter_vb_info* info	= (_hx_union_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_union_prime_results( info );
	}
	
	if (_hx_union_get_next_result( info ) == 0) {
		return 0;
	}
	if (info->current != NULL) {
		hx_free_variablebindings(info->current);
		info->current	= NULL;
	}
	info->finished	= 1;
	info->current	= NULL;
	return 1;
}

int _hx_union_iter_vb_free ( void* data ) {
	_hx_union_iter_vb_info* info	= (_hx_union_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_union_prime_results( info );
	}

	if (info->current != NULL) {
		hx_free_variablebindings(info->current);
		info->current	= NULL;
	}
	
	int i;
	int size	= hx_container_size( info->iters );
	for (i = 0; i < size; i++) {
		hx_free_variablebindings_iter( hx_container_item( info->iters, i ) );
	}
	hx_free_container( info->iters );
	free( info );
	return 0;
}

int _hx_union_iter_vb_size ( void* data ) {
	_hx_union_iter_vb_info* info	= (_hx_union_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_union_prime_results( info );
	}
	
	// XXX we should count up the unique columsn across all union subplans
	// XXX instead, this is a hack that just passes through the size call to the first subplan
	return hx_variablebindings_iter_size( hx_container_item( info->iters, 0 ) );
}

char** _hx_union_iter_vb_names ( void* data ) {
	_hx_union_iter_vb_info* info	= (_hx_union_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_union_prime_results( info );
	}

	// XXX we should get the unique columns across all union subplans
	// XXX instead, this is a hack that just passes through the names call to the first subplan
	return hx_variablebindings_iter_names( hx_container_item( info->iters, 0 ) );
}

int _hx_union_iter_sorted_by ( void* data, int index ) {
	_hx_union_iter_vb_info* info	= (_hx_union_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_union_prime_results( info );
	}
	
	return 0;	// union plans don't have a predictable sort order
}

int _hx_union_debug ( void* data, char* header, int _indent ) {
	_hx_union_iter_vb_info* info	= (_hx_union_iter_vb_info*) data;
	char* indent	= (char*) malloc( _indent + 1 );
	if (indent == NULL) {
		fprintf( stderr, "*** malloc failed in _hx_materialize_debug\n" );
		return 1;
	}
	char* p			= indent;
	int i;
	for (i = 0; i < _indent; i++) *(p++) = ' ';
	*p	= (char) 0;
	
	if (info->started == 0) {
		_hx_union_prime_results( info );
	}
	
	int size	= hx_container_size( info->iters );
	fprintf( stderr, "%s%sunion (%d subplans):\n", indent, header, size );
	for (i = 0; i < size; i++) {
		fprintf( stderr, "%s%s(%d) ", indent, header, i );
		hx_variablebindings_iter_debug( hx_container_item( info->iters, i ), header, _indent + 1 );
	}
	return 0;
}

int _hx_union_prime_results ( _hx_union_iter_vb_info* data ) {
	_hx_union_iter_vb_info* info	= (_hx_union_iter_vb_info*) data;
	if (_hx_union_get_next_result( info ) == 0) {
		info->started	= 1;
		return 0;
	}
	info->started	= 1;
	info->finished	= 1;
	info->current	= NULL;
	return 1;
}

int _hx_union_get_next_result ( _hx_union_iter_vb_info* info ) {
	int goto_next	= 1;
	if (info->iter_index < 0) {
// 		fprintf( stderr, "*** starting union iterator at iter0\n" );
		info->iter_index	= 0;
		goto_next			= 0;
	}
	
	int size	= hx_container_size( info->iters );
	hx_variablebindings_iter* iter	= hx_container_item( info->iters, info->iter_index );
	if (goto_next == 1) {
// 		fprintf( stderr, "*** moving union iterator to next result\n" );
		hx_variablebindings_iter_next(iter);
	}
	
	while (hx_variablebindings_iter_finished(iter)) {
// 		fprintf( stderr, "*** union iterator hit end. going to next iterator\n" );
		info->iter_index++;
		if (info->iter_index >= size) {
// 			fprintf( stderr, "*** no more iterators left. setting finished flag and returning.\n" );
			info->finished	= 1;
			if (info->current != NULL) {
				hx_free_variablebindings(info->current);
				info->current	= NULL;
			}
			return 1;
		}
		iter	= hx_container_item( info->iters, info->iter_index );
	}
	
	hx_variablebindings* b;
	hx_variablebindings_iter_current( iter, &b );
	if (info->current != NULL) {
		hx_free_variablebindings(info->current);
		info->current	= NULL;
	}
		
	info->current	= b;
	return 0;
}
