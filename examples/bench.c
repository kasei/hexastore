// PREFIX : <http://www.lehigh.edu/~zhp2/2004/0401/univ-bench.owl#>
// SELECT DISTINCT * WHERE {
// 	?y a :Faculty .
// 	?x :advisor ?y .
// 	?y :teacherOf ?z .
// 	?z a :Course .
// 	?x a :Student .
// 	?x :takesCourse ?z .
// }

#include <time.h>
#include <stdio.h>
#include "hexastore.h"
#include "algebra/variablebindings.h"
#include "engine/mergejoin.h"
#include "rdf/node.h"
#include "algebra/bgp.h"

#define DIFFTIME(a,b) ((b-a)/(double)CLOCKS_PER_SEC)
double bench ( hx_hexastore* hx, hx_bgp* b );

static int verbose		= 0;

static hx_node* x;
static hx_node* y;
static hx_node* z;
static hx_node* type;
static hx_node* dept;
static hx_node* subOrg;
static hx_node* univ;
static hx_node* degFrom;
static hx_node* gradstudent;
static hx_node* member;

double average ( hx_hexastore* hx, hx_bgp* b, int count ) {
	double total	= 0.0;
	int i;
	for (i = 0; i < count; i++) {
		total	+= bench( hx, b );
	}
	return (total / (double) count);
}

double bench ( hx_hexastore* hx, hx_bgp* b ) {
	hx_nodemap* map		= hx_get_nodemap( hx );
	clock_t st_time	= clock();
	
	hx_variablebindings_iter* iter	= hx_bgp_execute( b, hx );
//	hx_variablebindings_iter_debug( iter, "lubm8> ", 0 );
	
	int size		= hx_variablebindings_iter_size( iter );
	char** names	= hx_variablebindings_iter_names( iter );
	
	int xi, yi, zi;
	int i;
	for (i = 0; i < size; i++) {
		if (strcmp(names[i], "x") == 0) {
			xi	= i;
		} else if (strcmp(names[i], "y") == 0) {
			yi	= i;
		} else if (strcmp(names[i], "z") == 0) {
			zi	= i;
		}
	}
	
	uint64_t count	= 0;
	while (!hx_variablebindings_iter_finished( iter )) {
		count++;
		hx_variablebindings* b;
		hx_variablebindings_iter_current( iter, &b );
		
		if (0) {
			hx_node* x		= hx_variablebindings_node_for_binding_name( b, map, "x" );
			hx_node* y		= hx_variablebindings_node_for_binding_name( b, map, "y" );
			hx_node* z		= hx_variablebindings_node_for_binding_name( b, map, "z" );
		
			char *xs, *ys, *zs;
			hx_node_string( x, &xs );
			hx_node_string( y, &ys );
			hx_node_string( z, &zs );
			printf( "%s\t%s\t%s\n", xs, ys, zs );
			free( xs );
			free( ys );
			free( zs );
		}
		
		hx_free_variablebindings(b);
		hx_variablebindings_iter_next( iter );
	}
	if (verbose) {
		printf( "%llu results\n", (unsigned long long) count );
	}
	clock_t end_time	= clock();
	
	hx_free_variablebindings_iter( iter );
	return DIFFTIME(st_time, end_time);
}

void help (int argc, char** argv) {
	fprintf( stderr, "Usage:\n" );
	fprintf( stderr, "\t%s [-o] lubm.hx\n", argv[0] );
	fprintf( stderr, "\n\n" );
}

int main ( int argc, char** argv ) {
	if (argc < 2) {
		help(argc, argv);
		exit(1);
	}
	
	const char* filename;
	int i;
	int runs_count	= 5;
	int run_optimized	= 0;
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-o") == 0) {
			run_optimized	= 1;
		} else if (strcmp(argv[i], "-v") == 0) {
			verbose	= 1;
		} else if (strcmp(argv[i], "-c") == 0) {
			runs_count	= atoi( argv[++i] );
		} else {
			filename	= argv[i];
			break;
		}
	}
	
	FILE* f	= fopen( filename, "r" );
	if (f == NULL) {
		perror( "Failed to open hexastore file for reading: " );
		return 1;
	}
	

	hx_store* store			= hx_store_hexastore_read( NULL, f, 0 );
	hx_hexastore* hx		= hx_new_hexastore_with_store( NULL, store );
	hx_nodemap* map			= hx_store_hexastore_get_nodemap( store );
	if (verbose) {
		fprintf( stderr, "Finished loading hexastore...\n" );
	}
	
	const char* query_string	= "PREFIX : <http://www.lehigh.edu/~zhp2/2004/0401/univ-bench.owl#> { ?x :undergraduateDegreeFrom ?y ; a :GraduateStudent ; :memberOf ?z . ?z a :Department ; :subOrganizationOf ?y . ?y a :University }";
	
	if (verbose) {
		fprintf( stderr, "Benchmarking BGP with %d runs...\n", runs_count );
	}
	
	{
		hx_bgp* b	= hx_bgp_parse_string( query_string );
		if (verbose) {
			hx_bgp_debug( b );
		}
		fprintf( stderr, "execution time: %lf\n", average( hx, b, runs_count ) );
		hx_free_bgp( b );
	}
	if (run_optimized) {
		hx_bgp* b	= hx_bgp_parse_string( query_string );
		hx_bgp_reorder( b, hx );
		if (verbose) {
			hx_bgp_debug( b );
		}
		fprintf( stderr, "BGP-optimized execution time: %lf\n", average( hx, b, runs_count ) );
		hx_free_bgp( b );
	}
	
	hx_free_hexastore( hx );
	
	return 0;
}
