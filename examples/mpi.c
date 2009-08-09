#include <stdio.h>
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

char* read_file ( const char* qf ) {
	struct stat st;
	int fd	= open( qf, O_RDONLY );
	if (fd < 0) {
		perror( "Failed to open query file for reading: " );
		return NULL;
	}
	
	fstat( fd, &st );
	
	FILE* f	= fdopen( fd, "r" );
	if (f == NULL) {
		perror( "Failed to open query file for reading: " );
		return NULL;
	}
	
	char* query	= malloc( st.st_size + 1 );
	if (query == NULL) {
		fprintf( stderr, "*** malloc failed in parse_query.c:main\n" );
	}
	fread(query, st.st_size, 1, f);
	query[ st.st_size ]	= 0;
	return query;
}

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
	
	char* job				= (argc > 3) ? argv[3] : "";
//	hx_parallel_execution_context* ctx	= hx_parallel_new_execution_context( st, "/tmp", job );
	hx_parallel_execution_context* ctx	= hx_parallel_new_execution_context( st, "/gpfs/large/DSSW/rendezvous", job );
	
	TIME_T(load_start, load_end);
	TIME_T(exec_start, exec_end);
	
	
	if (0) { // XXX
		char hostname[256];
		gethostname(hostname, sizeof(hostname));
		printf("\n\n*** Rank %d PID %d on %s ready for attach\n\n", myrank, getpid(), hostname);
		fflush(stdout);
		sleep(5);
	}

	load_start	= TIME();
	hx_parallel_distribute_triples_from_file( ctx, data_filename, hx );
	load_end	= TIME();

	if (myrank == 0) {
		fprintf( stdout, "loading took %" PRINTTIME " seconds\n", DIFFTIME(load_end, load_start) );
	}
	

	char* query	= read_file( argv[2] );
	hx_bgp* b	= parse_bgp_query_string( query );
	free(query);
	
//	hx_bgp* b					= parse_bgp_query_string( "PREFIX foaf: <http://xmlns.com/foaf/0.1/> { ?p foaf:name ?name; foaf:nick ?nick . ?d foaf:maker ?p }" );
//	hx_bgp* b					= parse_bgp_query_string( "{ ?s a <http://simile.mit.edu/2006/01/ontologies/mods3#Record> . ?s <http://simile.mit.edu/2006/01/ontologies/mods3#origin> <info:marcorg/MYG> . }" );
//	hx_bgp* b					= parse_bgp_query_string( "{ ?s <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://simile.mit.edu/2006/01/ontologies/mods3#Text> . ?s <http://simile.mit.edu/2006/01/ontologies/mods3#language> <http://simile.mit.edu/2006/01/language/iso639-2b/fre> }" );
//	hx_bgp* b					= parse_bgp_query_string( "{ ?as <http://simile.mit.edu/2006/01/ontologies/mods3#point> \"end\" ; <http://simile.mit.edu/2006/01/ontologies/mods3#encoding> ?bo ; a ?co }" ); 
//	hx_bgp* b					= parse_bgp_query_string( "PREFIX : <http://www.lehigh.edu/~zhp2/2004/0401/univ-bench.owl#> { ?x a :GraduateStudent . ?x :takesCourse <http://www.Department0.University0.edu/GraduateCourse0> . } " ); 
//	hx_bgp* b					= parse_bgp_query_string( "PREFIX : <http://www.lehigh.edu/~zhp2/2004/0401/univ-bench.owl#> {?x a :Person .  ?x :memberOf <http://www.Department0.University0.edu> .} " ); 
//	hx_bgp* b					= parse_bgp_query_string( "PREFIX : <http://www.lehigh.edu/~zhp2/2004/0401/univ-bench.owl#> { <http://www.Department0.University0.edu/AssociateProfessor0> :teacherOf ?y . ?x :takesCourse ?y ; a :Student . ?y a :Course . } " ); 

	if (myrank == 0) {
		hx_bgp_debug( b );
	}
	
	int triple_count			= hx_bgp_size(b);
	int node_count				= triple_count * 3;
	
	hx_nodemap* nodemap	= hx_get_nodemap( hx );
	
	exec_start	= TIME();
	hx_variablebindings_nodes** nodes;
	
	hx_nodemap* results_map;
	hx_variablebindings_iter* iter	= hx_parallel_rendezvousjoin( ctx, hx, b, &results_map );
	
	int results	= 0;
	MPI_File file;
	MPI_Info info;
	MPI_Status status;
	MPI_Info_create(&info);
	MPI_File_open(MPI_COMM_SELF, (char*) ctx->local_output_file, MPI_MODE_WRONLY | MPI_MODE_CREATE, info, &file);
	//FILE* f	= fopen( ctx->local_output_file, "w" );
	while (!hx_variablebindings_iter_finished(iter)) {
		hx_variablebindings* b;
		hx_variablebindings_iter_current( iter, &b );
		
		char* string;
		hx_variablebindings_string( b, results_map, &string );
		char chars[1024];
		//fprintf(f, "process %d: binding %p %s\n", myrank, (void*) b, string);
		sprintf(chars, "process %d: binding %p %s\n", myrank, (void*) b, string);
		MPI_File_write_shared(file, chars, strlen(chars), MPI_BYTE, &status);
		free(string);
		
		hx_free_variablebindings(b);
		hx_variablebindings_iter_next(iter);
		results++;
	}
	//fclose( f );
	MPI_File_close(&file);
	MPI_Info_free(&info);
	fprintf( stderr, "process %d: %d results\n", myrank, results );
	
	exec_end	= TIME();
	
	int total;
	MPI_Allreduce(&results, &total, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	
	if (myrank == 0) {
		fprintf( stdout, "%d TOTAL RESULTS\n\n", total );
		fprintf( stdout, "\nExecution took %" PRINTTIME " seconds.\n", DIFFTIME(exec_end, exec_start) );
	}
	
	if (0) {
		hx_free_bgp(b);
		hx_free_hexastore( hx, st );
		hx_free_storage_manager( st );
		hx_parallel_free_parallel_execution_context( ctx );
	}
	
	MPI_Finalize(); 
	return 0;
}
