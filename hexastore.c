#include "hexastore.h"
#include "store/hexastore/hexastore.h"

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
	c->world	= world;
	c->hx		= hx;
	c->unsorted_mergejoin_penalty	= 2;
	c->hashjoin_penalty				= 1;
	c->nestedloopjoin_penalty		= 3;
	return c;
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

hx_nodemap* hx_get_nodemap ( hx_hexastore* hx ) {
	return hx_store_hexastore_get_nodemap( hx->store );
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
	uint64_t count	= hx_store_triple_count( hx->store, t );
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
	hx_nodemap* map	= hx_get_nodemap( hx );
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
		hx_variablebindings_debug( b, map );
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
