#include "mentok.h"
#include "store/hexastore/hexastore.h"
#include "engine/bgp.h"

// #define DEBUG_INDEX_SELECTION


// void* _hx_add_triple_threaded (void* arg);
// int _hx_iter_vb_finished ( void* iter );
// int _hx_iter_vb_current ( void* iter, void* results );
// int _hx_iter_vb_next ( void* iter );	
// int _hx_iter_vb_free ( void* iter );
// int _hx_iter_vb_size ( void* iter );
// int _hx_iter_vb_sorted_by (void* iter, int index );
// int _hx_iter_debug ( void* info, char* header, int indent );

char** _hx_iter_vb_names ( void* iter );


hx_execution_context* hx_new_execution_context ( void* world, hx_hexastore* hx ) {
	hx_execution_context* c	= (hx_execution_context*) calloc( 1, sizeof( hx_execution_context ) );
	hx_execution_context_init( c, world, hx );
	return c;
}

int hx_execution_context_init ( hx_execution_context* c, void* world, hx_hexastore* hx ) {
	c->world	= world;
	c->hx		= hx;
	c->unsorted_mergejoin_penalty	= 2;
	c->hashjoin_penalty				= 1;
	c->nestedloopjoin_penalty		= 3;
	c->lookup_node					= hx_execution_context_lookup_node;
	c->bgp_exec_func				= hx_bgp_execute2;
	c->bgp_exec_func_thunk			= NULL;
	return 0;
}

int hx_execution_context_set_bgp_exec_func ( hx_execution_context* ctx, hx_variablebindings_iter* (*func)( void*, hx_hexastore*, void* ), void* thunk ) {
	ctx->bgp_exec_func			= func;
	ctx->bgp_exec_func_thunk	= thunk;
	return 0;
}

hx_node* hx_execution_context_lookup_node ( hx_execution_context* ctx, hx_node_id nodeid ) {
	hx_store* store	= ctx->hx->store;
	return hx_store_get_node ( store, nodeid );
}

int hx_free_execution_context ( hx_execution_context* c ) {
	c->world	= NULL;
	c->hx		= NULL;
	free( c );
	return 0;
}

hx_hexastore* hx_new_hexastore ( void* world ) {
	hx_hexastore* hx	= (hx_hexastore*) calloc( 1, sizeof( hx_hexastore )  );
	hx->store			= hx_new_store_hexastore( world );
	hx->next_var		= -1;
	return hx;
}

hx_hexastore* hx_new_hexastore_with_store ( void* world, hx_store* store ) {
	hx_hexastore* hx	= (hx_hexastore*) calloc( 1, sizeof( hx_hexastore )  );
	hx->store			= store;
	hx->next_var		= -1;
	return hx;
}

int hx_free_hexastore ( hx_hexastore* hx ) {
	hx_free_store( hx->store );
	free( hx );
	return 0;
}

hx_container_t* hx_get_indexes ( hx_hexastore* hx ) {
	if (hx->indexes == NULL) {
		hx->indexes	= hx_new_container( 'I', 6 );
// 		if (hx->spo)
// 			hx_container_push_item( hx->indexes, hx->spo );
// 		if (hx->sop)
// 			hx_container_push_item( hx->indexes, hx->sop );
// 		if (hx->pso)
// 			hx_container_push_item( hx->indexes, hx->pso );
// 		if (hx->pos)
// 			hx_container_push_item( hx->indexes, hx->pos );
// 		if (hx->osp)
// 			hx_container_push_item( hx->indexes, hx->osp );
// 		if (hx->ops)
// 			hx_container_push_item( hx->indexes, hx->ops );
	}
	return hx->indexes;
}

int hx_add_triple( hx_hexastore* hx, hx_node* sn, hx_node* pn, hx_node* on ) {
	hx_triple* t	= hx_new_triple( sn, pn, on );
	int r	= hx_store_add_triple( hx->store, t );
	hx_free_triple( t );
	return r;
}

int hx_remove_triple( hx_hexastore* hx, hx_node* sn, hx_node* pn, hx_node* on ) {
	hx_triple* t	= hx_new_triple( sn, pn, on );
	int r	= hx_store_remove_triple( hx->store, t );
	hx_free_triple( t );
	return r;
}

uint64_t hx_count_statements( hx_hexastore* hx, hx_node* s, hx_node* p, hx_node* o ) {
	hx_triple* t	= hx_new_triple( s, p, o );
	uint64_t count	= hx_store_count( hx->store, t );
	hx_free_triple( t );
	return count;
}

uint64_t hx_triples_count ( hx_hexastore* hx ) {
	return hx_store_size( hx->store );
}

hx_variablebindings_iter* hx_new_variablebindings_iter_for_triple ( hx_hexastore* hx, hx_triple* t, hx_node_position_t sort_position ) {
	hx_node* sort_node	= hx_triple_node( t, sort_position );
	if (!hx_node_is_variable(sort_node)) {
		sort_node	= NULL;
	}
	return hx_store_get_statements( hx->store, t, sort_node );
}

hx_node* hx_new_variable ( hx_hexastore* hx ) {
	int v	= hx->next_var--;
	hx_node* n	= hx_new_node_variable( v );
	return n;
}

hx_node* hx_new_named_variable ( hx_hexastore* hx, char* name ) {
	int v	= hx->next_var--;
	hx_node* n	= hx_new_node_named_variable( v, name );
	return n;
}

int hx_debug ( hx_hexastore* hx ) {
	hx_node* s		= hx_new_named_variable( hx, "subj" );
	hx_node* p		= hx_new_named_variable( hx, "pred" );
	hx_node* o		= hx_new_named_variable( hx, "obj" );
	hx_triple* t	= hx_new_triple( s, p, o );
	hx_variablebindings_iter* iter	= hx_new_variablebindings_iter_for_triple( hx, t, HX_SUBJECT );
	int counter	= 0;
	fprintf( stderr, "--------------------\n" );
	while (!hx_variablebindings_iter_finished( iter )) {
		counter++;
		hx_variablebindings* b;
		hx_variablebindings_iter_current( iter, &b );
		hx_store_variablebindings_debug( hx->store, b );
		hx_variablebindings_iter_next( iter );
	}
	fprintf( stderr, "%d triples ---------\n", counter );
	hx_free_variablebindings_iter(iter);
	hx_free_triple(t);
	hx_free_node(s);
	hx_free_node(p);
	hx_free_node(o);
	return 0;
}
