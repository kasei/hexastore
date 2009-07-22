#include "parallel.h"
#include "triple.h"

typedef struct {
	int rank;
	hx_hexastore* hx;
	hx_storage_manager* st;
	hx_index_iter* iter;
	hx_nodemap* map;
} hx_parallel_send_triples_args_t;

typedef struct {
	hx_hexastore* hx;
	hx_storage_manager* st;
} hx_parallel_recv_triples_args_t;

typedef struct {
	int count;
	hx_storage_manager* st;
	hx_variablebindings_iter* iter;
} hx_parallel_send_vb_args_t;

typedef struct {
	void* padding;
} hx_parallel_recv_vb_args_t;

int _hx_parallel_send_triples_handler (async_mpi_session* ses, void* args);
int _hx_parallel_recv_triples_handler (async_mpi_session* ses, void* args);
int _hx_parallel_send_vb_handler (async_mpi_session* ses, void* args);
int _hx_parallel_recv_vb_handler (async_mpi_session* ses, void* args);

int hx_parallel_distribute_triples_from_hexastore( int rank, hx_hexastore* source, hx_storage_manager* st, hx_hexastore* destination ) {
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	hx_index_iter* iter	= NULL;
	hx_nodemap* map		= NULL;
	if (myrank == rank) {
		map			= hx_get_nodemap( source );
		hx_node* s	= hx_new_variable( source );
		hx_node* p	= hx_new_variable( source );
		hx_node* o	= hx_new_variable( source );
		iter		= hx_get_statements( source, st, s, p, o, HX_SUBJECT );
	}
	
	return hx_parallel_distribute_triples_from_iter( rank, iter, st, destination, map );
}

int hx_parallel_distribute_triples_from_iter ( int rank, hx_index_iter* source, hx_storage_manager* st, hx_hexastore* destination, hx_nodemap* map ) {
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	hx_parallel_send_triples_args_t send_args;
	send_args.rank			= rank;
	send_args.hx			= destination;
	send_args.st			= st;
	send_args.iter			= source;
	send_args.map			= map;
	
	hx_parallel_recv_triples_args_t recv_args;
	recv_args.hx			= destination;
	recv_args.st			= st;
	
	int msg_size			= 3 * sizeof(hx_node_id);
	int num_sends			= (myrank == rank) ? 4 : 0;
	async_des_session* ses	= async_des_session_create(num_sends, &_hx_parallel_send_triples_handler, &send_args, 20, &_hx_parallel_recv_triples_handler, &recv_args, msg_size);
	
	while (async_des(ses) == ASYNC_PENDING) {
		// continue until finished
	}
	
	if (myrank == rank) {
		hx_free_index_iter(source);
	}
	
	int size	= (int) hx_triples_count( destination, st );
// 	fprintf( stderr, "node %d has %d triples\n", myrank, size );
	async_des_session_destroy(ses);
	
	return 1;
}

int _hx_parallel_send_triples_handler(async_mpi_session* ses, void* args) {
	hx_parallel_send_triples_args_t* send_args	= (hx_parallel_send_triples_args_t*) args;
	hx_hexastore* hx		= send_args->hx;
	hx_storage_manager* st	= send_args->st;
	hx_index_iter* iter		= send_args->iter;
	
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	if (myrank != send_args->rank) {
		// only a head node can send triples to other nodes (one to many)
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
				fprintf( stderr, "*** malloc failed in mpi.c:_hx_parallel_send_triples_handler\n" );
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

int _hx_parallel_recv_triples_handler(async_mpi_session* ses, void* args) {
	hx_parallel_recv_triples_args_t* recv_args	= (hx_parallel_recv_triples_args_t*) args;
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

hx_variablebindings_iter* hx_parallel_distribute_variablebindings ( hx_storage_manager* st, hx_variablebindings_iter* iter ) {
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	hx_parallel_send_vb_args_t send_args;
	send_args.st			= st;
	send_args.iter			= iter;
	send_args.count			= 0;
		
	hx_parallel_recv_vb_args_t recv_args;
	
	async_des_session* ses	= async_des_session_create(4, &_hx_parallel_send_vb_handler, &send_args, 20, &_hx_parallel_recv_vb_handler, &recv_args, -1);
	
	int i	= 0;
	// while (async_des(ses) == ASYNC_PENDING) {
// 		fprintf( stderr, "loop iteration %d on node %d\n", i++, myrank );
// 		fprintf( stderr, "node %d: Message count %d\n", myrank, ses->msg_count );
		// continue until finished
	//}
	
	enum async_status astat;
	do {
			astat = async_des(ses);
	} while(astat == ASYNC_PENDING);
	
	if(astat == ASYNC_FAILURE) {
		fprintf(stderr, "ASYNC_FAILURE while distributing var bindings.\n");
	}
	
// 	fprintf( stderr, "%d: Message count %d\n", myrank, ses->msg_count );
// 	fprintf( stderr, "node %d finished async loop\n", myrank );
	
	if (iter != NULL) {
		hx_free_variablebindings_iter( iter, 0 );
	}
	
	async_des_session_destroy(ses);
	
	// XXX this should be the materialized iterator with the variablebindings collected in the recv handler:
	hx_variablebindings_iter* r	= hx_new_materialize_iter_with_data( 0, NULL, 0, NULL );	
	
	return r;
}

int _hx_parallel_send_vb_handler(async_mpi_session* ses, void* args) {
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
// 	fprintf( stderr, "node %d entered send handler\n", myrank );
	hx_parallel_send_vb_args_t* send_args	= (hx_parallel_send_vb_args_t*) args;
	hx_storage_manager* st			= send_args->st;
	hx_variablebindings_iter* iter	= send_args->iter;
	
	if (iter == NULL) {
		return 0;
	}
	
	while (!hx_variablebindings_iter_finished(iter)) {
// 		fprintf( stderr, "node %d iter loop\n", myrank );
		int len;
		hx_variablebindings* b;
		hx_variablebindings_iter_current( iter, &b );
		char* buffer	= hx_variablebindings_freeze( b, &len );
		int size		= hx_variablebindings_size( b );
		
		// hx_variablebindings_debug( b, NULL );

		char* string;
		hx_variablebindings_string( b, NULL, &string );
		uint64_t hash	= hx_triple_hash_string( string );
		int node		= hash % mysize;
// 		fprintf( stderr, "--- hash = %llu (%d)\n", hash, node );

// 		fprintf( stderr, "- sending variable bindings to node %d\n", node );
		send_args->count++;
		fprintf( stderr, "node %d sending variable bindings %s with length %d to node %d\n", myrank, string, len, node );
		async_mpi_session_reset3(ses, buffer, len, node, ses->flags | ASYNC_MPI_FLAG_FREE_BUF);
		free(string);
		hx_variablebindings_iter_next(iter);
		hx_free_variablebindings( b, 0 );
// 		fprintf( stderr, "node %d send handler sucessfully freed variablebindings\n", myrank );
		return 1;
	}
	
//	fprintf( stderr, "*** node %d sent %d variable bindings so far\n", myrank, send_args->count );
	return 0;
}

int _hx_parallel_recv_vb_handler(async_mpi_session* ses, void* args) {
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
// 	fprintf( stderr, "node %d entered recv handler\n", myrank );
	hx_parallel_recv_vb_args_t* recv_args	= (hx_parallel_recv_vb_args_t*) args;
	
	char* string;
	hx_variablebindings* b	= hx_variablebindings_thaw( ses->buf, ses->count );	
	hx_variablebindings_string( b, NULL, &string );
// 	fprintf( stderr, "node %d got variable bindings %s from %d\n", myrank, string, ses->peer );
	free(string);
	hx_free_variablebindings( b, 1 );
// 	fprintf( stderr, "node %d recv handler sucessfully freed variablebindings\n", myrank );
	return 1;
}
