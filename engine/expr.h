#ifndef _ENGINE_EXPR_H
#define _ENGINE_EXPR_H

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

#include "algebra/expr.h"
#include "store/store.h"

hx_expr* hx_expr_substitute_variables ( hx_expr* orig, hx_variablebindings* b, hx_store* store );
int hx_expr_eval ( hx_expr* e, hx_variablebindings* b, hx_store* store, hx_node** result );

#ifdef __cplusplus
}
#endif

#endif
