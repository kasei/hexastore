#include <stdio.h>
#include <stdlib.h>
#include "mentok.h"
#include "misc/nodemap.h"
#include "store/hexastore/hexastore.h"

hx_node_id map_old_to_new_id ( hx_nodemap* old, hx_nodemap* _new, hx_node_id id );
void help (int argc, char** argv) {
	fprintf( stderr, "Usage: %s in.hx out.hxo\n\n", argv[0] );
}

int main (int argc, char** argv) {
	const char* in_filename		= NULL;
	const char* out_filename	= NULL;
	
	if (argc != 3) {
		help(argc, argv);
		exit(1);
	}

	in_filename		= argv[1];
	out_filename	= argv[2];
	
	FILE* inf	= fopen( in_filename, "r" );
	if (inf == NULL) {
		perror( "Failed to open hexastore file for reading: " );
		return 1;
	}
	
	FILE* outf	= fopen( out_filename, "w" );
	if (outf == NULL) {
		perror( "Failed to open hexastore file for writing: " );
		return 1;
	}
	
	
	
	fprintf( stderr, "reading hexastore from file...\n" );
	hx_store* store			= hx_store_hexastore_read( NULL, inf, 0 );
	hx_model* hx		= hx_new_model_with_store( NULL, store );
	fprintf( stderr, "reading nodemap from file...\n" );
	hx_nodemap* map			= hx_store_hexastore_get_nodemap( store );
	
	fprintf( stderr, "re-sorting nodemap...\n" );
	hx_nodemap* smap	= hx_nodemap_sparql_order_nodes( map );
	
	hx_nodemap_debug( map );
	hx_nodemap_debug( smap );
	
	int count	= 0;
	fprintf( stderr, "creating new hexastore...\n" );
	
	hx_store* sstore	= hx_new_store_hexastore_with_nodemap( NULL, smap );
	hx_model* shx	= hx_new_model_with_store( NULL, sstore );
	
	hx_node* sn		= hx_new_named_variable(hx, "s");
	hx_node* pn		= hx_new_named_variable(hx, "p");
	hx_node* on		= hx_new_named_variable(hx, "o");
	hx_triple* t	= hx_new_triple( sn, pn, on );
	hx_variablebindings_iter* iter	= hx_new_variablebindings_iter_for_triple( hx, t, HX_SUBJECT );
	while (!hx_variablebindings_iter_finished(iter)) {
		hx_variablebindings* b;
		hx_variablebindings_iter_current( iter, &b );
		hx_node* s	= hx_variablebindings_node_for_binding_name( b, map, "s" );
		hx_node* p	= hx_variablebindings_node_for_binding_name( b, map, "p" );
		hx_node* o	= hx_variablebindings_node_for_binding_name( b, map, "o" );
		
		fprintf( stderr, "----------\n" );
		hx_node_debug(s);
		hx_node_debug(p);
		hx_node_debug(o);
		hx_add_triple( shx, s, p, o );
		
		hx_free_variablebindings( b );
		hx_variablebindings_iter_next(iter);
	}
	hx_free_variablebindings_iter( iter );
	
	hx_store_hexastore_debug( sstore );
	
	if (hx_store_hexastore_write( shx->store, outf ) != 0) {
		fprintf( stderr, "*** Couldn't write hexastore to disk.\n" );
		return 1;
	}
	
	hx_free_model( hx );
	hx_free_nodemap( smap );
	fclose( inf );
	fclose( outf );
	return 0;
}

char* node_string ( const char* nodestr ) {
	int len			= strlen( nodestr ) + 1 + 2;
	char* string	= (char*) malloc( len );
	if (string == NULL) {
		fprintf( stderr, "*** malloc failed in optimize.c:node_string\n" );
	}
	const char* value		= &(nodestr[1]);
	switch (*nodestr) {
		case 'R':
			sprintf( string, "<%s>", value );
			len	+= 2;
			break;
		case 'L':
			sprintf( string, "\"%s\"", value );
			len	+= 2;
			break;
		case 'B':
			sprintf( string, "_:%s", value );
			len	+= 2;
			break;
	};
	return string;
}

hx_node_id map_old_to_new_id ( hx_nodemap* old, hx_nodemap* _new, hx_node_id id ) {
	hx_node* node		= hx_nodemap_get_node( old, id );
	hx_node_id newid	= hx_nodemap_get_node_id( _new, node );
	return newid;
}



