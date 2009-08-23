#ifndef _PLAN_ACCESS_H
#define _PLAN_ACCESS_H

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
#include "engine/variablebindings_iter.h"
#include "rdf/triple.h"
#include "rdf/node.h"

typedef enum {
	HX_OPTIMIZER_PLAN_INDEX	= 0,	// access to triple pattern matching via hx_index
	HX_OPTIMIZER_ACCESS_PLAN_LAST	= HX_OPTIMIZER_PLAN_INDEX
} hx_optimizer_plan_type;

typedef struct {
	hx_optimizer_plan_type type;
	void* source;
	hx_triple* triple;
	int order_count;
	hx_variablebindings_iter_sorting** order;
} hx_optimizer_plan;

hx_optimizer_plan* hx_new_optimizer_access_plan ( hx_optimizer_plan_type type, void* source, hx_triple* t, int order_count, hx_variablebindings_iter_sorting** order );
int hx_free_optimizer_access_plan ( hx_optimizer_plan* plan );

#ifdef __cplusplus
}
#endif

#endif
