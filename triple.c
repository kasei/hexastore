#include "triple.h"

hx_triple* hx_new_triple( hx_node* s, hx_node* p, hx_node* o ) {
	hx_triple* t	= (hx_triple*) calloc( 1, sizeof( hx_triple ) );
	t->subject		= s;
	t->predicate	= p;
	t->object		= o;
	return t;
}

int hx_free_triple ( hx_triple* t ) {
	free( t );
	return 0;
}

int hx_triple_string ( hx_triple* t, char** string ) {
	int len	= 12; // "(triple * * *)"
	char *s, *p, *o;
	hx_node_string( t->subject, &s );
	hx_node_string( t->predicate, &p );
	hx_node_string( t->object, &o );
	len	+= strlen( s );
	len	+= strlen( p );
	len	+= strlen( o );
	
	*string	= malloc( len );
	if (*string == NULL) {
		return 1;
	}
	snprintf( *string, len, "(triple %s %s %s)", s, p, o );
	free( s );
	free( p );
	free( o );
	return 0;
}
