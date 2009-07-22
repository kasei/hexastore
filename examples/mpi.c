#include <stdio.h>
#include "hexastore.h"
#include "mergejoin.h"
#include "node.h"
#include "bgp.h"
#include "parallel.h"
#include "materialize.h"

hx_hexastore* distribute_triples_from_file ( hx_hexastore* hx, hx_storage_manager* st, const char* filename );

int main ( int argc, char** argv ) {
	MPI_Init(&argc, &argv);

	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	hx_storage_manager* st	= hx_new_memory_storage_manager();
	hx_hexastore* hx		= hx_new_hexastore( st );
	hx_hexastore* source	= distribute_triples_from_file( hx, st, argv[1] );
	
	hx_nodemap* map		= NULL;
	if (source) {
		map				= hx_get_nodemap( source );
	}
	
	hx_node* x			= hx_new_named_variable( hx, "x" );
	hx_node* y			= hx_new_named_variable( hx, "y" );
	hx_node* z			= hx_new_named_variable( hx, "z" );
	
	hx_node* type		= hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	hx_node* knows		= hx_new_node_resource("http://xmlns.com/foaf/0.1/knows");
	hx_node* name		= hx_new_node_resource("http://xmlns.com/foaf/0.1/name");
	hx_node* person		= hx_new_node_resource("http://xmlns.com/foaf/0.1/Person");
	
	hx_node_id nameid	= 0;
	
	if (myrank == 0) {
		nameid	= hx_nodemap_get_node_id( map, name );
		fprintf( stderr, "id of foaf:name node: %d (on node %d)\n", (int) nameid, myrank );
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Bcast(&nameid,sizeof(hx_node_id),MPI_BYTE,0,MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
	
	hx_variablebindings_iter* iter	= NULL;
	if (nameid == 0) {
		fprintf( stderr, "foaf:name doesn't exist on node: %d\n", myrank );
	} else {
		fprintf( stderr, "foaf:name is node id %d on node: %d\n", (int) nameid, myrank );
		hx_index* index;
		hx_node_id index_ordered[3];
		int order_position	= HX_OBJECT;
		hx_get_ordered_index_id( hx, st, -1, nameid, -2, order_position, &index, index_ordered, NULL );
		hx_index_iter* titer	= hx_index_new_iter1( index, st, -1, nameid, -2 );
		iter	= hx_new_iter_variablebindings( titer, st, "p", NULL, "name", 0 );
		fprintf( stderr, "node %d distributing triples from iterator %p\n", myrank, (void*) iter );
	}
	
// 	for (int i = 0; i < mysize; i++) {
// 		if (myrank == i) {
// 			char* header	= calloc( 20, sizeof(char) );
// 			sprintf( header, "node %d>", myrank );
// 			if (iter) {
// 				hx_variablebindings_iter_debug( iter, header, 0 );
// 			} else {
// 				fprintf( stderr, "%s no results\n", header );
// 			}
// 			free(header);
// 		}
// 		if (MPI_SUCCESS != MPI_Barrier( MPI_COMM_WORLD )) {
// 			fprintf( stderr, "*** Barrier failed on node %d\n", myrank );
// 		}
// 	}
	
	hx_parallel_distribute_variablebindings( st, iter );
	
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
	
	hx_free_hexastore( hx, st );
	hx_free_storage_manager( st );
	MPI_Finalize(); 
	return 0;
}

hx_hexastore* distribute_triples_from_file ( hx_hexastore* hx, hx_storage_manager* st, const char* filename ) {
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	hx_nodemap* map			= NULL;
	hx_index_iter* iter		= NULL;
	hx_hexastore* source	= NULL;
	if (myrank == 0) {
		FILE* f	= fopen( filename, "r" );
		if (f == NULL) {
			perror( "Failed to open hexastore file for reading: " );
			MPI_Abort(MPI_COMM_WORLD, 1);
		}
		source	= hx_read( st, f, 0 );
		map		= hx_get_nodemap( source );
//		hx_nodemap_debug( map );
		fprintf( stderr, "Finished loading hexastore...\n" );
	}
	
	MPI_Barrier( MPI_COMM_WORLD );
	hx_parallel_distribute_triples_from_hexastore( 0, source, st, hx );
	return source;
}

