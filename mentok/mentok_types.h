#ifndef _HEXASTORE_TYPES_H
#define _HEXASTORE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define PRIdHXID	PRId64
#define PRIxHXID	PRIx64

typedef int64_t hx_node_id;
typedef hx_node_id list_size_t;

typedef enum {
	HX_SUBJECT		= 0,
	HX_PREDICATE	= 1,
	HX_OBJECT		= 2,
	HX_GRAPH		= 3
} hx_node_position_t;

static char* HX_POSITION_NAMES[4]	= { "SUBJECT", "PREDICATE", "OBJECT", "GRAPH" };

#ifdef __cplusplus
}
#endif

#endif
