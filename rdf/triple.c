#include "rdf/triple.h"

hx_triple* hx_new_triple2( hx_node* s, hx_node* p, hx_node* o, hx_node* g ) {
	hx_triple* t	= (hx_triple*) calloc( 1, sizeof( hx_triple ) );
	t->subject		= hx_node_copy( s );
	t->predicate	= hx_node_copy( p );
	t->object		= hx_node_copy( o );
	t->graph		= hx_node_copy( g );
	return t;
}

hx_triple* hx_new_triple( hx_node* s, hx_node* p, hx_node* o ) {
	hx_triple* t	= (hx_triple*) calloc( 1, sizeof( hx_triple ) );
	t->subject		= hx_node_copy( s );
	t->predicate	= hx_node_copy( p );
	t->object		= hx_node_copy( o );
	t->graph		= NULL;
	return t;
}

hx_triple* hx_copy_triple ( hx_triple* t ) {
	hx_triple* c	= (hx_triple*) calloc( 1, sizeof( hx_triple ) );
	c->subject		= hx_node_copy( t->subject );
	c->predicate	= hx_node_copy( t->predicate );
	c->object		= hx_node_copy( t->object );
	c->graph		= hx_node_copy( t->graph );
	return c;
}

int hx_free_triple ( hx_triple* t ) {
	hx_free_node( t->subject );
	hx_free_node( t->predicate );
	hx_free_node( t->object );
	if (t->graph) {
		hx_free_node( t->graph );
	}
	free( t );
	return 0;
}

hx_node* hx_triple_node ( hx_triple* t, int i ) {
	switch (i) {
		case 0:
			return t->subject;
		case 1:
			return t->predicate;
		case 2:
			return t->object;
		case 3:
			return t->graph;
		default:
			fprintf( stderr, "*** Bad triple index value (%d) passed to hx_triple_node\n", i );
			return NULL;
	};
}

int hx_triple_bound_count ( hx_triple* t ) {
	int bound	= 0;
	int i;
	for (i = 0; i < 4; i++) {
		hx_node* n	= hx_triple_node( t, i );
		if (n != NULL) {
			if (!hx_node_is_variable(n)) {
				bound++;
			}
		}
	}
	return bound;
}

int hx_triple_id_string ( hx_triple_id* t, hx_nodemap* map, char** string ) {
	hx_node* subj	= hx_nodemap_get_node( map, t->subject );
	hx_node* pred	= hx_nodemap_get_node( map, t->predicate );
	hx_node* obj	= hx_nodemap_get_node( map, t->object );
	hx_node* graph	= hx_nodemap_get_node( map, t->graph );
	
	int len	= 12;	// "(triple * * *)"
					// "(quad * * * *)"
	char *s, *p, *o, *g;
	hx_node_string( subj, &s );
	hx_node_string( pred, &p );
	hx_node_string( obj, &o );
	hx_node_string( graph, &g );
	
	len	+= strlen( s );
	len	+= strlen( p );
	len	+= strlen( o );
	if (graph) {
		len	+= strlen( g );
	}
	
	*string	= malloc( len );
	if (*string == NULL) {
		fprintf( stderr, "*** malloc failed in hx_triple_id_string\n" );
	}
	if (*string == NULL) {
		return 1;
	}
	
	if (graph) {
		snprintf( *string, len, "(quad %s %s %s %s)", s, p, o, g );
		free( g );
	} else {
		snprintf( *string, len, "(triple %s %s %s)", s, p, o );
	}
	free( s );
	free( p );
	free( o );
	return 0;
	
}

int hx_triple_string ( hx_triple* t, char** string ) {
	int len	= 12;	// "(triple * * *)"
					// "(quad * * * *)"
	char *s, *p, *o, *g;
	
	hx_node_string( t->subject, &s );
	hx_node_string( t->predicate, &p );
	hx_node_string( t->object, &o );
	hx_node_string( t->graph, &g );
	len	+= strlen( s );
	len	+= strlen( p );
	len	+= strlen( o );
	if (t->graph) {
		len	+= strlen( g );
	}
	
	*string	= malloc( len );
	if (*string == NULL) {
		fprintf( stderr, "*** malloc failed in hx_triple_string\n" );
	}
	if (*string == NULL) {
		return 1;
	}
	
	if (t->graph) {
		snprintf( *string, len, "(quad %s %s %s %s)", s, p, o, g );
		free( g );
	} else {
		snprintf( *string, len, "(triple %s %s %s)", s, p, o );
	}
	free( s );
	free( p );
	free( o );
	return 0;
}

int hx_triple_debug ( hx_triple* t ) {
	char* string;
	hx_triple_string( t, &string );
	fprintf( stderr, "(%p): %s\n", (void*) t, string );
	free(string);
	return 0;
}

uint64_t hx_triple_hash_on_node ( hx_triple* t, hx_node_position_t pos, hx_hash_function* func ) {
	hx_hash_function* hashfunc	= (func == NULL) ? hx_util_hash_string : func;
	hx_node* n;
	switch (pos) {
		case HX_SUBJECT:
			n	= t->subject;
			break;
		case HX_PREDICATE:
			n	= t->predicate;
			break;
		case HX_OBJECT:
			n	= t->object;
			break;
		case HX_GRAPH:
			n	= t->graph;
			break;
		default:
			fprintf( stderr, "unrecognized triple node position %d in hx_triple_hash_on_node\n", pos );
	};
	char* string;
	hx_node_string( n, &string );
	uint64_t h	= hashfunc( string );
	free(string);
	return h;
}

uint64_t hx_triple_hash ( hx_triple* t, hx_hash_function* func ) {
	uint64_t s	= hx_triple_hash_on_node( t, HX_SUBJECT, func );
	uint64_t p	= hx_triple_hash_on_node( t, HX_PREDICATE, func );
	uint64_t o	= hx_triple_hash_on_node( t, HX_OBJECT, func );
	uint64_t g	= (t->graph) ? hx_triple_hash_on_node( t, HX_GRAPH, func ) : (uint64_t) 0;
	return s^p^o^g;
}
