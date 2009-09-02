#ifndef _ENGINE_GRAPHPATTERN_H
#define _ENGINE_GRAPHPATTERN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
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
#include "algebra/graphpattern.h"
#include "engine/mergejoin.h"
#include "algebra/variablebindings.h"

hx_variablebindings_iter* hx_graphpattern_execute ( hx_graphpattern*, hx_hexastore* );

#ifdef __cplusplus
}
#endif

#endif
