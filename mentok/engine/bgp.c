#include "mentok/engine/bgp.h"
#include "mentok/engine/mergejoin.h"
#include "mentok/engine/hashjoin.h"
#include "mentok/engine/project.h"

hx_variablebindings_iter* hx_bgp_execute2 ( hx_execution_context* ctx, hx_bgp* b, void* thunk ) {
	return hx_bgp_execute( ctx, b );
}

hx_variablebindings_iter* hx_bgp_execute ( hx_execution_context* ctx, hx_bgp* b ) {
	hx_model* hx	= ctx->hx;
	int size	= hx_bgp_size( b );
	
	hx_triple* t0	= hx_bgp_triple( b, 0 );
	int sort;
	if (size > 1) {
		sort	= _hx_bgp_sort_for_triple_join( t0, hx_bgp_triple( b, 1 ) );
	} else {
		sort	= HX_SUBJECT;
	}
	
	hx_variablebindings_iter* iter	= hx_model_new_variablebindings_iter_for_triple( hx, t0, sort );
	
	if (size > 1) {
		int i;
		for (i = 1; i < size; i++) {
			char *sname, *pname, *oname;
			hx_triple* t			= hx_bgp_triple( b, i );
			int jsort				= _hx_bgp_sort_for_vb_join( t, iter );
			hx_variablebindings_iter* interm	= hx_model_new_variablebindings_iter_for_triple( hx, t, jsort );
			
//			iter	= hx_new_nestedloopjoin_iter( interm, iter );			
//			iter	= hx_new_hashjoin_iter( interm, iter );
			iter	= hx_new_mergejoin_iter( interm, iter );
		}
	}
	
	hx_node** variables;
	int count	= hx_bgp_variables( b, &variables );
	char** proj_nodes	= (char**) calloc( count, sizeof( char* ) );
	int proj_count	= 0;
	
	// Now project away any variables that are non-distinguished
	int i;
	for (i = 0; i < count; i++) {
		hx_node* v	= variables[i];
		if (hx_node_is_distinguished_variable( v )) {
			char* string;
			hx_node_variable_name( v, &string );
			proj_nodes[ proj_count++ ]	= string;
		}
		hx_free_node(v);
	}
	free( variables );
	
	if (proj_count < count) {
		iter	= hx_new_project_iter( iter, proj_count, proj_nodes );
	}
	for (i = 0; i < proj_count; i++) {
		free( proj_nodes[i] );
	}
	free( proj_nodes );
	
	return iter;
}

