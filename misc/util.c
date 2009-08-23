#include "misc/util.h"

uint64_t hx_util_hash_string ( const char* s ) {
	int len	= strlen(s);
	return hx_util_hash_buffer( s, len );
}

uint64_t hx_util_hash_buffer ( const char* s, size_t len ) {
	uint64_t h	= 0;
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

hx_container_t* hx_copy_container ( hx_container_t* c ) {
	hx_container_t* container	= (hx_container_t*) calloc( 1, sizeof( hx_container_t ) );
	container->type			= c->type;
	container->allocated	= c->allocated;
	container->count		= c->count;
	container->items		= (void**) calloc( container->allocated, sizeof( void* ) );
	int i;
	for (i = 0; i < c->count; i++) {
		container->items[i]	= c->items[i];
	}
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

hx_hash_t* hx_new_hash ( int buckets ) {
	int i;
	hx_hash_t* h	= (hx_hash_t*) calloc( 1, sizeof( hx_hash_t ) );
	h->size			= buckets;
	h->buckets		= hx_new_container( 'B', buckets );
	for (i = 0; i < buckets; i++) {
		hx_container_push_item( h->buckets, hx_new_container( 'I', 4 ) );
	}
	return h;
}


int hx_hash_add ( hx_hash_t* hash, void* key, size_t klen, void* value ) {
	if (klen == 0) {
		klen	= strlen( (char*) key );
	}
	
	char* keycopy	= (char*) malloc( klen );
	memcpy( keycopy, key, klen );
	
	uint64_t hv;
	hv	= hx_util_hash_buffer( keycopy, klen );
	int bucket_number		= hv % hash->size;
	hx_container_t* bucket	= hx_container_item( hash->buckets, bucket_number );
	hx_hash_bucket_item_t* i	= (hx_hash_bucket_item_t*) calloc( 1, sizeof( hx_hash_bucket_item_t ) );
	i->key		= keycopy;
	i->klen		= klen;
	i->value	= value;
	hx_container_push_item( bucket, i );
	return 0;
}

int _hx_hash_apply_with_bucket ( hx_container_t* bucket, void* key, size_t klen, int apply_cb( void* key, void* value, void* thunk ), void* thunk ) {
	int i;
	int size	= hx_container_size( bucket );
	int counter	= 0;
	for (i = 0; i < size; i++) {
		hx_hash_bucket_item_t* item	= hx_container_item( bucket, i );
		
		if ((klen == 0) || (item->klen == klen && memcmp( key, item->key, klen ) == 0)) {
			apply_cb( item->key, item->value, thunk );
			counter++;
		}
	}
	return counter;
}


int hx_hash_apply ( hx_hash_t* hash, void* key, size_t klen, int apply_cb( void* key, void* value, void* thunk ), void* thunk ) {
	if (klen == 0) {
		int i;
		int counter	= 0;
		int size	= hash->size;
		for (i = 0; i < size; i++) {
			hx_container_t* bucket	= hx_container_item( hash->buckets, i );
			counter	+= _hx_hash_apply_with_bucket( bucket, key, klen, apply_cb, thunk );
		}
		return counter;
	} else {
		uint64_t hv	= hx_util_hash_buffer( key, klen );
		int bucket_number		= hv % hash->size;
		hx_container_t* bucket	= hx_container_item( hash->buckets, bucket_number );
		return _hx_hash_apply_with_bucket( bucket, key, klen, apply_cb, thunk );
	}
}

int hx_free_hash ( hx_hash_t* h, void free_cb( void* key, void* value ) ) {
	int i, j;
	int size	= h->size;
	for (i = 0; i < size; i++) {
		hx_container_t* c	= hx_container_item( h->buckets, i );
		int csize	= hx_container_size( c );
		for (j = 0; j < csize; j++) {
			hx_hash_bucket_item_t* i	= hx_container_item( c, j );
			if (free_cb) {
				free_cb( i->key, i->value );
			}
			free( i->key );
			free( i );
		}
		hx_free_container( c );
	}
	free(h);
	return 0;
}

int hx_hash_debug ( hx_hash_t* h, void debug_cb( void* key, void* value ) ) {
	int i, j;
	int size	= h->size;
	fprintf( stderr, "hash with %d buckets:\n", size );
	for (i = 0; i < size; i++) {
		hx_container_t* c	= hx_container_item( h->buckets, i );
		int csize	= hx_container_size( c );
		fprintf( stderr, "- bucket %d has %d elements:\n", i, csize );
		for (j = 0; j < csize; j++) {
			hx_hash_bucket_item_t* i	= hx_container_item( c, j );
			debug_cb( i->key, i->value );
		}
	}
	return 0;
}

