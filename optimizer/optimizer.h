#ifndef _OPTIMIZER_H
#define _OPTIMIZER_H

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

#include "mentok_types.h"
#include "mentok.h"
#include "rdf/node.h"
#include "rdf/triple.h"
#include "misc/util.h"
#include "algebra/variablebindings.h"
#include "algebra/bgp.h"
#include "optimizer/plan.h"
#include "engine/variablebindings_iter_sorting.h"

hx_optimizer_plan* hx_optimizer_optimize_bgp ( hx_execution_context* ctx, hx_bgp* b );

// - accessPlans (get vb iter from a triple pattern, which index to use?)
hx_container_t* hx_optimizer_access_plans ( hx_execution_context* ctx, hx_triple* t );

// - joinPlans (which join algorithm to use? is sorting required?)
hx_container_t* hx_optimizer_join_plans ( hx_execution_context* ctx, hx_container_t* lhs, hx_container_t* rhs, int leftjoin );

// - finalizePlans (add projection, ordering, filters)
hx_container_t* hx_optimizer_finalize_plans ( hx_execution_context* ctx, hx_container_t* plans, hx_container_t* requested_order, hx_container_t* project_variables, hx_container_t* filters );

// - prunePlans
hx_container_t* hx_optimizer_prune_plans ( hx_execution_context* ctx, hx_container_t* plans );

#ifdef __cplusplus
}
#endif

#endif
