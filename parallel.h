#ifndef _PARALLEL_H
#define _PARALLEL_H

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

#include "hexastore.h"
#include "bgp.h"
#include "variablebindings.h"
#include "async_des.h"

int hx_parallel_distribute_triples_from_hexastore ( int rank, hx_hexastore* source, hx_storage_manager* st, hx_hexastore* destination );
int hx_parallel_distribute_triples_from_iter ( int rank, hx_index_iter* source, hx_storage_manager* st, hx_hexastore* destination, hx_nodemap* map );

hx_variablebindings_iter* hx_parallel_distribute_variablebindings ( hx_storage_manager* st, hx_variablebindings_iter* iter );

int hx_parallel_collect_variablebindings ( int rank, hx_storage_manager* st, hx_variablebindings_iter* iter );
hx_variablebindings_iter* hx_parallel_new_rendezvousjoin_bgp ( hx_hexastore* hx, hx_storage_manager* st, hx_bgp* b );

#ifdef __cplusplus
}
#endif

#endif
