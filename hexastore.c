#include "hexastore.h"

// #define DEBUG_INDEX_SELECTION


void* _hx_add_triple_threaded (void* arg);
int _hx_iter_vb_finished ( void* iter );
int _hx_iter_vb_current ( void* iter, void* results );
int _hx_iter_vb_next ( void* iter );	
int _hx_iter_vb_free ( void* iter );
int _hx_iter_vb_size ( void* iter );
int _hx_iter_vb_sorted_by (void* iter, int index );
int _hx_iter_debug ( void* info, char* header, int indent );

char** _hx_iter_vb_names ( void* iter );


hx_execution_context* hx_new_execution_context ( void* world, hx_hexastore* hx ) {
	hx_execution_context* c	= (hx_execution_context*) calloc( 1, sizeof( hx_execution_context ) );
	c->world	= world;
	c->hx		= hx;
	return c;
}

int hx_free_execution_context ( hx_execution_context* c ) {
	c->world	= NULL;
	c->hx		= NULL;
	free( c );
	return 0;
}

hx_hexastore* hx_new_hexastore ( void* world ) {
	hx_nodemap* map	= hx_new_nodemap();
	return hx_new_hexastore_with_nodemap( world, map );
}

hx_hexastore* hx_new_hexastore_with_nodemap ( void* world, hx_nodemap* map ) {
	hx_hexastore* hx	= (hx_hexastore*) calloc( 1, sizeof( hx_hexastore )  );
	hx->map			= map;
	hx->spo			= ((uintptr_t) hx_new_index( world, HX_INDEX_ORDER_SPO ) );
	hx->sop			= ((uintptr_t) hx_new_index( world, HX_INDEX_ORDER_SOP ) );
	hx->pso			= ((uintptr_t) hx_new_index( world, HX_INDEX_ORDER_PSO ) );
	hx->pos			= ((uintptr_t) hx_new_index( world, HX_INDEX_ORDER_POS ) );
	hx->osp			= ((uintptr_t) hx_new_index( world, HX_INDEX_ORDER_OSP ) );
	hx->ops			= ((uintptr_t) hx_new_index( world, HX_INDEX_ORDER_OPS ) );
	hx->indexes		= NULL;
	hx->next_var	= -1;
	return hx;
}

int hx_free_hexastore ( hx_hexastore* hx ) {
	hx_free_nodemap( hx->map );
	if (hx->spo)
		hx_free_index( (hx_index*) hx->spo );
	if (hx->sop)
		hx_free_index( (hx_index*) hx->sop );
	if (hx->pso)
		hx_free_index( (hx_index*) hx->pso );
	if (hx->pos)
		hx_free_index( (hx_index*) hx->pos );
	if (hx->osp)
		hx_free_index( (hx_index*) hx->osp );
	if (hx->ops)
		hx_free_index( (hx_index*) hx->ops );
	if (hx->indexes)
		hx_free_container( hx->indexes );
	free( hx );
	return 0;
}

hx_nodemap* hx_get_nodemap ( hx_hexastore* hx ) {
	return hx->map;
}

hx_container_t* hx_get_indexes ( hx_hexastore* hx ) {
	if (hx->indexes == NULL) {
		hx->indexes	= hx_new_container( 'I', 6 );
		if (hx->spo)
			hx_container_push_item( hx->indexes, (hx_index*)hx->spo );
		if (hx->sop)
			hx_container_push_item( hx->indexes, (hx_index*)hx->sop );
		if (hx->pso)
			hx_container_push_item( hx->indexes, (hx_index*)hx->pso );
		if (hx->pos)
			hx_container_push_item( hx->indexes, (hx_index*)hx->pos );
		if (hx->osp)
			hx_container_push_item( hx->indexes, (hx_index*)hx->osp );
		if (hx->ops)
			hx_container_push_item( hx->indexes, (hx_index*)hx->ops );
	}
	return hx->indexes;
}

int hx_add_triple( hx_hexastore* hx, hx_node* sn, hx_node* pn, hx_node* on ) {
	hx_nodemap* map	= hx->map;
	hx_node_id s	= hx_nodemap_add_node( map, sn );
	hx_node_id p	= hx_nodemap_add_node( map, pn );
	hx_node_id o	= hx_nodemap_add_node( map, on );
	return hx_add_triple_id( hx, s, p, o );
}

int hx_add_triple_id( hx_hexastore* hx, hx_node_id s, hx_node_id p, hx_node_id o ) {
	hx_terminal* t;
	{
		int added	= hx_index_add_triple_terminal( (hx_index*) hx->spo, s, p, o, &t );
		if (hx->pso) {
			hx_index_add_triple_with_terminal( (hx_index*) hx->pso, t, s, p, o, added );
		}
	}

	{
		int added	= hx_index_add_triple_terminal( (hx_index*) hx->sop, s, p, o, &t );
		if (hx->osp) {
			hx_index_add_triple_with_terminal( (hx_index*) hx->osp, t, s, p, o, added );
		}
	}
	
	{
		int added	= hx_index_add_triple_terminal( (hx_index*) hx->pos, s, p, o, &t );
		if (hx->ops) {
			hx_index_add_triple_with_terminal( (hx_index*) hx->ops, t, s, p, o, added );
		}
	}
	
	return 0;
}

int hx_add_triples( hx_hexastore* hx, hx_triple* triples, int count ) {
#ifdef HAVE_LIBPTHREAD
	if (count < THREADED_BATCH_SIZE) {
#endif
		int i;
		for (i = 0; i < count; i++) {
			hx_add_triple( hx, triples[i].subject, triples[i].predicate, triples[i].object );
		}
#ifdef HAVE_LIBPTHREAD
	} else {
		hx_triple_id* triple_ids	= (hx_triple_id*) calloc( count, sizeof( hx_triple_id ) );
		for (i = 0; i < count; i++) {
			triple_ids[i].subject	= hx_nodemap_add_node( hx->map, triples[i].subject );
			triple_ids[i].predicate	= hx_nodemap_add_node( hx->map, triples[i].predicate );
			triple_ids[i].object	= hx_nodemap_add_node( hx->map, triples[i].object );
		}

		pthread_t* threads		= (pthread_t*) calloc( 6, sizeof( pthread_t ) );
		hx_thread_info* tinfo	= (hx_thread_info*) calloc( 6, sizeof( hx_thread_info ) );
		int thread_count;
#ifdef HX_SHARE_TERMINALS
		thread_count	= 3;
		for (i = 0; i < 3; i++) {
			tinfo[i].s			= s;
			tinfo[i].hx			= hx;
			tinfo[i].count		= count;
			tinfo[i].triples	= triple_ids;
		}
		
		{
			tinfo[0].index		= (hx_index*) hx->spo;
			if (hx->pso) {
				tinfo[0].secondary	= (hx_index*) hx->pso;
			}
			
			tinfo[1].index		= (hx_index*) hx->sop;
			if (hx->osp) {
				tinfo[1].secondary	= (hx_index*) hx->osp;
			}
			
			tinfo[2].index		= (hx_index*) hx->pos;
			if (hx->ops) {
				tinfo[2].secondary	= (hx_index*) hx->ops;
			}
			
			int i;
			for (i = 0; i < 3; i++) {
				pthread_create(&(threads[i]), NULL, _hx_add_triple_threaded, &( tinfo[i] ));
			}
		}
#else
		thread_count	= 6;
		int i;
		for (i = 0; i < 6; i++) {
			tinfo[i].s			= s;
			tinfo[i].hx			= hx;
			tinfo[i].count		= count;
			tinfo[i].triples	= triple_ids;
		}
		
		{
			tinfo[0].index		= (hx_index*) hx->spo;
			tinfo[0].secondary	= NULL;
			
			tinfo[1].index		= (hx_index*) hx->sop;
			tinfo[1].secondary	= NULL;
			
			tinfo[2].index		= (hx_index*) hx->pos;
			tinfo[2].secondary	= NULL;
			
			if (hx->pso) {
				tinfo[3].index		= (hx_index*) hx->pso;
				tinfo[3].secondary	= NULL;
			}
			
			if (hx->osp) {
				tinfo[4].index		= (hx_index*) hx->osp;
				tinfo[4].secondary	= NULL;
			}
			
			if (hx->ops) {
				tinfo[5].index		= (hx_index*) hx->ops;
				tinfo[5].secondary	= NULL;
			}
			
			int i;
			for (i = 0; i < 6; i++) {
				pthread_create(&(threads[i]), NULL, _hx_add_triple_threaded, &( tinfo[i] ));
			}
		}
#endif
		int i;
		for (i = 0; i < thread_count; i++) {
			pthread_join(threads[i], NULL);
		}
		free( tinfo );
		free( threads );
		free( triple_ids );
	}
#endif
	return 0;
}

void* _hx_add_triple_threaded (void* arg) {
	hx_thread_info* tinfo	= (hx_thread_info*) arg;
	int i;
	for (i = 0; i < tinfo->count; i++) {
		hx_node_id s	= tinfo->triples[i].subject;
		hx_node_id p	= tinfo->triples[i].predicate;
		hx_node_id o	= tinfo->triples[i].object;
		hx_terminal* t;
		int added	= hx_index_add_triple_terminal( tinfo->index, s, p, o, &t );
		if (tinfo->secondary != NULL) {
			hx_index_add_triple_with_terminal( tinfo->secondary, t, s, p, o, added );
		}
	}
	return NULL;
}

int hx_remove_triple( hx_hexastore* hx, hx_node* sn, hx_node* pn, hx_node* on ) {
	hx_node_id s	= hx_get_node_id( hx, sn );
	hx_node_id p	= hx_get_node_id( hx, pn );
	hx_node_id o	= hx_get_node_id( hx, on );
	hx_index_remove_triple( (hx_index*) hx->spo, s, p, o );
	hx_index_remove_triple( (hx_index*) hx->sop, s, p, o );
	if (hx->pso) {
		hx_index_remove_triple( (hx_index*) hx->pso, s, p, o );
	}
	hx_index_remove_triple( (hx_index*) hx->pos, s, p, o );
	if (hx->osp) {
		hx_index_remove_triple( (hx_index*) hx->osp, s, p, o );
	}
	if (hx->ops) {
		hx_index_remove_triple( (hx_index*) hx->ops, s, p, o );
	}
	return 0;
}

int _hx_get_ordered_index( hx_hexastore* hx, hx_node_id s, hx_node_id p, hx_node_id o, int order_position, hx_index** index, hx_node** nodes, int* var_count ) {
	int i		= 0;
	int vars	= 0;
	
#ifdef DEBUG_INDEX_SELECTION
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
#ifdef DEBUG_INDEX_SELECTION
					fprintf( stderr, "using spo index\n" );
#endif
					*index	= (hx_index*) hx->spo;
					break;
				case 2:
#ifdef DEBUG_INDEX_SELECTION
					fprintf( stderr, "using sop index\n" );
#endif
					*index	= (hx_index*) hx->sop;
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
						*index	= (hx_index*) hx->pso;
					} else {
#ifdef DEBUG_INDEX_SELECTION
						fprintf( stderr, "wanted pso index, but using spo...\n" );
#endif
						*index	= (hx_index*) hx->spo;
					}
					break;
				case 2:
#ifdef DEBUG_INDEX_SELECTION
					fprintf( stderr, "using pos index\n" );
#endif
					*index	= (hx_index*) hx->pos;
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
						*index	= (hx_index*) hx->osp;
					} else {
#ifdef DEBUG_INDEX_SELECTION
						fprintf( stderr, "wanted osp index, but using sop...\n" );
#endif
						*index	= (hx_index*) hx->sop;
					}
					break;
				case 1:
					if (hx->ops) {
#ifdef DEBUG_INDEX_SELECTION
						fprintf( stderr, "using ops index\n" );
#endif
						*index	= (hx_index*) hx->ops;
					} else {
#ifdef DEBUG_INDEX_SELECTION
						fprintf( stderr, "wanted ops index, but using pos...\n" );
#endif
						*index	= (hx_index*) hx->pos;
					}
					break;
			}
			break;
	}
	return 0;
}

int hx_get_ordered_index( hx_hexastore* hx, hx_node* sn, hx_node* pn, hx_node* on, int order_position, hx_index** index, hx_node** nodes, int* var_count ) {
	hx_node_id s	= hx_get_node_id( hx, sn );
	hx_node_id p	= hx_get_node_id( hx, pn );
	hx_node_id o	= hx_get_node_id( hx, on );
	if (!hx_node_is_variable( sn ) && s == 0) {
		return 1;
	}
	if (!hx_node_is_variable( pn ) && p == 0) {
		return 1;
	}
	if (!hx_node_is_variable( on ) && o == 0) {
		return 1;
	}
	
	_hx_get_ordered_index( hx, s, p, o, order_position, index, nodes, var_count );
	
	hx_node* triple_ordered[3]	= { sn, pn, on };
	nodes[0]	= triple_ordered[ (*index)->order[0] ];
	nodes[1]	= triple_ordered[ (*index)->order[1] ];
	nodes[2]	= triple_ordered[ (*index)->order[2] ];
	return 0;
}

hx_index_iter* hx_get_statements( hx_hexastore* hx, hx_node* sn, hx_node* pn, hx_node* on, int order_position ) {
	hx_node* index_ordered[3];
	hx_index* index;
	hx_get_ordered_index( hx, sn, pn, on, order_position, &index, index_ordered, NULL );
	
	hx_node_id s	= hx_get_node_id( hx, sn );
	hx_node_id p	= hx_get_node_id( hx, pn );
	hx_node_id o	= hx_get_node_id( hx, on );

	if (!hx_node_is_variable( sn ) && s == 0) {
		return NULL;
	}
	if (!hx_node_is_variable( pn ) && p == 0) {
		return NULL;
	}
	if (!hx_node_is_variable( on ) && o == 0) {
		return NULL;
	}
	
	hx_node_id index_ordered_id[3]	= { s, p, o };
	hx_index_iter* iter	= hx_index_new_iter1( index, index_ordered_id[0], index_ordered_id[1], index_ordered_id[2] );
	return iter;
}

uint64_t hx_count_statements( hx_hexastore* hx, hx_node* s, hx_node* p, hx_node* o ) {
	{
		int vars;
		hx_index* index;
		hx_node* index_ordered[3];
		hx_get_ordered_index( hx, s, p, o, HX_SUBJECT, &index, index_ordered, &vars );

		hx_node_id aid	= hx_get_node_id( hx, index_ordered[0] );
		hx_node_id bid	= hx_get_node_id( hx, index_ordered[1] );
		hx_node_id cid	= hx_get_node_id( hx, index_ordered[2] );
		if (aid == 0 || bid == 0 || cid == 0) {
			return (uint64_t) 0;
		}
		hx_node_id index_ordered_id[3]	= { aid, bid, cid };
		
		uint64_t size;
		hx_head* head;
		hx_index_iter* iter;
		hx_vector* vector;
		hx_terminal* terminal;
		switch (vars) {
			case 3:
				return hx_triples_count( hx );
			case 2:
				head	= hx_index_head( index );
				if (head == NULL) {
// 					fprintf( stderr, "*** Did not find the head pointer in hx_count_statements with %d vars\n", vars );
					return (uint64_t) 0;
				}
				vector	= hx_head_get_vector( head, index_ordered_id[0] );
				if (vector == NULL) {
//					fprintf( stderr, "*** Did not find the vector pointer in hx_count_statements with %d vars\n", vars );
					return (uint64_t) 0;
				}
				size	= hx_vector_triples_count( vector );
				return size;
				break;
			case 1:
				head	= hx_index_head( index );
				if (head == NULL) {
//					fprintf( stderr, "*** Did not find the head pointer in hx_count_statements with %d vars\n", vars );
					return (uint64_t) 0;
				}
				vector	= hx_head_get_vector( head, index_ordered_id[0] );
				if (vector == NULL) {
//					fprintf( stderr, "*** Did not find the vector pointer in hx_count_statements with %d vars\n", vars );
					return (uint64_t) 0;
				}
				terminal	= hx_vector_get_terminal( vector, index_ordered_id[1] );
				if (terminal == NULL) {
//					fprintf( stderr, "*** Did not find the terminal pointer in hx_count_statements with %d vars\n", vars );
					return (uint64_t) 0;
				}
				size	= (uint64_t) hx_terminal_size( terminal );
				return size;
			case 0:
				iter	= hx_get_statements( hx, s, p, o, HX_SUBJECT );
				break;
				return (uint64_t) ((hx_index_iter_finished(iter)) ? 0 : 1);
		};
	}
	// XXX NOT EFFICIENT... Needs to be updated to use the {head,vector,terminal} structs' triples_count field
	uint64_t count	= 0;
	hx_index_iter* iter	= hx_get_statements( hx, s, p, o, HX_SUBJECT );
	while (!hx_index_iter_finished(iter)) {
		count++;
		hx_index_iter_next(iter);
	}
	hx_free_index_iter(iter);
	return count;
}

uint64_t hx_triples_count ( hx_hexastore* hx ) {
	hx_index* i	= (hx_index*) hx->spo;
	return hx_index_triples_count( i );
}

int hx_write( hx_hexastore* h, FILE* f ) {
	fputc( 'X', f );
	if (hx_nodemap_write( h->map, f ) != 0) {
		fprintf( stderr, "*** Error while writing hexastore nodemap to disk.\n" );
		return 1;
	}
	
	int cond	= 0;
	if (h->spo)
		cond		|= hx_index_write( (hx_index*) h->spo, f );
	if (h->sop)
		cond		|= hx_index_write( (hx_index*) h->sop, f );
	if (h->pso)
		cond		|= hx_index_write( (hx_index*) h->pso, f );
	if (h->pos)
		cond		|= hx_index_write( (hx_index*) h->pos, f );
	if (h->osp)
		cond		|= hx_index_write( (hx_index*) h->osp, f );
	if (h->ops)
		cond		|= hx_index_write( (hx_index*) h->ops, f );
	
	if (cond != 0) {
		fprintf( stderr, "*** Error while writing hexastore indices to disk.\n" );
		return 1;
	} else {
		return 0;
	}
}

hx_hexastore* hx_read( FILE* f, int buffer ) {
	int c	= fgetc( f );
	if (c != 'X') {
		fprintf( stderr, "*** Bad header cookie trying to read hexastore from file.\n" );
		return NULL;
	}
	hx_hexastore* hx	= (hx_hexastore*) calloc( 1, sizeof( hx_hexastore )  );
	hx->map	= hx_nodemap_read( f, buffer );
	if (hx->map == NULL) {
		fprintf( stderr, "*** NULL nodemap returned while trying to read hexastore from disk.\n" );
		free( hx );
		return NULL;
	}
	
	hx->next_var	= -1;
	hx->spo		= ((uintptr_t) hx_index_read( f, buffer ));
	hx->sop		= ((uintptr_t) hx_index_read( f, buffer ));
	hx->pso		= ((uintptr_t) hx_index_read( f, buffer ));
	hx->pos		= ((uintptr_t) hx_index_read( f, buffer ));
	hx->osp		= ((uintptr_t) hx_index_read( f, buffer ));
	hx->ops		= ((uintptr_t) hx_index_read( f, buffer ));
	hx->indexes		= NULL;
	
	if ((hx->spo == 0) || (hx->spo == 0) || (hx->spo == 0) || (hx->spo == 0) || (hx->spo == 0) || (hx->spo == 0)) {
		fprintf( stderr, "*** NULL index returned while trying to read hexastore from disk.\n" );
		free( hx );
		return NULL;
	} else {
		return hx;
	}
}

hx_variablebindings_iter* hx_new_iter_variablebindings ( hx_index_iter* i, char* subj_name, char* pred_name, char* obj_name ) {
	hx_variablebindings_iter_vtable* vtable	= (hx_variablebindings_iter_vtable*) calloc( 1, sizeof( hx_variablebindings_iter_vtable ) );
	vtable->finished	= _hx_iter_vb_finished;
	vtable->current		= _hx_iter_vb_current;
	vtable->next		= _hx_iter_vb_next;
	vtable->free		= _hx_iter_vb_free;
	vtable->names		= _hx_iter_vb_names;
	vtable->size		= _hx_iter_vb_size;
	vtable->sorted_by_index	= _hx_iter_vb_sorted_by;
	vtable->debug		= _hx_iter_debug;
	
	int size	= 0;
	if (subj_name != NULL)
		size++;
	if (pred_name != NULL)
		size++;
	if (obj_name != NULL)
		size++;
	
	_hx_iter_vb_info* info			= (_hx_iter_vb_info*) calloc( 1, sizeof( _hx_iter_vb_info ) );
	info->size						= size;
	
	if (subj_name != NULL) {
		info->subject					= (char*) malloc( strlen(subj_name) + 1 );
		strcpy( info->subject, subj_name );
	}
	
	if (pred_name != NULL) {
		info->predicate					= (char*) malloc( strlen(pred_name) + 1 );
		strcpy( info->predicate, pred_name );
	}
	
	if (obj_name != NULL) {
		info->object					= (char*) malloc( strlen(obj_name) + 1 );
		strcpy( info->object, obj_name );
	}
	
	info->iter						= i;
	info->names						= (char**) calloc( 3, sizeof( char* ) );
	info->triple_pos_to_index		= (int*) calloc( 3, sizeof( int ) );
	info->index_to_triple_pos		= (int*) calloc( 3, sizeof( int ) );
	info->current					= NULL;
	
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
	
	hx_variablebindings_iter* iter	= hx_variablebindings_new_iter( vtable, (void*) info );
	return iter;
}

int _hx_iter_vb_finished ( void* data ) {
	_hx_iter_vb_info* info	= (_hx_iter_vb_info*) data;
	hx_index_iter* iter		= (hx_index_iter*) info->iter;
	return hx_index_iter_finished( iter );
}

int _hx_iter_vb_current ( void* data, void* results ) {
	_hx_iter_vb_info* info	= (_hx_iter_vb_info*) data;
	hx_variablebindings** bindings	= (hx_variablebindings**) results;
	if (info->current == NULL) {
		hx_index_iter* iter		= (hx_index_iter*) info->iter;
		hx_node_id triple[3];
		hx_index_iter_current ( iter, &(triple[0]), &(triple[1]), &(triple[2]) );
		hx_node_id* values	= (hx_node_id*) calloc( info->size, sizeof( hx_node_id ) );
		int i;
		for (i = 0; i < info->size; i++) {
			values[ i ]	= triple[ info->triple_pos_to_index[ i ] ];
		}
		info->current	= hx_new_variablebindings( info->size, info->names, values );
	}
	*bindings	= info->current;
	return 0;
}

int _hx_iter_vb_next ( void* data ) {
	_hx_iter_vb_info* info	= (_hx_iter_vb_info*) data;
	hx_index_iter* iter		= (hx_index_iter*) info->iter;
	info->current			= NULL;
	return hx_index_iter_next( iter );
}

int _hx_iter_vb_free ( void* data ) {
	_hx_iter_vb_info* info	= (_hx_iter_vb_info*) data;
	hx_index_iter* iter		= (hx_index_iter*) info->iter;
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

int _hx_iter_vb_size ( void* data ) {
	_hx_iter_vb_info* info	= (_hx_iter_vb_info*) data;
	return info->size;
}

char** _hx_iter_vb_names ( void* data ) {
	_hx_iter_vb_info* info	= (_hx_iter_vb_info*) data;
	return info->names;
}

int _hx_iter_vb_sorted_by (void* data, int index ) {
	_hx_iter_vb_info* info	= (_hx_iter_vb_info*) data;
	int triple_pos	= info->index_to_triple_pos[ index ];
// 	fprintf( stderr, "*** checking if index iterator is sorted by %d (triple %s)\n", index, HX_POSITION_NAMES[triple_pos] );
	return hx_index_iter_is_sorted_by_index( info->iter, triple_pos );
}

int _hx_iter_debug ( void* data, char* header, int indent ) {
	_hx_iter_vb_info* info	= (_hx_iter_vb_info*) data;
	int i;
	for (i = 0; i < indent; i++) fwrite( " ", sizeof( char ), 1, stderr );
	hx_index_iter* iter	= info->iter;
	fprintf( stderr, "%s hexastore triples iterator (%p)\n", header, (void*) iter );
	int counter	= 0;
	fprintf( stderr, "%s ------------------------\n", header );
	while (!hx_index_iter_finished( iter )) {
		counter++;
		hx_node_id triple[3];
		hx_index_iter_current( iter, &(triple[0]), &(triple[1]), &(triple[2]) );
		fprintf( stderr, "%s triple: { %d %d %d }\n", header, (int) triple[0], (int) triple[1], (int) triple[2] );
		hx_index_iter_next( iter );
	}
	fprintf( stderr, "%s --- %5d triples ------\n", header, counter );
	
	return 0;
}

hx_node_id hx_get_node_id ( hx_hexastore* hx, hx_node* node ) {
	if (node == NULL) {
		return (hx_node_id) 0;
	}
	hx_node_id id;
	hx_nodemap* map	= hx->map;
	if (hx_node_is_variable( node )) {
		id	= hx_node_iv( node );
	} else {
		id	= hx_nodemap_get_node_id( map, node );
	}
	return id;
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
	hx_node* s	= hx_new_named_variable( hx, "subj" );
	hx_node* p	= hx_new_named_variable( hx, "pred" );
	hx_node* o	= hx_new_named_variable( hx, "obj" );
	hx_index_iter* titer	= hx_get_statements( hx, s, p, o, HX_SUBJECT );
	hx_variablebindings_iter* iter	= hx_new_iter_variablebindings( titer, "subj", "pred", "obj" );
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
	return 0;
}
