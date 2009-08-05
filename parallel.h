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
#include "triple.h"
#include "genmap/avl_tree_map.h"
#include "genmap/iterator.h"
#include "async_des.h"
#include "async_mpi.h"
#include "genmap/map.h"

typedef struct {
	hx_storage_manager* storage;
	int root;
	const char* local_nodemap_file;
	const char* local_output_file;
	int join_iteration;
} hx_parallel_execution_context;

hx_parallel_execution_context* hx_parallel_new_execution_context ( hx_storage_manager* st, const char* path );
int hx_parallel_free_execution_context ( hx_parallel_execution_context* ctx );
int hx_parallel_distribute_triples_from_hexastore ( int rank, hx_hexastore* source, hx_storage_manager* st, hx_hexastore* destination );
int hx_parallel_distribute_triples_from_iter ( int rank, hx_index_iter* source, hx_storage_manager* st, hx_hexastore* destination, hx_nodemap* map );
int hx_parallel_distribute_triples_from_file ( hx_parallel_execution_context* ctx, const char* file, hx_hexastore* destination );
hx_node_id* hx_parallel_lookup_node_ids ( hx_parallel_execution_context* ctx, int count, hx_node** n );
hx_variablebindings_iter* hx_parallel_distribute_variablebindings ( hx_parallel_execution_context* ctx, hx_variablebindings_iter* iter, int shared_columns, char** shared_names );
int hx_parallel_collect_variablebindings ( int rank, hx_storage_manager* st, hx_variablebindings_iter* iter );
hx_variablebindings_iter* hx_parallel_new_rendezvousjoin_bgp ( hx_hexastore* hx, hx_storage_manager* st, hx_bgp* b );
char** hx_parallel_broadcast_variables(hx_parallel_execution_context* ctx, hx_node **nodes, size_t len, int* maxiv);
hx_variablebindings_iter* hx_parallel_rendezvousjoin( hx_parallel_execution_context* ctx, hx_hexastore* hx, int triple_count, hx_node_id* triple_nodes, char** variable_names, int maxiv );

hx_nodemap* hx_nodemap_read_mpi( hx_storage_manager* s, MPI_File f, int buffer );
int hx_nodemap_write_mpi ( hx_nodemap* t, MPI_File f );
char* hx_bgp_freeze_mpi( hx_parallel_execution_context* ctx, hx_bgp* b, int* len );
hx_node_id hx_nodemap_add_node_mpi ( hx_nodemap* m, hx_node* n );
int hx_parallel_nodemap_get_process_id ( hx_node_id id );

int hx_parallel_get_nodes ( hx_parallel_execution_context* ctx, hx_variablebindings_iter* iter, hx_variablebindings_nodes*** varbinds );

#ifdef __cplusplus
}
#endif

#endif
