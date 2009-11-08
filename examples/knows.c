#include <stdio.h>
#include "mentok/mentok.h"
#include "mentok/rdf/triple.h"
#include "mentok/engine/mergejoin.h"
#include "mentok/rdf/node.h"
#include "mentok/engine/bgp.h"
#include "mentok/store/hexastore/hexastore.h"

int main ( int argc, char** argv ) {
	const char* filename	= argv[1];
	FILE* f	= fopen( filename, "r" );
	if (f == NULL) {
		perror( "Failed to open hexastore file for reading: " );
		return 1;
	}
	hx_store* store			= hx_store_hexastore_read( NULL, f, 0 );
	hx_model* hx		= hx_new_model_with_store( NULL, store );
	hx_nodemap* map			= hx_store_hexastore_get_nodemap( store );
	fprintf( stderr, "Finished loading hexastore...\n" );
	
	hx_node* x			= hx_model_new_named_variable( hx, "x" );
	hx_node* y			= hx_model_new_named_variable( hx, "y" );
	hx_node* z			= hx_model_new_named_variable( hx, "z" );
	
	hx_node* type		= hx_new_node_resource("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	hx_node* knows		= hx_new_node_resource("http://xmlns.com/foaf/0.1/knows");
	hx_node* name		= hx_new_node_resource("http://xmlns.com/foaf/0.1/name");
	hx_node* person		= hx_new_node_resource("http://xmlns.com/foaf/0.1/Person");
	
	hx_triple* triples[3];
	{
		triples[0]	= hx_new_triple( x, type, person );
		triples[1]	= hx_new_triple( x, knows, y );
		triples[2]	= hx_new_triple( y, name, z );
	}
	
	hx_bgp* b	= hx_new_bgp( 3, triples );
	hx_execution_context* ctx	= hx_new_execution_context( NULL, hx );
	hx_variablebindings_iter* iter	= hx_bgp_execute( ctx, b );
	
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
	
	while (!hx_variablebindings_iter_finished( iter )) {
		hx_variablebindings* b;
		hx_variablebindings_iter_current( iter, &b );
		hx_node_id xid	= hx_variablebindings_node_id_for_binding ( b, xi );
		hx_node_id yid	= hx_variablebindings_node_id_for_binding ( b, yi );
		hx_node_id zid	= hx_variablebindings_node_id_for_binding ( b, zi );
		hx_node* x		= hx_nodemap_get_node( map, xid );
		hx_node* y		= hx_nodemap_get_node( map, yid );
		hx_node* z		= hx_nodemap_get_node( map, zid );
		
		char *xs, *ys, *zs;
		hx_node_string( x, &xs );
		hx_node_string( y, &ys );
		hx_node_string( z, &zs );
		printf( "%s\t%s\t%s\n", xs, ys, zs );
		free( xs );
		free( ys );
		free( zs );
		
		hx_free_variablebindings(b);
		hx_variablebindings_iter_next( iter );
	}
	hx_free_variablebindings_iter( iter );
	
	hx_free_bgp( b );
	hx_free_node( x );
	hx_free_node( y );
	hx_free_node( z );
	hx_free_node( type );
	hx_free_node( person );
	hx_free_node( knows );
	hx_free_node( name );
	hx_free_model( hx );
	hx_free_execution_context( ctx );
	
	return 0;
}

