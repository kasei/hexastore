#include "mentok/store/hexastore/hexastore.h"
#include "mentok/mentok.h"

hx_node_id _hx_store_hexastore_get_node_id ( hx_store_hexastore* hx, hx_node* node );
int _hx_store_hexastore_get_ordered_index( hx_store_hexastore* hx, hx_node* sn, hx_node* pn, hx_node* on, hx_node_position_t order_position, hx_store_hexastore_index** index, hx_node** nodes, int* var_count );
int _hx_store_hexastore_iter_vb_finished ( void* data );
int _hx_store_hexastore_iter_vb_current ( void* data, void* results );
int _hx_store_hexastore_iter_vb_next ( void* data );
int _hx_store_hexastore_iter_vb_free ( void* data );
int _hx_store_hexastore_iter_vb_size ( void* data );
char** _hx_store_hexastore_iter_vb_names ( void* data );
int _hx_store_hexastore_iter_vb_sorted_by (void* data, int index );
int _hx_store_hexastore_iter_vb_iter_debug ( void* data, char* header, int indent );
int _hx_store_hexastore_index_ok_for_triple( hx_store* store, hx_store_hexastore_index* idx, hx_triple* t );

int _hx_store_hexastore_add_triple_id ( hx_store_hexastore* hx, hx_node_id s, hx_node_id p, hx_node_id o );

hx_store* _hx_new_store_hexastore_with_hx ( void* world, void* hx ) {
	hx_store_vtable* vtable				= (hx_store_vtable*) calloc( 1, sizeof(hx_store_vtable) );
// 	vtable->init						= hx_store_hexastore_init;
	vtable->close						= hx_store_hexastore_close;
	vtable->size						= hx_store_hexastore_size;
	vtable->count						= hx_store_hexastore_count;
	vtable->add_triple					= hx_store_hexastore_add_triple;
	vtable->remove_triple				= hx_store_hexastore_remove_triple;
	vtable->contains_triple				= hx_store_hexastore_contains_triple;
	vtable->get_statements				= hx_store_hexastore_get_statements;
	vtable->get_statements_with_index	= hx_store_hexastore_get_statements_with_index;
	vtable->sync						= hx_store_hexastore_sync;
	vtable->triple_orderings			= hx_store_hexastore_triple_orderings;
	vtable->id2node						= hx_store_hexastore_id2node;
	vtable->node2id						= hx_store_hexastore_node2id;
	vtable->ordering_name				= hx_store_hexastore_ordering_name;
	vtable->iter_sorting				= hx_store_hexastore_iter_sorting;
	return hx_new_store( world, vtable, hx );
}

hx_store* hx_new_store_hexastore_with_nodemap ( void* world, hx_nodemap* map ) {
	hx_store_hexastore* hx	= (hx_store_hexastore*) calloc( 1, sizeof(hx_store_hexastore) );
	hx->map					= map;
	hx->spo					= hx_new_index( world, HX_INDEX_ORDER_SPO );
	hx->sop					= hx_new_index( world, HX_INDEX_ORDER_SOP );
	hx->pso					= hx_new_index( world, HX_INDEX_ORDER_PSO );
	hx->pos					= hx_new_index( world, HX_INDEX_ORDER_POS );
	hx->osp					= hx_new_index( world, HX_INDEX_ORDER_OSP );
	hx->ops					= hx_new_index( world, HX_INDEX_ORDER_OPS );
	return _hx_new_store_hexastore_with_hx( world, hx );
}

hx_store* hx_new_store_hexastore ( void* world ) {
	hx_nodemap* map			= hx_new_nodemap();
	return hx_new_store_hexastore_with_nodemap( world, map );
}

hx_store* hx_new_store_hexastore_with_indexes ( void* world, const char* index_string ) {
	hx_store_hexastore* hx	= (hx_store_hexastore*) calloc( 1, sizeof(hx_store_hexastore) );
	hx->map					= hx_new_nodemap();
	if (strcasestr(index_string, "spo")) {
// 		fprintf( stderr, "Adding SPO index\n" );
		hx->spo					= hx_new_index( world, HX_INDEX_ORDER_SPO );
	}
	if (strcasestr(index_string, "sop")) {
// 		fprintf( stderr, "Adding SOP index\n" );
		hx->sop					= hx_new_index( world, HX_INDEX_ORDER_SOP );
	}
	if (strcasestr(index_string, "pso")) {
// 		fprintf( stderr, "Adding PSO index\n" );
		hx->pso					= hx_new_index( world, HX_INDEX_ORDER_PSO );
	}
	if (strcasestr(index_string, "pos")) {
// 		fprintf( stderr, "Adding POS index\n" );
		hx->pos					= hx_new_index( world, HX_INDEX_ORDER_POS );
	}
	if (strcasestr(index_string, "osp")) {
// 		fprintf( stderr, "Adding OSP index\n" );
		hx->osp					= hx_new_index( world, HX_INDEX_ORDER_OSP );
	}
	if (strcasestr(index_string, "ops")) {
// 		fprintf( stderr, "Adding OPS index\n" );
		hx->ops					= hx_new_index( world, HX_INDEX_ORDER_OPS );
	}
	return _hx_new_store_hexastore_with_hx( world, hx );
}

hx_nodemap* hx_store_hexastore_get_nodemap ( hx_store* store ) {
	hx_store_hexastore* hx	= (hx_store_hexastore*) store->ptr;
	return hx->map;
}

int hx_store_hexastore_debug (hx_store* store) {
	hx_store_hexastore* hx	= (hx_store_hexastore*) store->ptr;
	hx_store_hexastore_index_debug( hx->spo );
	return 0;
}



/* ************************************************************************** */

/* Close storage/model context */
int hx_store_hexastore_close (hx_store* store) {
	hx_store_hexastore* hx	= (hx_store_hexastore*) store->ptr;
	hx_free_nodemap( hx->map );
	if (hx->spo)
		hx_free_index( hx->spo );
	if (hx->sop)
		hx_free_index( hx->sop );
	if (hx->pso)
		hx_free_index( hx->pso );
	if (hx->pos)
		hx_free_index( hx->pos );
	if (hx->osp)
		hx_free_index( hx->osp );
	if (hx->ops)
		hx_free_index( hx->ops );
	hx->map			= NULL;
	hx->spo			= NULL;
	hx->sop			= NULL;
	hx->pso			= NULL;
	hx->pos			= NULL;
	hx->osp			= NULL;
	hx->ops			= NULL;
	free( hx );
	return 0;
}

/* Return the number of triples in the storage for model */
uint64_t hx_store_hexastore_size (hx_store* store) {
	hx_store_hexastore* hx	= (hx_store_hexastore*) store->ptr;
	hx_store_hexastore_index* i	= hx->spo;	// XXX what if spo doesn't exist. use any available index
	return hx_store_hexastore_index_triples_count( i );
}

/* Return the number of triples matching a triple pattern */
uint64_t hx_store_hexastore_count (hx_store* store, hx_triple* triple) {
	hx_store_hexastore* hx	= (hx_store_hexastore*) store->ptr;
	hx_node* s	= triple->subject;
	hx_node* p	= triple->predicate;
	hx_node* o	= triple->object;
	
	int vars;
	hx_store_hexastore_index* index;
	hx_node* index_ordered[3];
	_hx_store_hexastore_get_ordered_index( hx, s, p, o, HX_SUBJECT, &index, index_ordered, &vars );

	hx_node_id aid	= _hx_store_hexastore_get_node_id( hx, index_ordered[0] );
	hx_node_id bid	= _hx_store_hexastore_get_node_id( hx, index_ordered[1] );
	hx_node_id cid	= _hx_store_hexastore_get_node_id( hx, index_ordered[2] );
	if (aid == 0 || bid == 0 || cid == 0) {
		return (uint64_t) 0;
	}
	hx_node_id index_ordered_id[3]	= { aid, bid, cid };
	
	uint64_t size;
	hx_head* head;
	hx_vector* vector;
	hx_terminal* terminal;
	switch (vars) {
		case 3:
			return hx_store_hexastore_size( store );
		case 2:
			head	= hx_store_hexastore_index_head( index );
			if (head == NULL) {
// 				fprintf( stderr, "*** Did not find the head pointer in hx_model_count_statements with %d vars\n", vars );
				return (uint64_t) 0;
			}
			vector	= hx_head_get_vector( head, index_ordered_id[0] );
			if (vector == NULL) {
//				fprintf( stderr, "*** Did not find the vector pointer in hx_model_count_statements with %d vars\n", vars );
				return (uint64_t) 0;
			}
			size	= hx_vector_triples_count( vector );
			return size;
			break;
		case 1:
			head	= hx_store_hexastore_index_head( index );
			if (head == NULL) {
//				fprintf( stderr, "*** Did not find the head pointer in hx_model_count_statements with %d vars\n", vars );
				return (uint64_t) 0;
			}
			vector	= hx_head_get_vector( head, index_ordered_id[0] );
			if (vector == NULL) {
//				fprintf( stderr, "*** Did not find the vector pointer in hx_model_count_statements with %d vars\n", vars );
				return (uint64_t) 0;
			}
			terminal	= hx_vector_get_terminal( vector, index_ordered_id[1] );
			if (terminal == NULL) {
//				fprintf( stderr, "*** Did not find the terminal pointer in hx_model_count_statements with %d vars\n", vars );
				return (uint64_t) 0;
			}
			size	= (uint64_t) hx_terminal_size( terminal );
			return size;
		case 0:
			return (uint64_t) hx_store_hexastore_contains_triple( store, triple );
	};
	
	return (uint64_t) 0;
}

/* Add a triple to the storage from the given model */
int hx_store_hexastore_add_triple (hx_store* store, hx_triple* triple) {
	hx_store_hexastore* hx	= (hx_store_hexastore*) store->ptr;
	hx_nodemap* map	= hx->map;
	hx_node_id s	= hx_nodemap_add_node( map, triple->subject );
	hx_node_id p	= hx_nodemap_add_node( map, triple->predicate );
	hx_node_id o	= hx_nodemap_add_node( map, triple->object );
	return _hx_store_hexastore_add_triple_id( hx, s, p, o );
	
}

/* Remove a triple from the storage */
int hx_store_hexastore_remove_triple (hx_store* store, hx_triple* triple) {
	hx_store_hexastore* hx	= (hx_store_hexastore*) store->ptr;
	hx_node_id s	= _hx_store_hexastore_get_node_id( hx, triple->subject );
	hx_node_id p	= _hx_store_hexastore_get_node_id( hx, triple->predicate );
	hx_node_id o	= _hx_store_hexastore_get_node_id( hx, triple->object );
	if (hx->spo) {
		hx_store_hexastore_index_remove_triple( hx->spo, s, p, o );
	}
	if (hx->sop) {
		hx_store_hexastore_index_remove_triple( hx->sop, s, p, o );
	}
	if (hx->pso) {
		hx_store_hexastore_index_remove_triple( hx->pso, s, p, o );
	}
	if (hx->pos) {
		hx_store_hexastore_index_remove_triple( hx->pos, s, p, o );
	}
	if (hx->osp) {
		hx_store_hexastore_index_remove_triple( hx->osp, s, p, o );
	}
	if (hx->ops) {
		hx_store_hexastore_index_remove_triple( hx->ops, s, p, o );
	}
	return 0;
}

/* Check if triple is in storage */
int hx_store_hexastore_contains_triple (hx_store* store, hx_triple* triple) {
	hx_variablebindings_iter* iter	= hx_store_hexastore_get_statements( store, triple, NULL );
	if (iter) {
		if (!hx_variablebindings_iter_finished(iter)) {
			hx_free_variablebindings_iter(iter);
			return (uint64_t) 1;
		}
		hx_free_variablebindings_iter(iter);
	}
	return (uint64_t) 0;
}

/* Return a stream of triples matching a triple pattern */
hx_variablebindings_iter* hx_store_hexastore_get_statements (hx_store* store, hx_triple* triple, hx_node* sort_variable) {
	hx_store_hexastore* hx	= (hx_store_hexastore*) store->ptr;
	hx_node* index_ordered[3];
	hx_store_hexastore_index* index;
	
	hx_node_position_t order_position	= HX_SUBJECT;
	if (sort_variable != NULL) {
		int i;
		for (i = 0; i < 3; i++) {
			hx_node* n	= hx_triple_node( triple, i );
			if (hx_node_cmp(n, sort_variable) == 0) {
				order_position	= i;
				break;
			}
		}
	}
	
	_hx_store_hexastore_get_ordered_index( hx, triple->subject, triple->predicate, triple->object, order_position, &index, index_ordered, NULL );
	return hx_store_hexastore_get_statements_with_index( store, triple, index );
}

/* Return a stream of triples matching a triple pattern with a specific index thunk (originating from the triple_orderings function) */
hx_variablebindings_iter* hx_store_hexastore_get_statements_with_index (hx_store* store, hx_triple* triple, hx_store_hexastore_index* index) {
	hx_store_hexastore* hx	= (hx_store_hexastore*) store->ptr;
	hx_node_id s	= _hx_store_hexastore_get_node_id( hx, triple->subject );
	hx_node_id p	= _hx_store_hexastore_get_node_id( hx, triple->predicate );
	hx_node_id o	= _hx_store_hexastore_get_node_id( hx, triple->object );

	if (!hx_node_is_variable( triple->subject ) && s == 0) {
		return NULL;
	}
	if (!hx_node_is_variable( triple->predicate ) && p == 0) {
		return NULL;
	}
	if (!hx_node_is_variable( triple->object ) && o == 0) {
		return NULL;
	}
	
	hx_node_id index_ordered_id[3]	= { s, p, o };
	hx_store_hexastore_index_iter* iter	= hx_store_hexastore_index_new_iter1( index, index_ordered_id[0], index_ordered_id[1], index_ordered_id[2] );
	
	char* subj_name	= NULL;
	char* pred_name	= NULL;
	char* obj_name	= NULL;
	if (hx_node_is_variable(triple->subject)) {
		hx_node_variable_name( triple->subject, &subj_name );
	}
	if (hx_node_is_variable(triple->predicate)) {
		hx_node_variable_name( triple->predicate, &pred_name );
	}
	if (hx_node_is_variable(triple->object)) {
		hx_node_variable_name( triple->object, &obj_name );
	}
	
	hx_variablebindings_iter_vtable* vtable	= (hx_variablebindings_iter_vtable*) calloc( 1, sizeof( hx_variablebindings_iter_vtable ) );
	vtable->finished	= _hx_store_hexastore_iter_vb_finished;
	vtable->current		= _hx_store_hexastore_iter_vb_current;
	vtable->next		= _hx_store_hexastore_iter_vb_next;
	vtable->free		= _hx_store_hexastore_iter_vb_free;
	vtable->names		= _hx_store_hexastore_iter_vb_names;
	vtable->size		= _hx_store_hexastore_iter_vb_size;
	vtable->sorted_by_index	= _hx_store_hexastore_iter_vb_sorted_by;
	vtable->debug		= _hx_store_hexastore_iter_vb_iter_debug;
	
	int size	= 0;
	if (subj_name != NULL)
		size++;
	if (pred_name != NULL)
		size++;
	if (obj_name != NULL)
		size++;
	
	_hx_store_hexastore_iter_vb_info* info			= (_hx_store_hexastore_iter_vb_info*) calloc( 1, sizeof( _hx_store_hexastore_iter_vb_info ) );
	info->size						= size;
	
	if (subj_name != NULL) {
		info->subject	= subj_name;
	}
	
	if (pred_name != NULL) {
		info->predicate	= pred_name;
	}
	
	if (obj_name != NULL) {
		info->object	= obj_name;
	}
	
	info->iter						= iter;
	info->names						= (char**) calloc( 3, sizeof( char* ) );
	info->triple_pos_to_index		= (int*) calloc( 3, sizeof( int ) );
	info->index_to_triple_pos		= (int*) calloc( 3, sizeof( int ) );
	info->current					= NULL;
	info->store						= store;
	
	int j	= 0;
	if (subj_name != NULL) {
		int idx	= j++;
		info->names[ idx ]		= info->subject;
		info->triple_pos_to_index[ idx ]	= 0;
		info->index_to_triple_pos[ 0 ]		= idx;
	}
	
	if (pred_name != NULL) {
		int idx	= j++;
		info->names[ idx ]		= info->predicate;
		info->triple_pos_to_index[ idx ]	= 1;
		info->index_to_triple_pos[ 1 ]		= idx;
	}
	if (obj_name != NULL) {
		int idx	= j++;
		info->names[ idx ]		= info->object;
		info->triple_pos_to_index[ idx ]	= 2;
		info->index_to_triple_pos[ 2 ]		= idx;
	}
	
	hx_variablebindings_iter* vbiter	= hx_variablebindings_new_iter( vtable, (void*) info );
	return vbiter;
}

/* Synchronise to underlying storage */
int hx_store_hexastore_sync (hx_store* store) {
	return 0;
}

/* Return a list of ordering arrays, giving the possible access patterns for the given triple */
hx_container_t* hx_store_hexastore_triple_orderings (hx_store* store, hx_triple* t) {
	hx_store_hexastore* hx	= (hx_store_hexastore*) store->ptr;
	hx_container_t* indexes	= hx_new_container('I', 6);
	if (hx->spo && _hx_store_hexastore_index_ok_for_triple(store, hx->spo, t))
		hx_container_push_item(indexes, hx->spo);
	if (hx->sop && _hx_store_hexastore_index_ok_for_triple(store, hx->sop, t))
		hx_container_push_item(indexes, hx->sop);
	if (hx->pos && _hx_store_hexastore_index_ok_for_triple(store, hx->pos, t))
		hx_container_push_item(indexes, hx->pos);
	if (hx->pso && _hx_store_hexastore_index_ok_for_triple(store, hx->pso, t))
		hx_container_push_item(indexes, hx->pso);
	if (hx->osp && _hx_store_hexastore_index_ok_for_triple(store, hx->osp, t))
		hx_container_push_item(indexes, hx->osp);
	if (hx->ops && _hx_store_hexastore_index_ok_for_triple(store, hx->ops, t))
		hx_container_push_item(indexes, hx->ops);
	return indexes;
}

int _hx_store_hexastore_index_ok_for_triple( hx_store* store, hx_store_hexastore_index* idx, hx_triple* t ) {
	if (t == NULL) {
		return 1;
	}
	int i, j;
	int bound	= hx_triple_bound_count(t);
	int repeated_variable	= 0;
	int bound_nodes[3]	= {1,1,1};
	if (bound == 3) {
	} else {
		for (i = 0; i < 3; i++) {
			hx_node* n	= hx_triple_node( t, i );
			if (hx_node_is_variable(n)) {
// 				fprintf( stderr, "- %s is unbound\n", HX_POSITION_NAMES[i] );
				bound_nodes[i]	= 0;
				// scan through the other node positions, checking for repeated variables
				for (j = i+1; j < 3; j++) {
					hx_node* m	= hx_triple_node( t, j );
					if (hx_node_is_variable(m)) {
						if (hx_node_iv(n) == hx_node_iv(m)) {
							repeated_variable	= hx_node_iv(n);
						}
					}
				}
			}
		}
	}
	
// 	if (repeated_variable) {
// 		fprintf( stderr, "triple has a shared variable\n" );
// 	}
// 	
// 	for (i = 0; i < 3; i++) {
// 		if (bound_nodes[i]) {
// 			fprintf( stderr, "prefix of index must have %s\n", HX_POSITION_NAMES[i] );
// 		}
// 	}
	
	char* name	= hx_store_ordering_name( store, idx );
//	fprintf( stderr, "store has index %s (%p)\n", name, (void*) idx );
		
	int index_is_ok	= 1;
	for (j = 0; j < bound; j++) {
		if (!bound_nodes[ idx->order[j] ]) {
			index_is_ok	= 0;
//				fprintf( stderr, "- won't work because %s comes before some bound terms\n", HX_POSITION_NAMES[ idx->order[j] ] );
		}
	}
	
	if (bound == 0) {
		// there are 3 variables. if we have shared variables, they must appear sequentially, and as a prefix of the index order
		// (the nodes in index->order[0] and index->order[1] have to be the shared variable)
		if (repeated_variable) {
			hx_node* first	= hx_triple_node( t, idx->order[0] );
			hx_node* middle	= hx_triple_node( t, idx->order[1] );
			
// 			char* string;
// 			hx_node_string( middle, &string );
// 			fprintf( stderr, "middle-of-index node: %s (%d)\n", string, hx_node_iv(middle) );
// 			free(string);
//			fprintf( stderr, "repeated variable: %d\n", repeated_variable );
			
			if (hx_node_iv(first) != repeated_variable) {
// 					fprintf( stderr, "- %s won't work because the %s isn't the shared variable\n", name, HX_POSITION_NAMES[ idx->order[0] ] );
				index_is_ok	= 0;
			}
			if (hx_node_iv(middle) != repeated_variable) {
// 					fprintf( stderr, "- %s won't work because the %s isn't the shared variable\n", name, HX_POSITION_NAMES[ idx->order[1] ] );
				index_is_ok	= 0;
			}
		}
	}
	return index_is_ok;
}

hx_container_t* hx_store_hexastore_iter_sorting ( hx_store* storage, hx_triple* t, void* ordering ) {
	hx_store_hexastore_index* idx	= (hx_store_hexastore_index*) ordering;
	int bound	= hx_triple_bound_count(t);
	int order_count	= 3 - bound;
	
	hx_container_t* order	= hx_new_container( 'S', order_count );
	
	int k;
	int l	= 0;
	for (k = bound; k < 3; k++) {
		int ordered_by	= idx->order[ k ];
		hx_node* n	= hx_triple_node( t, ordered_by );
		char* string;
		hx_node_string( n, &string );
// 		fprintf( stderr, "\tSorted by %s (%s)\n", HX_POSITION_NAMES[ ordered_by ], string );
		free(string);
		
		hx_variablebindings_iter_sorting* sorting	= hx_variablebindings_iter_new_node_sorting( HX_VARIABLEBINDINGS_ITER_SORT_ASCENDING, 0, n );
		hx_container_push_item( order, sorting );
	}
	return order;
}

char* hx_store_hexastore_ordering_name (hx_store* store, void* ordering) {
	return hx_store_hexastore_index_name( ordering );
}

/* Return an ID value for a node. */
hx_node_id hx_store_hexastore_node2id (hx_store* store, hx_node* node) {
	hx_nodemap* map	= hx_store_hexastore_get_nodemap( store );
	return hx_nodemap_get_node_id( map, node );
}

/* Return a node object for an ID. Caller is responsible for freeing the node. */
hx_node* hx_store_hexastore_id2node (hx_store* store, hx_node_id id) {
	hx_nodemap* map	= hx_store_hexastore_get_nodemap( store );
	return hx_nodemap_get_node( map, id );
}




/* ************************************************************************** */

int _hx_store_hexastore_iter_vb_finished ( void* data ) {
	_hx_store_hexastore_iter_vb_info* info	= (_hx_store_hexastore_iter_vb_info*) data;
	hx_store_hexastore_index_iter* iter		= (hx_store_hexastore_index_iter*) info->iter;
	return hx_store_hexastore_index_iter_finished( iter );
}

int _hx_store_hexastore_iter_vb_current ( void* data, void* results ) {
	_hx_store_hexastore_iter_vb_info* info	= (_hx_store_hexastore_iter_vb_info*) data;
	hx_variablebindings** bindings	= (hx_variablebindings**) results;
	if (info->current == NULL) {
		hx_store_hexastore_index_iter* iter		= (hx_store_hexastore_index_iter*) info->iter;
		hx_node_id triple[3];
		hx_store_hexastore_index_iter_current ( iter, &(triple[0]), &(triple[1]), &(triple[2]) );
		hx_node_id* values	= (hx_node_id*) calloc( info->size, sizeof( hx_node_id ) );
		int i;
		for (i = 0; i < info->size; i++) {
			values[ i ]	= triple[ info->triple_pos_to_index[ i ] ];
		}
		info->current	= hx_model_new_variablebindings( info->size, info->names, values );
	}
	*bindings	= hx_copy_variablebindings( info->current );
	return 0;
}

int _hx_store_hexastore_iter_vb_next ( void* data ) {
	_hx_store_hexastore_iter_vb_info* info	= (_hx_store_hexastore_iter_vb_info*) data;
	hx_store_hexastore_index_iter* iter		= (hx_store_hexastore_index_iter*) info->iter;
	if (hx_store_hexastore_index_iter_finished(iter)) {
		return 1;
	}
	if (info->current) {
		hx_free_variablebindings( info->current );
	}
	info->current			= NULL;
	return hx_store_hexastore_index_iter_next( iter );
}

int _hx_store_hexastore_iter_vb_free ( void* data ) {
	_hx_store_hexastore_iter_vb_info* info	= (_hx_store_hexastore_iter_vb_info*) data;
	hx_store_hexastore_index_iter* iter		= (hx_store_hexastore_index_iter*) info->iter;
	hx_free_index_iter( iter );
	if (info->current != NULL) {
		hx_free_variablebindings( info->current );
	}
	free( info->names );
	free( info->triple_pos_to_index );
	free( info->index_to_triple_pos );
	if (info->subject)
		free( info->subject );
	if (info->predicate)
		free( info->predicate );
	if (info->object)
		free( info->object );
	free( info );
	return 0;
}

int _hx_store_hexastore_iter_vb_size ( void* data ) {
	_hx_store_hexastore_iter_vb_info* info	= (_hx_store_hexastore_iter_vb_info*) data;
	return info->size;
}

char** _hx_store_hexastore_iter_vb_names ( void* data ) {
	_hx_store_hexastore_iter_vb_info* info	= (_hx_store_hexastore_iter_vb_info*) data;
	return info->names;
}

int _hx_store_hexastore_iter_vb_sorted_by (void* data, int index ) {
	_hx_store_hexastore_iter_vb_info* info	= (_hx_store_hexastore_iter_vb_info*) data;
	int triple_pos	= info->index_to_triple_pos[ index ];
// 	fprintf( stderr, "*** checking if index iterator is sorted by %d (triple %s)\n", index, HX_POSITION_NAMES[triple_pos] );
	return hx_store_hexastore_index_iter_is_sorted_by_index( info->iter, triple_pos );
}

int _hx_store_hexastore_iter_vb_iter_debug ( void* data, char* header, int indent ) {
	_hx_store_hexastore_iter_vb_info* info	= (_hx_store_hexastore_iter_vb_info*) data;
	int i;
	for (i = 0; i < indent; i++) fwrite( " ", sizeof( char ), 1, stderr );
	hx_store_hexastore_index_iter* iter	= info->iter;
	fprintf( stderr, "%s hexastore triples iterator (iter=%p, store=%p)\n", header, (void*) iter, (void*) info->store );
	int counter	= 0;
	fprintf( stderr, "%s ------------------------\n", header );
	while (!hx_store_hexastore_index_iter_finished( iter )) {
		counter++;
		hx_node_id triple[3];
		hx_store_hexastore_index_iter_current( iter, &(triple[0]), &(triple[1]), &(triple[2]) );
		fprintf( stderr, "%s triple: { %d %d %d }\n", header, (int) triple[0], (int) triple[1], (int) triple[2] );
		hx_store_hexastore_index_iter_next( iter );
	}
	fprintf( stderr, "%s --- %5d triples ------\n", header, counter );
	
	return 0;
}

int _hx_store_hexastore_add_triple_id ( hx_store_hexastore* hx, hx_node_id s, hx_node_id p, hx_node_id o ) {
	hx_terminal* t;
	if (hx->spo && hx->pso) {
// 		fprintf( stderr, "using shared terminal between SPO and PSO indexes\n" );
		int added	= hx_store_hexastore_index_add_triple_terminal( hx->spo, s, p, o, &t );
		hx_store_hexastore_index_add_triple_with_terminal( hx->pso, t, s, p, o, added );
	} else {
		if (hx->spo) hx_store_hexastore_index_add_triple( hx->spo, s, p, o );
		if (hx->pso) hx_store_hexastore_index_add_triple( hx->pso, s, p, o );
	}
	

	if (hx->osp && hx->sop) {
// 		fprintf( stderr, "using shared terminal between OSP and SOP indexes\n" );
		int added	= hx_store_hexastore_index_add_triple_terminal( hx->osp, s, p, o, &t );
		hx_store_hexastore_index_add_triple_with_terminal( hx->sop, t, s, p, o, added );
	} else {
		if (hx->osp) hx_store_hexastore_index_add_triple( hx->osp, s, p, o );
		if (hx->sop) hx_store_hexastore_index_add_triple( hx->sop, s, p, o );
	}
	
	if (hx->pos && hx->ops) {
// 		fprintf( stderr, "using shared terminal between POS and OPS indexes\n" );
		int added	= hx_store_hexastore_index_add_triple_terminal( hx->pos, s, p, o, &t );
		hx_store_hexastore_index_add_triple_with_terminal( hx->ops, t, s, p, o, added );
	} else {
		if (hx->pos) hx_store_hexastore_index_add_triple( hx->pos, s, p, o );
		if (hx->ops) hx_store_hexastore_index_add_triple( hx->ops, s, p, o );
	}
	
	return 0;
}

hx_node_id _hx_store_hexastore_get_node_id ( hx_store_hexastore* hx, hx_node* node ) {
	if (node == NULL) {
		return (hx_node_id) 0;
	}
	hx_node_id id;
	hx_nodemap* map	= hx->map;
	return hx_nodemap_get_node_id( map, node );
	return id;
}

int _hx_store_hexastore_get_ordered_index( hx_store_hexastore* hx, hx_node* sn, hx_node* pn, hx_node* on, hx_node_position_t order_position, hx_store_hexastore_index** index, hx_node** nodes, int* var_count ) {
	hx_node_id s	= _hx_store_hexastore_get_node_id( hx, sn );
	hx_node_id p	= _hx_store_hexastore_get_node_id( hx, pn );
	hx_node_id o	= _hx_store_hexastore_get_node_id( hx, on );
	if (!hx_node_is_variable( sn ) && s == 0) {
		return 1;
	}
	if (!hx_node_is_variable( pn ) && p == 0) {
		return 1;
	}
	if (!hx_node_is_variable( on ) && o == 0) {
		return 1;
	}
	
	int i		= 0;
	int vars	= 0;
	
#ifdef DEBUG_INDEX_SELECTION
	fprintf( stderr, "Determining appropriate index to use for triple pattern { %"PRIdHXID" %"PRIdHXID" %"PRIdHXID" }\n", s, p, o );
	if (hx->spo) fprintf( stderr, "SPO index is present...\n" );
	if (hx->sop) fprintf( stderr, "SOP index is present...\n" );
	if (hx->pos) fprintf( stderr, "POS index is present...\n" );
	if (hx->pso) fprintf( stderr, "PSO index is present...\n" );
	if (hx->osp) fprintf( stderr, "OSP index is present...\n" );
	if (hx->ops) fprintf( stderr, "OPS index is present...\n" );
	
	const char* pnames[3]	= { "SUBJECT", "PREDICATE", "OBJECT" };
	fprintf( stderr, "triple: { %d, %d, %d }\n", (int) s, (int) p, (int) o );
#endif
	int used[3]	= { 0, 0, 0 };
	hx_node_id triple_id[3]	= { s, p, o };
//	hx_node* triple[3]		= { sn, pn, on };
	int index_order[3]		= { 0xdeadbeef, 0xdeadbeef, 0xdeadbeef };
	
	if (s > (hx_node_id) 0) {
#ifdef DEBUG_INDEX_SELECTION
		fprintf( stderr, "- bound subject\n" );
#endif
		index_order[ i++ ]	= HX_SUBJECT;
		used[ HX_SUBJECT ]++;
	} else if (s < (hx_node_id) 0) {
		vars++;
	}
	if (p > (hx_node_id) 0) {
#ifdef DEBUG_INDEX_SELECTION
		fprintf( stderr, "- bound predicate\n" );
#endif
		index_order[ i++ ]	= HX_PREDICATE;
		used[ HX_PREDICATE ]++;
	} else if (p < (hx_node_id) 0) {
		vars++;
	}
	if (o > (hx_node_id) 0) {
#ifdef DEBUG_INDEX_SELECTION
		fprintf( stderr, "- bound object\n" );
#endif
		index_order[ i++ ]	= HX_OBJECT;
		used[ HX_OBJECT ]++;
	} else if (o < (hx_node_id) 0) {
		vars++;
	}
	
	if (var_count != NULL) {
		*var_count	= vars;
	}
	
#ifdef DEBUG_INDEX_SELECTION
	fprintf( stderr, "index order: { %d, %d, %d }\n", (int) index_order[0], (int) index_order[1], (int) index_order[2] );
#endif
	if (i < 3 && !(used[order_position]) && triple_id[order_position] != (hx_node_id) 0) {
#ifdef DEBUG_INDEX_SELECTION
		fprintf( stderr, "requested ordering position: %s\n", pnames[order_position] );
#endif
		index_order[ i++ ]	= order_position;
		used[order_position]++;
	}
#ifdef DEBUG_INDEX_SELECTION
	fprintf( stderr, "index order: { %d, %d, %d }\n", (int) index_order[0], (int) index_order[1], (int) index_order[2] );
#endif	
	// check for any duplicated variables. if they haven't been added to the index order, add them now:
	int j;
	for (j = 0; j < 3; j++) {
		if (!(used[j])) {
			int current_chosen	= i;
//			fprintf( stderr, "checking if %s (%d) matches already chosen nodes:\n", pnames[j], (int) triple_id[j] );
			int k;
			for (k = 0; k < current_chosen; k++) {
//				fprintf( stderr, "- %s (%d)?\n", pnames[k], triple_id[ index_order[k] ] );
				if (triple_id[index_order[k]] == triple_id[j] && triple_id[j] != (hx_node_id) 0) {
//					fprintf( stderr, "*** MATCHED\n" );
					if (i < 3) {
						index_order[ i++ ]	= j;
						used[ j ]++;
					}
				}
			}
		}
	}
	
	// add any remaining triple positions to the index order:
	if (i == 0) {
		int j;
		for (j = 0; j < 3; j++) {
			if (j != order_position) {
				index_order[ i++ ]	= j;
			}
		}
	} else if (i == 1) {
		int j;
		for (j = 0; j < 3; j++) {
			if (j != order_position && !(used[j])) {
				index_order[ i++ ]	= j;
			}
		}
	} else if (i == 2) {
		int j;
		for (j = 0; j < 3; j++) {
			if (!(used[j])) {
				index_order[ i++ ]	= j;
			}
		}
	}
	
	switch (index_order[0]) {
		case 0:
			switch (index_order[1]) {
				case 1:
					if (hx->spo) {
#ifdef DEBUG_INDEX_SELECTION
						fprintf( stderr, "using spo index\n" );
#endif
						*index	= hx->spo;
					} else {
#ifdef DEBUG_INDEX_SELECTION
						fprintf( stderr, "wanted spo index, but using pso...\n" );
#endif
						*index	= hx->pso;
					}
					break;
				case 2:
					if (hx->sop) {
#ifdef DEBUG_INDEX_SELECTION
						fprintf( stderr, "using sop index\n" );
#endif
						*index	= hx->sop;
					} else {
#ifdef DEBUG_INDEX_SELECTION
						fprintf( stderr, "wanted sop index, but using osp...\n" );
#endif
						*index	= hx->osp;
					}
					break;
			}
			break;
		case 1:
			switch (index_order[1]) {
				case 0:
					if (hx->pso) {
#ifdef DEBUG_INDEX_SELECTION
						fprintf( stderr, "using pso index\n" );
#endif
						*index	= hx->pso;
					} else {
#ifdef DEBUG_INDEX_SELECTION
						fprintf( stderr, "wanted pso index, but using spo...\n" );
#endif
						*index	= hx->spo;
					}
					break;
				case 2:
					if (hx->pos) {
#ifdef DEBUG_INDEX_SELECTION
						fprintf( stderr, "using pos index\n" );
#endif
						*index	= hx->pos;
					} else {
#ifdef DEBUG_INDEX_SELECTION
						fprintf( stderr, "wanted pos index, but using ops...\n" );
#endif
						*index	= hx->ops;
					}
					break;
			}
			break;
		case 2:
			switch (index_order[1]) {
				case 0:
					if (hx->osp) {
#ifdef DEBUG_INDEX_SELECTION
						fprintf( stderr, "using osp index\n" );
#endif
						*index	= hx->osp;
					} else {
#ifdef DEBUG_INDEX_SELECTION
						fprintf( stderr, "wanted osp index, but using sop...\n" );
#endif
						*index	= hx->sop;
					}
					break;
				case 1:
					if (hx->ops) {
#ifdef DEBUG_INDEX_SELECTION
						fprintf( stderr, "using ops index\n" );
#endif
						*index	= hx->ops;
					} else {
#ifdef DEBUG_INDEX_SELECTION
						fprintf( stderr, "wanted ops index, but using pos...\n" );
#endif
						*index	= hx->pos;
					}
					break;
			}
			break;
	}
	
	hx_node* triple_ordered[3]	= { sn, pn, on };
	nodes[0]	= triple_ordered[ (*index)->order[0] ];
	nodes[1]	= triple_ordered[ (*index)->order[1] ];
	nodes[2]	= triple_ordered[ (*index)->order[2] ];
	return 0;
}


int hx_store_hexastore_write( hx_store* store, FILE* f ) {
	hx_store_hexastore* h	= (hx_store_hexastore*) store->ptr;
	fputc( 'X', f );
	if (hx_nodemap_write( h->map, f ) != 0) {
		fprintf( stderr, "*** Error while writing hexastore nodemap to disk.\n" );
		return 1;
	}
	
	int cond	= 0;
	
	uint8_t index_count	= 0;
	if (h->spo) index_count++;
	if (h->sop) index_count++;
	if (h->pso) index_count++;
	if (h->pos) index_count++;
	if (h->osp) index_count++;
	if (h->ops) index_count++;
	fprintf( stderr, "Writing %d indexes to disk\n", (int) index_count );
	fwrite( &index_count, sizeof(uint8_t), 1, f );
	
	if (h->spo) {
		fwrite( "spo", 3, 1, f );
		cond		|= hx_store_hexastore_index_write( h->spo, f );
	}
	if (h->sop) {
		fwrite( "sop", 3, 1, f );
		cond		|= hx_store_hexastore_index_write( h->sop, f );
	}
	if (h->pso) {
		fwrite( "pso", 3, 1, f );
		cond		|= hx_store_hexastore_index_write( h->pso, f );
	}
	if (h->pos) {
		fwrite( "pos", 3, 1, f );
		cond		|= hx_store_hexastore_index_write( h->pos, f );
	}
	if (h->osp) {
		fwrite( "osp", 3, 1, f );
		cond		|= hx_store_hexastore_index_write( h->osp, f );
	}
	if (h->ops) {
		fwrite( "ops", 3, 1, f );
		cond		|= hx_store_hexastore_index_write( h->ops, f );
	}
	
	if (cond != 0) {
		fprintf( stderr, "*** Error while writing hexastore indices to disk.\n" );
		return 1;
	} else {
		return 0;
	}
}

hx_store* hx_store_hexastore_read( void* world, FILE* f, int buffer ) {
	int c	= fgetc( f );
	if (c != 'X') {
		fprintf( stderr, "*** Bad header cookie trying to read hexastore from file.\n" );
		return NULL;
	}
	hx_store_hexastore* hx	= (hx_store_hexastore*) calloc( 1, sizeof( hx_store_hexastore )  );
	hx->map	= hx_nodemap_read( f, buffer );
	if (hx->map == NULL) {
		fprintf( stderr, "*** NULL nodemap returned while trying to read hexastore from disk.\n" );
		free( hx );
		return NULL;
	}
	
	uint8_t index_count;
	fread( &index_count, sizeof(uint8_t), 1, f );
	
	int i;
	for (i = 0; i < index_count; i++) {
		char index_name[4];
		index_name[3]	= (char) 0;
		fread( index_name, 3, 1, f );
		
		hx_store_hexastore_index* index	= hx_store_hexastore_index_read( f, buffer );
		if (index == NULL) {
			fprintf( stderr, "*** NULL index returned while trying to read hexastore from disk.\n" );
			free( hx );
			return NULL;
		}
		
		if (strncmp(index_name, "spo", 3) == 0) {
			hx->spo		= index;
		} else if (strncmp(index_name, "sop", 3) == 0) {
			hx->sop		= index;
		} else if (strncmp(index_name, "pso", 3) == 0) {
			hx->pso		= index;
		} else if (strncmp(index_name, "pos", 3) == 0) {
			hx->pos		= index;
		} else if (strncmp(index_name, "osp", 3) == 0) {
			hx->osp		= index;
		} else if (strncmp(index_name, "ops", 3) == 0) {
			hx->ops		= index;
		} else {
			fprintf( stderr, "*** Unrecognized index ordering in hx_store_hexastore_read\n" );
			free(hx);
			return NULL;
		}
	}
	
	return _hx_new_store_hexastore_with_hx( world, hx );
}

