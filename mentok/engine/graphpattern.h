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

#include "mentok/mentok_types.h"
#include "mentok/mentok.h"
#include "mentok/algebra/graphpattern.h"
#include "mentok/engine/mergejoin.h"
#include "mentok/algebra/variablebindings.h"

hx_variablebindings_iter* hx_graphpattern_execute ( hx_execution_context*, hx_graphpattern* );
hx_graphpattern* hx_graphpattern_substitute_variables ( hx_graphpattern* pat, hx_variablebindings* b, hx_store* store );

#ifdef __cplusplus
}
#endif

#endif
