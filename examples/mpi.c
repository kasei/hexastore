#include <stdio.h>
#include "hexastore.h"
#include "mergejoin.h"
#include "node.h"
#include "bgp.h"
#include "parallel.h"
#include "materialize.h"
#include "nestedloopjoin.h"

#include "timing_choices.h"
#define TIMING_CPU_FREQUENCY 2600000000
#define TIMING_USE TIMING_RDTSC
#include "timing.h"

int DEBUG_NODE	= -1;

extern hx_bgp* parse_bgp_query_string ( char* );
hx_hexastore* distribute_triples_from_file ( hx_hexastore* hx, hx_storage_manager* st, const char* filename );
int distribute_bgp ( hx_parallel_execution_context* ctx, hx_bgp** b, hx_node_id** triple_nodes, char*** variable_names, int* maxiv );

int main ( int argc, char** argv ) {
	MPI_Init(&argc, &argv);

	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	hx_storage_manager* st	= hx_new_memory_storage_manager();
	hx_hexastore* hx		= hx_new_hexastore( st );
	
	
	hx_parallel_execution_context* ctx	= hx_parallel_new_execution_context( st, "/tmp" );
//	hx_parallel_execution_context* ctx	= hx_parallel_new_execution_context( st, "/scratch" );
	
	TIME_T(load_start, load_end);
	TIME_T(exec_start, exec_end);
	
	
if (0) { // XXX
    int i = 0;
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    printf("\n\n*** PID %d on %s ready for attach\n\n", getpid(), hostname);
    fflush(stdout);
    sleep(10);
}

	
	
	load_start	= TIME();
	hx_parallel_distribute_triples_from_file( ctx, argv[1], hx );
	load_end	= TIME();

	if (myrank == 0) {
		fprintf( stdout, "loading took %" PRINTTIME " seconds\n", DIFFTIME(load_end, load_start) );
	}
	
//	hx_bgp* b					= parse_bgp_query_string( "PREFIX foaf: <http://xmlns.com/foaf/0.1/> { ?p foaf:name ?name; foaf:nick ?nick . ?d foaf:maker ?p }" );
	hx_bgp* b					= parse_bgp_query_string( "{ ?s <http://simile.mit.edu/2006/01/ontologies/mods3#point> \"end\" . ?s <http://simile.mit.edu/2006/01/ontologies/mods3#encoding> ?bo . ?s <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> ?co . }" );
	
	int triple_count			= hx_bgp_size(b);
	int node_count				= triple_count * 3;
	
	int maxiv;
	char** variable_names;
	hx_node_id* triple_nodes;
	distribute_bgp( ctx, &b, &triple_nodes, &variable_names, &maxiv );

	if (myrank == DEBUG_NODE) {
		int i;
		for (i = 0; i <= maxiv; i++) {
			fprintf( stderr, "variable map slot %d: '%s'\n", i, variable_names[i] );
		}
	}

	exec_start	= TIME();
	hx_variablebindings_nodes** nodes;
	hx_variablebindings_iter* iter	= hx_parallel_rendezvousjoin( ctx, hx, triple_count, triple_nodes, variable_names, maxiv );
	int results	= hx_parallel_get_nodes( ctx, iter, &nodes );
	MPI_File file;
	MPI_Info info;
	MPI_Status status;
	MPI_Info_create(&info);
	MPI_File_open(MPI_COMM_SELF, (char*) ctx->local_output_file, MPI_MODE_WRONLY | MPI_MODE_CREATE, info, &file);
	//FILE* f	= fopen( ctx->local_output_file, "w" );
	int i;
	for (i = 0; i < results; i++) {
		hx_variablebindings_nodes* b	= nodes[i];
		char* string;
		hx_variablebindings_nodes_string( b, &string );
		char chars[1024];
		//fprintf(f, "process %d: binding %p %s\n", myrank, (void*) b, string);
		sprintf(chars, "process %d: binding %p %s\n", myrank, (void*) b, string);
		MPI_File_write_shared(file, chars, strlen(chars), MPI_BYTE, &status);
		free(string);
		hx_free_variablebindings_nodes(b);
	}
	//fclose( f );
	MPI_File_close(&file);
	MPI_Info_free(&info);
	
	exec_end	= TIME();
	
	fprintf( stderr, "process %d: %d results\n", myrank, results );
	if (myrank == 0) {
		fprintf( stdout, "execution took %" PRINTTIME " seconds\n", DIFFTIME(exec_end, exec_start) );
	}
	
	for (i = 0; i <= maxiv; i++) {
		if (variable_names[i] != NULL) {
			free(variable_names[i]);
		}
	}
	free(variable_names);
	hx_free_hexastore( hx, st );
	hx_free_storage_manager( st );
	MPI_Finalize(); 
	return 0;
}

int distribute_bgp ( hx_parallel_execution_context* ctx, hx_bgp** b, hx_node_id** triple_nodes, char*** variable_names, int* maxiv ) {
	int root					= ctx->root;
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	int triple_count			= hx_bgp_size(*b);
	int node_count				= triple_count * 3;
	hx_node_id* _triple_nodes	= (hx_node_id*) calloc( node_count, sizeof( hx_node_id ) );
	hx_node** variables			= NULL;
	int variable_count			= 0;

	
	// parse triples from ~SPARQL syntax, and place the node IDs into the _triple_nodes[] buffer
	int len;
// 	fprintf( stderr, "freezing bgp...\n" );
		char* bfreeze		= hx_bgp_freeze_mpi( ctx, *b, &len );
// 	fprintf( stderr, "done freezing...\n" );
	
	hx_node_id* _ptr	= (hx_node_id*) bfreeze;
	hx_node_id* ptr		= &( _ptr[1] );
	
	int i;
	for (i = 0; i < node_count; i++) {
		_triple_nodes[i]	= ptr[i];
	}
	
	int* is_variable	= (int*) calloc( node_count, sizeof( int ) );
	for (i = 0; i < node_count; i++) {
		if (_triple_nodes[i] < 0) {
			is_variable[i]	= 1;
		}
	}
	for (i = 0; i < node_count; i++) {
		if (is_variable[i]) {
			variable_count++;
		}
	}
	variables	= (hx_node**) calloc( variable_count, sizeof( hx_node* ) );
	int j	= 0;
	for (i = 0; i < node_count; i++) {
		hx_triple* t	= hx_bgp_triple( *b, (i/3) );
		char* string;
		hx_triple_string ( t, &string );

		if (is_variable[i]) {
			hx_node* node	= NULL;
			switch (i % 3) {
				case 0:
// 						fprintf( stderr, "variable is subject of %s\n", string );
					node	= t->subject;
					break;
				case 1:
// 						fprintf( stderr, "variable is predicate of %s\n", string );
					node	= t->predicate;
					break;
				case 2:
// 						fprintf( stderr, "variable is object of %s\n", string );
					node	= t->object;
					break;
				default:
					fprintf( stderr, "*** uh oh.\n" );
					exit(1);
			};
			free(string);
			variables[ j++ ]	= node;
		}
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	
	// broadcast the mapping from variable number to variable name to all processes
	*variable_names	= hx_parallel_broadcast_variables(ctx, variables, variable_count, maxiv);	
	
	// now broadcast the _triple_nodes[] buffer to all nodes, so everyone can execute the query
	MPI_Bcast(_triple_nodes,sizeof(hx_node_id)*node_count,MPI_BYTE,0,MPI_COMM_WORLD);
	*triple_nodes	= _triple_nodes;
	
	return 0;
}
