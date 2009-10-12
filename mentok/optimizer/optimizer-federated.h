#ifndef _OPTIMIZER_FEDERATED_H
#define _OPTIMIZER_FEDERATED_H

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

#include "mentok/optimizer/optimizer.h"

// - accessPlans (get vb iter from a triple pattern, which index to use?)
hx_container_t* hx_optimizer_federated_access_plans ( hx_execution_context* ctx, hx_triple* t );
hx_container_t* hx_optimizer_federated_join_plans ( hx_execution_context* ctx, hx_container_t* lhs, hx_container_t* rhs, int leftjoin );

#ifdef __cplusplus
}
#endif

#endif
