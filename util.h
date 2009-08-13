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

typedef uint64_t hx_hash_function ( const char* s );

#include "hexastore_types.h"

uint64_t hx_util_hash_string ( const char* s );

#ifdef __cplusplus
}
#endif

#endif
