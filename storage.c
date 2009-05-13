#include "storage.h"

hx_storage_manager* hx_new_memory_storage_manager( void ) {
	hx_storage_manager* s	= (hx_storage_manager*) calloc( 1, sizeof( hx_storage_manager ) );
	s->flags				= HX_STORAGE_MEMORY;
	return s;
}

hx_storage_manager* hx_new_file_storage_manager( const char* filename );
hx_storage_manager* hx_open_file_storage_manager( const char* filename );

int hx_free_storage_manager( hx_storage_manager* s ) {
	if (s->flags & HX_STORAGE_MEMORY) {
		free( s );
		return 0;
	} else {
		fprintf( stderr, "*** trying to free unimplemented storage manager\n" );
		return 1;
	}
}

void* hx_storage_new_block( hx_storage_manager* s, size_t size ) {
	if (s->flags & HX_STORAGE_MEMORY) {
		return calloc( 1, size );
	} else {
		fprintf( stderr, "*** trying to create new block with unimplemented storage manager\n" );
		return NULL;
	}
}

int hx_storage_release_block( hx_storage_manager* s, void* block ) {
	if (s->flags & HX_STORAGE_MEMORY) {
		free( block );
		return 0;
	} else {
		fprintf( stderr, "*** trying to free block with unimplemented storage manager\n" );
		return 1;
	}
}

int hx_storage_sync_block( hx_storage_manager* s, void* block ) {
	if (s->flags & HX_STORAGE_MEMORY) {
		return 0;
	} else {
		fprintf( stderr, "*** trying to sync block with unimplemented storage manager\n" );
		return 1;
	}
}

hx_storage_id_t hx_storage_id_from_block ( hx_storage_manager* s, void* block ) {
	if (s->flags & HX_STORAGE_MEMORY) {
		return (hx_storage_id_t) block;
	} else {
		fprintf( stderr, "*** trying to get block id with unimplemented storage manager\n" );
		return 0;
	}
}

void* hx_storage_block_from_id ( hx_storage_manager* s, hx_storage_id_t id ) {
	if (s->flags & HX_STORAGE_MEMORY) {
//		fprintf( stderr, "%x <> %p\n", id, (void*) id );
		return (void*) id;
	} else {
		fprintf( stderr, "*** trying to get block pointer with unimplemented storage manager\n" );
		return NULL;
	}
}

void* hx_storage_first_block ( hx_storage_manager* s ) {
	return hx_storage_block_from_id( s, HX_STORAGE_HEADER_SIZE + HX_STORAGE_BLOCK_HEADER_SIZE );
}

int hx_storage_set_freeze_remap_handler ( hx_storage_manager* s, hx_storage_handler* h, void* arg ) {
	s->freeze_handler	= h;
	s->freeze_arg		= arg;
	return 0;
}

int hx_storage_set_thaw_remap_handler ( hx_storage_manager* s, hx_storage_handler* h, void* arg ) {
	s->thaw_handler	= h;
	s->thaw_arg		= arg;
	return 0;
}

