#include <stdio.h>
#include "mpi.h"
#include "hexastore.h"
#include "mergejoin.h"
#include "node.h"
#include "bgp.h"

typedef struct {
	int allocated;
	int count;
	hx_node_id* items;
} triple_set_t;
triple_set_t* new_triple_set ( int size );
int free_triple_set ( triple_set_t* c, int free_contained_objects );
void triple_set_push_item( triple_set_t* set, hx_node_id t );
void triple_set_unshift_item( triple_set_t* set, hx_node_id t );

int distribute_triples ( hx_hexastore* hx, hx_storage_manager* s );
int receive_triples ( hx_hexastore* hx, hx_storage_manager* s );

int main ( int argc, char** argv ) {
	int mysize, myrank;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	hx_hexastore* hx;
	hx_storage_manager* s	= hx_new_memory_storage_manager();
	
	if (myrank == 0) {
		const char* filename	= argv[1];
		FILE* f	= fopen( filename, "r" );
		if (f == NULL) {
			perror( "Failed to open hexastore file for reading: " );
			MPI_Abort(MPI_COMM_WORLD, 1);
		}
		hx					= hx_read( s, f, 0 );
		fprintf( stderr, "Finished loading hexastore...\n" );
		
		// send triples to the right nodes
		distribute_triples( hx, s );
	} else {
		hx					= hx_new_hexastore( s );
		receive_triples( hx, s );
	}
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
	hx_free_hexastore( hx, s );
	hx_free_storage_manager( s );
	
	MPI_Finalize(); 
	return 0;
}

int distribute_triples ( hx_hexastore* hx, hx_storage_manager* st ) {
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	hx_node* s	= hx_new_variable( hx );
	hx_node* p	= hx_new_variable( hx );
	hx_node* o	= hx_new_variable( hx );
	hx_index_iter* iter	= hx_get_statements( hx, st, s, p, o, HX_SUBJECT );
	
	int counter	= 0;
	int *triple_counts	= calloc( mysize, sizeof( int ) );
	hx_node_id nodes[3];
	while (!hx_index_iter_finished(iter)) {
		hx_index_iter_current ( iter, &(nodes[0]), &(nodes[1]), &(nodes[2]) );
		int hash	= (nodes[0] ^ nodes[1] ^ nodes[2]) % mysize;
		
		if (hash > 0) {
			// if the triple hashes to a node other than this (master) node, send it out
//			fprintf( stderr, "sending triple { %d, %d, %d } to node %d\n", (int) nodes[0], (int) nodes[1], (int) nodes[2], hash );
			int rc	= MPI_Send( nodes, 3 * sizeof(hx_node_id), MPI_BYTE, hash, 1, MPI_COMM_WORLD );
			triple_counts[hash]++;
		} else {
			// do something with the triple to keep it on this node
			counter++;
		}
		hx_index_iter_next(iter);
	}
	
// 	fprintf( stderr, "master handled %d triples\n", counter );
// 	fprintf( stderr, "broadcasting end-of-stream triple\n" );
	fprintf( stdout, "node %d recieved %d triples\n", myrank, counter );
	for (int i = 1; i < mysize; i++) {
		nodes[0]	= (hx_node_id) 0;
		nodes[1]	= (hx_node_id) triple_counts[i];
		int rc	= MPI_Send( nodes, 3 * sizeof(hx_node_id), MPI_BYTE, i, 1, MPI_COMM_WORLD );
	}
	hx_free_index_iter(iter);
	return counter;
}

int receive_triples ( hx_hexastore* hx, hx_storage_manager* st ) {
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	int total	= -1;
	int counter	= 0;
	while (counter != total) {
		MPI_Status status;
		hx_node_id nodes[3];
		int rc	= MPI_Recv(nodes, 3 * sizeof(hx_node_id), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
//		fprintf( stderr, "node %d got triple: { %d, %d, %d }\n", myrank, (int) nodes[0], (int) nodes[1], (int) nodes[2] );
		if (nodes[0] == (hx_node_id) 0) {
			// this is the end-of-stream marker.
			// nodes[1] contains the number of triples we've been sent.
			fprintf( stdout, "node %d recieved end-of-stream marker with %d expected triples (currently got %d)\n", myrank, (int) nodes[1], counter );
			total	= nodes[1];
		} else {
			counter++;
		}
	}
	return counter;
}
















triple_set_t* new_triple_set ( int size ) {
	triple_set_t* container	= (triple_set_t*) calloc( 1, sizeof( triple_set_t ) );
	container->allocated	= size;
	container->count		= 0;
	container->items		= (hx_node_id*) calloc( container->allocated, sizeof( void* ) );
	return container;
}

int free_triple_set ( triple_set_t* c, int free_contained_objects ) {
	free(c->items);
	free(c);
	return 0;
}

void triple_set_push_item( triple_set_t* set, hx_node_id t ) {
	if (set->allocated <= (set->count + 1)) {
		int i;
		hx_node_id* old;
		hx_node_id* newlist;
		set->allocated	*= 2;
		newlist	= (hx_node_id*) calloc( set->allocated, sizeof( hx_node_id ) );
		for (i = 0; i < set->count; i++) {
			newlist[i]	= set->items[i];
		}
		old	= set->items;
		set->items	= newlist;
		free( old );
	}
	
	set->items[ set->count++ ]	= t;
}

void triple_set_unshift_item( triple_set_t* set, hx_node_id t ) {
	if (set->allocated <= (set->count + 1)) {
		int i;
		hx_node_id* old;
		hx_node_id* newlist;
		set->allocated	*= 2;
		newlist	= (hx_node_id*) calloc( set->allocated, sizeof( hx_node_id ) );
		for (i = 0; i < set->count; i++) {
			newlist[i+1]	= set->items[i];
		}
		old	= set->items;
		set->items	= newlist;
		free( old );
	} else {
		int i;
		for (i = set->count; i > 0; i--) {
			set->items[i]	= set->items[i-1];
		}
	}
	
	set->count++;
	set->items[ 0 ]	= t;
}
