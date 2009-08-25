#ifndef _UTIL_H
#define _UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

typedef struct {
	char type;
	int allocated;
	int count;
	void** items;
} hx_container_t;

typedef struct {
	void* key;
	size_t klen;
	void* value;
} hx_hash_bucket_item_t;

typedef struct {
	int size;
	hx_container_t* buckets;
} hx_hash_t;

typedef uint64_t hx_hash_function ( const char* s );

#include "hexastore_types.h"

uint64_t hx_util_hash_string ( const char* s );
uint64_t hx_util_hash_buffer ( const char* s, size_t len );

hx_container_t* hx_new_container ( char type, int size );
hx_container_t* hx_copy_container ( hx_container_t* c );
int hx_free_container ( hx_container_t* c );
int hx_container_set_item( hx_container_t* set, int index, void* t );
void hx_container_push_item( hx_container_t* set, void* t );
void hx_container_unshift_item( hx_container_t* set, void* t );
int hx_container_size( hx_container_t* c );
void* hx_container_item ( hx_container_t* c, int i );
char hx_container_type ( hx_container_t* c );

hx_hash_t* hx_new_hash ( int buckets );
int hx_hash_add ( hx_hash_t* hash, void* key, size_t klen, void* value );
int hx_hash_apply ( hx_hash_t* hash, void* key, size_t klen, int apply_cb( void* key, int klen, void* value, void* thunk ), void* thunk );
int hx_free_hash ( hx_hash_t* hash, void free_cb( void* key, size_t klen, void* value ) );
int hx_hash_debug ( hx_hash_t* h, void debug_cb( void* key, int klen, void* value ) );
void* hx_hash_get ( hx_hash_t* h, void* key, size_t klen );

#ifdef __cplusplus
}
#endif

#endif
