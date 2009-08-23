#include "optimizer/optimizer.h"

char* _hx_optimizer_index_name ( hx_index* idx ) {
	int size	= idx->size;
	int* order	= idx->order;
	char* name		= (char*) calloc( 5, sizeof( char ) );
	char* p			= name;
	int i;
	for (i = 0; i < size; i++) {
		int o	= order[i];
		switch (o) {
			case HX_SUBJECT:
				*(p++)	= 'S';
				break;
			case HX_PREDICATE:
				*(p++)	= 'P';
				break;
			case HX_OBJECT:
				*(p++)	= 'O';
				break;
			default:
				fprintf( stderr, "Unrecognized index order (%d) in _hx_optimizer_index_name\n", o );
		};
	}
	return name;
}

// - accessPlans (get vb iter from a triple pattern, which index to use?)
hx_container_t* hx_optimizer_access_plans ( hx_execution_context* ctx, hx_triple* t ) {
	
	hx_hexastore* hx		= ctx->hx;
	hx_container_t* indexes	= hx_get_indexes( hx );
	int size				= hx_container_size(indexes);
	
	
	int i;
	char* string;
	hx_triple_string( t, &string );
	fprintf( stderr, "triple: %s\n", string );
	free(string);
	
	int bound	= hx_triple_bound_count(t);
	fprintf( stderr, "triple has %d bound terms\n", bound );
	
	int ok[3]	= {1,1,1};
	if (bound == 3) {
	} else {
		for (i = 0; i < 3; i++) {
			hx_node* n	= hx_triple_node( t, i );
			if (hx_node_is_variable(n)) {
				fprintf( stderr, "- %s is unbound\n", HX_POSITION_NAMES[i] );
				ok[i]	= 0;
			}
		}
	}
	
	for (i = 0; i < 3; i++) {
		if (ok[i]) {
			fprintf( stderr, "prefix of index must have %s\n", HX_POSITION_NAMES[i] );
		}
	}
	
	int j;
	for (i = 0; i < size; i++) {
		hx_index* idx	= (hx_index*) hx_container_item( indexes, i );
		int isize	= idx->size;
		char* name	= _hx_optimizer_index_name( idx );
//		fprintf( stderr, "hexastore has index %s (%p)\n", name, (void*) idx );
		
		int index_is_ok	= 1;
		for (j = 0; j < bound; j++) {
			if (!ok[ idx->order[j] ]) {
				index_is_ok	= 0;
//				fprintf( stderr, "- won't work because %s comes before some bound terms\n", HX_POSITION_NAMES[ idx->order[j] ] );
			}
		}
		
		if (index_is_ok) {
			fprintf( stderr, "%s index can be used\n", name );
			int k;
			for (k = bound; k < 3; k++) {
				int ordered_by	= idx->order[ k ];
				fprintf( stderr, "\tSorted by %s\n", HX_POSITION_NAMES[ ordered_by ] );
			}
		}
		free(name);
	}
	
	
	return NULL;
}

// - joinPlans (which join algorithm to use? is sorting required?)
hx_container_t* hx_optimizer_join_plans ( hx_execution_context* ctx, hx_container_t* lhs, hx_container_t* rhs ) {
	return NULL;
}

// - finalizePlans (add projection, ordering, filters)
hx_container_t* hx_optimizer_finalize_plans ( hx_execution_context* ctx, hx_container_t* plans ) {
	return NULL;
}

// - prunePlans
hx_container_t* hx_optimizer_prune_plans ( hx_execution_context* ctx, hx_container_t* plans ) {
	return NULL;
}

