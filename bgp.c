#include "bgp.h"
#include "mergejoin.h"
#include "project.h"

int _hx_bgp_selectivity_cmp ( const void* a, const void* b );
int _hx_bgp_sort_for_triple_join ( hx_triple* l, hx_triple* r );
int _hx_bgp_sort_for_vb_join ( hx_triple* l, hx_variablebindings_iter* iter );
int _hx_bgp_triple_joins_with_seen ( hx_bgp* b, hx_triple* t, int* seen, int size );
void _hx_bgp_triple_add_seen_variables ( hx_bgp* b, hx_triple* t, int* seen, int size );
	

typedef struct {
	uint64_t cost;
	hx_triple* triple;
} _hx_bgp_selectivity_t;

hx_bgp* hx_new_bgp ( int size, hx_triple** triples ) {
	hx_bgp* b	= (hx_bgp*) calloc( 1, sizeof( hx_bgp ) );
	b->size		= size;
	b->triples	= (hx_triple**) calloc( size, sizeof( hx_triple* ) );
	int vars	= 0;
	int i;
	for (i = 0; i < size; i++) {
		b->triples[i]	= triples[i];
		if (hx_node_is_variable( triples[i]->subject )) {
			int vid	= abs(hx_node_iv( triples[i]->subject ));
			if (vid > vars) {
				vars	= vid;
			}
		}
		if (hx_node_is_variable( triples[i]->predicate )) {
			int vid	= abs(hx_node_iv( triples[i]->predicate ));
			if (vid > vars) {
				vars	= vid;
			}
		}
		if (hx_node_is_variable( triples[i]->object )) {
			int vid	= abs(hx_node_iv( triples[i]->object ));
			if (vid > vars) {
				vars	= vid;
			}
		}
	}
	b->variables		= vars;
	b->variable_names	= (vars == 0) ? NULL : (char**) calloc( vars + 1, sizeof( char* ) );
	for (i = 0; i < size; i++) {
		if (hx_node_is_variable( triples[i]->subject )) {
			int vid	= abs(hx_node_iv( triples[i]->subject ));
			if (b->variable_names[ vid ] == NULL) {
				hx_node_variable_name( triples[i]->subject, &( b->variable_names[ vid ] ) );
			}
		}
		if (hx_node_is_variable( triples[i]->predicate )) {
			int vid	= abs(hx_node_iv( triples[i]->predicate ));
			if (b->variable_names[ vid ] == NULL) {
				hx_node_variable_name( triples[i]->predicate, &( b->variable_names[ vid ] ) );
			}
		}
		if (hx_node_is_variable( triples[i]->object )) {
			int vid	= abs(hx_node_iv( triples[i]->object ));
			if (b->variable_names[ vid ] == NULL) {
				hx_node_variable_name( triples[i]->object, &( b->variable_names[ vid ] ) );
			}
		}
	}
	return b;
}

hx_bgp* hx_new_bgp1 ( hx_triple* t1 ) {
	hx_bgp* b	= hx_new_bgp( 1, &t1 );
	return b;
}

hx_bgp* hx_new_bgp2 ( hx_triple* t1, hx_triple* t2 ) {
	hx_triple* triples[2];
	triples[0]	= t1;
	triples[1]	= t2;
	hx_bgp* b	= hx_new_bgp( 2, triples );
	return b;	
}

int hx_free_bgp ( hx_bgp* b ) {
	int i;
	if (b->variable_names != NULL) {
		for (i = 1; i <= b->variables; i++) {
			if (b->variable_names[i] != NULL) {
				free( b->variable_names[i] );
			}
		}
		free( b->variable_names );
	}
	
	for (i = 0; i < b->size; i++) {
		hx_free_triple( b->triples[i] );
		b->triples[i]	= NULL;
	}
	
	free( b->triples );
	free( b );
	return 0;
}


hx_bgp* hx_bgp_substitute_variables ( hx_bgp* orig, hx_variablebindings* b, hx_nodemap* map ) {
	int size	= hx_bgp_size ( orig );
	hx_triple** triples	= (hx_triple**) calloc( size, sizeof( hx_triple* ) );
	int i;
	for (i = 0; i < size; i++) {
		hx_node* nodes[3];
		hx_triple* t	= hx_bgp_triple( orig, i );
		nodes[0]	= t->subject;
		nodes[1]	= t->predicate;
		nodes[2]	= t->object;
		
		int j;
		for (j = 0; j < 3; j++) {
			if (hx_node_is_variable(nodes[j])) {
				char* name;
				hx_node_variable_name( nodes[j], &name );
				hx_node* n	= hx_variablebindings_node_for_binding_name( b, map, name );
				free(name);
				if (n != NULL) {
					nodes[j]	= n;
				}
			}
		}
		triples[i]	= hx_new_triple( nodes[0], nodes[1], nodes[2] );
	}
	hx_bgp* bgp	= hx_new_bgp( size, triples );
	free(triples);
	return bgp;
}

int hx_bgp_size ( hx_bgp* b ) {
	return b->size;
}

int hx_bgp_variables ( hx_bgp* b, hx_node*** v ) {
	int i;
	int counter	= 0;
	int uniq_count;
	int s	= hx_bgp_size( b );
	hx_node** vars	= (hx_node**) calloc( 3*s, sizeof( hx_node* ) );	// at most, 3*size(bgp) variables
	for (i = 0; i < s; i++) {
		hx_triple* t	= hx_bgp_triple( b, i );
		if (hx_node_is_variable( t->subject )) {
			vars[ counter++ ]	= t->subject;
		}
		if (hx_node_is_variable( t->predicate )) {
			vars[ counter++ ]	= t->predicate;
		}
		if (hx_node_is_variable( t->object )) {
			vars[ counter++ ]	= t->object;
		}
	}
	
	if (counter == 0) {
		return 0;
	}
	
	hx_node** uniq;
	uniq_count	= hx_node_uniq_set( counter, vars, &uniq, 1 );
	free( vars );
	
	if (v != NULL) {
		*v	= uniq;
	}
	return uniq_count;
}

hx_triple* hx_bgp_triple ( hx_bgp* b, int i ) {
	return b->triples[ i ];
}

int _hx_bgp_string_concat ( char** string, char* _new, int* alloc ) {
	int sl	= strlen(*string);
	int nl	= strlen(_new);
	while (sl + nl + 1 >= *alloc) {
		*alloc	*= 2;
		char* newstring	= (char*) malloc( *alloc );
		if (newstring == NULL) {
			fprintf( stderr, "*** malloc failed in _hx_bgp_string_concat\n" );
			return 1;
		}
		strcpy( newstring, *string );
		free( *string );
		*string	= newstring;
	}
	char* str	= *string;
	char* base	= &( str[ sl ] );
	strcpy( base, _new );
	return 0;
}

int hx_bgp_string ( hx_bgp* b, char** string ) {
	*string	= NULL;
	int alloc	= 256;
	char* str	= (char*) calloc( 1, alloc );
	
	int size	= hx_bgp_size( b );
	if (_hx_bgp_string_concat( &str, "{\n", &alloc )) return 1;
	
	hx_node *last_s	= NULL;
	hx_node *last_p	= NULL;
	int i;
	for (i = 0; i < size; i++) {
		char *s, *p, *o;
		hx_triple* t	= hx_bgp_triple( b, i );
		hx_node_string( t->subject, &s );
		hx_node_string( t->predicate, &p );
		hx_node_string( t->object, &o );
		
		if (last_s != NULL) {
			if (hx_node_cmp(t->subject, last_s) == 0) {
				if (hx_node_cmp(t->predicate, last_p) == 0) {
					if (_hx_bgp_string_concat( &str, ", ", &alloc )) return 1;
					if (_hx_bgp_string_concat( &str, o, &alloc )) return 1;
				} else {
					if (_hx_bgp_string_concat( &str, " ;\n\t\t", &alloc )) return 1;
					if (_hx_bgp_string_concat( &str, p, &alloc )) return 1;
					if (_hx_bgp_string_concat( &str, " ", &alloc )) return 1;
					if (_hx_bgp_string_concat( &str, o, &alloc )) return 1;
				}
			} else {
				if (_hx_bgp_string_concat( &str, " .\n\t", &alloc )) return 1;
				if (_hx_bgp_string_concat( &str, s, &alloc )) return 1;
				if (_hx_bgp_string_concat( &str, " ", &alloc )) return 1;
				if (_hx_bgp_string_concat( &str, p, &alloc )) return 1;
				if (_hx_bgp_string_concat( &str, " ", &alloc )) return 1;
				if (_hx_bgp_string_concat( &str, o, &alloc )) return 1;
			}
		} else {
			if (_hx_bgp_string_concat( &str, "\t", &alloc )) return 1;
			if (_hx_bgp_string_concat( &str, s, &alloc )) return 1;
			if (_hx_bgp_string_concat( &str, " ", &alloc )) return 1;
			if (_hx_bgp_string_concat( &str, p, &alloc )) return 1;
			if (_hx_bgp_string_concat( &str, " ", &alloc )) return 1;
			if (_hx_bgp_string_concat( &str, o, &alloc )) return 1;
		}
		last_s	= t->subject;
		last_p	= t->predicate;
		free( s );
		free( p );
		free( o );
	}
	if (_hx_bgp_string_concat( &str, " .\n}\n", &alloc )) return 1;
	*string	= str;
	return 0;
}

int hx_bgp_sse ( hx_bgp* b, char** string, char* indent, int level ) {
	*string	= NULL;
	int alloc	= 256;
	char* str	= (char*) calloc( 1, alloc );
	
	int size	= hx_bgp_size( b );
	if (_hx_bgp_string_concat( &str, "(bgp\n", &alloc )) return 1;
	
	int i;
	for (i = 0; i < size; i++) {
		int j;
		for (j = 0; j < level+1; j++) {
			if (_hx_bgp_string_concat( &str, indent, &alloc )) return 1;
		}
		char *tstring;
		hx_triple* t	= hx_bgp_triple( b, i );
		if (hx_triple_string( t, &tstring ) != 0) {
			return 1;
		}
		if (_hx_bgp_string_concat( &str, tstring, &alloc )) {
			free( tstring );
			return 1;
		}
		if (_hx_bgp_string_concat( &str, "\n", &alloc )) return 1;
		free( tstring );
	}
	int j;
	for (j = 0; j < level; j++) {
		if (_hx_bgp_string_concat( &str, indent, &alloc )) return 1;
	}
	if (_hx_bgp_string_concat( &str, ")\n", &alloc )) return 1;
	*string	= str;
	return 0;
}

int hx_bgp_debug ( hx_bgp* b ) {
	char* string;
	int r	= hx_bgp_string( b, &string );
	if (r == 0) {
		fprintf( stderr, string );
		free( string );
		return 0;
	} else {
		fprintf( stderr, "hx_bgp_string didn't return success\n" );
		return 1;
	}
}

int hx_bgp_reorder ( hx_bgp* b, hx_hexastore* hx ) {
	int size	= hx_bgp_size( b );
	_hx_bgp_selectivity_t* s	= (_hx_bgp_selectivity_t*) calloc( size, sizeof( _hx_bgp_selectivity_t ) );
	int i;
	for (i = 0; i < size; i++) {
		hx_triple* t	= hx_bgp_triple( b, i );
		s[i].triple		= t;
		s[i].cost		= hx_count_statements( hx, t->subject, t->predicate, t->object );
		if (s[i].cost == 0) {
			fprintf( stderr, "*** no results will be found, because this pattern has no associated triples\n" );
			// there are no triples for this pattern, so no sense in continuing
			return 1;
		}
	}
	
	qsort( s, size, sizeof( _hx_bgp_selectivity_t ), _hx_bgp_selectivity_cmp );
	
	
	int* seen	= (int*) calloc( b->variables + 1, sizeof( int ) );
	for (i = 0; i < size; i++) {
		hx_triple* t	= s[i].triple;
		if (i > 0) {
			int joins	= _hx_bgp_triple_joins_with_seen( b, t, seen, size );
			int j		= i;
			while (joins == 0) {
				j++;
				if (j >= size) {
					fprintf( stderr, "cartesian product\n" );
					return 1;
				} else {
					hx_triple* u	= s[j].triple;
					joins	= _hx_bgp_triple_joins_with_seen( b, u, seen, size );
				}
			}
			if (j != i) {
				uint64_t temp_cost	= s[j].cost;
				hx_triple* temp_t	= s[j].triple;
				s[j].cost	= s[i].cost;
				s[j].triple	= s[i].triple;
				s[i].cost	= temp_cost;
				s[i].triple	= temp_t;
			}
		}
		_hx_bgp_triple_add_seen_variables( b, t, seen, size );
	}
	
	for (i = 0; i < size; i++) {
		b->triples[i]	= s[i].triple;
	}
	
	free( seen );
	free( s );
	return 0;
}

void _hx_bgp_triple_add_seen_variables ( hx_bgp* b, hx_triple* t, int* seen, int size ) {
	if (hx_node_is_variable( t->subject )) {
		seen[ abs(hx_node_iv( t->subject )) ]++;
	}
	if (hx_node_is_variable( t->predicate )) {
		seen[ abs(hx_node_iv( t->predicate )) ]++;
	}
	if (hx_node_is_variable( t->object )) {
		seen[ abs(hx_node_iv( t->object )) ]++;
	}
}

int _hx_bgp_triple_joins_with_seen ( hx_bgp* b, hx_triple* t, int* seen, int size ) {
	int join_with_previously_seen	= 0;
	if (hx_node_is_variable( t->subject ) && seen[ abs(hx_node_iv( t->subject )) ] > 0) {
		join_with_previously_seen++;
	}
	if (hx_node_is_variable( t->predicate ) && seen[ abs(hx_node_iv( t->predicate )) ] > 0) {
		join_with_previously_seen++;
	}
	if (hx_node_is_variable( t->object ) && seen[ abs(hx_node_iv( t->object )) ] > 0) {
		join_with_previously_seen++;
	}
	return join_with_previously_seen;
}


hx_variablebindings_iter* hx_bgp_execute ( hx_bgp* b, hx_hexastore* hx ) {
	int size	= hx_bgp_size( b );
	
	hx_triple* t0	= hx_bgp_triple( b, 0 );
	int sort;
	if (size > 1) {
		sort	= _hx_bgp_sort_for_triple_join( t0, hx_bgp_triple( b, 1 ) );
	} else {
		sort	= HX_SUBJECT;
	}
	
	hx_index_iter* titer0	= hx_get_statements( hx, t0->subject, t0->predicate, t0->object, sort );
	
	char *sname, *pname, *oname;
	hx_node_variable_name( t0->subject, &sname );
	hx_node_variable_name( t0->predicate, &pname );
	hx_node_variable_name( t0->object, &oname );
	hx_variablebindings_iter* iter	= hx_new_iter_variablebindings( titer0, sname, pname, oname );
	free(sname);
	free(pname);
	free(oname);
	
	if (size > 1) {
		int i;
		for (i = 1; i < size; i++) {
			char *sname, *pname, *oname;
			hx_triple* t			= hx_bgp_triple( b, i );
			int jsort				= _hx_bgp_sort_for_vb_join( t, iter );
			hx_index_iter* titer	= hx_get_statements( hx, t->subject, t->predicate, t->object, jsort );
			if (titer == NULL) {
				return NULL;
			}
			hx_node_variable_name( t->subject, &sname );
			hx_node_variable_name( t->predicate, &pname );
			hx_node_variable_name( t->object, &oname );
			hx_variablebindings_iter* interm	= hx_new_iter_variablebindings( titer, sname, pname, oname );
			free(sname);
			free(pname);
			free(oname);
			
			iter					= hx_new_mergejoin_iter( interm, iter );
		}
	}
	
	hx_node** variables;
	int count	= hx_bgp_variables( b, &variables );
	char** proj_nodes	= (char**) calloc( count, sizeof( char* ) );
	int proj_count	= 0;
	
	// Now project away any variables that are non-distinguished
	int i;
	for (i = 0; i < count; i++) {
		hx_node* v	= variables[i];
		if (hx_node_is_distinguished_variable( v )) {
			char* string;
			hx_node_variable_name( v, &string );
			proj_nodes[ proj_count++ ]	= string;
		}
	}
	free( variables );
	
	if (proj_count < count) {
		iter	= hx_new_project_iter( iter, proj_count, proj_nodes );
	}
	for (i = 0; i < proj_count; i++) {
		free( proj_nodes[i] );
	}
	free( proj_nodes );
	
	return iter;
}

int _hx_bgp_selectivity_cmp ( const void* _a, const void* _b ) {
	const _hx_bgp_selectivity_t* a	= (_hx_bgp_selectivity_t*) _a;
	const _hx_bgp_selectivity_t* b	= (_hx_bgp_selectivity_t*) _b;
	int64_t d	= (a->cost - b->cost);
	if (d < 0) {
		return -1;
	} else if (d > 0) {
		return 1;
	} else {
		return 0;
	}
}

int _hx_bgp_sort_for_triple_join ( hx_triple* l, hx_triple* r ) {
	int pos[3]			= { HX_SUBJECT, HX_PREDICATE, HX_OBJECT };
	hx_node* lnodes[3]	= { l->subject, l->predicate, l->object };
	hx_node* rnodes[3]	= { r->subject, r->predicate, r->object };
	int i;
	for (i = 0; i < 3; i++) {
		if (hx_node_is_variable( lnodes[i] )) {
			int j;
			for (j = 0; j < 3; j++) {
				if (hx_node_is_variable( rnodes[j] )) {
					if (hx_node_cmp(lnodes[i], rnodes[j]) == 0) {
// 						fprintf( stderr, "should sort on %d\n", pos[i] );
						return pos[i];
					}
				}
			}
		}
	}
	return HX_SUBJECT;
}

int _hx_bgp_sort_for_vb_join ( hx_triple* l, hx_variablebindings_iter* iter ) {
	int pos[3]			= { HX_SUBJECT, HX_PREDICATE, HX_OBJECT };
	hx_node* lnodes[3]	= { l->subject, l->predicate, l->object };
	int rsize			= hx_variablebindings_iter_size( iter );
	char** rnames		= hx_variablebindings_iter_names( iter );
	int j;
	for (j = 0; j < rsize; j++) {
		if (hx_variablebindings_iter_is_sorted_by_index(iter, j)) {
			int i;
			for (i = 0; i < 3; i++) {
				if (hx_node_is_variable( lnodes[i] )) {
					char* lname;
					hx_node_variable_name( lnodes[i], &lname );
					if (strcmp(lname, rnames[j]) == 0) {
						free( lname );
						return pos[i];
					}
					free( lname );
				}
			}
		}
	}
	
	
	int i;
	for (i = 0; i < 3; i++) {
		if (hx_node_is_variable( lnodes[i] )) {
			char* lname;
			hx_node_variable_name( lnodes[i], &lname );
			int j;
			for (j = 0; j < rsize; j++) {
				if (strcmp(lname, rnames[j]) == 0) {
					free( lname );
//					fprintf( stderr, "should sort on %d (%s)\n", pos[i], lname );
					return pos[i];
				}
			}
			free( lname );
		}
	}
	return HX_SUBJECT;
}

hx_node_id* hx_bgp_thaw_ids ( char* ptr, int* len ) {
	hx_node_id* buf	= (hx_node_id*) ptr;
	int size		= (int) buf[0];
	*len			= size * 3;		// size is the number of triples, not nodes
	hx_node_id* ids	= (hx_node_id*) calloc( 3*size, sizeof( hx_node_id ) );
	int i;
	for (i = 0; i < size*3; i++) {
		ids[i]	= buf[i+1];
	}
	return ids;
}

hx_node_id _hx_bgp_get_node_id ( hx_nodemap* map, hx_node* node ) {
	if (hx_node_is_variable(node)) {
		return (hx_node_id) hx_node_iv(node);
	} else {
		return hx_nodemap_get_node_id( map, node );
	}
}

char* hx_bgp_freeze( hx_bgp* b, int* len, hx_nodemap* map ) {
	int size		= hx_bgp_size(b);
	int _len		= (1 + (3 * size)) * sizeof( hx_node_id );
	*len			= _len;
	hx_node_id* buf	= (hx_node_id*) calloc( 1, _len );
	buf[0]			= size;				// the first slot is the size of the bgp
	hx_node_id* ptr	= &( buf[1] );	// the rest is a set of node IDs, in triple groups
	int i;
	for (i = 0; i < size; i++) {
		hx_triple* t	= b->triples[i];
		hx_node_id s	= _hx_bgp_get_node_id( map, t->subject );
		hx_node_id p	= _hx_bgp_get_node_id( map, t->predicate );
		hx_node_id o	= _hx_bgp_get_node_id( map, t->object );
		ptr[ i*3 ]		= s;
		ptr[ i*3 + 1 ]	= p;
		ptr[ i*3 + 2 ]	= o;
	}
	
	return (char*) buf;
}

