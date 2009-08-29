#include <stdio.h>
#include <dirent.h>
#include "hexastore.h"
#include "rdf/node.h"
#include "engine/mergejoin.h"
#include "engine/materialize.h"
#include "algebra/bgp.h"
#include "parallel/parallel.h"

#include "misc/timing_choices.h"
#define TIMING_CPU_FREQUENCY 2600000000.0
#define TIMING_USE TIMING_RDTSC
#include "misc/timing.h"

int DEBUG_NODE	= -1;

extern hx_bgp* parse_bgp_query_string ( char* );
hx_hexastore* distribute_triples_from_file ( hx_hexastore* hx, const char* filename );

int directory_exists ( const char* dir ) {
	int exists	= 0;
	DIR* d	= opendir( dir );
	if (d) {
		exists	= 1;
		closedir(d);
	}
	return exists;
}

char* read_file ( const char* qf ) {
	MPI_File file;
	MPI_Info info;
	MPI_Status status;
	MPI_Offset filesize;
	MPI_Info_create(&info);
	MPI_File_open(MPI_COMM_WORLD, (char*) qf, MPI_MODE_RDONLY, info, &file);
	MPI_File_get_size(file, &filesize);

	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	char* query	= calloc( filesize + 1, sizeof( char ) );
	if (query == NULL) {
		fprintf( stderr, "*** malloc failed in parse_query.c:main\n" );
	}
	
	int r	= MPI_File_read_at(file, 0, query, filesize, MPI_BYTE, &status);
	if (r != MPI_SUCCESS) {
		fprintf( stderr, "rank %d failed MPI_File_read_at with error %d\n", myrank, status.MPI_ERROR );
	}
	
	MPI_File_close(&file);
	MPI_Info_free(&info);
	
	return query;
}

int main ( int argc, char** argv ) {
	MPI_Init(&argc, &argv);
	
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	

	if (0) { // XXX
		char hostname[256];
		gethostname(hostname, sizeof(hostname));
		printf("\n\n*** Rank %d PID %d on %s ready for attach\n\n", myrank, getpid(), hostname);
		fflush(stdout);
		sleep(5);
	}

	hx_hexastore* hx		= hx_new_hexastore( NULL );
	
	const char* data_filename	= argv[1];
	const char* query_filename	= argv[2];
	
	char* job				= (argc > 3) ? argv[3] : "";
	const char* dir			= (directory_exists("/gpfs"))
							? "/gpfs/large/DSSW/rendezvous"
							: "/tmp";
	hx_parallel_execution_context* ctx	= hx_parallel_new_execution_context( dir, job );
	if (myrank == 0)
		fprintf( stderr, "using %s for output files\n", dir );
	
	TIME_T(load_start, load_end);
	TIME_T(exec_start, exec_end);
	
	
	load_start	= TIME();
	hx_parallel_distribute_triples_from_file( ctx, data_filename, hx );
	load_end	= TIME();

	if (myrank == 0) {
		fprintf( stdout, "Loading took %lf seconds\n", DIFFTIME(load_end, load_start) );
	}
	

	char* query	= read_file( query_filename );
	hx_bgp* b	= parse_bgp_query_string( query );
	
	if (b == NULL) {
		fprintf( stderr, "*** rank %d failed to parse BGP\n", myrank );
		fflush( stderr );
		sleep(10);
		MPI_Barrier( MPI_COMM_WORLD );
		MPI_Abort( MPI_COMM_WORLD, 1 );
	}
	MPI_Barrier( MPI_COMM_WORLD );
	
	
//	hx_bgp_reorder_mpi( b, hx );
	
//	hx_bgp* b					= parse_bgp_query_string( "PREFIX foaf: <http://xmlns.com/foaf/0.1/> { ?p foaf:name ?name; foaf:nick ?nick . ?d foaf:maker ?p }" );
//	hx_bgp* b					= parse_bgp_query_string( "{ ?s a <http://simile.mit.edu/2006/01/ontologies/mods3#Record> . ?s <http://simile.mit.edu/2006/01/ontologies/mods3#origin> <info:marcorg/MYG> . }" );
//	hx_bgp* b					= parse_bgp_query_string( "{ ?s <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://simile.mit.edu/2006/01/ontologies/mods3#Text> . ?s <http://simile.mit.edu/2006/01/ontologies/mods3#language> <http://simile.mit.edu/2006/01/language/iso639-2b/fre> }" );
//	hx_bgp* b					= parse_bgp_query_string( "{ ?as <http://simile.mit.edu/2006/01/ontologies/mods3#point> \"end\" ; <http://simile.mit.edu/2006/01/ontologies/mods3#encoding> ?bo ; a ?co }" ); 
//	hx_bgp* b					= parse_bgp_query_string( "PREFIX : <http://www.lehigh.edu/~zhp2/2004/0401/univ-bench.owl#> { ?x a :GraduateStudent . ?x :takesCourse <http://www.Department0.University0.edu/GraduateCourse0> . } " ); 
//	hx_bgp* b					= parse_bgp_query_string( "PREFIX : <http://www.lehigh.edu/~zhp2/2004/0401/univ-bench.owl#> {?x a :Person .  ?x :memberOf <http://www.Department0.University0.edu> .} " ); 
//	hx_bgp* b					= parse_bgp_query_string( "PREFIX : <http://www.lehigh.edu/~zhp2/2004/0401/univ-bench.owl#> { <http://www.Department0.University0.edu/AssociateProfessor0> :teacherOf ?y . ?x :takesCourse ?y ; a :Student . ?y a :Course . } " ); 

	if (myrank == 0) {
		fprintf( stderr, "BGP DEBUG (on rank 0):\n" );
		hx_bgp_debug( b );
		fprintf( stderr, "\n" );
	}
	
	exec_start	= TIME();
	
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
		char* chars	= malloc( strlen( string ) + 2 );
		sprintf(chars, "%s\n", string);
		MPI_File_write_shared(file, chars, strlen(chars), MPI_BYTE, &status);
		free(string);
		free(chars);
		
		hx_free_variablebindings(b);
		hx_variablebindings_iter_next(iter);
		results++;
	}
	//fclose( f );
	MPI_File_close(&file);
	MPI_Info_free(&info);
//	fprintf( stderr, "process %d: %d results\n", myrank, results );
	
	exec_end	= TIME();
	
	int total;
	MPI_Allreduce(&results, &total, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	
	if (myrank == 0) {
		fprintf( stdout, "%d Total results\n\n", total );
		fprintf( stdout, "\nExecution took %lf seconds.\n", DIFFTIME(exec_end, exec_start) );
		fprintf( stdout, "RESULTS: load=%lf exec=%lf total=%d np=%d data=%s query=%s\n", DIFFTIME(load_end, load_start), DIFFTIME(exec_end, exec_start), total, mysize, data_filename, query_filename);
	}
	
	if (1) {
		free(query);
		hx_free_variablebindings_iter(iter);
		hx_free_nodemap( results_map );
		hx_free_bgp(b);
		hx_free_hexastore( hx );
		hx_parallel_free_parallel_execution_context( ctx );
	}
	
	MPI_Finalize(); 
	return 0;
}
