#include "engine/project.h"

hx_variablebindings_iter* hx_new_project_iter ( hx_variablebindings_iter* _iter, int size, char** names ) {
	hx_variablebindings_iter_vtable* vtable	= (hx_variablebindings_iter_vtable*) malloc( sizeof( hx_variablebindings_iter_vtable ) );
	if (vtable == NULL) {
		fprintf( stderr, "*** malloc failed in hx_new_project_iter\n" );
		return NULL;
	}
	vtable->finished	= _hx_project_iter_vb_finished;
	vtable->current		= _hx_project_iter_vb_current;
	vtable->next		= _hx_project_iter_vb_next;
	vtable->free		= _hx_project_iter_vb_free;
	vtable->names		= _hx_project_iter_vb_names;
	vtable->size		= _hx_project_iter_vb_size;
	vtable->sorted_by_index	= _hx_project_iter_sorted_by;
	vtable->debug		= _hx_project_debug;
	
	_hx_project_iter_vb_info* info	= (_hx_project_iter_vb_info*) calloc( 1, sizeof( _hx_project_iter_vb_info ) );
	info->finished	= 0;
	info->iter		= _iter;
	info->size		= size;
	info->current	= NULL;
	info->names		= (char**) calloc( size, sizeof( char* ) );
	info->columns	= (int*) calloc( size, sizeof( int ) );
	int i;
	for (i = 0; i < size; i++) {
		int len	= strlen( names[i] );
		info->names[i]	= (char*) calloc( len, sizeof( char ) );
		if (names[i] == NULL) {
			fprintf( stderr, "*** hx_new_project_iter called with NULL project variable name\n" );
		}
		strcpy( info->names[i], names[i] );
		info->columns[i]	= hx_variablebindings_column_index( info->iter, names[i] );
	}
	hx_variablebindings_iter* iter	= hx_variablebindings_new_iter( vtable, (void*) info );
	return iter;
}

int _hx_project_iter_vb_finished ( void* data ) {
	_hx_project_iter_vb_info* info	= (_hx_project_iter_vb_info*) data;
	return hx_variablebindings_iter_finished(  info->iter );
}

int _hx_project_iter_vb_current ( void* data, void* results ) {
	_hx_project_iter_vb_info* info	= (_hx_project_iter_vb_info*) data;
	if (info->finished == 1 || hx_variablebindings_iter_finished(info->iter)) {
		info->finished	= 1;
		return 1;
	}
	
	if (info->current == NULL) {
		hx_variablebindings* b;
		hx_variablebindings_iter_current( info->iter, &b );
		hx_variablebindings* p	= hx_variablebindings_project( b, info->size, info->columns );
		info->current	= p;
	}
	
	hx_variablebindings** b	= (hx_variablebindings**) results;
	*b	= hx_copy_variablebindings( info->current );
	
	return 0;
}

int _hx_project_iter_vb_next ( void* data ) {
	_hx_project_iter_vb_info* info	= (_hx_project_iter_vb_info*) data;
	int r	= hx_variablebindings_iter_next( info->iter );
	if (info->current != NULL) {
		hx_free_variablebindings(info->current);
		info->current	= NULL;
	}
	return r;
}

int _hx_project_iter_vb_free ( void* data ) {
	_hx_project_iter_vb_info* info	= (_hx_project_iter_vb_info*) data;
	hx_free_variablebindings_iter( info->iter );
	if (info->current != NULL) {
		hx_free_variablebindings(info->current);
		info->current	= NULL;
	}
	int i;
	for (i = 0; i < info->size; i++) {
		free( info->names[i] );
	}
	free( info->columns );
	free( info->names );
	free( info );
	return 0;
}

int _hx_project_iter_vb_size ( void* data ) {
	_hx_project_iter_vb_info* info	= (_hx_project_iter_vb_info*) data;
	return info->size;
}

char** _hx_project_iter_vb_names ( void* data ) {
	_hx_project_iter_vb_info* info	= (_hx_project_iter_vb_info*) data;
	return info->names;
}

int _hx_project_iter_sorted_by ( void* data, int index ) {
	_hx_project_iter_vb_info* info	= (_hx_project_iter_vb_info*) data;
	// XXX need to re-map the index number to a varname, then map that varname
	// XXX onto an index number for info->iter. then call sorted_by on info->iter.
	return 0;
}

int _hx_project_debug ( void* data, char* header, int _indent ) {
	_hx_project_iter_vb_info* info	= (_hx_project_iter_vb_info*) data;
	char* indent	= (char*) malloc( _indent + 1 );
	if (indent == NULL) {
		fprintf( stderr, "*** malloc failed in _hx_project_debug\n" );
		return NULL;
	}
	char* p			= indent;
	int i;
	for (i = 0; i < _indent; i++) *(p++) = ' ';
	*p	= (char) 0;

	fprintf( stderr, "%s%s Project iterator\n", indent, header );
	fprintf( stderr, "%s%s Info: %p\n", indent, header, (void*) info );
	fprintf( stderr, "%s%s Size: %d\n", indent, header, (int) info->size );
	fprintf( stderr, "%s%s Project columns:\n", indent, header );
	for (i = 0; i < info->size; i++) {
		fprintf( stderr, "%s%s   [%d] %s\n", indent, header, i, info->names[i] );
	}
	hx_variablebindings_iter_debug( info->iter, header, _indent+1 );
	free(indent);
	return 0;
}

