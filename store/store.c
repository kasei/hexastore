#include "store/store.h"

/* Called by a store implementation constructor (i.e. hx_new_store_hexastore) */
hx_store* hx_new_store ( void* world, hx_store_vtable* vtable, void* ptr ) {
	hx_store* s	= (hx_store*) calloc( 1, sizeof( hx_store ) );
	s->world	= world;
	s->vtable	= vtable;
	s->ptr		= ptr;
	return s;
}

int hx_free_store ( hx_store* store ) {
	store->vtable->close( store );
	free( store->vtable );
	free( store );
	return 0;
}

uint64_t hx_store_size ( hx_store* store ) {
	return store->vtable->size( store );
}

uint64_t hx_store_triple_count ( hx_store* store, hx_triple* triple ) {
	return store->vtable->triple_count( store, triple );
}

int hx_store_add_triple ( hx_store* store, hx_triple* triple ) {
	return store->vtable->add_triple( store, triple );
}

int hx_store_remove_triple ( hx_store* store, hx_triple* triple ) {
	return store->vtable->remove_triple( store, triple );
}

int hx_store_contains_triple ( hx_store* store, hx_triple* triple ) {
	return store->vtable->contains_triple( store, triple );
}

hx_variablebindings_iter* hx_store_get_statements ( hx_store* store, hx_triple* triple, hx_node* sort_variable ) {
	hx_variablebindings_iter* iter	= store->vtable->get_statements( store, triple, sort_variable );
	if (iter == NULL) {
		char* names[3];
		int i;
		int j	= 0;
		for (i = 0; i < 3; i++) {
			hx_node* n	= hx_triple_node(triple,i);
			if (hx_node_is_variable(n)) {
				hx_node_variable_name(n, &(names[j++]));
			}
		}
		iter	= hx_variablebindings_new_empty_iter_with_names(j, names);
		for (i = 0; i < j; i++) {
			free(names[i]);
		}
	}
	return iter;
}

hx_container_t* hx_store_triple_orderings ( hx_store* store, hx_triple* triple ) {
	return store->vtable->triple_orderings( store, triple );
}
