#include <stdio.h>
#include "hexastore.h"
#include "mergejoin.h"
#include "node.h"
#include "bgp.h"
#include "parallel.h"
#include "materialize.h"
#include "nestedloopjoin.h"

int DEBUG_NODE	= -1;

extern hx_bgp* parse_bgp_query_string ( char* );
hx_hexastore* distribute_triples_from_file ( hx_hexastore* hx, hx_storage_manager* st, const char* filename );
int distribute_bgp ( int root, hx_bgp** b, hx_nodemap* map, hx_node_id** triple_nodes, char*** variable_names, int* maxiv );

hx_variablebindings_iter* _hx_parallel_variablebindings_iter_for_triple ( int triple, hx_parallel_execution_context* ctx, hx_hexastore* hx, int node_count, hx_node_id* triple_nodes, char** node_names ) {
	int myrank; MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	hx_index* index;
	hx_node_id index_ordered[3];
	int order_position	= HX_OBJECT;
	hx_get_ordered_index_id( hx, ctx->storage, triple_nodes[(3*triple)+0], triple_nodes[(3*triple)+1], triple_nodes[(3*triple)+2], order_position, &index, index_ordered, NULL );
	hx_index_iter* titer	= hx_index_new_iter1( index, ctx->storage, triple_nodes[(3*triple)+0], triple_nodes[(3*triple)+1], triple_nodes[(3*triple)+2] );
	
	char* names[3];
	int i;
	for (i = 0; i < 3; i++) {
		char* n	= node_names[ (3*triple) + i ];
		if (n != NULL) {
			int len	= strlen(n);
			names[i]	= (char*) calloc( len + 1, sizeof( char ) );
			strcpy( names[i], n );
		} else {
			names[i]	= NULL;
		}
	}
	
	hx_variablebindings_iter* iter	= hx_new_iter_variablebindings( titer, ctx->storage, names[0], names[1], names[2], 0 );
	return iter;
}

int _hx_parallel_variablebindings_iter_shared_columns2( hx_node_id* triple_nodes, char** node_names, char** variable_names, int maxiv, hx_variablebindings_iter* lhs, int rhs_triple, char*** columns ) {
	int myrank; MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	int* shared	= (int*) calloc( maxiv, sizeof( int ) );
	
// 	fprintf( stderr, "names for lhs:\n" );
// 	
// 	int lhs_start	= 3 * lhs_triple;
// 	for (int i = lhs_start; i < 3+lhs_start; i++) {
// 		if (triple_nodes[i] < 0) {
// 			fprintf( stderr, "- %s (%d)\n", node_names[i], (int) triple_nodes[i] );
// 			shared[ -1 * triple_nodes[i] ]++;
// 		}
// 	}

// 	fprintf( stderr, "names for rhs:\n" );
	int rhs_start	= 3 * rhs_triple;
	int i;
	for (i = rhs_start; i < 3+rhs_start; i++) {
		if (triple_nodes[i] < 0) {
// 			fprintf( stderr, "- %s (%d)\n", node_names[i], (int) triple_nodes[i] );
			shared[ -1 * triple_nodes[i] ]++;
		}
	}
	
	int lhs_size		= hx_variablebindings_iter_size( lhs );
// 	fprintf( stderr, "(%d total) names for lhs:\n", lhs_size );
	char** lhs_names	= hx_variablebindings_iter_names( lhs );
	for (i = 0; i < lhs_size; i++) {
		char* lhs_name	= lhs_names[i];
// 		fprintf( stderr, "- %s\n", lhs_name );
		int j;
		for (j = 0; j <= maxiv; j++) {
			if (variable_names[j] != NULL) {
// 				fprintf( stderr, "\tchecking with existing RHS variable %s\n", variable_names[j] );
				if (strcmp(variable_names[j], lhs_name) == 0) {
					shared[j]++;
				}
			}
		}
	}
	
	int shared_count	= 0;
	for (i = 0; i <= maxiv; i++) {
		if (shared[i] == 2) {
			shared_count++;
// 			fprintf( stderr, "variable is shared: %s\n", variable_names[i] );
		}
	}
	
	int j		= 0;
	*columns	= (char**) calloc( shared_count, sizeof( char* ) );
//	fprintf( stderr, "node %d allocated column array %p\n", myrank, *columns );
	
	for (i = 0; i <= maxiv; i++) {
		if (shared[i] == 2) {
			(*columns)[ j++ ]	= variable_names[i];
		}
	}
	
	return shared_count;
}

hx_variablebindings_iter* hx_parallel_rendezvousjoin( hx_parallel_execution_context* ctx, hx_hexastore* hx, int triple_count, hx_node_id* triple_nodes, char** variable_names, int maxiv ) {
	int myrank; MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	int node_count	= triple_count * 3;
	// create node_names[] that maps node positions in the BGP (blocks of 3 nodes per triple) to the name of the variable node in that position (NULL if not a variable)
	char** node_names	= (char**) calloc( node_count, sizeof( char* ) );
	int i;
	for (i = 0; i < node_count; i++) {
		if (triple_nodes[i] < 0) {
			node_names[i]	= variable_names[ -1 * triple_nodes[i] ];
		} else {
			node_names[i]	= NULL;
		}
	}
	
	hx_variablebindings_iter* lhs		= _hx_parallel_variablebindings_iter_for_triple( 0, ctx, hx, node_count, triple_nodes, node_names );
	
	int j;
	for (j = 1; j < triple_count; j++) {
		ctx->join_iteration	= j;
		/**********************************************************************/
		MPI_Barrier(MPI_COMM_WORLD);
		if (myrank == 0) {
			fprintf( stderr, "Performing join #%d\n", j );
		}
		MPI_Barrier(MPI_COMM_WORLD);
		/**********************************************************************/
		


		char** columns						= NULL;
		hx_variablebindings_iter* rhs		= _hx_parallel_variablebindings_iter_for_triple( j, ctx, hx, node_count, triple_nodes, node_names );
		int shared_count					= _hx_parallel_variablebindings_iter_shared_columns2( triple_nodes, node_names, variable_names, maxiv, lhs, j, &columns );
		
		
		
		/**********************************************************************/
		MPI_Barrier(MPI_COMM_WORLD);
		if (myrank == 0) {
			int i;
			for (i = 0; i < shared_count; i++) {
				fprintf( stderr, "- shared column: '%s'\n", columns[i] );
			}
			fprintf( stderr, "\n" );
		}
		MPI_Barrier(MPI_COMM_WORLD);
		/**********************************************************************/
		
		hx_variablebindings_iter* lhsr	= hx_parallel_distribute_variablebindings( ctx, lhs, shared_count, columns );
		hx_variablebindings_iter* rhsr	= hx_parallel_distribute_variablebindings( ctx, rhs, shared_count, columns );
		lhs								= hx_new_nestedloopjoin_iter( lhsr, rhsr );
		
//		break;
// 		fprintf( stderr, "node %d freeing column array %p\n", myrank, columns );
// 		free(columns);
	}
	
// 	fprintf( stderr, "node %d returning results iterator %p\n", myrank, (void*) lhsr );
	return lhs;
}

int main ( int argc, char** argv ) {
	MPI_Init(&argc, &argv);

	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	hx_nodemap* map			= NULL;
	hx_storage_manager* st	= hx_new_memory_storage_manager();
	hx_hexastore* hx		= hx_new_hexastore( st );
	hx_hexastore* source	= distribute_triples_from_file( hx, st, argv[1] );
	if (source) {
		map					= hx_get_nodemap( source );
	}
	
	hx_parallel_execution_context ctx;
	ctx.storage	= st;
	ctx.root	= 0;
	
	hx_bgp* b					= parse_bgp_query_string( "PREFIX foaf: <http://xmlns.com/foaf/0.1/> { ?p foaf:name ?name ; foaf:nick ?nick . ?d foaf:maker ?p }" );
	
	int triple_count			= hx_bgp_size(b);
	int node_count				= triple_count * 3;
	
	hx_node_id* triple_nodes;
	char** variable_names;
	int maxiv;
	distribute_bgp( 0, &b, map, &triple_nodes, &variable_names, &maxiv );

	if (myrank == DEBUG_NODE) {
		int i;
		for (i = 0; i <= maxiv; i++) {
			fprintf( stderr, "variable map slot %d: '%s'\n", i, variable_names[i] );
		}
	}

	hx_variablebindings_iter* iter	= hx_parallel_rendezvousjoin( &ctx, hx, triple_count, triple_nodes, variable_names, maxiv );
	
	if (iter) {
		while (!hx_variablebindings_iter_finished( iter )) {
			hx_variablebindings* b;
			hx_variablebindings_iter_current( iter, &b );
			fprintf( stderr, "node %d: ", myrank );
			hx_variablebindings_debug( b, NULL );
			hx_free_variablebindings( b, 1 );
			hx_variablebindings_iter_next( iter );
		}
		hx_free_variablebindings_iter( iter, 1 );
	}
	
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

int distribute_bgp ( int root, hx_bgp** b, hx_nodemap* map, hx_node_id** triple_nodes, char*** variable_names, int* maxiv ) {
	int mysize, myrank;
	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	int triple_count			= hx_bgp_size(*b);
	int node_count				= triple_count * 3;
	hx_node_id* _triple_nodes	= (hx_node_id*) calloc( node_count, sizeof( hx_node_id ) );
	hx_node** variables			= NULL;
	int variable_count			= 0;
	
	// parse triples from ~SPARQL syntax, and place the node IDs into the _triple_nodes[] buffer
	if (myrank == root) {
		int len;
		char* bfreeze		= hx_bgp_freeze( *b, &len, map );
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
	}
	
	// broadcast the mapping from variable number to variable name to all processes
	*variable_names	= hx_parallel_broadcast_variables(variables, variable_count, maxiv);	
	
	// now broadcast the _triple_nodes[] buffer to all nodes, so everyone can execute the query
	MPI_Bcast(_triple_nodes,sizeof(hx_node_id)*node_count,MPI_BYTE,0,MPI_COMM_WORLD);
	*triple_nodes	= _triple_nodes;
	
	return 0;
}
