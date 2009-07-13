#include <stdio.h>
#include "hexastore.h"
#include "mergejoin.h"
#include "node.h"
#include "bgp.h"
#include "async_des.h"

typedef struct {
	hx_hexastore* hx;
	hx_storage_manager* st;
	hx_index_iter* iter;
	hx_nodemap* map;
} send_args_t;

typedef struct {
	hx_hexastore* hx;
	hx_storage_manager* st;
} recv_args_t;
	
int send_handler(async_mpi_session* ses, void* args);
int recv_handler(async_mpi_session* ses, void* args);

int main ( int argc, char** argv ) {
	int mysize, myrank;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	hx_storage_manager* st	= hx_new_memory_storage_manager();
	hx_hexastore* hx		= hx_new_hexastore( st );
	
	hx_nodemap* map		= NULL;
	hx_index_iter* iter	= NULL;
	if (myrank == 0) {
		const char* filename	= argv[1];
		FILE* f	= fopen( filename, "r" );
		if (f == NULL) {
			perror( "Failed to open hexastore file for reading: " );
			MPI_Abort(MPI_COMM_WORLD, 1);
		}
		hx_hexastore* source_data	= hx_read( st, f, 0 );
		fprintf( stderr, "Finished loading hexastore...\n" );
		map			= hx_get_nodemap( source_data );
		hx_node* s	= hx_new_variable( hx );
		hx_node* p	= hx_new_variable( hx );
		hx_node* o	= hx_new_variable( hx );
		iter		= hx_get_statements( source_data, st, s, p, o, HX_SUBJECT );
	}
	
	send_args_t send_args;
	send_args.hx		= hx;
	send_args.st		= st;
	send_args.iter		= iter;
	send_args.map		= map;
	
	recv_args_t recv_args;
	recv_args.hx	= hx;
	recv_args.st	= st;
	
	int msg_size			= 3 * sizeof(hx_node_id);
	int num_sends			= (myrank == 0) ? 4 : 0;
	async_des_session* ses	= async_des_session_create(num_sends, &send_handler, &send_args, 20, &recv_handler, &recv_args, msg_size);
	
//	fprintf( stderr, "beginning async IO\n" );
	while(async_des(ses) == ASYNC_PENDING) {
		// continue until finished
	}
//	fprintf( stderr, "done with async IO\n" );
	
	if (myrank == 0) {
		hx_free_index_iter(iter);
	}
	
	int size	= (int) hx_triples_count( hx, st );
	fprintf( stderr, "node %d has %d triples\n", myrank, size );
	async_des_session_destroy(ses);
	
	
/**	
	hx_nodemap* map		= hx_get_nodemap( hx );
	
	hx_node* x			= hx_new_named_variable( hx, "x" );
	hx_node* y			= hx_new_named_variable( hx, "y" );
	hx_node* z			= hx_new_named_variable( hx, "z" );
	
	hx_node* type		= hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	hx_node* knows		= hx_new_node_resource("http://xmlns.com/foaf/0.1/knows");
	hx_node* name		= hx_new_node_resource("http://xmlns.com/foaf/0.1/name");
	hx_node* person		= hx_new_node_resource("http://xmlns.com/foaf/0.1/Person");
	
// 	hx_triple* triples[3];
// 	triples[0]	= hx_new_triple( x, knows, y );
// 	triples[1]	= hx_new_triple( y, name, z );
// 	triples[2]	= hx_new_triple( x, type, person );
// 	
// 	hx_bgp* b	= hx_new_bgp( 2, triples );
// 	hx_variablebindings_iter* iter	= hx_bgp_execute( b, hx, s );
// 	if (iter == NULL) {
// 		return 1;
// 	}
// 	
// 	int size		= hx_variablebindings_iter_size( iter );
// 	char** names	= hx_variablebindings_iter_names( iter );
// 	int xi, yi, zi;
// 	for (int i = 0; i < size; i++) {
// 		if (strcmp(names[i], "x") == 0) {
// 			xi	= i;
// 		} else if (strcmp(names[i], "y") == 0) {
// 			yi	= i;
// 		} else if (strcmp(names[i], "z") == 0) {
// 			zi	= i;
// 		}
// 	}
// 	
// 	while (!hx_variablebindings_iter_finished( iter )) {
// 		hx_variablebindings* b;
// 		hx_variablebindings_iter_current( iter, &b );
// 		hx_node_id xid	= hx_variablebindings_node_id_for_binding ( b, xi );
// 		hx_node_id yid	= hx_variablebindings_node_id_for_binding ( b, yi );
// 		hx_node_id zid	= hx_variablebindings_node_id_for_binding ( b, zi );
// 		hx_node* x		= hx_nodemap_get_node( map, xid );
// 		hx_node* y		= hx_nodemap_get_node( map, yid );
// 		hx_node* z		= hx_nodemap_get_node( map, zid );
// 		
// 		char *xs, *ys, *zs;
// 		hx_node_string( x, &xs );
// 		hx_node_string( y, &ys );
// 		hx_node_string( z, &zs );
// 		printf( "%s\t%s\t%s\n", xs, ys, zs );
// 		free( xs );
// 		free( ys );
// 		free( zs );
// 		
// 		hx_free_variablebindings( b, 1 );
// 		hx_variablebindings_iter_next( iter );
// 	}
// 	hx_free_variablebindings_iter( iter, 1 );
// 	for (int i = 0; i < 3; i++) {
// 		hx_free_triple( triples[i] );
// 	}
// 	hx_free_bgp( b );
	
	hx_free_node( x );
	hx_free_node( y );
	hx_free_node( z );
	hx_free_node( type );
	hx_free_node( person );
	hx_free_node( knows );
	hx_free_node( name );
	**/
	
	hx_free_hexastore( hx, st );
	hx_free_storage_manager( st );
	MPI_Finalize(); 
	return 0;
}


// int distribute_triples ( hx_hexastore* hx, hx_storage_manager* st, async_des_session* ses ) {
int send_handler(async_mpi_session* ses, void* args) {
	send_args_t* send_args	= (send_args_t*) args;
	hx_hexastore* hx		= send_args->hx;
	hx_storage_manager* st	= send_args->st;
	hx_index_iter* iter		= send_args->iter;
	
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	if (myrank != 0) {
		// only the head node can send triples to other nodes
		return 0;
	}
	
	hx_node_id nodes[3];
	while (!hx_index_iter_finished(iter)) {
		hx_index_iter_current( iter, &(nodes[0]), &(nodes[1]), &(nodes[2]) );
		int hash	= (nodes[0] ^ nodes[1] ^ nodes[2]) % mysize;
		
		char *string;
		hx_triple_id t;
		t.subject	= nodes[0];
		t.predicate	= nodes[1];
		t.object	= nodes[2];
		hx_triple_id_string( &t, send_args->map, &string );
//		fprintf( stderr, "%s hashed to node %d\n", string, hash );
		free(string);
		
		if (hash > 0) {
//			fprintf( stderr, "- sending triple to node %d\n", hash );
			int size			= 3 * sizeof( hx_node_id );
			hx_node_id* buffer	= malloc( size );
			if (buffer == NULL) {
				fprintf( stderr, "*** malloc failed in mpi.c:send_handler\n" );
			}
			memcpy( buffer, nodes, size );
			async_mpi_session_reset3(ses, buffer, size, hash, ses->flags | ASYNC_MPI_FLAG_FREE_BUF);
			hx_index_iter_next(iter);
			return 1;
		} else {
//			fprintf( stderr, "- adding triple to local triplestore\n" );
			// do something with the triple to keep it on this node
			hx_add_triple_id( hx, st, nodes[0], nodes[1], nodes[2] );
			hx_index_iter_next(iter);
		}
	}
	
	return 0;
}

// int receive_triples ( hx_hexastore* hx, hx_storage_manager* st, async_des_session* ses ) {
int recv_handler(async_mpi_session* ses, void* args) {
	recv_args_t* recv_args	= (recv_args_t*) args;
	hx_hexastore* hx		= recv_args->hx;
	hx_storage_manager* st	= recv_args->st;
	
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	hx_node_id* nodes	= (hx_node_id*) ses->buf;
	hx_add_triple_id( hx, st, nodes[0], nodes[1], nodes[2] );
//	fprintf( stderr, "node %d got triple: { %d, %d, %d }\n", myrank, (int) nodes[0], (int) nodes[1], (int) nodes[2] );
	return 1;
}

