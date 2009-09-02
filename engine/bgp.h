#ifndef _ENGINE_BGP_H
#define _ENGINE_BGP_H

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

#include "hexastore_types.h"
#include "hexastore.h"
#include "algebra/bgp.h"

hx_variablebindings_iter* hx_bgp_execute ( hx_bgp*, hx_hexastore* );

#ifdef __cplusplus
}
#endif

#endif
