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

typedef uint64_t hx_hash_function ( const char* s );

#include "hexastore_types.h"

uint64_t hx_util_hash_string ( const char* s );
hx_container_t* hx_new_container ( char type, int size );
int hx_free_container ( hx_container_t* c );
void hx_container_push_item( hx_container_t* set, void* t );
void hx_container_unshift_item( hx_container_t* set, void* t );
int hx_container_size( hx_container_t* c );
void* hx_container_item ( hx_container_t* c, int i );

#ifdef __cplusplus
}
#endif

#endif
