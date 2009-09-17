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
#include <mpi.h>

#include "mentok/mentok.h"
#include "mentok/algebra/bgp.h"
#include "mentok/algebra/variablebindings.h"
#include "mentok/rdf/triple.h"
#include "genmap/avl_tree_map.h"
#include "genmap/iterator.h"
#include "mentok/parallel/async_des.h"
#include "mentok/parallel/async_mpi.h"
#include "genmap/map.h"

typedef enum {
	HX_JOIN_LHS,
	HX_JOIN_RHS
} hx_join_side;

typedef struct {
	void* world;
	hx_model* hx;
	int64_t nestedloopjoin_penalty;
	int64_t hashjoin_penalty;
	int64_t unsorted_mergejoin_penalty;
	hx_variablebindings_iter* (*bgp_exec_func)( void*, hx_model*, void* thunk );
	hx_node* (*lookup_node)( void*, hx_node_id );
	void* bgp_exec_func_thunk;
	
	int root;
	const char* local_nodemap_file;
	const char* local_output_file;
	const char* job_id;
	int join_iteration;
	hx_nodemap** local_nodemap;
} hx_parallel_execution_context;

hx_parallel_execution_context* hx_parallel_new_execution_context ( void* world, hx_model* hx, const char* path, char* job_id );
int hx_parallel_free_parallel_execution_context ( hx_parallel_execution_context* ctx );
int hx_parallel_distribute_triples_from_file ( hx_parallel_execution_context* ctx, const char* file, hx_model* destination );
hx_node_id* hx_parallel_lookup_node_ids ( hx_parallel_execution_context* ctx, int count, hx_node** n );
hx_variablebindings_iter* hx_parallel_distribute_variablebindings ( hx_parallel_execution_context* ctx, hx_variablebindings_iter* iter, int shared_columns, char** shared_names, hx_nodemap* source, hx_nodemap* destination, hx_join_side side );
int hx_parallel_collect_variablebindings ( int rank, hx_variablebindings_iter* iter );
hx_variablebindings_iter* hx_parallel_new_rendezvousjoin_bgp ( hx_model* hx, hx_bgp* b );
char** hx_parallel_broadcast_variables(hx_parallel_execution_context* ctx, hx_node **nodes, size_t len, int* maxiv);
hx_variablebindings_iter* hx_parallel_rendezvousjoin( hx_parallel_execution_context* ctx, hx_bgp* b, hx_nodemap** results_map );

hx_nodemap* hx_nodemap_read_mpi( MPI_File f, int buffer );
int hx_nodemap_write_mpi ( hx_nodemap* t, MPI_File f );
int hx_node_write_mpi( hx_node* n, MPI_File f );
hx_node* hx_node_read_mpi( MPI_File f, int buffer );

char* hx_bgp_freeze_mpi( hx_parallel_execution_context* ctx, hx_bgp* b, int* len );
hx_node_id hx_nodemap_add_node_mpi ( hx_nodemap* m, hx_node* n );
int hx_parallel_nodemap_get_process_id ( hx_node_id id );

int hx_parallel_get_nodes ( hx_parallel_execution_context* ctx, hx_variablebindings_iter* iter, hx_variablebindings_nodes*** varbinds );
int hx_bgp_reorder_mpi ( hx_bgp* b, hx_model* hx );

#ifdef __cplusplus
}
#endif

#endif
