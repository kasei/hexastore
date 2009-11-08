#ifndef _IDMAP_H
#define _IDMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include "mentok/mentok_types.h"
#include "mentok/misc/avl.h"
#include "mentok/misc/util.h"

typedef struct {
	uint64_t next_id;
	struct avl_table* id2string;
	struct avl_table* string2id;
} hx_idmap;

typedef struct {
	uint64_t id;
	char* string;
} hx_idmap_item;

hx_idmap* hx_new_idmap( void );
int hx_free_idmap ( hx_idmap* m );

uint64_t hx_idmap_add_string ( hx_idmap* m, char* n );
int hx_idmap_remove_id ( hx_idmap* m, uint64_t id );
int hx_idmap_remove_string ( hx_idmap* m, char* n );
uint64_t hx_idmap_get_id ( hx_idmap* m, char* n );
char* hx_idmap_get_string ( hx_idmap* m, uint64_t id );
int hx_idmap_debug ( hx_idmap* map );

hx_container_t* hx_idmap_strings ( hx_idmap* map );

#ifdef __cplusplus
}
#endif

#endif
