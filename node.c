#include "node.h"

int _hx_node_parse_datatypes ( hx_node* n );
int _hx_node_array_cmp( const void* _a, const void* _b );

hx_node* _hx_new_node ( char type, char* value, int padding, int flags, int iv, double nv ) {
	hx_node* n	= (hx_node*) calloc( 1, sizeof( hx_node ) + padding );
	n->type		= type;
	n->flags	= flags;
	if (flags & HX_NODE_IOK) {
		n->iv		= iv;
	}
	
	if (flags & HX_NODE_NOK) {
		n->nv		= nv;
	}
	
	if (value != NULL) {
		n->value	= (char*) malloc( strlen( value ) + 1 );
		if (n->value == NULL) {
			fprintf( stderr, "*** malloc failed in _hx_new_node\n" );
		}
		strcpy( n->value, value );
	}
	return n;
}

hx_node* hx_new_node_variable ( int value ) {
	hx_node* n	= _hx_new_node( '?', NULL, 0, HX_NODE_IOK, value, 0.0 );
	return n;
}

hx_node* hx_new_node_variable_nondistinguished ( int value ) {
	hx_node* n	= _hx_new_node( '[', NULL, 0, HX_NODE_IOK, value, 0.0 );
	return n;
}

hx_node* hx_new_node_named_variable( int value, char* name ) {
	hx_node* n	= _hx_new_node( '?', name, 0, HX_NODE_IOK, value, 0.0 );
	return n;
}

hx_node* hx_new_node_named_variable_nondistinguished( int value, char* name ) {
	hx_node* n	= _hx_new_node( '[', name, 0, HX_NODE_IOK, value, 0.0 );
	return n;
}

hx_node* hx_new_node_resource ( char* value ) {
	hx_node* n	= _hx_new_node( 'R', value, 0, HX_NODE_NONE, 0, 0.0 );
	return n;
}

hx_node* hx_new_node_blank ( char* value ) {
	hx_node* n	= _hx_new_node( 'B', value, 0, HX_NODE_NONE, 0, 0.0 );
	return n;
}

hx_node* hx_new_node_literal ( char* value ) {
	hx_node* n	= _hx_new_node( 'L', value, 0, HX_NODE_NONE, 0, 0.0 );
	_hx_node_parse_datatypes( n );
	return n;
}

hx_node_lang_literal* hx_new_node_lang_literal ( char* value, char* lang ) {
	int padding	= sizeof( hx_node_lang_literal ) - sizeof( hx_node );
	hx_node_lang_literal* n	= (hx_node_lang_literal*) _hx_new_node( 'G', value, padding, HX_NODE_NONE, 0, 0.0 );
	n->lang		= (char*) malloc( strlen( lang ) + 1 );
	if (n->lang == NULL) {
		fprintf( stderr, "*** malloc failed in hx_new_node_lang_literal\n" );
	}
	if (n->lang == NULL) {
		free( n->value );
		free( n );
		return NULL;
	}
	strcpy( n->lang, lang );
	_hx_node_parse_datatypes( (hx_node*) n );
	return n;
}

hx_node_dt_literal* hx_new_node_dt_literal ( char* value, char* dt ) {
	int padding	= sizeof( hx_node_dt_literal ) - sizeof( hx_node );
	hx_node_dt_literal* n	= (hx_node_dt_literal*) _hx_new_node( 'D', value, padding, HX_NODE_NONE, 0, 0.0 );
	n->dt		= (char*) malloc( strlen( dt ) + 1 );
	if (n->dt == NULL) {
		fprintf( stderr, "*** malloc failed in hx_new_node_dt_literal\n" );
	}
	if (n->dt == NULL) {
		free( n->value );
		free( n );
		return NULL;
	}
	strcpy( n->dt, dt );
	_hx_node_parse_datatypes( (hx_node*) n );
	return n;
}

hx_node* hx_node_copy( hx_node* n ) {
	if (hx_node_is_literal( n )) {
		if (hx_node_is_lang_literal( n )) {
			hx_node_lang_literal* d	= (hx_node_lang_literal*) n;
			int padding	= sizeof( hx_node_lang_literal ) - sizeof( hx_node );
			hx_node_lang_literal* copy	= (hx_node_lang_literal*) _hx_new_node( 'G', d->value, padding, HX_NODE_NONE, 0, 0.0 );
			copy->lang		= (char*) malloc( strlen( d->lang ) + 1 );
			if (copy->lang == NULL) {
				fprintf( stderr, "*** malloc failed in hx_node_copy\n" );
			}
			copy->flags		= d->flags;
			copy->iv		= d->iv;
			copy->nv		= d->nv;
			strcpy( copy->lang, d->lang );
			_hx_node_parse_datatypes( (hx_node*) copy );
			return (hx_node*) copy;
		} else if (hx_node_is_dt_literal( n )) {
			hx_node_dt_literal* d	= (hx_node_dt_literal*) n;
			int padding	= sizeof( hx_node_dt_literal ) - sizeof( hx_node );
			hx_node_dt_literal* copy	= (hx_node_dt_literal*) _hx_new_node( 'D', d->value, padding, HX_NODE_NONE, 0, 0.0 );
			copy->dt		= (char*) malloc( strlen( d->dt ) + 1 );
			if (copy->dt == NULL) {
				fprintf( stderr, "*** malloc failed in hx_node_copy\n" );
			}
			copy->flags		= d->flags;
			copy->iv		= d->iv;
			copy->nv		= d->nv;
			strcpy( copy->dt, d->dt );
			_hx_node_parse_datatypes( (hx_node*) copy );
			return (hx_node*) copy;
		} else {
			hx_node* copy	= hx_new_node_literal( n->value );
			return copy;
		}
	} else if (hx_node_is_resource( n )) {
		hx_node* copy	= hx_new_node_resource( n->value );
		return copy;
	} else if (hx_node_is_blank( n )) {
		hx_node* copy	= hx_new_node_blank( n->value );
		return copy;
	} else if (hx_node_is_distinguished_variable( n )) {
		if (n->value == NULL) {
			return hx_new_node_variable( n->iv );
		} else {
			return hx_new_node_named_variable( n->iv, n->value );
		}
	} else if (hx_node_is_variable( n )) {
		if (n->value == NULL) {
			return hx_new_node_variable_nondistinguished( n->iv );
		} else {
			return hx_new_node_named_variable_nondistinguished( n->iv, n->value );
		}
	} else {
		fprintf( stderr, "*** not a recognized node type in hx_node_copy\n" );
	}
	return NULL;
}

int hx_free_node( hx_node* n ) {
	if (n->type == 'G') {
		hx_node_lang_literal* l	= (hx_node_lang_literal*) n;
		free( l->lang );
	} else if (n->type == 'D') {
		hx_node_dt_literal* d	= (hx_node_dt_literal*) n;
		free( d->dt );
	}
	if (n->value != NULL) {
		free( n->value );
	}
	
	n->type		= (char) 0;
	n->value	= NULL;
	free( n );
	return 0;
}

size_t hx_node_alloc_size( hx_node* n ) {
	if (hx_node_is_literal( n )) {
		if (hx_node_is_lang_literal( n )) {
			return sizeof( hx_node_lang_literal );
		} else if (hx_node_is_dt_literal( n )) {
			return sizeof( hx_node_dt_literal );
		} else {
			return sizeof( hx_node );
		}
	} else if (hx_node_is_resource( n )) {
		return sizeof( hx_node );
	} else if (hx_node_is_blank( n )) {
		return sizeof( hx_node );
	} else {
		fprintf( stderr, "*** Unrecognized node type '%c' in hx_node_alloc_size\n", n->type );
		return 0;
	}
}

int hx_node_is_variable ( hx_node* n ) {
	return ((n->type == '?') || (n->type == '['));
}

int hx_node_is_distinguished_variable ( hx_node* n ) {
	return (n->type == '?');
}

int hx_node_is_literal ( hx_node* n ) {
	return (n->type == 'L' || n->type == 'G' || n->type == 'D');
}

int hx_node_is_lang_literal ( hx_node* n ) {
	return (n->type == 'G');
}

int hx_node_is_dt_literal ( hx_node* n ) {
	return (n->type == 'D');
}

int hx_node_is_resource ( hx_node* n ) {
	return (n->type == 'R');
}

int hx_node_is_blank ( hx_node* n ) {
	return (n->type == 'B');
}

char* hx_node_value ( hx_node* n ) {
	return n->value;
}

int hx_node_variable_name ( hx_node* n, char** name ) {
	if (n->type == '?' || n->type == '[') {
		if (n->value == NULL) {
			int alloc	= 10 + 7;
			*name	= (char*) calloc( 1, alloc );
			if (*name == NULL) {
				fprintf( stderr, "*** malloc failed in hx_node_variable_name\n" );
				return 1;
			}
			sprintf( *name, "__%s_%d", (n->type == '?' ? "var" : "ndv"), n->iv );
		} else if (n->type == '?') {
			*name	= (char*) malloc( strlen( n->value ) + 1 );
			if (*name == NULL) {
				fprintf( stderr, "*** malloc failed in hx_node_variable_name\n" );
				return 1;
			}
			strcpy( *name, n->value );
		} else {
			*name	= (char*) malloc( strlen( n->value ) + 10 );
			if (*name == NULL) {
				fprintf( stderr, "*** malloc failed in hx_node_variable_name\n" );
				return 1;
			}
			sprintf( *name, "__ndv_%s", n->value );
		}
	} else {
		*name	= NULL;
	}
	return 0;
}

int hx_node_ivok( hx_node* n ) {
	return (n->flags & HX_NODE_IOK) ? 1 : 0;
}

int hx_node_nvok( hx_node* n ) {
	return (n->flags & HX_NODE_NOK) ? 1 : 0;
}

int hx_node_iv ( hx_node* n ) {
	return n->iv;
}

double hx_node_nv ( hx_node* n ) {
	return n->nv;
}

char* hx_node_lang ( hx_node_lang_literal* n ) {
	return n->lang;
}

char* hx_node_dt ( hx_node_dt_literal* n ) {
	return n->dt;
}

int hx_node_string ( hx_node* n, char** str ) {
	int alloc	= 0;
	if (hx_node_is_literal( n )) {
		alloc	= strlen(n->value) + 3;
		if (hx_node_is_lang_literal( n )) {
			hx_node_lang_literal* l	= (hx_node_lang_literal*) n;
			alloc	+= 1 + strlen( l->lang );
		} else if (hx_node_is_dt_literal( n )) {
			hx_node_dt_literal* d	= (hx_node_dt_literal*) n;
			alloc	+= 4 + strlen( d->dt );
		}
		*str	= (char*) calloc( 1, alloc );
		if (*str == NULL) {
			return 0;
		}
		
		if (hx_node_is_lang_literal( n )) {
			hx_node_lang_literal* l	= (hx_node_lang_literal*) n;
			sprintf( *str, "\"%s\"@%s", l->value, l->lang );
		} else if (hx_node_is_dt_literal( n )) {
			hx_node_dt_literal* d	= (hx_node_dt_literal*) n;
			sprintf( *str, "\"%s\"^^<%s>", d->value, d->dt );
		} else {
			sprintf( *str, "\"%s\"", n->value );
		}
	} else if (hx_node_is_resource( n )) {
		alloc	= strlen(n->value) + 3;
		*str	= (char*) calloc( 1, alloc );
		if (*str == NULL) {
			return 0;
		}
		sprintf( *str, "<%s>", n->value );
	} else if (hx_node_is_blank( n )) {
		alloc	= strlen(n->value) + 3;
		*str	= (char*) calloc( 1, alloc );
		if (*str == NULL) {
			return 0;
		}
		sprintf( *str, "_:%s", n->value );
	} else if (hx_node_is_variable( n )) {
		char* vname;
		hx_node_variable_name( n, &vname );
		alloc	= 2 + strlen( vname );
		*str	= (char*) calloc( 1, alloc );
		if (*str == NULL) {
			return 0;
		}
		sprintf( *str, "?%s", vname );
		free( vname );
	} else {
		fprintf( stderr, "*** Unrecognized node type '%c'\n", n->type );
		return 0;
	}
	return alloc;
}

hx_node* hx_node_parse ( char* ntnode ) {
	switch(ntnode[0]) {
		case '<': {
			ntnode[strlen(ntnode)-1] = '\0';
			return hx_new_node_resource(&ntnode[1]);
		}
		case '_': {
			return hx_new_node_blank(&ntnode[2]);
		}
		case '"': {
			int max_idx = strlen(ntnode) - 1;
			switch(ntnode[max_idx]) {
				case '"': {
					ntnode[max_idx] = '\0';
					return hx_new_node_literal(&ntnode[1]);
				}
				case '>': {
					ntnode[max_idx] = '\0';
					char* last_quote_p = strrchr(ntnode, '"');
					if(last_quote_p == NULL) {
						fprintf(stderr, "%s:%u: Error in _mpi_rdfio_to_hx_node_p; expected typed literal, but found %s\n", __FILE__, __LINE__, ntnode);
						return NULL;
					}
					last_quote_p[0] = '\0';
					return (hx_node*)hx_new_node_dt_literal(&ntnode[1], &last_quote_p[4]);
				}
				default: {
					char* last_quote_p = strrchr(ntnode, '"');
					if(last_quote_p == NULL) {
						fprintf(stderr, "%s:%u: Error in _mpi_rdfio_to_hx_node_p; expected literal with language tag, but found %s\n", __FILE__, __LINE__, ntnode);
						return NULL;
					}
					last_quote_p[0] = '\0';
					return (hx_node*)hx_new_node_lang_literal(&ntnode[1], &last_quote_p[2]);
				}
			}
		}
		default: {
			fprintf(stderr, "%s:%u: Error in _mpi_rdfio_to_hx_node_p; invalid N-triples node %s\n", __FILE__, __LINE__, ntnode);
			return NULL;
		}
	}
}

int hx_node_uniq_set ( int size, hx_node** set, hx_node*** v, int copy ) {
	int i, j, uniq_count;
	hx_node** vars	= (hx_node**) calloc( size, sizeof( hx_node* ) );
	for (i = 0; i < size; i++) {
		vars[i]	= set[i];
	}
	
	qsort( vars, size, sizeof( hx_node* ), _hx_node_array_cmp );
	j	= 0;
	for (i = 1; i < size; i++) {
		hx_node* last	= vars[j];
		hx_node* new	= vars[i];
		if (hx_node_cmp(last,new) != 0) {
			vars[++j]	= new;
		}
	}
	uniq_count	= j+1;
	for (i = j+1; i < size; i++) {
		vars[i]	= NULL;
	}
	
// 	for (i = 0; i < uniq_count; i++) {
// 		char* string;
// 		hx_node_string( vars[i], &string );
// 		fprintf( stderr, "uniq bgp variable: %s\n", string );
// 		free(string);
// 	}
	
	if (v != NULL) {
		*v	= (hx_node**) calloc( uniq_count, sizeof( hx_node* ) );
		for (i = 0; i < uniq_count; i++) {
			if (copy) {
				(*v)[i]	= hx_node_copy( vars[i] );
			} else {
				(*v)[i]	= vars[i];
			}
		}
	}
	free( vars );
	return uniq_count;
}

int hx_node_uniq_set2 ( int size1, hx_node** set1, int size2, hx_node** set2, hx_node*** v, int copy ) {
	int i;
	hx_node** vars	= (hx_node**) calloc( size1 + size2, sizeof( hx_node* ) );
	for (i = 0; i < size1; i++) {
		vars[i]	= set1[i];
	}
	for (i = 0; i < size2; i++) {
		vars[size1+i]	= set2[i];
	}
	int r	= hx_node_uniq_set( size1 + size2, vars, v, copy );
	free( vars );
	return r;
}

int hx_node_nodestr( hx_node* n, char** str ) {
	int alloc	= 0;
	if (hx_node_is_literal( n )) {
		alloc	= strlen(n->value) + 4;
		char* lang	= NULL;
		char* dt	= NULL;
		if (hx_node_is_lang_literal( n )) {
			hx_node_lang_literal* l	= (hx_node_lang_literal*) n;
			lang	= hx_node_lang( l );
			alloc	+= strlen( l->lang );
		} else if (hx_node_is_dt_literal( n )) {
			hx_node_dt_literal* d	= (hx_node_dt_literal*) n;
			dt	= hx_node_dt( d );
			alloc	+= strlen( d->dt );
		}
		*str	= (char*) calloc( 1, alloc );
		if (*str == NULL) {
			return 0;
		}
		sprintf( *str, "L%s<%s>%s", n->value, lang, dt );
	} else if (hx_node_is_resource( n )) {
		alloc	= strlen(n->value) + 2;
		*str	= (char*) calloc( 1, alloc );
		if (*str == NULL) {
			return 0;
		}
		sprintf( *str, "R%s", n->value );
	} else if (hx_node_is_blank( n )) {
		alloc	= strlen(n->value) + 2;
		*str	= (char*) calloc( 1, alloc );
		if (*str == NULL) {
			return 0;
		}
		sprintf( *str, "B%s", n->value );
	} else {
		fprintf( stderr, "*** Unrecognized node type '%c'\n", n->type );
		return 0;
	}
	return alloc;
}

int hx_node_cmp( const void* _a, const void* _b ) {
	hx_node* a	= (hx_node*) _a;
	hx_node* b	= (hx_node*) _b;
	
	if (a->type == b->type) {
		if (hx_node_is_blank( a )) {
			return strcmp( a->value, b->value );
		} else if (hx_node_is_resource( a )) {
			return strcmp( a->value, b->value );
		} else if (hx_node_is_literal( a )) {
			// XXX need to deal with language and datatype literals
			return strcmp( a->value, b->value );
		} else if (hx_node_is_variable( a )) {
			return (hx_node_iv( b ) - hx_node_iv( a ));
		} else {
			fprintf( stderr, "*** Unknown node type %c in _sparql_sort_cmp\n", a->type );
			return 0;
		}
	} else {
		int a_blank	= hx_node_is_blank( a );
		int b_blank	= hx_node_is_blank( b );
		int a_lit	= hx_node_is_literal( a );
		int b_lit	= hx_node_is_literal( b );
		int a_var	= hx_node_is_variable( a );
		int b_var	= hx_node_is_variable( b );
		int a_res	= hx_node_is_resource( a );
		int b_res	= hx_node_is_resource( b );
		
		if (a_blank) {
			return -1;
		} else if (a_res) {
			if (b_blank) {
				return 1;
			} else {
				return -1;
			}
		} else if (a_lit) {
			if (b_var) {
				return -1;
			} else {
				return 1;
			}
		} else if (a_var) {
			return 1;
		} else {
			fprintf( stderr, "*** Unrecognized node type in hx_node_cmp:\n" );
			char* string;
			hx_node_string( a, &string );
			fprintf( stderr, "\t%s\n", string );
			free(string);
			return 0;
		}
	}
	return 0;
}

// returns 1 if the EBV of n is true, 0 if the EBV is false, and -1 if the EBV of n produces a type error
int hx_node_ebv ( hx_node* n ) {
	if (hx_node_is_dt_literal(n)) {
		char* dt	= hx_node_dt( (hx_node_dt_literal*) n );
		if (strcmp(dt,"http://www.w3.org/2001/XMLSchema#boolean") == 0) {
			char* value	= hx_node_value(n);
			if (strcmp(value,"true") == 0) {
				return 1;
			} else {
				return 0;
			}
		} else if (strcmp(dt,"http://www.w3.org/2001/XMLSchema#string") == 0) {
			char* value	= hx_node_value(n);
			return (strlen(value) > 0);
		} else if (strncmp(dt,"http://www.w3.org/2001/XMLSchema#",33) == 0) {
			char* type	= &( dt[33] );
			// XXX need to generalize this to handle all the xsd numeric types
			if (strcmp(type,"integer") == 0) {
				return (hx_node_iv(n) != 0);
			} else if (strcmp(type,"float") == 0) {
				return (hx_node_nv(n) != 0.0);
			} else {
				fprintf( stderr, "*** unhandled xsd type in hx_node_ebv: %s\n", type );
				return 0;
			}
		} else {
			return 0;
		}
	} else if (hx_node_is_literal(n)) {
			char* value	= hx_node_value(n);
			return (strlen(value) > 0);
	} else {
		return -1;
	}
}

int hx_node_write( hx_node* n, FILE* f ) {
	if (n->type == '?' || n->type == '[') {
//		fprintf( stderr, "*** Cannot write variable nodes to a file.\n" );
		return 1;
	}
	fputc( 'N', f );
	fputc( n->type, f );
	size_t len	= (size_t) strlen( n->value );
	fwrite( &len, sizeof( size_t ), 1, f );
	fwrite( n->value, 1, strlen( n->value ), f );
	if (hx_node_is_literal( n )) {
		if (hx_node_is_lang_literal( n )) {
			hx_node_lang_literal* l	= (hx_node_lang_literal*) n;
			size_t len	= strlen( l->lang );
			fwrite( &len, sizeof( size_t ), 1, f );
			fwrite( l->lang, 1, strlen( l->lang ), f );
		}
		if (hx_node_is_dt_literal( n )) {
			hx_node_dt_literal* d	= (hx_node_dt_literal*) n;
			size_t len	= strlen( d->dt );
			fwrite( &len, sizeof( size_t ), 1, f );
			fwrite( d->dt, 1, strlen( d->dt ), f );
		}
	}
	return 0;
}

hx_node* hx_node_read( FILE* f, int buffer ) {
	size_t used, read;
	int c	= fgetc( f );
	if (c != 'N') {
		fprintf( stderr, "*** Bad header cookie ('%c') trying to read node from file.\n", c );
		return NULL;
	}
	
	char* value;
	char* extra	= NULL;
	hx_node* node;
	c	= fgetc( f );
	switch (c) {
		case 'R':
			read	= fread( &used, sizeof( size_t ), 1, f );
			value	= (char*) calloc( 1, used + 1 );
			read	= fread( value, 1, used, f );
			node	= hx_new_node_resource( value );
			free( value );
			return node;
		case 'B':
			read	= fread( &used, sizeof( size_t ), 1, f );
			value	= (char*) calloc( 1, used + 1 );
			read	= fread( value, 1, used, f );
			node	= hx_new_node_blank( value );
			free( value );
			return node;
		case 'L':
		case 'G':
		case 'D':
			read	= fread( &used, sizeof( size_t ), 1, f );
			value	= (char*) calloc( 1, used + 1 );
			read	= fread( value, 1, used, f );
			if (c == 'G' || c == 'D') {
				read	= fread( &used, sizeof( size_t ), 1, f );
				extra	= (char*) calloc( 1, used + 1 );
				read	= fread( extra, 1, used, f );
			}
			if (c == 'G') {
				node	= (hx_node*) hx_new_node_lang_literal( value, extra );
			} else if (c == 'D') {
				node	= (hx_node*) hx_new_node_dt_literal( value, extra );
			} else {
				node	= hx_new_node_literal( value );
			}
			free( value );
			if (extra != NULL)
				free( extra );
			return node;
		default:
			fprintf( stderr, "*** Bad node type '%c' trying to read node from file.\n", (char) c );
			return NULL;
	};
	
}

int _hx_node_parse_datatypes ( hx_node* n ) {
	if (!hx_node_is_dt_literal(n)) {
		return 1;
	}
	
	char* dt	= hx_node_dt( (hx_node_dt_literal*) n );
	if (strcmp( dt, "http://www.w3.org/2001/XMLSchema#integer" ) == 0) {
		char* value	= hx_node_value( n );
		int iv		= atoi( value );
		n->iv		= iv;
		n->flags	|= HX_NODE_IOK;
	} else if (strcmp( dt, "http://www.w3.org/2001/XMLSchema#float" ) == 0) {
		char* ptr;
		char* value	= hx_node_value( n );
		double nv	= strtod( value, &ptr );
		int diff	= ptr - value;
		n->nv		= nv;
		n->flags	|= HX_NODE_NOK;
	}
	
	return 0;
}

/* takes two hx_node**, derefs them, and calls hx_node_cmp -- allows calling qsort on an array of hx_node*s */
int _hx_node_array_cmp( const void* _a, const void* _b ) {
	hx_node** a	= (hx_node**) _a;
	hx_node** b	= (hx_node**) _b;
	return hx_node_cmp( *a, *b );
}

//	R	- IRI resource
//	B	- Blank node
//	L	- Plain literal
//	G	- Language-tagged literal
//	D	- Datatyped literal
