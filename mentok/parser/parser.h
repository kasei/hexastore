#ifndef _PARSER_H
#define _PARSER_H

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
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <raptor.h>

#include "mentok/mentok_types.h"
#include "mentok/mentok.h"
#include "mentok/rdf/node.h"

static int TRIPLES_BATCH_SIZE	= 5000;
typedef void (*hx_parser_logger)( uint64_t count, void* thunk );

typedef struct {
	uint64_t next_bnode;
	struct timeval tv;
	int count;
	uint64_t total;
	hx_model* hx;
	hx_triple* triples;
	hx_parser_logger logger;
	struct avl_table* bnode_map;
	void* thunk;
} hx_parser;

hx_parser* hx_new_parser ( void );
int hx_parser_set_logger( hx_parser* p, hx_parser_logger l, void* thunk );

uint64_t hx_parser_parse_file_into_model ( hx_parser* p, hx_model* hx, const char* filename );
int hx_parser_parse_string_into_model ( hx_parser* parser, hx_model* hx, const char* string, const char* base, char* parser_name );
int hx_free_parser ( hx_parser* p );

#ifdef __cplusplus
}
#endif

#endif
