#include "mentok/engine/variablebindings_iter_sorting.h"

hx_variablebindings_iter_sorting* hx_variablebindings_iter_new_node_sorting ( hx_variablebindings_iter_sort_order order, int sparql_order, hx_node* node ) {
	hx_expr* e	= hx_new_node_expr( node );
	hx_variablebindings_iter_sorting* s	= hx_variablebindings_iter_new_sorting( order, sparql_order, e );
	hx_free_expr(e);
	return s;
}

hx_variablebindings_iter_sorting* hx_variablebindings_iter_new_sorting ( hx_variablebindings_iter_sort_order order, int sparql_order, hx_expr* expr ) {
	hx_variablebindings_iter_sorting* s	= (hx_variablebindings_iter_sorting*) calloc( 1, sizeof(hx_variablebindings_iter_sorting) );
	s->order		= order;
	s->sparql_order	= sparql_order;
	s->expr			= hx_copy_expr( expr );
	return s;
}

hx_variablebindings_iter_sorting* hx_copy_variablebindings_iter_sorting ( hx_variablebindings_iter_sorting* s ) {
	hx_variablebindings_iter_sorting* c	= (hx_variablebindings_iter_sorting*) calloc( 1, sizeof(hx_variablebindings_iter_sorting) );
	c->order		= s->order;
	c->sparql_order	= s->sparql_order;
	c->expr			= hx_copy_expr( s->expr );
	return c;
}

int hx_free_variablebindings_iter_sorting ( hx_variablebindings_iter_sorting* sorting ) {
	hx_free_expr( sorting->expr );
	free( sorting );
}

int hx_variablebindings_iter_sorting_string ( hx_variablebindings_iter_sorting* sorting, char** string ) {
	char* expr_string;
	hx_expr_sse( sorting->expr, &expr_string, "", 0 );
	int len	= 7 + strlen(expr_string);
	*string	= (char*) calloc( len, sizeof(char) );
	snprintf( *string, len, "%s(%s)", (sorting->order == HX_VARIABLEBINDINGS_ITER_SORT_ASCENDING ? "ASC" : "DESC"), expr_string );
	free(expr_string);
	return 0;
}

