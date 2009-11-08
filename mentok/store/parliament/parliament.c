#include <stdint.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include "mentok/mentok.h"
#include "mentok/store/parliament/parliament.h"

#define ID2OFFSET(id)	(id-1)
#define OFFSET2ID(o)	(o+1)

typedef struct {
	hx_node_id id;
	hx_node* node;
} hx_store_parliament_map_item;

int _hx_store_parliament_contains_triple (hx_store* store, hx_triple* triple);

hx_node_id _hx_store_parliament_add_node (hx_store* store, hx_node* n) {
	hx_store_parliament* hx	= (hx_store_parliament*) store->ptr;
	struct avl_table* map	= hx->node2id;
	hx_store_parliament_map_item i;
	i.node			= n;
	i.id			= (hx_node_id) 0;
	hx_store_parliament_map_item* item	= (hx_store_parliament_map_item*) avl_find( map, &i );
	if (item == NULL) {
		if (0) {
			char* nodestr;
			hx_node_string( n, &nodestr );
			fprintf( stderr, "nodemap %p adding key '%s'\n", (void*) map, nodestr );
			free(nodestr);
		}
		
		item	= (hx_store_parliament_map_item*) calloc( 1, sizeof( hx_store_parliament_map_item ) );
		if (item == NULL) {
			fprintf( stderr, "*** Failed to allocate memory in _hx_store_parliament_add_node\n" );
			return 0;
		}
		
		hx_store_parliament_resource_table* r	= hx->resources;
		assert( r->used < r->allocated );
		hx_node_id id	= OFFSET2ID( r->used++ );
		
		hx_store_parliament_resource_record* rec	= &( r->records[ ID2OFFSET(id) ] );
		rec->fs			= 0;
		rec->fp			= 0;
		rec->fo			= 0;
		rec->sc			= 0;
		rec->pc			= 0;
		rec->oc			= 0;
		rec->value		= hx_node_copy( n );
		
		item->node		= rec->value;
		item->id		= id;
		avl_insert( map, item );
		
// 		fprintf( stderr, "*** new item %d -> %p\n", (int) item->id, (void*) item->node );
		
		return item->id;
	} else {
		if (0) {
			char* nodestr;
			hx_node_string( n, &nodestr );
			fprintf( stderr, "nodemap already has key '%s'\n", nodestr );
			free(nodestr);
		}
		return item->id;
	}
}

int _hx_store_parliament_node_cmp_str ( const void* a, const void* b, void* param ) {
	hx_store_parliament_map_item* ia	= (hx_store_parliament_map_item*) a;
	hx_store_parliament_map_item* ib	= (hx_store_parliament_map_item*) b;
	int c	= hx_node_cmp(ia->node, ib->node);
	return c;
}

hx_store* hx_new_store_parliament ( void* world ) {
	hx_store_parliament* hx	= (hx_store_parliament*) calloc( 1, sizeof(hx_store_parliament) );
	hx->world		= world;
	hx->count		= 0;
	hx->node2id		= avl_create( _hx_store_parliament_node_cmp_str, NULL, &avl_allocator_default );
	
	hx_store_parliament_resource_table* r	= (hx_store_parliament_resource_table*) calloc( 1, sizeof(hx_store_parliament_resource_table) );
	r->used			= 0;
	r->allocated	= 1024;
	r->records		= (hx_store_parliament_resource_record*) calloc( r->allocated, sizeof(hx_store_parliament_resource_record) );
	hx->resources	= r;
	
	hx_store_parliament_statement_list* s	= (hx_store_parliament_statement_list*) calloc( 1, sizeof(hx_store_parliament_statement_list) );
	s->used			= 0;
	s->allocated	= 4096;
	s->records		= (hx_store_parliament_statement_record*) calloc( s->allocated, sizeof(hx_store_parliament_statement_record) );
	hx->statements	= s;
	
	hx_store_vtable* vtable				= (hx_store_vtable*) calloc( 1, sizeof(hx_store_vtable) );
	vtable->close						= hx_store_parliament_close;
	vtable->size						= hx_store_parliament_size;
	vtable->count						= hx_store_parliament_count;
	vtable->add_triple					= hx_store_parliament_add_triple;
	vtable->remove_triple				= hx_store_parliament_remove_triple;
	vtable->contains_triple				= hx_store_parliament_contains_triple;
	vtable->get_statements				= hx_store_parliament_get_statements;
	vtable->get_statements_with_index	= hx_store_parliament_get_statements_with_index;
	vtable->sync						= hx_store_parliament_sync;
	vtable->triple_orderings			= hx_store_parliament_triple_orderings;
	vtable->id2node						= hx_store_parliament_id2node;
	vtable->node2id						= hx_store_parliament_node2id;
	vtable->begin_bulk_load				= hx_store_parliament_begin_bulk_load;
	vtable->end_bulk_load				= hx_store_parliament_end_bulk_load;
	vtable->ordering_name				= hx_store_parliament_ordering_name;
	return hx_new_store( world, vtable, hx );
}

/***** STORE METHODS: *****/


/* Close storage/model context */
int hx_store_parliament_close (hx_store* store) {
	hx_store_parliament* hx	= (hx_store_parliament*) store->ptr;
	hx_store_parliament_resource_table* r	= hx->resources;
	hx_store_parliament_statement_list* s	= hx->statements;
	free(r->records);
	free(r);
	
	free(s->records);
	free(s);
	
	avl_destroy( hx->node2id, NULL );
	
	free(hx);
	return 0;
}

/* Return the number of triples in the storage for model */
uint64_t hx_store_parliament_size (hx_store* store) {
	hx_store_parliament* hx	= (hx_store_parliament*) store->ptr;
	return (uint64_t) hx->count;
}	

/* Return the number of triples matching a triple pattern */
uint64_t hx_store_parliament_count (hx_store* store, hx_triple* triple) {
	hx_store_parliament* hx	= (hx_store_parliament*) store->ptr;
	hx_node_id sid	= hx_store_parliament_node2id( store, triple->subject );
	hx_node_id pid	= hx_store_parliament_node2id( store, triple->predicate );
	hx_node_id oid	= hx_store_parliament_node2id( store, triple->object );
	
	if (sid == 0 || pid == 0 || oid == 0) {
		// one of the nodes wasn't in the nodemap, so it definitely isn't in the statement table
		return 0;
	}
	
	int b	= hx_triple_bound_count( triple );
	if (b == 0) {
		return hx_store_parliament_size(store);
	} else if (b == 1) {
		hx_store_parliament_resource_table* r	= hx->resources;
		hx_node* n	= NULL;
		int i	= -1;
		for (i = 0; i < 3; i++) {
			n	= hx_triple_node( triple, i );
			if (n != NULL)
				break;
		}
		assert( n != NULL);
		assert( i >= 0 );
		assert( i < 3 );
		hx_node_id id	= hx_store_parliament_node2id( store, n );
		hx_store_parliament_resource_record* rec	= &( r->records[ ID2OFFSET(id) ] );
		switch (i) {
			case 0:
				return (uint64_t) rec->sc;
				break;
			case 1:
				return (uint64_t) rec->pc;
				break;
			case 2:
				return (uint64_t) rec->oc;
				break;
		}
	} else if (b == 2) {
		uint64_t count	= 0;
		hx_store_parliament_statement_list* s	= hx->statements;
		hx_store_parliament_resource_table* r	= hx->resources;
		hx_store_parliament_statement_record* sp;
		
		if (sid > 0) {
			// follow the next-subject pointers
			hx_store_parliament_offset sp_id	= r->records[ ID2OFFSET(sid) ].fs;
			while (sp_id != 0) {
				sp	= &( s->records[ ID2OFFSET(sp_id) ] );
				int deleted	= (sp->flags & HX_STORE_PARLIAMENT_STATEMENT_DELETED);
				if (!deleted) {
					if ((pid > 0 && sp->p == pid) || (pid < 0)) {
						if ((oid > 0 && sp->o == oid) || (oid < 0)) {
							count++;
						}
					}
				}
				sp_id	= sp->ns;
			}
		} else {
			// follow the next-object pointers
			hx_store_parliament_offset sp_id	= r->records[ ID2OFFSET(oid) ].fo;
			while (sp_id != 0) {
				sp	= &( s->records[ ID2OFFSET(sp_id) ] );
				int deleted	= (sp->flags & HX_STORE_PARLIAMENT_STATEMENT_DELETED);
				if (!deleted) {
					if ((pid > 0 && sp->p == pid) || (pid < 0)) {
						if ((sid > 0 && sp->s == sid) || (sid < 0)) {
							count++;
						}
					}
				}
				sp_id	= sp->no;
			}
		}
		return count;
	} else {
		return (uint64_t) hx_store_parliament_contains_triple( store, triple );
	}
}	

/* Begin a bulk load processes. Indexes and counts won't be accurate again until finish_bulk_load is called. */
int hx_store_parliament_begin_bulk_load ( hx_store* storage ) {
	return 0;
}

/* End a bulk load processes. */
int hx_store_parliament_end_bulk_load ( hx_store* storage ) {
	return 0;
}

/* Add a triple to the storage from the given model */
int hx_store_parliament_add_triple (hx_store* store, hx_triple* triple) {
	hx_store_parliament* hx	= (hx_store_parliament*) store->ptr;
	hx_store_parliament_statement_list* s	= hx->statements;
	hx_store_parliament_resource_table* r	= hx->resources;

	int c	= _hx_store_parliament_contains_triple( store, triple );
	if (c == 1) {
// 		fprintf( stderr, "*** attempt to add duplicate triple\n" );
		return 0;
	}
	
	hx_node_id sid	= _hx_store_parliament_add_node( store, triple->subject );
	hx_node_id pid	= _hx_store_parliament_add_node( store, triple->predicate );
	hx_node_id oid	= _hx_store_parliament_add_node( store, triple->object );
	
	if (c == 0) {
		assert( s->used < s->allocated );
		
		assert( s->used < s->allocated );
		hx_node_id id	= OFFSET2ID( s->used++ );
		
		hx_store_parliament_statement_record* rec	= &( s->records[ ID2OFFSET(id) ] );
		rec->s	= sid;
		rec->ns	= r->records[ ID2OFFSET(sid) ].fs;
		r->records[ ID2OFFSET(sid) ].fs	= id;
		(r->records[ ID2OFFSET(sid) ].sc)++;
		
		rec->p	= pid;
		rec->np	= r->records[ ID2OFFSET(pid) ].fp;
		r->records[ ID2OFFSET(pid) ].fp	= id;
		(r->records[ ID2OFFSET(pid) ].pc)++;
		
		rec->o	= oid;
		rec->no	= r->records[ ID2OFFSET(oid) ].fo;
		r->records[ ID2OFFSET(oid) ].fo	= id;
		(r->records[ ID2OFFSET(oid) ].oc)++;
		
		hx->count++;
// 		fprintf( stderr, "added triple\n" );
		
		return 0;
	} else if (c == -1) {
		// the triple is marked as deleted in the store. let's undelete it
		hx_store_parliament_statement_record* sp;
		hx_store_parliament_offset sp_id	= r->records[ ID2OFFSET(sid) ].fs;
		while (sp_id != 0) {
			sp	= &( s->records[ ID2OFFSET(sp_id) ] );
			if (sp->p == pid && sp->o == oid) {
				int deleted	= (sp->flags & HX_STORE_PARLIAMENT_STATEMENT_DELETED);
				if (deleted) {
					sp->flags	^= HX_STORE_PARLIAMENT_STATEMENT_DELETED;
					
					(r->records[ ID2OFFSET(sid) ].sc)++;
					(r->records[ ID2OFFSET(pid) ].pc)++;
					(r->records[ ID2OFFSET(oid) ].oc)++;
					hx->count++;
				}
				return 0;
			} else {
				sp_id	= sp->ns;
			}
		}
		return 1;
	}
}

/* Remove a triple from the storage */
int hx_store_parliament_remove_triple (hx_store* store, hx_triple* triple) {
	hx_store_parliament* hx	= (hx_store_parliament*) store->ptr;
	hx_store_parliament_statement_list* s	= hx->statements;
	hx_store_parliament_resource_table* r	= hx->resources;
	
	hx_node_id sid	= hx_store_parliament_node2id( store, triple->subject );
	hx_node_id pid	= hx_store_parliament_node2id( store, triple->predicate );
	hx_node_id oid	= hx_store_parliament_node2id( store, triple->object );

	if (sid == 0 || pid == 0 || oid == 0) {
		// one of the nodes wasn't in the nodemap, so it definitely isn't in the statement table
		return 0;
	}
	
	hx_store_parliament_statement_record* sp;
	hx_store_parliament_offset sp_id	= r->records[ ID2OFFSET(sid) ].fs;
	while (sp_id != 0) {
		sp	= &( s->records[ ID2OFFSET(sp_id) ] );
		if (sp->p == pid && sp->o == oid) {
			int deleted	= (sp->flags & HX_STORE_PARLIAMENT_STATEMENT_DELETED);
			if (!deleted) {
				sp->flags	|= HX_STORE_PARLIAMENT_STATEMENT_DELETED;
				
				(r->records[ ID2OFFSET(sid) ].sc)--;
				(r->records[ ID2OFFSET(pid) ].pc)--;
				(r->records[ ID2OFFSET(oid) ].oc)--;
				hx->count--;
			}
			return 0;
		} else {
			sp_id	= sp->ns;
		}
	}
	
	return 1;
}

// unlike the public function, this one returns:
//	1 if the triple is valid in the store
// -1 if the triple is present but deleted in the store
//	0 if the triple doesn't exist
int _hx_store_parliament_contains_triple (hx_store* store, hx_triple* triple) {
	hx_store_parliament* hx	= (hx_store_parliament*) store->ptr;
	hx_store_parliament_statement_list* s	= hx->statements;
	hx_store_parliament_resource_table* r	= hx->resources;
	
	hx_node_id sid	= hx_store_parliament_node2id( store, triple->subject );
	hx_node_id pid	= hx_store_parliament_node2id( store, triple->predicate );
	hx_node_id oid	= hx_store_parliament_node2id( store, triple->object );
	
// 	fprintf( stderr, "*** contains triple : (%d %d %d)?\n", (int) sid, (int) pid, (int) oid );
	
	if ((sid == 0) || (pid == 0) || (oid == 0)) {
// 		fprintf( stderr, "- no.\n" );
		// one of the nodes wasn't in the nodemap, so it definitely isn't in the statement table
		return 0;
	}
	
	hx_store_parliament_statement_record* sp;
	hx_store_parliament_offset sp_id	= r->records[ ID2OFFSET(sid) ].fs;
	while (sp_id != 0) {
		sp	= &( s->records[ ID2OFFSET(sp_id) ] );
		if (sp->p == pid && sp->o == oid) {
			int deleted	= (sp->flags & HX_STORE_PARLIAMENT_STATEMENT_DELETED);
			if (deleted) {
// 				fprintf( stderr, "- no (deleted).\n" );
				return -1;
			} else {
// 				fprintf( stderr, "- yes.\n" );
				return 1;
			}
		} else {
			sp_id	= sp->ns;
		}
	}
	
// 	fprintf( stderr, "- no.\n" );
	return 0;
}

/* Check if triple is in storage */
int hx_store_parliament_contains_triple (hx_store* store, hx_triple* triple) {
	int c	= _hx_store_parliament_contains_triple( store, triple );
	if (c == 1) {
		return 1;
	} else {
		return 0;
	}
}

/* Return a stream of triples matching a triple pattern */
hx_variablebindings_iter* hx_store_parliament_get_statements (hx_store* store, hx_triple* triple, hx_node* sort_variable) {
	/* XXX */
}

/* Return a stream of triples matching a triple pattern with a specific index thunk (originating from the triple_orderings function) */
hx_variablebindings_iter* hx_store_parliament_get_statements_with_index (hx_store* storage, hx_triple* triple, void* thunk) {
	/* XXX */
}

/* Synchronise to underlying storage */
int hx_store_parliament_sync (hx_store* store) {
	return 0;
}

/* Return a list of ordering arrays, giving the possible access patterns for the given triple */
hx_container_t* hx_store_parliament_triple_orderings (hx_store* store, hx_triple* t	) {
	hx_container_t* c	= hx_new_container( 'I', 1 );
	hx_container_push_item( c, NULL );
	return c;
}

/* Get a string representation of a triple ordering returned by triple_orderings */
char* hx_store_parliament_ordering_name (hx_store* store, void* ordering) {
	return hx_copy_string( "pindex" );
}

/* Return an ID value for a node. */
hx_node_id hx_store_parliament_node2id (hx_store* store, hx_node* node) {
	hx_store_parliament* hx	= (hx_store_parliament*) store->ptr;
	if (hx_node_is_variable(node)) {
		hx_node_id id	= hx_node_iv(node);
		return id;
	}
	
	hx_store_parliament_map_item i;
	i.node	= node;
// 	char* nodestr;
// 	hx_node_string( node, &nodestr );
// 	fprintf( stderr, "nodemap getting id for key '%s'\n", nodestr );
// 	free(nodestr);
	hx_store_parliament_map_item* item	= (hx_store_parliament_map_item*) avl_find( hx->node2id, &i );
	if (item == NULL) {
//		fprintf( stderr, "hx_store_parliament_node2id: did not find node in nodemap\n" );
		return (hx_node_id) 0;
	} else {
		return item->id;
	}
}

/* Return a node object for an ID. Caller is responsible for freeing the node. */
hx_node* hx_store_parliament_id2node (hx_store* store, hx_node_id id) {
	hx_store_parliament* hx	= (hx_store_parliament*) store->ptr;
	hx_store_parliament_resource_table* r	= hx->resources;
	hx_node* n	= r->records[ ID2OFFSET(id) ].value;
	return hx_node_copy( n );
}
