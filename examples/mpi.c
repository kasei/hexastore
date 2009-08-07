#include <stdio.h>
#include <tcutil.h>
#include <tcbdb.h>

#include "hexastore.h"
#include "mergejoin.h"
#include "node.h"
#include "bgp.h"
#include "parallel.h"
#include "materialize.h"

#include "timing_choices.h"
#define TIMING_CPU_FREQUENCY 2600000000
#define TIMING_USE TIMING_RDTSC
#include "timing.h"

int DEBUG_NODE	= -1;

extern hx_bgp* parse_bgp_query_string ( char* );
hx_hexastore* distribute_triples_from_file ( hx_hexastore* hx, hx_storage_manager* st, const char* filename );
int distribute_bgp ( hx_parallel_execution_context* ctx, TCBDB* nodemap, hx_bgp** b, hx_node_id** triple_nodes, char*** variable_names, int* maxiv );

int main ( int argc, char** argv ) {
	int i;
	MPI_Init(&argc, &argv);

	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	hx_storage_manager* st	= hx_new_memory_storage_manager();
	hx_hexastore* hx		= hx_new_hexastore( st );
	
	int ecode;
	const char* data_filename	= argv[1];
	const char* map_filename	= argv[2];
	TCBDB* nodemap				= tcbdbnew();
	if(!tcbdbopen(nodemap, map_filename, HDBOREADER)) {
		ecode = tcbdbecode(nodemap);
		fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
	}
	
	char* job				= (argc > 3) ? argv[3] : "";
	hx_parallel_execution_context* ctx	= hx_parallel_new_execution_context( st, "/tmp", job );
// 	hx_parallel_execution_context* ctx	= hx_parallel_new_execution_context( st, "/gpfs/large/DSSW/rendezvous", job );
	
	TIME_T(load_start, load_end);
	TIME_T(exec_start, exec_end);
	
	
	if (0) { // XXX
		char hostname[256];
		gethostname(hostname, sizeof(hostname));
		printf("\n\n*** PID %d on %s ready for attach\n\n", getpid(), hostname);
		fflush(stdout);
		sleep(5);
	}

	load_start	= TIME();
	hx_parallel_distribute_triples_from_file( ctx, data_filename, hx );
	load_end	= TIME();

	if (myrank == 0) {
		fprintf( stdout, "loading took %" PRINTTIME " seconds\n", DIFFTIME(load_end, load_start) );
	}
	
//	hx_bgp* b					= parse_bgp_query_string( "PREFIX foaf: <http://xmlns.com/foaf/0.1/> { ?p foaf:name ?name; foaf:nick ?nick . ?d foaf:maker ?p }" );
//	hx_bgp* b					= parse_bgp_query_string( "{ ?s <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://simile.mit.edu/2006/01/ontologies/mods3#Text> . ?s <http://simile.mit.edu/2006/01/ontologies/mods3#language> <http://simile.mit.edu/2006/01/language/iso639-2b/fre> }" );
//	hx_bgp* b					= parse_bgp_query_string( "{ ?s a <http://simile.mit.edu/2006/01/ontologies/mods3#Record> . ?s <http://simile.mit.edu/2006/01/ontologies/mods3#origin> <info:marcorg/MYG> . }" );
	hx_bgp* b					= parse_bgp_query_string( "PREFIX : <http://www.lehigh.edu/~zhp2/2004/0401/univ-bench.owl#> { ?x a :GraduateStudent . ?x :takesCourse <http://www.Department0.University0.edu/GraduateCourse0> . } " ); 
//	hx_bgp* b					= parse_bgp_query_string( "PREFIX : <http://www.lehigh.edu/~zhp2/2004/0401/univ-bench.owl#> {?x a :Person .  ?x :memberOf <http://www.Department0.University0.edu> .} " ); 
	int triple_count			= hx_bgp_size(b);
	int node_count				= triple_count * 3;
	
	int maxiv;
	char** variable_names;
	hx_node_id* triple_nodes;
	distribute_bgp( ctx, nodemap, &b, &triple_nodes, &variable_names, &maxiv );

	if (myrank == DEBUG_NODE) {
		for (i = 0; i <= maxiv; i++) {
			fprintf( stderr, "variable map slot %d: '%s'\n", i, variable_names[i] );
		}
	}

	exec_start	= TIME();
	hx_variablebindings_nodes** nodes;
	
	int missing_node	= 0;
	for (i = 0; i < triple_count; i++) {
		int j;
		for (j = 0; j < 3; j++) {
			hx_node_id id	= triple_nodes[(3*i)+j];
			if (id == (hx_node_id) 0) {
				missing_node	= 1;
			}
		}
	}
	
	hx_variablebindings_iter* iter	= (missing_node)
									? hx_variablebindings_new_empty_iter()
									: hx_parallel_rendezvousjoin( ctx, hx, triple_count, triple_nodes, variable_names, maxiv );
	
	while (!hx_variablebindings_iter_finished(iter)) {
		hx_variablebindings* b;
		hx_variablebindings_iter_current( iter, &b );
		hx_variablebindings_debug( b, NULL );
		hx_variablebindings_iter_next(iter);
	}
	
// 	int results	= hx_parallel_get_nodes( ctx, iter, &nodes );
// 	MPI_File file;
// 	MPI_Info info;
// 	MPI_Status status;
// 	MPI_Info_create(&info);
// 	MPI_File_open(MPI_COMM_SELF, (char*) ctx->local_output_file, MPI_MODE_WRONLY | MPI_MODE_CREATE, info, &file);
// 	//FILE* f	= fopen( ctx->local_output_file, "w" );
// 	for (i = 0; i < results; i++) {
// 		hx_variablebindings_nodes* b	= nodes[i];
// 		char* string;
// 		hx_variablebindings_nodes_string( b, &string );
// 		char chars[1024];
// 		//fprintf(f, "process %d: binding %p %s\n", myrank, (void*) b, string);
// 		sprintf(chars, "process %d: binding %p %s\n", myrank, (void*) b, string);
// 		MPI_File_write_shared(file, chars, strlen(chars), MPI_BYTE, &status);
// 		free(string);
// 		hx_free_variablebindings_nodes(b);
// 	}
// 	//fclose( f );
// 	MPI_File_close(&file);
// 	MPI_Info_free(&info);
// 	fprintf( stderr, "process %d: %d results\n", myrank, results );
	
	exec_end	= TIME();
	
	if (myrank == 0) {
		fprintf( stdout, "execution took %" PRINTTIME " seconds\n", DIFFTIME(exec_end, exec_start) );
	}
	
	for (i = 0; i <= maxiv; i++) {
		if (variable_names[i] != NULL) {
			free(variable_names[i]);
		}
	}
	free(variable_names);
	free(triple_nodes);
	hx_free_bgp(b);
	hx_free_hexastore( hx, st );
	hx_free_storage_manager( st );
	hx_parallel_free_parallel_execution_context( ctx );
	
	MPI_Finalize(); 
	return 0;
}

hx_node_id get_node_id ( hx_parallel_execution_context* ctx, TCBDB* nodemap, hx_node* n ) {
	hx_node_id id	= 0;
	if (hx_node_is_variable(n)) {
		id	= (hx_node_id) hx_node_iv(n);
	} else {
		int len;
		void* ptr;
		char* string;
		hx_node_string( n, &string );
		if ((ptr = tcbdbget(nodemap, string, strlen(string), &len))) {
			id	= *((hx_node_id*)ptr);
		}
		free(string);
	}
	return id;
}

int distribute_bgp ( hx_parallel_execution_context* ctx, TCBDB* nodemap, hx_bgp** b, hx_node_id** triple_nodes, char*** variable_names, int* maxiv ) {
	int i;
	int root					= ctx->root;
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	int triple_count			= hx_bgp_size(*b);
	int node_count				= triple_count * 3;
	hx_node_id* _triple_nodes	= (hx_node_id*) calloc( node_count, sizeof( hx_node_id ) );
	hx_node** variables			= NULL;
	int variable_count			= 0;
	
	for (i = 0; i < triple_count; i++) {
		hx_triple* t	= hx_bgp_triple( *b, i );
		hx_node* s		= t->subject;
		hx_node* p		= t->predicate;
		hx_node* o		= t->object;
		
		hx_node_id sid	= get_node_id(ctx, nodemap, s);
		hx_node_id pid	= get_node_id(ctx, nodemap, p);
		hx_node_id oid	= get_node_id(ctx, nodemap, o);
		
		_triple_nodes[(3*i)+0]	= sid;
		_triple_nodes[(3*i)+1]	= pid;
		_triple_nodes[(3*i)+2]	= oid;
	}
	
// 
// 	// parse triples from ~SPARQL syntax, and place the node IDs into the _triple_nodes[] buffer
// 	int len;
// // 	fprintf( stderr, "freezing bgp...\n" );
// 	char* bfreeze		= hx_bgp_freeze_mpi( ctx, *b, &len );
// // 	fprintf( stderr, "done freezing...\n" );
// 	
// 	hx_node_id* _ptr	= (hx_node_id*) bfreeze;
// 	hx_node_id* ptr		= &( _ptr[1] );
// 	
// 	for (i = 0; i < node_count; i++) {
// 		_triple_nodes[i]	= ptr[i];
// 	}
	
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
		hx_triple_string( t, &string );

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
			variables[ j++ ]	= node;
		}
		free(string);
	}
	free(is_variable);
	
	MPI_Barrier(MPI_COMM_WORLD);
	
	// broadcast the mapping from variable number to variable name to all processes
	*variable_names	= hx_parallel_broadcast_variables(ctx, variables, variable_count, maxiv);	
	
	// now broadcast the _triple_nodes[] buffer to all nodes, so everyone can execute the query
	MPI_Bcast(_triple_nodes,sizeof(hx_node_id)*node_count,MPI_BYTE,0,MPI_COMM_WORLD);
	*triple_nodes	= _triple_nodes;
	
	free(variables);
// 	free(bfreeze);
	
	return 0;
}
