#include "engine/filter.h"

int _hx_filter_prime_results ( _hx_filter_iter_vb_info* info );
int _hx_filter_get_next_result ( _hx_filter_iter_vb_info* info );

hx_variablebindings_iter* hx_new_filter_iter ( hx_variablebindings_iter* iter, hx_expr* e, hx_nodemap* map ) {
	hx_variablebindings_iter_vtable* vtable	= (hx_variablebindings_iter_vtable*) malloc( sizeof( hx_variablebindings_iter_vtable ) );
	if (vtable == NULL) {
		fprintf( stderr, "*** malloc failed in hx_new_filter_iter\n" );
	}
	if (iter == NULL) {
		fprintf( stderr, "*** NULL iterator passed to hx_new_filter_iter\n" );
	}
	vtable->finished	= _hx_filter_iter_vb_finished;
	vtable->current		= _hx_filter_iter_vb_current;
	vtable->next		= _hx_filter_iter_vb_next;
	vtable->free		= _hx_filter_iter_vb_free;
	vtable->names		= _hx_filter_iter_vb_names;
	vtable->size		= _hx_filter_iter_vb_size;
	vtable->sorted_by_index	= _hx_filter_iter_sorted_by;
	vtable->debug		= _hx_filter_debug;
	
	_hx_filter_iter_vb_info* info	= (_hx_filter_iter_vb_info*) calloc( 1, sizeof( _hx_filter_iter_vb_info ) );
	info->started	= 0;
	info->finished	= 0;
	info->iter		= iter;
	info->expr		= e;
	info->current	= NULL;
	info->map		= map;
	hx_variablebindings_iter* fiter	= hx_variablebindings_new_iter( vtable, (void*) info );
	return fiter;
}

int _hx_filter_iter_vb_finished ( void* data ) {
	_hx_filter_iter_vb_info* info	= (_hx_filter_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_filter_prime_results( info );
	}
	return info->finished;
}

int _hx_filter_iter_vb_current ( void* data, void* results ) {
	_hx_filter_iter_vb_info* info	= (_hx_filter_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_filter_prime_results( info );
	}

	hx_variablebindings** b	= (hx_variablebindings**) results;
	*b	= hx_copy_variablebindings( info->current );
	return 0;
}

int _hx_filter_iter_vb_next ( void* data ) {
	_hx_filter_iter_vb_info* info	= (_hx_filter_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_filter_prime_results( info );
	}
	
	hx_variablebindings_iter* iter	= info->iter;
	hx_expr* e						= info->expr;
	hx_nodemap* map					= info->map;
	if (_hx_filter_get_next_result( info ) == 0) {
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

int _hx_filter_iter_vb_free ( void* data ) {
	_hx_filter_iter_vb_info* info	= (_hx_filter_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_filter_prime_results( info );
	}

	if (info->current != NULL) {
		hx_free_variablebindings(info->current);
		info->current	= NULL;
	}
	hx_free_variablebindings_iter( info->iter );
	free( info );
	return 0;
}

int _hx_filter_iter_vb_size ( void* data ) {
	_hx_filter_iter_vb_info* info	= (_hx_filter_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_filter_prime_results( info );
	}

	return hx_variablebindings_iter_size( info->iter );
}

char** _hx_filter_iter_vb_names ( void* data ) {
	_hx_filter_iter_vb_info* info	= (_hx_filter_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_filter_prime_results( info );
	}

	return hx_variablebindings_iter_names( info->iter );
}

int _hx_filter_iter_sorted_by ( void* data, int index ) {
	_hx_filter_iter_vb_info* info	= (_hx_filter_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_filter_prime_results( info );
	}
	
	return hx_variablebindings_iter_is_sorted_by_index( info->iter, index );
}

int _hx_filter_debug ( void* data, char* header, int _indent ) {
	_hx_filter_iter_vb_info* info	= (_hx_filter_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_filter_prime_results( info );
	}
	
	return hx_variablebindings_iter_debug( info->iter, header, _indent );
}

int _hx_filter_prime_results ( _hx_filter_iter_vb_info* data ) {
	_hx_filter_iter_vb_info* info	= (_hx_filter_iter_vb_info*) data;
	if (_hx_filter_get_next_result( info ) == 0) {
		info->started	= 1;
		return 0;
	}
	info->started	= 1;
	info->finished	= 1;
	info->current	= NULL;
	return 1;
}

int _hx_filter_get_next_result ( _hx_filter_iter_vb_info* info ) {
	hx_variablebindings_iter* iter	= info->iter;
	hx_expr* e						= info->expr;
	hx_nodemap* map					= info->map;
	
	if (info->started == 1) {
		hx_variablebindings_iter_next( iter );
	}
// 	fprintf( stderr, "getting next valid result in FILTER\n" );
	while (!hx_variablebindings_iter_finished( iter )) {
// 		fprintf( stderr, "- got result\n" );
		hx_node* value;
		hx_variablebindings* b;
		hx_variablebindings_iter_current( iter, &b );
		
// 		{
// 			char* string;
// 			hx_variablebindings_string( b, map, &string );
// 			fprintf( stderr, "- got variablebindings in filter: %s\n", string );
// 			free(string);
// 		}
		
		
		int r		= hx_expr_eval( e, b, map, &value );
		if (r != 0) {
// 			fprintf( stderr, "type error in filter\n" );
			hx_free_variablebindings(b);
			hx_variablebindings_iter_next( iter );
			continue;
		}
		
		int ebv	= hx_node_ebv(value);
		if (ebv == -1) {
// 			fprintf( stderr, "type error in EBV\n" );
			hx_free_variablebindings(b);
			hx_variablebindings_iter_next( iter );
			continue;
		}
		
		if (ebv == 0) {
// 			fprintf( stderr, "false EBV in filter\n" );
			hx_free_variablebindings(b);
			hx_variablebindings_iter_next( iter );
			continue;
		}
		
		if (info->current != NULL) {
			hx_free_variablebindings(info->current);
			info->current	= NULL;
		}
		
// 		fprintf( stderr, "true EBV in filter\n" );
		info->current	= b;
		return 0;
	}
	info->finished	= 1;
	if (info->current != NULL) {
		hx_free_variablebindings(info->current);
		info->current	= NULL;
	}
	return 1;
}
