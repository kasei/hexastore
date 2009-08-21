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
#include "variablebindings.h"
#include "mergejoin.h"
#include "rdf/node.h"
#include "bgp.h"

#define DIFFTIME(a,b) ((b-a)/(double)CLOCKS_PER_SEC)
double bench ( hx_hexastore* hx, hx_bgp* b );

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
	printf( "%llu results\n", (unsigned long long) count );
	clock_t end_time	= clock();
	
	hx_free_variablebindings_iter( iter );
	return DIFFTIME(st_time, end_time);
}

int main ( int argc, char** argv ) {
	const char* filename	= argv[1];
	FILE* f	= fopen( filename, "r" );
	if (f == NULL) {
		perror( "Failed to open hexastore file for reading: " );
		return 1;
	}
	
	hx_hexastore* hx		= hx_read( f, 0 );
	hx_nodemap* map			= hx_get_nodemap( hx );
	fprintf( stderr, "Finished loading hexastore...\n" );
	
	{
		hx_bgp* b	= parse_bgp_query_string( "PREFIX : <http://www.lehigh.edu/~zhp2/2004/0401/univ-bench.owl#> { ?x :undergraduateDegreeFrom ?y ; a :GraduateStudent ; :memberOf ?z . ?z a :Department ; :subOrganizationOf ?y . ?y a :University }" );
		hx_bgp_debug( b );
		fprintf( stderr, "running time: %lf\n", average( hx, b, 4 ) );
		hx_free_bgp( b );
	}
	{
		hx_bgp* b	= parse_bgp_query_string( "PREFIX : <http://www.lehigh.edu/~zhp2/2004/0401/univ-bench.owl#> { ?x :undergraduateDegreeFrom ?y ; a :GraduateStudent ; :memberOf ?z . ?z a :Department ; :subOrganizationOf ?y . ?y a :University }" );
		hx_bgp_reorder( b, hx );
		hx_bgp_debug( b );
		fprintf( stderr, "BGP-optimized running time: %lf\n", average( hx, b, 4 ) );
		hx_free_bgp( b );
	}
	
	hx_free_hexastore( hx );
	
	return 0;
}
