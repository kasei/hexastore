#include "mentok/algebra/graphpattern.h"
#include "mentok/parser/SPARQLParser.h"

extern hx_graphpattern* parse_query_string ( char* );

int _hx_graphpattern_string_concat ( char** string, char* _new, int* alloc );

hx_graphpattern* hx_new_graphpattern ( hx_graphpattern_type_t type, ... ) {
	int i;
	va_list argp;
	hx_graphpattern* pat	= (hx_graphpattern*) calloc( 1, sizeof( hx_graphpattern ) );
	pat->type	= type;
	va_start(argp, type);
	hx_graphpattern** p;
	hx_node* n;
	void** vp;
	hx_bgp* bgp;
	switch (type) {
		case HX_GRAPHPATTERN_BGP:
			pat->arity	= 1;
			bgp			= va_arg( argp, hx_bgp* );
			pat->data	= bgp;
			break;
		case HX_GRAPHPATTERN_OPTIONAL:
			pat->arity	= 2;
			p			= calloc( 2, sizeof( hx_graphpattern* ) );
			p[0]		= va_arg( argp, hx_graphpattern* );
			p[1]		= va_arg( argp, hx_graphpattern* );
			pat->data	= p;
			break;
		case HX_GRAPHPATTERN_GROUP:
			pat->arity	= va_arg( argp, int );
			p			= calloc( pat->arity, sizeof( hx_graphpattern* ) );
			for (i = 0; i < pat->arity; i++) {
				p[i]	= va_arg( argp, hx_graphpattern* );
			}
			pat->data	= p;
			break;
		case HX_GRAPHPATTERN_UNION:
			pat->arity		= 2;
			p			= calloc( 2, sizeof( hx_graphpattern* ) );
			p[0]		= va_arg( argp, hx_graphpattern* );
			p[1]		= va_arg( argp, hx_graphpattern* );
			pat->data	= p;
			break;
		case HX_GRAPHPATTERN_GRAPH:
			pat->arity	= 2;
			vp			= calloc( 2, sizeof( void* ) );
			n			= (void*) va_arg( argp, hx_node* );
			vp[0]		= hx_node_copy( n );
			vp[1]		= (void*) va_arg( argp, hx_graphpattern* );
			pat->data	= vp;
			break;
		case HX_GRAPHPATTERN_SERVICE:
			pat->arity	= 2;
			vp			= calloc( 2, sizeof( void* ) );
			n			= (void*) va_arg( argp, hx_node* );
			vp[0]		= hx_node_copy( n );
			vp[1]		= (void*) va_arg( argp, hx_graphpattern* );
			pat->data	= vp;
			break;
		case HX_GRAPHPATTERN_FILTER:
			pat->arity	= 2;
			vp			= calloc( 2, sizeof( void* ) );
			vp[0]		= (void*) va_arg( argp, hx_expr* );
			vp[1]		= (void*) va_arg( argp, hx_graphpattern* );
			pat->data	= vp;
			break;
		default:
			fprintf( stderr, "*** Unrecognized graph pattern type '%c' in hx_new_graphpattern\n", type );
			return NULL;
	};
	
	va_end(argp);
	return pat;
}

hx_graphpattern* hx_new_graphpattern_ptr ( hx_graphpattern_type_t type, int size, void* ptr ) {
	int i;
	hx_graphpattern* pat	= (hx_graphpattern*) calloc( 1, sizeof( hx_graphpattern ) );
	pat->type	= type;
	hx_graphpattern** p;
	pat->arity	= size;
	hx_graphpattern** patterns	= (hx_graphpattern**) ptr;
	p			= calloc( pat->arity, sizeof( hx_graphpattern* ) );
	for (i = 0; i < size; i++) {
		p[i]	= patterns[i];
	}
	pat->data	= p;
	return pat;
}

hx_graphpattern_type_t hx_graphpattern_type ( hx_graphpattern* p ) {
	return p->type;
}

hx_graphpattern* hx_graphpattern_parse_string ( const char* string ) {
	return parse_query_string( (char*) string );
}

int hx_free_graphpattern ( hx_graphpattern* pat ) {
	int i;
	void** vp;
	hx_graphpattern** p;
	switch (pat->type) {
		case HX_GRAPHPATTERN_BGP:
			hx_free_bgp( (hx_bgp*) pat->data );
			break;
		case HX_GRAPHPATTERN_UNION:
		case HX_GRAPHPATTERN_OPTIONAL:
			p	= (hx_graphpattern**) pat->data;
			hx_free_graphpattern( p[0] );
			hx_free_graphpattern( p[1] );
			free( p );
			break;
		case HX_GRAPHPATTERN_GROUP:
			p	= (hx_graphpattern**) pat->data;
			for (i = 0; i < pat->arity; i++) {
				hx_free_graphpattern( p[i] );
			}
			free( p );
			break;
		case HX_GRAPHPATTERN_GRAPH:
		case HX_GRAPHPATTERN_SERVICE:
			vp	= (void**) pat->data;
			hx_free_node( (hx_node*) vp[0] );
			hx_free_graphpattern( (hx_graphpattern*) vp[1] );
			free( vp );
			break;
		case HX_GRAPHPATTERN_FILTER:
			vp	= (void**) pat->data;
			hx_free_expr( (hx_expr*) vp[0] );
			hx_free_graphpattern( (hx_graphpattern*) vp[1] );
			free( vp );
			break;
	};
	free( pat );
	return 0;
}

/* returns the number of variables referenced in the graph pattern, and sets vars to an array containing copies of those variables (must be freed by the caller) */
int hx_graphpattern_variables ( hx_graphpattern* pat, hx_node*** vars ) {
	int i, j, r;
	int size1, size2, size3;
	hx_node **set1, **set2, **set3;
	void** vp;
	hx_expr* e;
	hx_graphpattern *gp		= NULL;
	hx_graphpattern *gp2	= NULL;
	hx_variablebindings_iter *iter, *iter2;
	hx_graphpattern** p;
	switch (pat->type) {
		case HX_GRAPHPATTERN_BGP:
			return hx_bgp_variables( pat->data, vars );
		case HX_GRAPHPATTERN_FILTER:
			vp		= (void**) pat->data;
			gp		= (hx_graphpattern*) vp[1];
			return hx_graphpattern_variables( gp, vars );
		case HX_GRAPHPATTERN_UNION:
		case HX_GRAPHPATTERN_OPTIONAL:
		case HX_GRAPHPATTERN_GROUP:
			p		= (hx_graphpattern**) pat->data;
			size1	= hx_graphpattern_variables( p[0], &set1 );
			for (i = 1; i < pat->arity; i++) {
				size2	= hx_graphpattern_variables( p[i], &set2 );
				size3	= hx_node_uniq_set2( size1, set1, size2, set2, &set3, 1 );
				for (j = 0; j < size2; j++) {
					hx_free_node( set2[j] );
				}
				free(set2);
				for (j = 0; j < size1; j++) {
					hx_free_node( set1[j] );
				}
				free( set1 );
				set1	= set3;
				size1	= size3;
			}
			if (vars != NULL) {
				*vars	= set1;
			}
			return size1;
		case HX_GRAPHPATTERN_SERVICE:
			p		= (hx_graphpattern**) pat->data;
			size1	= hx_graphpattern_variables( p[1], &set1 );
			if (vars != NULL) {
				*vars	= set1;
			}
			return size1;
		case HX_GRAPHPATTERN_GRAPH:
		default:
			fprintf( stderr, "*** Unrecognized or unimplemented graph pattern type '%c' in hx_graphpattern_variables\n", pat->type );
			return 0;
	};
}

int hx_graphpattern_sse ( hx_graphpattern* pat, char** string, char* indent, int level ) {
	int i;
	char* indent1	= calloc( 1, strlen(indent) * level + 1 );
	char* indent2	= calloc( 1, strlen(indent) * (level+1) + 1 );
	for (i = 0; i < level; i++) {
		strcat( indent1, indent );
		strcat( indent2, indent );
	}
	strcat( indent2, indent );
	
	if (pat->type == HX_GRAPHPATTERN_BGP) {
		hx_bgp* b	= (hx_bgp*) pat->data;
		free(indent1);
		free(indent2);
		return hx_bgp_sse( b, string, indent, level );
	} else if (pat->type == HX_GRAPHPATTERN_GRAPH || pat->type == HX_GRAPHPATTERN_FILTER || pat->type == HX_GRAPHPATTERN_SERVICE) {
		void** vp	= pat->data;
		int alloc	= 256 + 15 + (strlen(indent) * level);
		char* str	= (char*) calloc( 1, alloc );
		if (pat->type == HX_GRAPHPATTERN_GRAPH) {
			char* ns;
			hx_node* n	= (hx_node*) vp[0];
			hx_node_string( n, &ns );
			snprintf( str, alloc, "(named-graph %s\n", ns );
			free( ns );
		} else if (pat->type == HX_GRAPHPATTERN_SERVICE) {
			char* ss;
			hx_node* n	= (hx_node*) vp[0];
			hx_node_string( n, &ss );
			snprintf( str, alloc, "(service %s\n", ss );
			free( ss );
		} else {
			char* es;
			hx_expr* e	= (hx_expr*) vp[0];
			hx_expr_sse( e, &es, indent, level+1 );
			snprintf( str, alloc, "(filter\n" );
			if (_hx_graphpattern_string_concat( &str, indent2, &alloc )) {
				free( es );
				free( str );
				free(indent1);
				free(indent2);
				return 1;
			}
			if (_hx_graphpattern_string_concat( &str, es, &alloc )) {
				free( es );
				free( str );
				free(indent1);
				free(indent2);
				return 1;
			}
			if (_hx_graphpattern_string_concat( &str, "\n", &alloc )) {
				free( es );
				free( str );
				free(indent1);
				free(indent2);
				return 1;
			}
			free( es );
		}
		
		char* gps;
		if (_hx_graphpattern_string_concat( &str, indent2, &alloc )) {
			free( str );
			free(indent1);
			free(indent2);
			return 1;
		}
		
		hx_graphpattern_sse( (hx_graphpattern*) vp[1], &gps, indent, level+1 );
		if (_hx_graphpattern_string_concat( &str, gps, &alloc )) {
			free( gps );
			free( str );
			free(indent1);
			free(indent2);
			return 1;
		}
		free(gps);
		if (_hx_graphpattern_string_concat( &str, indent1, &alloc )) {
			free( str );
			free(indent1);
			free(indent2);
			return 1;
		}
		if (_hx_graphpattern_string_concat( &str, ")\n", &alloc )) {
			free( str );
			free(indent1);
			free(indent2);
			return 1;
		}
		*string	= str;
	} else {
		char* name;
		int arity	= pat->arity;
		if (pat->type == HX_GRAPHPATTERN_OPTIONAL) {
			name	= "optional";
		} else if (pat->type == HX_GRAPHPATTERN_UNION) {
			name	= "union";
		} else if (pat->type == HX_GRAPHPATTERN_GROUP) {
			name	= "ggp";
		} else {
			fprintf( stderr, "*** unrecognized graph pattern type '%c' in hx_graphpattern_sse\n", pat->type );
			name	= NULL;
		}
		
		int alloc	= 256 + strlen(name) + 3 + (strlen(indent) * level);
		char* str	= (char*) calloc( 1, alloc );
		snprintf( str, alloc, "(%s\n", name );
		hx_graphpattern** p	= (hx_graphpattern**) pat->data;
		for (i = 0; i < arity; i++) {
			char* gps;
			if (_hx_graphpattern_string_concat( &str, indent2, &alloc )) {
				free( str );
				free(indent1);
				free(indent2);
				return 1;
			}
			
			hx_graphpattern_sse( p[i], &gps, indent, level+1 );
			if (_hx_graphpattern_string_concat( &str, gps, &alloc )) {
				free( gps );
				free( str );
				free(indent1);
				free(indent2);
				return 1;
			}
			free(gps);
		}
		if (_hx_graphpattern_string_concat( &str, indent1, &alloc )) {
			free( str );
			free(indent1);
			free(indent2);
			return 1;
		}
		if (_hx_graphpattern_string_concat( &str, ")\n", &alloc )) {
			free( str );
			free(indent1);
			free(indent2);
			return 1;
		}
		*string	= str;
	}
	
	free(indent1);
	free(indent2);
	return 0;
}

int hx_graphpattern_debug ( hx_graphpattern* p ) {
	char* string;
	hx_graphpattern_sse( p, &string, "  ", 0 );
	fprintf( stderr, "%s\n", string );
	free( string );
	return 0;
}

int _hx_graphpattern_string_concat ( char** string, char* _new, int* alloc ) {
	int sl	= strlen(*string);
	int nl	= strlen(_new);
	while (sl + nl + 1 >= *alloc) {
		*alloc	*= 2;
		char* newstring	= (char*) malloc( *alloc );
		if (newstring == NULL) {
			fprintf( stderr, "*** malloc failed in _hx_graphpattern_string_concat\n" );
		}
		if (newstring == NULL) {
			fprintf( stderr, "*** could not allocate memory for graphpattern string\n" );
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
