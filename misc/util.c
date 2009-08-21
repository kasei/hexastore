#include "misc/util.h"

uint64_t hx_util_hash_string ( const char* s ) {
	uint64_t h	= 0;
	int len	= strlen(s);
	int i;
	for (i = 0; i < len; i++) {
		unsigned char ki	= (unsigned char) s[i];
		uint64_t highorder	= h & 0xfffffffff8000000;	// extract high-order 37 bits from h
		h	= h << 37;									// shift h left by 37 bits
		h	= h ^ (highorder >> 37);					// move the highorder 37 bits to the low-order end and XOR into h
		h = h ^ ki;										// XOR h and ki
	}
	return h;
}

hx_container_t* hx_new_container ( char type, int size ) {
	hx_container_t* container	= (hx_container_t*) calloc( 1, sizeof( hx_container_t ) );
	container->type			= type;
	container->allocated	= size;
	container->count		= 0;
	container->items		= (void**) calloc( container->allocated, sizeof( void* ) );
	return container;
}

int hx_free_container ( hx_container_t* c ) {
	free(c);
	return 0;
}

void hx_container_push_item( hx_container_t* set, void* t ) {
	if (set->allocated <= (set->count + 1)) {
		int i;
		void** old;
		void** newlist;
		set->allocated	*= 2;
		newlist	= (void**) calloc( set->allocated, sizeof( void* ) );
		for (i = 0; i < set->count; i++) {
			newlist[i]	= set->items[i];
		}
		old	= set->items;
		set->items	= newlist;
		free( old );
	}
	
	set->items[ set->count++ ]	= t;
}

void hx_container_unshift_item( hx_container_t* set, void* t ) {
	if (set->allocated <= (set->count + 1)) {
		int i;
		void** old;
		void** newlist;
		set->allocated	*= 2;
		newlist	= (void**) calloc( set->allocated, sizeof( void* ) );
		for (i = 0; i < set->count; i++) {
			newlist[i+1]	= set->items[i];
		}
		old	= set->items;
		set->items	= newlist;
		free( old );
	} else {
		int i;
		for (i = set->count; i > 0; i--) {
			set->items[i]	= set->items[i-1];
		}
	}
	
	set->count++;
	set->items[ 0 ]	= t;
}

int hx_container_size( hx_container_t* c ) {
	return c->count;
}

void* hx_container_item ( hx_container_t* c, int i ) {
	return c->items[i];
}
