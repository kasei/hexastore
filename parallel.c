#include "parallel.h"
#include "nodemap.h"
#include "mergejoin.h"
#include "nestedloopjoin.h"

// #define HPGN_DEBUG(s, ...) fprintf(stderr, "%s:%u: "s"", __FILE__, __LINE__, __VA_ARGS__)
#define HPGN_DEBUG(s, ...)

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
	hx_parallel_execution_context* ctx;
	hx_variablebindings_iter* iter;
	int shared_columns;
	char** shared_names;
	hx_nodemap* map;
} hx_parallel_send_vb_args_t;

typedef struct {
	int allocated;
	int used;
	hx_variablebindings** buffer;
	hx_nodemap* map;
} hx_parallel_recv_vb_args_t;

typedef struct {
	hx_node_id gid;
	int pid;
} _hx_parallel_gid2node_lookup;

int _hx_parallel_compare_hx_node_id(const void *id1, const void *id2, void *p);
void _hx_parallel_destroy_gid2node_entry(void *k, void *v, void *p);
int _hx_parallel_send_gid2node_lookup(async_mpi_session* ses, void* p);
int _hx_parallel_recv_gid2node_lookup(async_mpi_session* ses, void* p);
int _hx_parallel_send_gid2node_answer(async_mpi_session* ses, void* p);
int _hx_parallel_recv_gid2node_answer(async_mpi_session* ses, void* p);

int _hx_parallel_send_triples_handler (async_mpi_session* ses, void* args);
int _hx_parallel_recv_triples_handler (async_mpi_session* ses, void* args);
int _hx_parallel_send_vb_handler (async_mpi_session* ses, void* args);
int _hx_parallel_recv_vb_handler (async_mpi_session* ses, void* args);



hx_parallel_execution_context* hx_parallel_new_execution_context ( hx_storage_manager* st, const char* path, char* job_id ) {
	int myrank;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	hx_parallel_execution_context* ctx	= (hx_parallel_execution_context*) calloc( 1, sizeof( hx_parallel_execution_context ) );
	ctx->storage				= st;
	ctx->root					= 0;
	
	ctx->job_id					= malloc( strlen(job_id) + 1 );
	sprintf( (char*) ctx->job_id, job_id );
	
	static const char* map_template	= "%s/rendezvous-map.%s.%d";
	char *mapfile		= malloc(strlen(path) + strlen(map_template) + strlen(job_id) + 6 + 1);
	sprintf(mapfile, map_template, path, job_id, myrank);
	ctx->local_nodemap_file	= mapfile;
	
	static const char* out_template	= "%s/rendezvous-out.%s.%d";
	char *outfile		= malloc(strlen(path) + strlen(out_template) + strlen(job_id) + 6 + 1);
	sprintf(outfile, out_template, path, job_id, myrank);
	ctx->local_output_file	= outfile;
	
	return ctx;
}

int hx_parallel_free_parallel_execution_context ( hx_parallel_execution_context* ctx ) {
	free( (char*) ctx->local_nodemap_file );
	free( (char*) ctx->local_output_file );
	free( (char*) ctx->job_id );
	free( ctx );
	return 0;
}

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

int hx_parallel_distribute_triples_from_file ( hx_parallel_execution_context* ctx, const char* file, hx_hexastore* destination ) {
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	const char* mapfile		= ctx->local_nodemap_file;
	hx_storage_manager *st	= ctx->storage;
//	if (!mpi_rdfio_readids(file, 4800, &destination, &st, MPI_COMM_WORLD)) {
	if(!mpi_rdfio_readnt(file, mapfile, 1048576, &destination, &st, MPI_COMM_WORLD)) {
		fprintf(stderr, "%s:%u:%i: Error; read failed.\n", __FILE__, __LINE__, rank);
		MPI_Abort(MPI_COMM_WORLD, 127);
	}
	return 0;
}

hx_node_id* hx_parallel_lookup_node_ids (hx_parallel_execution_context* ctx, int count, hx_node** n) {
	hx_node_id *ids = malloc(count*sizeof(hx_node_id));
	hx_node_id *allids = malloc(count*sizeof(hx_node_id));
	if(ids == NULL || allids == NULL) {
		fprintf(stderr, "%s:%u: Error; cannot allocate %u bytes for arrays of hx_node_id.\n", __FILE__, __LINE__, 2*count*sizeof(hx_node_id));
		return NULL;
	}
	MPI_File file;
	MPI_Info info;
	MPI_Info_create(&info);
	MPI_File_open(MPI_COMM_SELF, (char*) ctx->local_nodemap_file, MPI_MODE_RDONLY|MPI_MODE_SEQUENTIAL, info, &file);
	hx_nodemap *nodemap	= hx_nodemap_read_mpi(ctx->storage, file, 1);
	MPI_File_close(&file);
	MPI_Info_free(&info);
	int i;
	for(i = 0; i < count; i++) {
		if (hx_node_is_variable(n[i])) {
			ids[i]	= hx_node_iv(n[i]);
		} else {
			ids[i] = hx_nodemap_get_node_id(nodemap, n[i]);
		}
	}
	MPI_Allreduce(ids, allids, count*sizeof(hx_node_id), MPI_BYTE, MPI_BOR, MPI_COMM_WORLD);
	free(ids);
	hx_free_nodemap( nodemap );
	return allids;
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

hx_variablebindings_iter* hx_parallel_distribute_variablebindings ( hx_parallel_execution_context* ctx, hx_variablebindings_iter* iter, int shared_columns, char** shared_names, hx_nodemap* source, hx_nodemap* destination ) {
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	hx_storage_manager* st		= ctx->storage;
	
	hx_parallel_send_vb_args_t send_args;
	send_args.ctx				= ctx;
	send_args.iter				= iter;
	send_args.count				= 0;
	send_args.shared_columns	= shared_columns;
	send_args.shared_names		= shared_names;
	send_args.map				= source;
		
	hx_parallel_recv_vb_args_t recv_args;
	recv_args.allocated			= 0;
	recv_args.used				= 0;
	recv_args.buffer			= NULL;
	recv_args.map				= destination;
	
	async_des_session* ses		= async_des_session_create(4, &_hx_parallel_send_vb_handler, &send_args, 10, &_hx_parallel_recv_vb_handler, &recv_args, -1);
	
//	int i	= 0;
	// while (async_des(ses) == ASYNC_PENDING) {
// 		fprintf( stderr, "loop iteration %d on node %d\n", i++, myrank );
// 		fprintf( stderr, "node %d: Message count %d\n", myrank, ses->msg_count );
		// continue until finished
	//}
	
	enum async_status astat;
	do {
		astat = async_des(ses);
	} while (astat == ASYNC_PENDING);
	
	if (astat == ASYNC_FAILURE) {
		fprintf(stderr, "ASYNC_FAILURE while distributing var bindings.\n");
	}
	async_des_session_destroy(ses);
	
// 	fprintf( stderr, "%d: Message count %d\n", myrank, ses->msg_count );
// 	fprintf( stderr, "node %d finished async loop\n", myrank );
	
	int size		= hx_variablebindings_iter_size(iter);
	if (size == 0) {
		fprintf( stderr, "%d>>> (empty)\n", myrank );
		return iter;
	} else {
		char** names	= hx_variablebindings_iter_names(iter);
		char** newnames	= (char**) calloc( size, sizeof( char* ) );
		int i;
		for (i = 0; i < size; i++) {
			char* name		= names[i];
			int len			= strlen(name);
			char* newname	= (char*) calloc( len + 1, sizeof( char ) );
			strcpy( newname, name );
			newnames[i]		= newname;
		}
		
		// fprintf( stderr, "node %d creating materialized iterator with size %d, length %d\n", myrank, size, recv_args.used );
		hx_variablebindings_iter* r	= hx_new_materialize_iter_with_data( size, newnames, recv_args.used, recv_args.buffer );	
		
		for (i = 0; i < size; i++) {
			free( newnames[i] );
		}
		free(newnames);
		
		if (iter != NULL) {
			hx_free_variablebindings_iter( iter );
		}
		
		return r;
	}
}

int _hx_parallel_send_vb_handler(async_mpi_session* ses, void* args) {
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
// 	fprintf( stderr, "node %d entered send handler\n", myrank );
	hx_parallel_send_vb_args_t* send_args	= (hx_parallel_send_vb_args_t*) args;
	hx_parallel_execution_context* ctx	= send_args->ctx;
	hx_storage_manager* st				= ctx->storage;
	hx_variablebindings_iter* iter		= send_args->iter;
	
	if (iter == NULL) {
		return 0;
	}
	
	
	while (!hx_variablebindings_iter_finished(iter)) {
// 		fprintf( stderr, "node %d iter loop\n", myrank );
		int len;
		hx_variablebindings* b;
		hx_variablebindings_iter_current( iter, &b );
		char* buffer	= hx_variablebindings_freeze( b, send_args->map, &len );
		int size		= hx_variablebindings_size( b );
//		hx_variablebindings_debug( b, NULL );
		
		uint64_t hash	= 0;
		
		int i;
		for (i = 0; i < send_args->shared_columns; i++) {
//			fprintf( stderr, "using name '%s'\n", send_args->shared_names[i] );
			hx_node_id id	= hx_variablebindings_node_id_for_binding_name( b, send_args->shared_names[i] );
//			fprintf( stderr, "using node %lld in send handler\n", id );
			hx_node* n		= hx_nodemap_get_node( send_args->map, id );
			char* string;
			hx_node_string( n, &string );
			hash			+= hx_triple_hash_string( string );
			free(string);
		}
		
		int node		= hash % mysize;
		
// 		fprintf( stderr, "--- hash = %"PRIuHXID" (%d)\n", hash, node );
// 		fprintf( stderr, "- sending variable bindings to node %d\n", node );
		send_args->count++;
		
// 		if (ctx->join_iteration > 1) {
// 			char* string;
// 			hx_variablebindings_string( b, NULL, &string );
// 			fprintf( stderr, "\t{J%d} node %d sending variable bindings %s with length %d to node %d\n", ctx->join_iteration, myrank, string, len, node );
// 			free(string);
// 		}
		
		async_mpi_session_reset3(ses, buffer, len, node, ses->flags | ASYNC_MPI_FLAG_FREE_BUF);
		hx_variablebindings_iter_next(iter);
		hx_free_variablebindings(b);
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
	
	hx_variablebindings* b	= hx_variablebindings_thaw( ses->buf, ses->count, recv_args->map );	
	if (0) {
		char* string;
		hx_variablebindings_string( b, NULL, &string );
		fprintf( stderr, "node %d got variable bindings %s from %d\n", myrank, string, ses->peer );
		free(string);
	}
	
	if (recv_args->used == recv_args->allocated) {
		int newsize	= (recv_args->allocated == 0) ? 8 : (recv_args->allocated * 1.5);
		hx_variablebindings** oldbuffer	= recv_args->buffer;
		hx_variablebindings** newbuffer	= (hx_variablebindings**) calloc( newsize, sizeof( hx_variablebindings* ) );
		int i;
		for (i = 0; i < recv_args->allocated; i++) {
			newbuffer[i]	= oldbuffer[i];
		}
		
		recv_args->buffer		= newbuffer;
		recv_args->allocated	= newsize;
		
		if (oldbuffer != NULL) {
			free(oldbuffer);
		}
	}
	
	recv_args->buffer[ recv_args->used++ ]	= b;
	
//	hx_free_variablebindings(b);
// 	fprintf( stderr, "node %d recv handler sucessfully freed variablebindings\n", myrank );
	return 1;
}

char** hx_parallel_broadcast_variables(hx_parallel_execution_context* ctx, hx_node **nodes, size_t len, int* maxiv) {
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

// 	fprintf( stderr, "entering hx_parallel_broadcast_variables on process %d\n", rank );
	
	size_t size_of_size_t = sizeof(size_t);
	size_t size_of_int = sizeof(int);
	size_t bufsize;
	size_t i;
	unsigned char* buffer;
	
	if (rank == ctx->root) {
		bufsize = (len+1)*(size_of_size_t + size_of_int);
		for(i = 0; i < len; i++) {
			char* name;
			hx_node_variable_name(nodes[i], &name);
			bufsize += strlen(name);
			free(name);
		}
// 		fprintf( stderr, "broadcasting %d from process %d\n", (int) bufsize, rank );
	}
	
	MPI_Bcast(&bufsize, size_of_size_t, MPI_BYTE, 0, MPI_COMM_WORLD);
// 	fprintf( stderr, "done broadcasting on process %d\n", rank );

	if(bufsize <= 0) {
		return NULL;
	}

	buffer = malloc(bufsize);
	if(buffer == NULL) {
		fprintf(stderr, "%s:%u:%d: Error in broadcast_nodes; cannot allocate %lu bytes for buffer.\n", __FILE__, __LINE__, rank, bufsize);
		bufsize = 0;
	}

	if (rank == ctx->root) {
		unsigned char* b = buffer;
		memcpy(b, &len, size_of_size_t);
		b = &b[size_of_size_t + size_of_int];
		int min_iv = 0;
		for(i = 0; i < len; i++) {
			int iv = hx_node_iv(nodes[i]);
			if(iv < min_iv) {
				min_iv = iv;
			}
			memcpy(b, &iv, size_of_int);
			b = &b[size_of_int];
			char* name;
			hx_node_variable_name(nodes[i], &name);
			size_t namelen = strlen(name);
			memcpy(b, &namelen, size_of_size_t);
			b = &b[size_of_size_t];
			memcpy(b, name, namelen);
			if(i < len - 1) {
				b = &b[namelen];
			}
			free(name);
		}
		min_iv *= -1;
		memcpy(&buffer[size_of_size_t], &min_iv, size_of_int);
	}

	MPI_Bcast(buffer, bufsize, MPI_BYTE, 0, MPI_COMM_WORLD);
	
	unsigned char* b = buffer;
	memcpy(&len, b, size_of_size_t);
	b = &b[size_of_size_t];

	int min_iv;
	memcpy(&min_iv, b, size_of_int);
	b = &b[size_of_int];

	*maxiv = min_iv;
	char** map = calloc(min_iv+1, sizeof(char*));
	if(map == NULL) {
		fprintf(stderr, "%s:%u:%d: Error in broadcast_nodes; cannot allocate %lu bytes for map.\n", __FILE__, __LINE__, rank, min_iv*sizeof(char*));
		return NULL;
	}

	for(i = 0; i < len; i++) {
		size_t namelen;
		int iv;
		memcpy(&iv, b, size_of_int);
		b = &b[size_of_int];
		iv *= -1;
		memcpy(&namelen, b, size_of_size_t);
		b = &b[size_of_size_t];
		if(namelen > 0) {
			map[iv] = malloc(namelen+1);
			if(map[iv] == NULL) {
				fprintf(stderr, "%s:%u:%d: Error in broadcast_nodes; cannot allocate %lu bytes for node name.\n", __FILE__, __LINE__, rank, namelen + 1);
				return NULL;
			}
			memcpy(map[iv], b, namelen);
			map[iv][namelen] = '\0';
			if(i < len - 1) {
				b = &b[namelen];
			}
		}
	}
	free(buffer);
	
	MPI_Barrier(MPI_COMM_WORLD);

// 	fprintf( stderr, "leaving hx_parallel_broadcast_variables\n" );
	return map;
}

char* hx_bgp_freeze_mpi( hx_parallel_execution_context* ctx, hx_bgp* b, int* len ) {
	int size		= hx_bgp_size(b);
	int _len		= (1 + (3 * size)) * sizeof( hx_node_id );
	*len			= _len;
	hx_node_id* buf	= (hx_node_id*) calloc( 1, _len );
	buf[0]			= size;				// the first slot is the size of the bgp
	hx_node_id* ptr	= &( buf[1] );	// the rest is a set of node IDs, in triple groups
	hx_node** nodes	= (hx_node**) calloc( 3 * size, sizeof( hx_node* ) );
	
	int i;
	for (i = 0; i < size; i++) {
		hx_triple* t	= b->triples[i];
		nodes[ (3*i) + 0 ]	= t->subject;
		nodes[ (3*i) + 1 ]	= t->predicate;
		nodes[ (3*i) + 2 ]	= t->object;
	}
	
	hx_node_id* nodeids	= hx_parallel_lookup_node_ids( ctx, size*3, nodes );
// 	fprintf( stderr, "BGP nodes:\n" );
// 	for (i = 0; i < size*3; i++) {
// 		fprintf( stderr, "- %d\n", (int) nodeids[i] );
// 	}
	memcpy( ptr, nodeids, (3*size*sizeof(hx_node_id)) );
	free( nodes );
	free( nodeids );
	return (char*) buf;
}

hx_node_id hx_nodemap_add_node_mpi ( hx_nodemap* m, hx_node* n ) {
	hx_node* node	= hx_node_copy( n );
	hx_nodemap_item i;
	i.node	= node;
	hx_nodemap_item* item	= (hx_nodemap_item*) avl_find( m->node2id, &i );
	if (item == NULL) {
		if (0) {
			char* nodestr;
			hx_node_string( node, &nodestr );
// 			fprintf( stderr, "nodemap adding key '%s'\n", nodestr );
			free(nodestr);
		}
		
		item	= (hx_nodemap_item*) calloc( 1, sizeof( hx_nodemap_item ) );
		item->node	= node;
		
		
		int myrank;
		MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
		hx_node_id id	= (m->next_id++) | ((uint64_t) myrank << 48);
		item->id		= id;
		
		avl_insert( m->node2id, item );
		avl_insert( m->id2node, item );
// 		fprintf( stderr, "*** new item %d -> %p\n", (int) item->id, (void*) item->node );
		
		if (0) {
			hx_node_id id	= hx_nodemap_get_node_id( m, node );
//			fprintf( stderr, "*** After adding: %"PRIuHXID"\n", (unsigned long long) id );
		}
		
// 		fprintf( stderr, "adding MPI node %"PRIx64"\n", item->id );
		return item->id;
	} else {
		hx_free_node( node );
		return item->id;
	}
}

int hx_nodemap_write_mpi ( hx_nodemap* m, MPI_File f ) {
	int flag = 0;
	MPI_Status status;
	MPIO_Request request;
	
	size_t used	= avl_count( m->id2node );

	MPI_File_write_shared(f, "M", 1, MPI_BYTE, &status);
	MPI_File_write_shared(f, &used, sizeof(size_t), MPI_BYTE, &status);
	MPI_File_write_shared(f, &(m->next_id), sizeof( hx_node_id ), MPI_BYTE, &status);
	
	struct avl_traverser iter;
	avl_t_init( &iter, m->id2node );
	hx_nodemap_item* item;
	
	int counter	= 0;
	while ((item = (hx_nodemap_item*) avl_t_next( &iter )) != NULL) {
		MPI_File_write_shared(f, &( item->id ), sizeof( hx_node_id ), MPI_BYTE, &status);
		hx_node_write_mpi( item->node, f );
		counter++;
	}
	
	return 0;
}

hx_nodemap* hx_nodemap_read_mpi( hx_storage_manager* s, MPI_File f, int buffer ) {
	size_t used, read;
	hx_node_id next_id;
	
	char c;
	MPI_Status status;
	
	MPI_File_read_shared(f, &c, 1, MPI_BYTE, &status);
	
	if (c != 'M') {
		fprintf( stderr, "*** Bad header cookie trying to read nodemap from file.\n" );
		return NULL;
	}
	
	hx_nodemap* m	= hx_new_nodemap();

	MPI_File_read_shared(f, &used, sizeof( size_t ), MPI_BYTE, &status);
	MPI_File_read_shared(f, &next_id, sizeof( hx_node_id ), MPI_BYTE, &status);

	m->next_id	= next_id;
	int i;
	for (i = 0; i < used; i++) {
		hx_nodemap_item* item	= (hx_nodemap_item*) malloc( sizeof( hx_nodemap_item ) );
		if (item == NULL) {
			fprintf( stderr, "*** malloc failed in hx_nodemap_read\n" );
		}
		
		MPI_File_read_shared(f, &( item->id ), sizeof( hx_node_id ), MPI_BYTE, &status);
		item->node	= hx_node_read_mpi( f, 0 );
		avl_insert( m->node2id, item );
		avl_insert( m->id2node, item );
	}
	return m;
}

int hx_node_write_mpi ( hx_node* n, MPI_File f ) {
	int flag = 0;
	MPI_Status status;
	MPIO_Request request;
	
	if (n->type == '?') {
//		fprintf( stderr, "*** Cannot write variable nodes to a file.\n" );
		return 1;
	}
	
	MPI_File_write_shared(f, "N", 1, MPI_BYTE, &status);
	MPI_File_write_shared(f, &(n->type), 1, MPI_BYTE, &status);
	
	size_t len	= (size_t) strlen( n->value );
	MPI_File_write_shared(f, &len, sizeof(size_t), MPI_BYTE, &status);
	MPI_File_write_shared(f, n->value, strlen(n->value), MPI_BYTE, &status);
	
	if (hx_node_is_literal( n )) {
		if (hx_node_is_lang_literal( n )) {
			hx_node_lang_literal* l	= (hx_node_lang_literal*) n;
			size_t len	= strlen( l->lang );
			MPI_File_write_shared(f, &len, sizeof(size_t), MPI_BYTE, &status);
			MPI_File_write_shared(f, l->lang, strlen(l->lang), MPI_BYTE, &status);
		}
		if (hx_node_is_dt_literal( n )) {
			hx_node_dt_literal* d	= (hx_node_dt_literal*) n;
			size_t len	= strlen( d->dt );
			MPI_File_write_shared(f, &len, sizeof(size_t), MPI_BYTE, &status);
			MPI_File_write_shared(f, d->dt, strlen(d->dt), MPI_BYTE, &status);
		}
	}
	return 0;
}

hx_node* hx_node_read_mpi( MPI_File f, int buffer ) {
	char c;
	MPI_Status status;
	size_t used, read;
	
	MPI_File_read_shared(f, &c, 1, MPI_BYTE, &status);
	if (c != 'N') {
		fprintf( stderr, "*** Bad header cookie ('%c') trying to read node from MPI file.\n", c );
		return NULL;
	}
	
	char* value;
	char* extra	= NULL;
	hx_node* node;

	MPI_File_read_shared(f, &c, 1, MPI_BYTE, &status);
	switch (c) {
		case 'R':
			MPI_File_read_shared(f, &used, sizeof( size_t ), MPI_BYTE, &status);
			value	= (char*) calloc( 1, used + 1 );
			MPI_File_read_shared(f, value, used, MPI_BYTE, &status);
			node	= hx_new_node_resource( value );
			free( value );
			return node;
		case 'B':
			MPI_File_read_shared(f, &used, sizeof( size_t ), MPI_BYTE, &status);
			value	= (char*) calloc( 1, used + 1 );
			MPI_File_read_shared(f, value, used, MPI_BYTE, &status);
			node	= hx_new_node_blank( value );
			free( value );
			return node;
		case 'L':
		case 'G':
		case 'D':
			MPI_File_read_shared(f, &used, sizeof( size_t ), MPI_BYTE, &status);
			value	= (char*) calloc( 1, used + 1 );
			MPI_File_read_shared(f, value, used, MPI_BYTE, &status);
			if (c == 'G' || c == 'D') {
				MPI_File_read_shared(f, &used, sizeof( size_t ), MPI_BYTE, &status);
				extra	= (char*) calloc( 1, used + 1 );
				MPI_File_read_shared(f, extra, used, MPI_BYTE, &status);
			}
			if (c == 'G') {
				node	= (hx_node*) hx_new_node_lang_literal( value, extra );
			} else if (c == 'D') {
				node	= (hx_node*) hx_new_node_dt_literal( value, extra );
			} else {
				node	= hx_new_node_literal( value );
			}
			free( value );
			if (extra != NULL)
				free( extra );
			return node;
		default:
			fprintf( stderr, "*** Bad node type '%c' trying to read node from MPI file.\n", (char) c );
			return NULL;
	};
	
}

int hx_parallel_nodemap_get_process_id ( hx_node_id id ) {
	return (int) (id >> 48);
}

int hx_parallel_get_nodes(hx_parallel_execution_context* ctx, hx_variablebindings_iter* iter, hx_variablebindings_nodes*** varbinds) {
	int rank, commsize;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); 
	MPI_Comm_size(MPI_COMM_WORLD, &commsize);
	HPGN_DEBUG("%i: Starting hx_parallel_get_nodes\n", rank);
	
	hx_variablebindings *bindings;
	
	hx_variablebindings_iter* miter = hx_new_materialize_iter(iter);
	
	int rankandsize[2];
	rankandsize[0] = rank; 
	rankandsize[1] = commsize;
	
	map_t gid2node;
	gid2node = avl_tree_map_create(&_hx_parallel_compare_hx_node_id, NULL, &_hx_parallel_destroy_gid2node_entry, NULL, rankandsize);
	
	HPGN_DEBUG("%i: Reading nodemap from file %s.\n", rank, (char*)ctx->local_nodemap_file);
	
	MPI_File file;
	MPI_Info info;
	MPI_Info_create(&info);
	MPI_File_open(MPI_COMM_SELF, (char*)ctx->local_nodemap_file, MPI_MODE_RDONLY | MPI_MODE_SEQUENTIAL, info, &file);

	hx_nodemap *nodemap = hx_nodemap_read_mpi(ctx->storage, file, 4096);

	MPI_File_close(&file);
	MPI_Info_free(&info);

	HPGN_DEBUG("%i: Loading gid2node map with gid keys.\n", rank);

	int numvbs;
	for(numvbs = 0; !hx_variablebindings_iter_finished(miter); numvbs++) {
		hx_variablebindings_iter_current(miter, &bindings);

		int size = hx_variablebindings_size(bindings);
		int i;
		for(i = 0; i < size; i++) {
			hx_node_id *id = malloc(sizeof(hx_node_id));
			if(id == NULL) {
				fprintf(stderr, "%s:%u: Unable to allocate an hx_node_id.\n", __FILE__, __LINE__);
				return -1;
			}
			*id = hx_variablebindings_node_id_for_binding(bindings, i);
			map_put(gid2node, id, NULL);
		}

		hx_variablebindings_iter_next(miter);
	}

	hx_materialize_reset_iter(miter);

	iterator_t giditer = map_key_iterator(gid2node);
	if(giditer == NULL) {
		fprintf(stderr, "%s:%u: Unable to allocate iterator over gid2node.\n", __FILE__, __LINE__);
		return -1;
	}

	size_t lookupsize = 0;
	size_t lookupmaxsize = 256;
	_hx_parallel_gid2node_lookup *lookups = malloc(lookupmaxsize*sizeof(_hx_parallel_gid2node_lookup));
	if(lookups == NULL) {
		fprintf(stderr, "%s:%u: Unable to allocate %u bytes for lookup table.\n", __FILE__, __LINE__, lookupmaxsize*sizeof(_hx_parallel_gid2node_lookup));
		return -1;
	}

	MPI_Barrier(MPI_COMM_WORLD);

	void* pack[1];
	pack[0] = giditer;

	void* pack2[4];
	pack2[0] = &lookupsize;
	pack2[1] = &lookupmaxsize;
	pack2[2] = &lookups;
	pack2[3] = nodemap;

	async_des_session *des = async_des_session_create(2, &_hx_parallel_send_gid2node_lookup, &pack, 4, &_hx_parallel_recv_gid2node_lookup, &pack2, sizeof(hx_node_id));
	if(des == NULL) {
		fprintf(stderr, "%s:%u: Error; cannot allocate des session.\n", __FILE__, __LINE__);
		return -1;
	}

	enum async_status astat;
	do {
		astat = async_des(des);
	} while(astat == ASYNC_PENDING);

	if(astat == ASYNC_FAILURE) {
		fprintf(stderr, "%s:%u: Error; des session failed.\n", __FILE__, __LINE__);
		async_des_session_destroy(des);
		return -1;
	}

	async_des_session_destroy(des);

	iterator_destroy(giditer);

	HPGN_DEBUG("%i: Ended first des session.\n", rank);

	lookups = realloc(lookups, lookupsize*sizeof(_hx_parallel_gid2node_lookup));
	lookupmaxsize = lookupsize;
	lookupsize = 0;
	pack2[2] = &lookups;

	pack[0] = gid2node;

	des = async_des_session_create(2, &_hx_parallel_send_gid2node_answer, &pack2, 4, &_hx_parallel_recv_gid2node_answer, &pack, 0);
	if(des == NULL) {
                fprintf(stderr, "%s:%u: Error; cannot allocate second des session.\n", __FILE__, __LINE__);
                return -1;
        }

	do {
                astat = async_des(des);
        } while(astat == ASYNC_PENDING);

        if(astat == ASYNC_FAILURE) {
                fprintf(stderr, "%s:%u: Error; second des session failed.\n", __FILE__, __LINE__);
                async_des_session_destroy(des);
                return -1;
        }

	async_des_session_destroy(des);

	hx_free_nodemap(nodemap);
	free(lookups);
	
	//int numvbs = hx_variablebindings_iter_size(miter);
	*varbinds = malloc(numvbs*sizeof(hx_variablebindings_nodes*));
	if(*varbinds == NULL) {
		fprintf(stderr, "%s:%u: Error; cannot allocate %u bytes for new variable bindings nodes.\n", __FILE__, __LINE__, numvbs*sizeof(hx_variablebindings_nodes*));
		return -1;
	}

	int cnt;
	for(cnt = 0; !hx_variablebindings_iter_finished(miter); cnt++) {
                hx_variablebindings_iter_current(miter, &bindings);

                int size = hx_variablebindings_size(bindings);

		char **names = malloc(size*sizeof(char*));
		hx_node **nodes = malloc(size*sizeof(hx_node*));
		if(names == NULL || nodes == NULL) {
			fprintf(stderr, "%s:%u: Error; cannot allocate %u bytes for names and nodes pointers.\n", __FILE__, __LINE__, size*(sizeof(char*) + sizeof(hx_node*)));
			return -1;
		}

                int i;
                for(i = 0; i < size; i++) {
			char *nametocopy = hx_variablebindings_name_for_binding(bindings, i);
			size_t namelen = strlen(nametocopy);
			names[i] = malloc(namelen + 1);
			if(names[i] == NULL) {
				fprintf(stderr, "%s:%u: Error; cannot allocate %u bytes for variable binding name.\n", __FILE__, __LINE__, namelen + 1);
				return -1;
			}
			strcpy(names[i], nametocopy);
			hx_node_id hxni = hx_variablebindings_node_id_for_binding(bindings, i);
			nodes[i] = hx_node_copy((hx_node*)map_get(gid2node, &hxni));
			if(nodes[i] == NULL) {
				fprintf(stderr, "%s:%u: Logical error; nodes[i] shouldn't be NULL.\n", __FILE__, __LINE__);
			}
                }

		(*varbinds)[cnt] = hx_new_variablebindings_nodes(size, names, nodes);
		if((*varbinds)[cnt] == NULL) {
			fprintf(stderr, "%s:%u: Error; cannot instantiate variablebindings_nodes.\n", __FILE__, __LINE__);
			return -1;
		}

                hx_variablebindings_iter_next(miter);
        }

	hx_free_variablebindings_iter(miter);
	map_destroy(gid2node);

	HPGN_DEBUG("Returning %i successfully.\n", numvbs);

	return numvbs;
}

// literally cut and pasted from mpi_rdfio.c where the
// function is named _mpi_rdfio_to_hx_node_p.  Consider
// putting in a common place?  Maybe in node.c and declared
// in node.h?
hx_node* _hx_parallel_to_hx_node_p(char* ntnode) {
	switch(ntnode[0]) {
		case '<': {
			ntnode[strlen(ntnode)-1] = '\0';
			return hx_new_node_resource(&ntnode[1]);
		}
		case '_': {
			return hx_new_node_blank(&ntnode[2]);
		}
		case '"': {
			int max_idx = strlen(ntnode) - 1;
			switch(ntnode[max_idx]) {
				case '"': {
					ntnode[max_idx] = '\0';
					return hx_new_node_literal(&ntnode[1]);
				}
				case '>': {
					ntnode[max_idx] = '\0';
					char* last_quote_p = strrchr(ntnode, '"');
					if(last_quote_p == NULL) {
						fprintf(stderr, "%s:%u: Error in _mpi_rdfio_to_hx_node_p; expected typed literal, but found %s\n", __FILE__, __LINE__, ntnode);
						return NULL;
					}
					last_quote_p[0] = '\0';
					return (hx_node*)hx_new_node_dt_literal(&ntnode[1], &last_quote_p[4]);
				}
				default: {
					char* last_quote_p = strrchr(ntnode, '"');
					if(last_quote_p == NULL) {
						fprintf(stderr, "%s:%u: Error in _mpi_rdfio_to_hx_node_p; expected literal with language tag, but found %s\n", __FILE__, __LINE__, ntnode);
						return NULL;
					}
					last_quote_p[0] = '\0';
					return (hx_node*)hx_new_node_lang_literal(&ntnode[1], &last_quote_p[2]);
				}
			}
		}
		default: {
			fprintf(stderr, "%s:%u: Error in _mpi_rdfio_to_hx_node_p; invalid N-triples node %s\n", __FILE__, __LINE__, ntnode);
			return NULL;
		}
	}
}


int _hx_parallel_compare_hx_node_id(const void *id1, const void *id2, void *p) {
	int *ras = (int*)p;
	int rank = ras[0];
	int size = ras[1];
	hx_node_id *hid1 = (hx_node_id*)id1;
	hx_node_id *hid2 = (hx_node_id*)id2;
	hx_node_id pid1 = (hx_parallel_nodemap_get_process_id(*hid1) + rank) % size;
	hx_node_id pid2 = (hx_parallel_nodemap_get_process_id(*hid2) + rank) % size;
	hx_node_id nid1 = (0x0000ffffffffffffLL & *hid1) | (pid1 << 48);
	hx_node_id nid2 = (0x0000ffffffffffffLL & *hid2) | (pid2 << 48);
	
	return nid1 < nid2 ? -1 : (nid1 > nid2 ? 1 : 0);
}

void _hx_parallel_destroy_gid2node_entry(void *k, void *v, void *p) {
	if(k != NULL) {
		HPGN_DEBUG("Freeing key %p.\n", k);
		free(k);
	}
	if(v != NULL) {
		HPGN_DEBUG("Freeing value %p.\n", v);
		hx_free_node((hx_node*)v);
	}
}

int _hx_parallel_send_gid2node_lookup(async_mpi_session* ses, void* p) {
	void **pack = (void**)p;
	iterator_t giditer = (iterator_t)pack[0];
	if(!iterator_has_next(giditer)) {
		return 0;
	}

	// Maybe change to batching, but for now, take advantage of
	// fixed size messages.

	hx_node_id* id = (hx_node_id*) iterator_next(giditer);
	memcpy(ses->buf, id, sizeof(hx_node_id));
	int peer = hx_parallel_nodemap_get_process_id(*id);
	HPGN_DEBUG("Sending to %i [gid=%"PRIuHXID"]\n", peer, *id);

	async_mpi_session_reset3(ses, ses->buf, sizeof(hx_node_id), peer, ses->flags);
	HPGN_DEBUG("Returning from sending lookup.\n", NULL);
	return 1;
}

int _hx_parallel_recv_gid2node_lookup(async_mpi_session* ses, void* p) {
	HPGN_DEBUG("Receiving lookup.\n", NULL);
	void **pack = (void**)p;
	size_t *lookupsize = (size_t*)pack[0];
	size_t *lookupmaxsize = (size_t*)pack[1];
	_hx_parallel_gid2node_lookup **lookups = (_hx_parallel_gid2node_lookup**)pack[2];

	HPGN_DEBUG("pack=%p, *lookupsize=%u, *lookupmaxsize=%u, *lookups=%p\n", pack, *lookupsize, *lookupmaxsize, *lookups);
	
	if(*lookupsize >= *lookupmaxsize) {
		*lookupmaxsize *= 2;
		_hx_parallel_gid2node_lookup *tmp = realloc(*lookups, (*lookupmaxsize)*sizeof(_hx_parallel_gid2node_lookup));
		if(tmp == NULL) {
			fprintf(stderr, "%s:%u; Error; cannot allocate %u bytes for resizing lookups table.\n", __FILE__, __LINE__, (*lookupmaxsize)*sizeof(_hx_parallel_gid2node_lookup));
			return -1;
		}
		*lookups = tmp;
	}
	_hx_parallel_gid2node_lookup *lookup = &(*lookups)[(*lookupsize)++];
	lookup->pid = ses->peer;
	memcpy(&(lookup->gid), ses->buf, sizeof(hx_node_id));

	HPGN_DEBUG("Received [gid=%"PRIuHXID"] from pid=%i\n", lookup->gid, lookup->pid);

	return 1;
}

int _hx_parallel_send_gid2node_answer(async_mpi_session* ses, void* p) {
	void **pack = (void**)p;
	size_t *cnt = (size_t*)pack[0];
	size_t *max = (size_t*)pack[1];
	_hx_parallel_gid2node_lookup **lookups = (_hx_parallel_gid2node_lookup**)pack[2];
	hx_nodemap *gid2node = (hx_nodemap*)pack[3];
	
	if(*cnt >= *max) {
		return 0;
	}
	_hx_parallel_gid2node_lookup *lookup = &((*lookups)[(*cnt)++]);
	hx_node *node = hx_nodemap_get_node(gid2node, lookup->gid);
	if(node == NULL) {
		fprintf(stderr, "%s:%u: Logical error; node should not be NULL.\n", __FILE__, __LINE__);
		return -1;
	}
	char *str;
	int sz = hx_node_string(node, &str);
	str = realloc(str, sz + sizeof(hx_node_id));
	if(str == NULL) {
		fprintf(stderr, "%s:%u: Error; cannot reallocate string to %u bytes to fit gid.\n", __FILE__, __LINE__, sz + sizeof(hx_node_id));
		return -1;
	}
	memmove(&str[sizeof(hx_node_id)], str, sz);
	memcpy(str, &(lookup->gid), sizeof(hx_node_id));
	async_mpi_session_reset3(ses, str, sz + sizeof(hx_node_id), lookup->pid, ses->flags | ASYNC_MPI_FLAG_FREE_BUF);
	return 1;
}

int _hx_parallel_recv_gid2node_answer(async_mpi_session* ses, void* p) {
	void **pack = (void**)p;
	map_t gid2node = (map_t)pack[0];

	hx_node_id *gid = malloc(sizeof(hx_node_id));
	if(gid == NULL) {
		fprintf(stderr, "%s:%u: Error; cannot allocate hx_node_it.\n", __FILE__, __LINE__);
		return -1;
	}
	memcpy(gid, ses->buf, sizeof(hx_node_id));
	hx_node *node = _hx_parallel_to_hx_node_p(&((char*)ses->buf)[sizeof(hx_node_id)]);
	map_put(gid2node, gid, node);
	return 1;
}

hx_variablebindings_iter* _hx_parallel_variablebindings_iter_for_triple ( hx_parallel_execution_context* ctx, hx_hexastore* hx, hx_triple* t ) {
	int myrank; MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	hx_index_iter* titer	= hx_get_statements( hx, ctx->storage, t->subject, t->predicate, t->object, HX_OBJECT );
	if (titer == NULL) {
		char* names[3];
		int i;
		int j	= 0;
		for (i = 0; i < 3; i++) {
			hx_node* n	= hx_triple_node(t,i);
			if (hx_node_is_variable(n)) {
				hx_node_variable_name(n, &(names[j++]));
			}
		}
		hx_variablebindings_iter* iter	= hx_variablebindings_new_empty_iter_with_names(j, names);
		for (i = 0; i < j; i++) {
			free(names[i]);
		}
		return iter;
	} else {
		char *sname, *pname, *oname;
		hx_node_variable_name( t->subject, &sname );
		hx_node_variable_name( t->predicate, &pname );
		hx_node_variable_name( t->object, &oname );
		hx_variablebindings_iter* iter	= hx_new_iter_variablebindings( titer, ctx->storage, sname, pname, oname );
		free(sname);
		free(pname);
		free(oname);
		return iter;
	}
	
// 	hx_index* index;
// 	hx_node* index_ordered[3];
// 	int order_position	= HX_OBJECT;
// 	hx_get_ordered_index( hx, ctx->storage, t->subject, t->predicate, t->object, order_position, &index, index_ordered, NULL );
// 	hx_index_iter* titer	= hx_index_new_iter1( index, ctx->storage, t->subject, t->predicate, t->object );
// 	
// 	char* names[3];
// 	int i;
// 	for (i = 0; i < 3; i++) {
// 		char* n	= node_names[ (3*triple) + i ];
// 		if (n != NULL) {
// 			int len	= strlen(n);
// 			names[i]	= (char*) calloc( len + 1, sizeof( char ) );
// 			strcpy( names[i], n );
// 		} else {
// 			names[i]	= NULL;
// 		}
// 	}
// 	
// 	hx_variablebindings_iter* iter	= hx_new_iter_variablebindings( titer, ctx->storage, names[0], names[1], names[2] );
// 	free( names[0] );
// 	free( names[1] );
// 	free( names[2] );
// 	
// 	return iter;

}

int _hx_parallel_variablebindings_iter_shared_columns( hx_triple* t, char** node_names, char** variable_names, int maxiv, hx_variablebindings_iter* lhs, char*** columns ) {
	int myrank; MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	int* shared	= (int*) calloc( maxiv+1, sizeof( int ) );
	
// 	fprintf( stderr, "names for rhs:\n" );
	int i;
	for (i = 0; i < 3; i++) {
		hx_node* n	= hx_triple_node( t, i );
		if (hx_node_is_variable(n)) {
			int index	= -1 * hx_node_iv(n);
//			fprintf( stderr, "variable: -%d\n", index );
			shared[ index ]	= 1;
		}
	}
	
	int lhs_size		= hx_variablebindings_iter_size( lhs );
//  	fprintf( stderr, "%d: (%d total) names for lhs:\n", myrank, lhs_size );
	char** lhs_names	= hx_variablebindings_iter_names( lhs );
	for (i = 0; i < lhs_size; i++) {
		char* lhs_name	= lhs_names[i];
// 		fprintf( stderr, "- %s\n", lhs_name );
		int j;
		for (j = 0; j <= maxiv; j++) {
			if (variable_names[j] != NULL) {
//				fprintf( stderr, "\tchecking with existing RHS variable %s\n", variable_names[j] );
				if (strcmp(variable_names[j], lhs_name) == 0) {
//					fprintf( stderr, "shared variable: -%d\n", j );
					shared[j]++;
				}
			}
		}
	}
	
	int shared_count	= 0;
	for (i = 0; i <= maxiv; i++) {
		if (shared[i] == 2) {
			shared_count++;
//			fprintf( stderr, "(%d) variable is shared: %s\n", myrank, variable_names[i] );
		}
	}
	
// 	fprintf( stderr, "%d: shared_count = %d\n", myrank, shared_count );
	
	int j		= 0;
	char** c	= (char**) calloc( shared_count, sizeof( char* ) );
// 	fprintf( stderr, "%d: allocated column array %p\n", myrank, (void*) c );
	
	for (i = 0; i <= maxiv; i++) {
		if (shared[i] == 2) {
			c[ j++ ]	= variable_names[i];
		}
	}
	
	*columns	= c;
//	fprintf( stderr, "(%d) shared_count = %d\n", myrank, shared_count );
	return shared_count;
}

hx_variablebindings_iter* hx_parallel_rendezvousjoin( hx_parallel_execution_context* ctx, hx_hexastore* hx, hx_bgp* b, hx_nodemap** results_map ) {
	int myrank; MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	int triple_count	= hx_bgp_size(b);
	int node_count		= triple_count * 3;
	// create node_names[] that maps node positions in the BGP (blocks of 3 nodes per triple) to the name of the variable node in that position (NULL if not a variable)
	char** node_names	= (char**) calloc( node_count, sizeof( char* ) );
	int i, j;
	int maxiv	= 0;
	for (j = 0; j < triple_count; j++) {
		hx_triple* t	= hx_bgp_triple( b, j );
		for (i = 0; i < 3; i++) {
			hx_node* node	= hx_triple_node( t, i );
			if (hx_node_is_variable(node)) {
				hx_node_variable_name(node, &(node_names[(3*j)+i]));
				int v	= hx_node_iv(node);
				if ((-1*v) > maxiv) {
					maxiv	= -1 * v;
				}
			} else {
				node_names[(3*j)+i]	= NULL;
			}
		}
	}
	
	char** variable_names	= (char**) calloc( maxiv, sizeof( char* ) );
	hx_node** variables;
	int var_count	= hx_bgp_variables ( b, &variables );
	for (i = 0; i < var_count; i++) {
		hx_node* v	= variables[i];
		int id		= hx_node_iv(v);
		hx_node_variable_name(v, &(variable_names[-1 * id]));
	}

	hx_nodemap* triples_map				= hx_get_nodemap( hx );
	hx_nodemap* lhs_map					= triples_map;
	hx_triple* t0						= hx_bgp_triple( b, 0 );
	hx_variablebindings_iter* lhs		= _hx_parallel_variablebindings_iter_for_triple( ctx, hx, t0 );
	
	for (j = 1; j < triple_count; j++) {
		hx_triple* t	= hx_bgp_triple( b, j );
		ctx->join_iteration	= j;
		hx_nodemap* vb_map	= hx_new_nodemap();
		
		/**********************************************************************/
// 		MPI_Barrier(MPI_COMM_WORLD);
// 		if (myrank == 0) {
// 			fprintf( stderr, "Performing join #%d\n", j );
// 		}
// 		MPI_Barrier(MPI_COMM_WORLD);
		/**********************************************************************/
		
		char** columns						= NULL;
// 		fprintf(stderr, "%i: _hx_parallel_variablebinding_iter_for_triple\n", myrank);
		hx_variablebindings_iter* rhs		= _hx_parallel_variablebindings_iter_for_triple( ctx, hx, t );
// 		fprintf(stderr, "%i: _hx_parallel_variablebinding_iter_shared_columns2\n", myrank);
		int shared_count					= _hx_parallel_variablebindings_iter_shared_columns( t, node_names, variable_names, maxiv, lhs, &columns );
// 		fprintf( stderr, "%d: columns = %p\n", myrank, (void*) columns );
		
		
		/**********************************************************************/
// 		MPI_Barrier(MPI_COMM_WORLD);
// 		if (myrank == 0) {
// 			int i;
// 			for (i = 0; i < shared_count; i++) {
// 				fprintf( stderr, "- shared column: '%s'\n", columns[i] );
// 			}
// 			fprintf( stderr, "\n" );
// 		}
// 		MPI_Barrier(MPI_COMM_WORLD);
		/**********************************************************************/
		
// 		fprintf(stderr, "%i: _hx_parallelhx_parallel_distribute_variablebindings(1)\n", myrank);
		hx_variablebindings_iter* lhsr	= hx_parallel_distribute_variablebindings( ctx, lhs, shared_count, columns, lhs_map, vb_map );
// 		fprintf(stderr, "%i: _hx_parallelhx_parallel_distribute_variablebindings(2)\n", myrank);
		hx_variablebindings_iter* rhsr	= hx_parallel_distribute_variablebindings( ctx, rhs, shared_count, columns, triples_map, vb_map );
		
		if (shared_count > 0) {
			lhs		= hx_new_mergejoin_iter( lhsr, rhsr );
		} else {
			lhs		= hx_new_nestedloopjoin_iter( lhsr, rhsr );
		}
		
		if (lhs_map != triples_map) {
			hx_free_nodemap(lhs_map);
		}
		lhs_map	= vb_map;
		
		free(columns);
	}
	
	char** all_names	= (char**) calloc( node_count, sizeof( char* ) );
	int all_count	= 0;
	for (i = 0; i <= maxiv; i++) {
		if (variable_names[i] != NULL) {
			all_names[ all_count++ ]	= variable_names[i];
		}
	}
	
	if (lhs == NULL) {
		lhs	= hx_variablebindings_new_empty_iter();
	}
	
	*results_map	= hx_new_nodemap();
	hx_variablebindings_iter* results	= hx_parallel_distribute_variablebindings( ctx, lhs, all_count, all_names, lhs_map, *results_map );
	hx_free_nodemap( lhs_map );
	
// 	fprintf( stderr, "node %d returning results iterator %p\n", myrank, (void*) lhsr );
	return results;
}

