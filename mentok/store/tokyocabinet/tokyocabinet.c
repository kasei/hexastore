#include <stdint.h>
#include <sys/stat.h>
#include <dirent.h>
#include "mentok/mentok.h"
#include "mentok/store/tokyocabinet/tokyocabinet.h"

hx_node_id _hx_store_tokyocabinet_get_node_id ( hx_store_tokyocabinet* hx, hx_node* node );
int _hx_store_tokyocabinet_get_ordered_index( hx_store_tokyocabinet* hx, hx_node* sn, hx_node* pn, hx_node* on, hx_node_position_t order_position, hx_store_tokyocabinet_index** index, hx_node** nodes, int* var_count );
int _hx_store_tokyocabinet_iter_vb_finished ( void* data );
int _hx_store_tokyocabinet_iter_vb_current ( void* data, void* results );
int _hx_store_tokyocabinet_iter_vb_next ( void* data );
int _hx_store_tokyocabinet_iter_vb_free ( void* data );
int _hx_store_tokyocabinet_iter_vb_size ( void* data );
char** _hx_store_tokyocabinet_iter_vb_names ( void* data );
int _hx_store_tokyocabinet_iter_vb_sorted_by (void* data, int index );
int _hx_store_tokyocabinet_iter_vb_iter_debug ( void* data, char* header, int indent );
int _hx_store_tokyocabinet_add_triple_id ( hx_store_tokyocabinet* hx, hx_node_id s, hx_node_id p, hx_node_id o );

int _hx_store_tokyocabinet_directory_exists ( const char* dir ) {
	int exists	= 0;
	DIR* d	= opendir( dir );
	if (d) {
		exists	= 1;
		closedir(d);
	}
	return exists;
}

hx_store* hx_new_store_tokyocabinet ( void* world, const char* directory ) {
	if (!_hx_store_tokyocabinet_directory_exists( directory )) {
		if (mkdir( directory, 0755) == -1) {
			perror( "*** Failed to create directory" );
		}
	}
	
	hx_store_tokyocabinet* hx	= (hx_store_tokyocabinet*) calloc( 1, sizeof(hx_store_tokyocabinet) );
	hx->world				= world;
	hx->directory			= directory;
	
	hx->bulk_load			= 0;
	hx->next_id				= 1;
	hx->id2node				= tcbdbnew();
	hx->node2id				= tcbdbnew();
	hx->counts				= tcbdbnew();

	tcbdbtune(hx->id2node, 0, 0, 0, -1, -1, BDBTLARGE|BDBTDEFLATE);
	tcbdbtune(hx->node2id, 0, 0, 0, -1, -1, BDBTLARGE|BDBTDEFLATE);
	tcbdbtune(hx->counts, 0, 0, 0, -1, -1, BDBTLARGE|BDBTDEFLATE);
	tcbdbsetcache( hx->node2id, 4096, 2048 );
//	tcbdbsetcache( hx->counts, 4096, 1024 );
	
	char* id2node_file		= malloc( strlen(directory) + 13 );
	char* node2id_file		= malloc( strlen(directory) + 13 );
	char* counts_file		= malloc( strlen(directory) + 13 );
	sprintf( id2node_file, "%s/%s", directory, "id2node.tcb" );
	sprintf( node2id_file, "%s/%s", directory, "node2id.tcb" );
	sprintf( counts_file, "%s/%s", directory, "counts.tcb" );
	if (!tcbdbopen( hx->id2node, id2node_file, BDBOWRITER|BDBOCREAT )) {	// |BDBOTSYNC
		int ecode = tcbdbecode(hx->id2node);
		fprintf( stderr, "*** error opening id2node file '%s': %s\n", id2node_file, tcbdberrmsg(ecode) );
	}
	if (!tcbdbopen( hx->node2id, node2id_file, BDBOWRITER|BDBOCREAT )) {	// |BDBOTSYNC
		int ecode = tcbdbecode(hx->node2id);
		fprintf( stderr, "*** error opening node2id file '%s': %s\n", node2id_file, tcbdberrmsg(ecode) );
	}
	tcbdbopen( hx->counts, counts_file, BDBOWRITER|BDBOCREAT );	// |BDBOTSYNC
	free(node2id_file);
	free(id2node_file);
	free(counts_file);
	
	int size;
	hx_node_id __next_key	= (hx_node_id) 0;
	void* p	= tcbdbget( hx->id2node, &__next_key, sizeof(hx_node_id), &size );
	if (p == NULL) {
		tcbdbput( hx->id2node, &__next_key, sizeof(hx_node_id), &(hx->next_id), sizeof(hx_node_id) );
	}
	
	hx->spo					= hx_new_tokyocabinet_index( world, HX_STORE_TCINDEX_ORDER_SPO, directory, "spo.tcb" );
	hx->sop					= hx_new_tokyocabinet_index( world, HX_STORE_TCINDEX_ORDER_SOP, directory, "sop.tcb" );
	hx->pso					= hx_new_tokyocabinet_index( world, HX_STORE_TCINDEX_ORDER_PSO, directory, "pso.tcb" );
	hx->pos					= hx_new_tokyocabinet_index( world, HX_STORE_TCINDEX_ORDER_POS, directory, "pos.tcb" );
	hx->osp					= hx_new_tokyocabinet_index( world, HX_STORE_TCINDEX_ORDER_OSP, directory, "osp.tcb" );
	hx->ops					= hx_new_tokyocabinet_index( world, HX_STORE_TCINDEX_ORDER_OPS, directory, "ops.tcb" );
	hx->bulk_load_index		= NULL;
	
	hx_store_vtable* vtable				= (hx_store_vtable*) calloc( 1, sizeof(hx_store_vtable) );
	vtable->close						= hx_store_tokyocabinet_close;
	vtable->size						= hx_store_tokyocabinet_size;
	vtable->count						= hx_store_tokyocabinet_count;
	vtable->add_triple					= hx_store_tokyocabinet_add_triple;
	vtable->remove_triple				= hx_store_tokyocabinet_remove_triple;
	vtable->contains_triple				= hx_store_tokyocabinet_contains_triple;
	vtable->get_statements				= hx_store_tokyocabinet_get_statements;
	vtable->get_statements_with_index	= hx_store_tokyocabinet_get_statements_with_index;
	vtable->sync						= hx_store_tokyocabinet_sync;
	vtable->triple_orderings			= hx_store_tokyocabinet_triple_orderings;
	vtable->id2node						= hx_store_tokyocabinet_id2node;
	vtable->node2id						= hx_store_tokyocabinet_node2id;
	vtable->begin_bulk_load				= hx_store_tokyocabinet_begin_bulk_load;
	vtable->end_bulk_load				= hx_store_tokyocabinet_end_bulk_load;
	vtable->ordering_name				= hx_store_tokyocabinet_ordering_name;
	return hx_new_store( world, vtable, hx );
}

int hx_store_tokyocabinet_debug (hx_store* store) {
	hx_store_tokyocabinet* hx	= (hx_store_tokyocabinet*) store->ptr;
//	hx_store_tokyocabinet_index_debug( hx->spo );
	
	{
		BDBCUR *cur	= tcbdbcurnew(hx->id2node);
		tcbdbcurfirst(cur);
		fprintf( stderr, "Nodemap (id2node):\n" );
		do {
			int size;
			void* k	= tcbdbcurkey(cur, &size);
			char* v	= tcbdbcurval(cur, &size);
			hx_node_id id	= *( (hx_node_id*) k );
			if (id == 0) {
				fprintf( stderr, "\t(next id = %"PRIdHXID")\n", *( (hx_node_id*) v ) );
			} else {
				fprintf( stderr, "\t%"PRIdHXID" -> %s\n", id, v );
			}
			free(k);
			free(v);
		} while (tcbdbcurnext(cur));
		tcbdbcurdel(cur);
	}
	
	{
		BDBCUR *cur	= tcbdbcurnew(hx->node2id);
		tcbdbcurfirst(cur);
		fprintf( stderr, "Nodemap (node2id):\n" );
		do {
			int size;
			char* k	= tcbdbcurkey(cur, &size);
			void* v	= tcbdbcurval(cur, &size);
			hx_node_id id	= *( (hx_node_id*) v );
			fprintf( stderr, "\t%s -> %"PRIdHXID"\n", k, id );
			free(k);
			free(v);
		} while (tcbdbcurnext(cur));
		tcbdbcurdel(cur);
	}
	
	hx_store_tokyocabinet_index_debug( hx->spo );
	
	return 0;
}

/* ************************************************************************** */

/* Close storage/model context */
int hx_store_tokyocabinet_close (hx_store* store) {
	hx_store_tokyocabinet* hx	= (hx_store_tokyocabinet*) store->ptr;
	
	// free the nodemap
	tcbdbdel( hx->id2node );
	tcbdbdel( hx->node2id );
	tcbdbdel( hx->counts );
	
	if (hx->spo)
		hx_free_tokyocabinet_index( hx->spo );
	if (hx->sop)
		hx_free_tokyocabinet_index( hx->sop );
	if (hx->pso)
		hx_free_tokyocabinet_index( hx->pso );
	if (hx->pos)
		hx_free_tokyocabinet_index( hx->pos );
	if (hx->osp)
		hx_free_tokyocabinet_index( hx->osp );
	if (hx->ops)
		hx_free_tokyocabinet_index( hx->ops );
	hx->id2node		= NULL;
	hx->node2id		= NULL;
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
uint64_t hx_store_tokyocabinet_size (hx_store* store) {
	hx_store_tokyocabinet* hx	= (hx_store_tokyocabinet*) store->ptr;
	hx_store_tokyocabinet_index* i	= hx->spo;	// XXX what if spo doesn't exist. use any available index
	return hx_store_tokyocabinet_index_triples_count( i );
}

/* Return the number of triples matching a triple pattern */
uint64_t hx_store_tokyocabinet_count (hx_store* store, hx_triple* triple) {
	hx_store_tokyocabinet* hx	= (hx_store_tokyocabinet*) store->ptr;
	hx_node* s	= triple->subject;
	hx_node* p	= triple->predicate;
	hx_node* o	= triple->object;
	
	int vars;
	hx_store_tokyocabinet_index* index;
	hx_node* index_ordered[3];
	_hx_store_tokyocabinet_get_ordered_index( hx, s, p, o, HX_SUBJECT, &index, index_ordered, &vars );

	hx_node_id sid	= _hx_store_tokyocabinet_get_node_id( hx, s );
	hx_node_id pid	= _hx_store_tokyocabinet_get_node_id( hx, p );
	hx_node_id oid	= _hx_store_tokyocabinet_get_node_id( hx, o );
	
	if (sid == 0 || pid == 0 || oid == 0) {
		return (uint64_t) 0;
	}
	hx_node_id triple_ids[3]	= { sid, pid, oid };
	
	switch (vars) {
		case 3:
			return hx_store_tokyocabinet_size( store );
		case 0:
			return hx_store_tokyocabinet_contains_triple( store, triple );
	};
	
	int size;
	int i;
	for (i = 0; i < 3; i++) {
		if (triple_ids[i] < 0) {
			triple_ids[i]	= 0;
		}
	}

	void* ptr	= tcbdbget(hx->counts, triple_ids, 3*sizeof(hx_node_id), &size);
	if (ptr == NULL) {
		return 0;
	} else {
		uint64_t count	= (uint64_t) *( (int*) ptr );
		free(ptr);
		return count;
	}
}

int _hx_nodemap_add_node ( hx_store_tokyocabinet* hx, hx_node* node ) {
	TCBDB* id2node	= hx->id2node;
	TCBDB* node2id	= hx->node2id;
	
	char* string;
	hx_node_string( node, &string );
	int len	= strlen(string) + 1;
	int size;
	
	hx_node_id id;
	void* p	= tcbdbget( node2id, string, len, &size );
	if (p == NULL) {
		static const hx_node_id __next_key	= (hx_node_id) 0;
		id			= hx->next_id++;
		
		if (!tcbdbput( node2id, string, len, &id, sizeof( hx_node_id ) )) {
			int ecode = tcbdbecode(node2id);
			fprintf( stderr, "*** error adding to node2id: %s\n", tcbdberrmsg(ecode) );
		}
		if (!tcbdbput( id2node, &id, sizeof(hx_node_id), string, len )) {
			int ecode = tcbdbecode(id2node);
			fprintf( stderr, "*** error adding to id2node: %s\n", tcbdberrmsg(ecode) );
		}
		tcbdbput( id2node, &__next_key, sizeof(hx_node_id), &(hx->next_id), sizeof(hx_node_id) );
	} else {
		id	= *( (hx_node_id*) p );
	}
	
	free(string);
	return id;
}

/* Begin a bulk load processes. Indexes and counts won't be accurate again until finish_bulk_load is called. */
int hx_store_tokyocabinet_begin_bulk_load ( hx_store* store ) {
	hx_store_tokyocabinet* hx	= (hx_store_tokyocabinet*) store->ptr;
	hx->bulk_load				= 1;
	hx_container_t* indexes		= hx_store_tokyocabinet_triple_orderings(store, NULL);
	hx->bulk_load_index			= hx_container_item(indexes, 0);
	return 0;
}

/* Begin a bulk load processes. Indexes and counts won't be accurate again until end_bulk_load is called. */
int hx_store_tokyocabinet_end_bulk_load ( hx_store* store ) {
	hx_store_tokyocabinet* hx	= (hx_store_tokyocabinet*) store->ptr;
	hx->bulk_load				= 0;
	hx->bulk_load_index			= NULL;
	hx_container_t* indexes		= hx_store_tokyocabinet_triple_orderings(store, NULL);
	int size					= hx_container_size(indexes);
	hx_store_tokyocabinet_index* source	= hx_container_item( indexes, 0 );
	
	int i;
	hx_node* s	= hx_new_node_named_variable( -1, "s" );
	hx_node* p	= hx_new_node_named_variable( -2, "p" );
	hx_node* o	= hx_new_node_named_variable( -3, "o" );
	hx_triple* triple	= hx_new_triple( s, p, o );
	for (i = 1; i < size; i++) {
		hx_store_tokyocabinet_index* dest	= hx_container_item( indexes, i );
		if (1) {
			char* name	= hx_store_tokyocabinet_index_name( dest );
			fprintf( stderr, "bulk loading triples into index %s\n", name );
			free(name);
		}
		hx_variablebindings_iter* iter	= hx_store_tokyocabinet_get_statements_with_index(store, triple, source);
		while (!hx_variablebindings_iter_finished(iter)) {
			hx_variablebindings* b;
			hx_variablebindings_iter_current( iter, &b );
			hx_node_id subj	= hx_variablebindings_node_id_for_binding_name( b, "s" );
			hx_node_id pred	= hx_variablebindings_node_id_for_binding_name( b, "p" );
			hx_node_id obj	= hx_variablebindings_node_id_for_binding_name( b, "o" );
			hx_store_tokyocabinet_index_add_triple( dest, subj, pred, obj );
			hx_variablebindings_iter_next(iter);
		}
		hx_free_variablebindings_iter(iter);
	}
	
	hx_free_node(s);
	hx_free_node(p);
	hx_free_node(o);
	hx_free_triple(triple);
	
	return 0;
}

/* Add a triple to the storage from the given model */
int hx_store_tokyocabinet_add_triple (hx_store* store, hx_triple* triple) {
	hx_store_tokyocabinet* hx	= (hx_store_tokyocabinet*) store->ptr;
	hx_node_id s	= _hx_nodemap_add_node( hx, triple->subject );
	hx_node_id p	= _hx_nodemap_add_node( hx, triple->predicate );
	hx_node_id o	= _hx_nodemap_add_node( hx, triple->object );
	
	return _hx_store_tokyocabinet_add_triple_id( hx, s, p, o );
}

/* Remove a triple from the storage */
int hx_store_tokyocabinet_remove_triple (hx_store* store, hx_triple* triple) {
	hx_store_tokyocabinet* hx	= (hx_store_tokyocabinet*) store->ptr;
	hx_node_id s	= _hx_store_tokyocabinet_get_node_id( hx, triple->subject );
	hx_node_id p	= _hx_store_tokyocabinet_get_node_id( hx, triple->predicate );
	hx_node_id o	= _hx_store_tokyocabinet_get_node_id( hx, triple->object );
	if (hx->spo) {
		hx_store_tokyocabinet_index_remove_triple( hx->spo, s, p, o );
	}
	if (hx->sop) {
		hx_store_tokyocabinet_index_remove_triple( hx->sop, s, p, o );
	}
	if (hx->pso) {
		hx_store_tokyocabinet_index_remove_triple( hx->pso, s, p, o );
	}
	if (hx->pos) {
		hx_store_tokyocabinet_index_remove_triple( hx->pos, s, p, o );
	}
	if (hx->osp) {
		hx_store_tokyocabinet_index_remove_triple( hx->osp, s, p, o );
	}
	if (hx->ops) {
		hx_store_tokyocabinet_index_remove_triple( hx->ops, s, p, o );
	}
	return 0;
}

/* Check if triple is in storage */
int hx_store_tokyocabinet_contains_triple (hx_store* store, hx_triple* triple) {
	hx_variablebindings_iter* iter	= hx_store_tokyocabinet_get_statements( store, triple, NULL );
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
hx_variablebindings_iter* hx_store_tokyocabinet_get_statements (hx_store* store, hx_triple* triple, hx_node* sort_variable) {
	hx_store_tokyocabinet* hx	= (hx_store_tokyocabinet*) store->ptr;
	hx_node* index_ordered[3];
	hx_store_tokyocabinet_index* index;
	
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
	
	_hx_store_tokyocabinet_get_ordered_index( hx, triple->subject, triple->predicate, triple->object, order_position, &index, index_ordered, NULL );
	return hx_store_tokyocabinet_get_statements_with_index( store, triple, index );
}

/* Return a stream of triples matching a triple pattern with a specific index thunk (originating from the triple_orderings function) */
hx_variablebindings_iter* hx_store_tokyocabinet_get_statements_with_index (hx_store* store, hx_triple* triple, hx_store_tokyocabinet_index* index) {
	hx_store_tokyocabinet* hx	= (hx_store_tokyocabinet*) store->ptr;
	hx_node_id s	= _hx_store_tokyocabinet_get_node_id( hx, triple->subject );
	hx_node_id p	= _hx_store_tokyocabinet_get_node_id( hx, triple->predicate );
	hx_node_id o	= _hx_store_tokyocabinet_get_node_id( hx, triple->object );

	if (0) {
		fprintf( stderr, "get statements matching { %"PRIdHXID" %"PRIdHXID" %"PRIdHXID" }\n", s, p, o );
		hx_store_tokyocabinet_index_debug( index );
	}
	

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
	
	
	hx_store_tokyocabinet_index_iter* iter	= hx_store_tokyocabinet_index_new_iter1( index, index_ordered_id[0], index_ordered_id[1], index_ordered_id[2] );
	
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
	vtable->finished		= _hx_store_tokyocabinet_iter_vb_finished;
	vtable->current			= _hx_store_tokyocabinet_iter_vb_current;
	vtable->next			= _hx_store_tokyocabinet_iter_vb_next;
	vtable->free			= _hx_store_tokyocabinet_iter_vb_free;
	vtable->names			= _hx_store_tokyocabinet_iter_vb_names;
	vtable->size			= _hx_store_tokyocabinet_iter_vb_size;
	vtable->sorted_by_index	= _hx_store_tokyocabinet_iter_vb_sorted_by;
	vtable->debug			= _hx_store_tokyocabinet_iter_vb_iter_debug;
	
	int size	= 0;
	if (subj_name != NULL)
		size++;
	if (pred_name != NULL)
		size++;
	if (obj_name != NULL)
		size++;
	
	_hx_store_tokyocabinet_iter_vb_info* info			= (_hx_store_tokyocabinet_iter_vb_info*) calloc( 1, sizeof( _hx_store_tokyocabinet_iter_vb_info ) );
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
	
	info->iter						= iter;
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
	
	hx_variablebindings_iter* vbiter	= hx_variablebindings_new_iter( vtable, (void*) info );
	return vbiter;
}

/* Synchronise to underlying storage */
int hx_store_tokyocabinet_sync (hx_store* store) {
	// XXX look up API for tc sync
	return 0;
}

/* Return a list of ordering arrays, giving the possible access patterns for the given triple */
hx_container_t* hx_store_tokyocabinet_triple_orderings (hx_store* store, hx_triple* t) {
	hx_store_tokyocabinet* hx	= (hx_store_tokyocabinet*) store->ptr;
	hx_container_t* indexes	= hx_new_container('I', 6);
	if (hx->spo && _hx_store_tokyocabinet_index_ok_for_triple(store, hx->spo, t))
		hx_container_push_item(indexes, hx->spo);
	if (hx->sop && _hx_store_tokyocabinet_index_ok_for_triple(store, hx->sop, t))
		hx_container_push_item(indexes, hx->sop);
	if (hx->pos && _hx_store_tokyocabinet_index_ok_for_triple(store, hx->pos, t))
		hx_container_push_item(indexes, hx->pos);
	if (hx->pso && _hx_store_tokyocabinet_index_ok_for_triple(store, hx->pso, t))
		hx_container_push_item(indexes, hx->pso);
	if (hx->osp && _hx_store_tokyocabinet_index_ok_for_triple(store, hx->osp, t))
		hx_container_push_item(indexes, hx->osp);
	if (hx->ops && _hx_store_tokyocabinet_index_ok_for_triple(store, hx->ops, t))
		hx_container_push_item(indexes, hx->ops);
	return indexes;
}

int _hx_store_tokyocabinet_index_ok_for_triple( hx_store* store, hx_store_tokyocabinet_index* idx, hx_triple* t ) {
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

/* Get a string representation of a triple ordering returned by triple_orderings */
char* hx_store_tokyocabinet_ordering_name (hx_store* store, void* ordering) {
	hx_store_tokyocabinet* hx	= (hx_store_tokyocabinet*) store->ptr;
	if (ordering == hx->spo) {
		return hx_copy_string("SPO");
	} else if (ordering == hx->sop) {
		return hx_copy_string("SOP");
	} else if (ordering == hx->pos) {
		return hx_copy_string("POS");
	} else if (ordering == hx->pso) {
		return hx_copy_string("PSO");
	} else if (ordering == hx->osp) {
		return hx_copy_string("OSP");
	} else if (ordering == hx->ops) {
		return hx_copy_string("OPS");
	}
}

/* Return an ID value for a node. */
hx_node_id hx_store_tokyocabinet_node2id (hx_store* store, hx_node* node) {
	hx_store_tokyocabinet* hx	= (hx_store_tokyocabinet*) store->ptr;
	TCBDB* node2id	= hx->node2id;
	int size;
	
	char* string;
	hx_node_string( node, &string );
	int len	= strlen(string) + 1;
	
	hx_node_id id;
	void* p	= tcbdbget( node2id, string, len, &size );
	free(string);
	if (p == NULL) {
		return 0;
	} else {
		return *( (hx_node_id*) p );
	}
}

/* Return a node object for an ID. Caller is responsible for freeing the node. */
hx_node* hx_store_tokyocabinet_id2node (hx_store* store, hx_node_id id) {
	hx_store_tokyocabinet* hx	= (hx_store_tokyocabinet*) store->ptr;
	TCBDB* id2node	= hx->id2node;
	int size;
	
	void* p	= tcbdbget( id2node, &id, sizeof(hx_node_id), &size );
	if (p == NULL) {
		return NULL;
	} else {
		hx_node* node	= hx_node_parse( p );
		free(p);
		return node;
	}
}




/* ************************************************************************** */

int _hx_store_tokyocabinet_iter_vb_finished ( void* data ) {
	_hx_store_tokyocabinet_iter_vb_info* info	= (_hx_store_tokyocabinet_iter_vb_info*) data;
	hx_store_tokyocabinet_index_iter* iter		= (hx_store_tokyocabinet_index_iter*) info->iter;
	return hx_store_tokyocabinet_index_iter_finished( iter );
}

int _hx_store_tokyocabinet_iter_vb_current ( void* data, void* results ) {
	_hx_store_tokyocabinet_iter_vb_info* info	= (_hx_store_tokyocabinet_iter_vb_info*) data;
	hx_variablebindings** bindings	= (hx_variablebindings**) results;
	if (info->current == NULL) {
		hx_store_tokyocabinet_index_iter* iter		= (hx_store_tokyocabinet_index_iter*) info->iter;
		hx_node_id triple[3];
		hx_store_tokyocabinet_index_iter_current ( iter, &(triple[0]), &(triple[1]), &(triple[2]) );
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

int _hx_store_tokyocabinet_iter_vb_next ( void* data ) {
	_hx_store_tokyocabinet_iter_vb_info* info	= (_hx_store_tokyocabinet_iter_vb_info*) data;
	hx_store_tokyocabinet_index_iter* iter		= (hx_store_tokyocabinet_index_iter*) info->iter;
	if (info->current) {
		hx_free_variablebindings( info->current );
	}
	info->current			= NULL;
	return hx_store_tokyocabinet_index_iter_next( iter );
}

int _hx_store_tokyocabinet_iter_vb_free ( void* data ) {
	_hx_store_tokyocabinet_iter_vb_info* info	= (_hx_store_tokyocabinet_iter_vb_info*) data;
	hx_store_tokyocabinet_index_iter* iter		= (hx_store_tokyocabinet_index_iter*) info->iter;
	hx_free_tokyocabinet_index_iter( iter );
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

int _hx_store_tokyocabinet_iter_vb_size ( void* data ) {
	_hx_store_tokyocabinet_iter_vb_info* info	= (_hx_store_tokyocabinet_iter_vb_info*) data;
	return info->size;
}

char** _hx_store_tokyocabinet_iter_vb_names ( void* data ) {
	_hx_store_tokyocabinet_iter_vb_info* info	= (_hx_store_tokyocabinet_iter_vb_info*) data;
	return info->names;
}

int _hx_store_tokyocabinet_iter_vb_sorted_by (void* data, int index ) {
	_hx_store_tokyocabinet_iter_vb_info* info	= (_hx_store_tokyocabinet_iter_vb_info*) data;
	int triple_pos	= info->index_to_triple_pos[ index ];
// 	fprintf( stderr, "*** checking if index iterator is sorted by %d (triple %s)\n", index, HX_POSITION_NAMES[triple_pos] );
	return hx_store_tokyocabinet_index_iter_is_sorted_by_index( info->iter, triple_pos );
}

int _hx_store_tokyocabinet_iter_vb_iter_debug ( void* data, char* header, int indent ) {
	_hx_store_tokyocabinet_iter_vb_info* info	= (_hx_store_tokyocabinet_iter_vb_info*) data;
	int i;
	for (i = 0; i < indent; i++) fwrite( " ", sizeof( char ), 1, stderr );
	hx_store_tokyocabinet_index_iter* iter	= info->iter;
	fprintf( stderr, "%s tokyocabinet triples iterator (%p)\n", header, (void*) iter );
	int counter	= 0;
	fprintf( stderr, "%s ------------------------\n", header );
	while (!hx_store_tokyocabinet_index_iter_finished( iter )) {
		counter++;
		hx_node_id triple[3];
		hx_store_tokyocabinet_index_iter_current( iter, &(triple[0]), &(triple[1]), &(triple[2]) );
		fprintf( stderr, "%s triple: { %d %d %d }\n", header, (int) triple[0], (int) triple[1], (int) triple[2] );
		hx_store_tokyocabinet_index_iter_next( iter );
	}
	fprintf( stderr, "%s --- %5d triples ------\n", header, counter );
	
	return 0;
}

int _hx_store_tokyocabinet_add_triple_id ( hx_store_tokyocabinet* hx, hx_node_id s, hx_node_id p, hx_node_id o ) {
	if (hx->bulk_load) {
		hx_store_tokyocabinet_index_add_triple( hx->bulk_load_index, s, p, o );
	} else {
		if (hx->spo) hx_store_tokyocabinet_index_add_triple( hx->spo, s, p, o );
		if (hx->sop) hx_store_tokyocabinet_index_add_triple( hx->sop, s, p, o );
		if (hx->pso) hx_store_tokyocabinet_index_add_triple( hx->pso, s, p, o );
		if (hx->pos) hx_store_tokyocabinet_index_add_triple( hx->pos, s, p, o );
		if (hx->osp) hx_store_tokyocabinet_index_add_triple( hx->osp, s, p, o );
		if (hx->ops) hx_store_tokyocabinet_index_add_triple( hx->ops, s, p, o );
	}
	
	int son, pon, oon;
	hx_node_id count_key[3];
	for (son = 0; son <= 1; son++) {
		if (son == 1) {
			count_key[0]	= s;
		} else {
			count_key[0]	= 0;
		}
		for (pon = 0; pon <= 1; pon++) {
			if (pon == 1) {
				count_key[1]	= p;
			} else {
				count_key[1]	= 0;
			}
			for (oon = 0; oon <= 1; oon++) {
				if (oon == 1) {
					count_key[2]	= o;
				} else {
					count_key[2]	= 0;
				}
				
				if (!(son == pon && pon == oon)) {
					tcbdbaddint(hx->counts, count_key, 3*sizeof(hx_node_id), 1);
				}
			}
		}
	}
	return 0;
}

hx_node_id _hx_store_tokyocabinet_get_node_id ( hx_store_tokyocabinet* hx, hx_node* node ) {
	if (hx_node_is_variable(node)) {
		return hx_node_iv(node);
	}
	
	char* string;
	hx_node_string( node, &string );
	int len	= strlen(string) + 1;
	
	int size;
	void* p	= tcbdbget( hx->node2id, string, len, &size );
	if (p == NULL) {
		fprintf( stderr, "*** can't find node in nodemap: %s\n", string );
		free(string);
		
		if (1) {
			BDBCUR *cur	= tcbdbcurnew(hx->node2id);
			if (tcbdbcurfirst(cur)) {
				fprintf( stderr, "Nodemap (node2id):\n" );
				do {
					int size;
					char* v	= tcbdbcurkey(cur, &size);
					void* k	= tcbdbcurval(cur, &size);
					hx_node_id id	= *( (hx_node_id*) k );
					if (id == 0) {
						fprintf( stderr, "\t(next id = %"PRIdHXID")\n", *( (hx_node_id*) v ) );
					} else {
						fprintf( stderr, "\t%"PRIdHXID" -> %s\n", id, v );
					}
					free(k);
					free(v);
				} while (tcbdbcurnext(cur));
			} else {
				fprintf( stderr, "*** no records in node2id?\n" );
				uint64_t count	= tcbdbrnum(hx->node2id);
				fprintf( stderr, "\t%"PRIu64"\n", count );
				
			}
			tcbdbcurdel(cur);
			exit(1);
		}
		
		return 0;
	} else {
		free(string);
		hx_node_id id	= *( (hx_node_id*) p );
		free(p);
		return id;
	}
}

int _hx_store_tokyocabinet_get_ordered_index( hx_store_tokyocabinet* hx, hx_node* sn, hx_node* pn, hx_node* on, hx_node_position_t order_position, hx_store_tokyocabinet_index** index, hx_node** nodes, int* var_count ) {
	hx_node_id s	= _hx_store_tokyocabinet_get_node_id( hx, sn );
	hx_node_id p	= _hx_store_tokyocabinet_get_node_id( hx, pn );
	hx_node_id o	= _hx_store_tokyocabinet_get_node_id( hx, on );
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
#ifdef DEBUG_INDEX_SELECTION
					fprintf( stderr, "using spo index\n" );
#endif
					*index	= hx->spo;
					break;
				case 2:
#ifdef DEBUG_INDEX_SELECTION
					fprintf( stderr, "using sop index\n" );
#endif
					*index	= hx->sop;
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
#ifdef DEBUG_INDEX_SELECTION
					fprintf( stderr, "using pos index\n" );
#endif
					*index	= hx->pos;
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

