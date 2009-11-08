#include "mentok/algebra/expr.h"

int _hx_expr_string_concat ( char** string, char* _new, int* alloc );

hx_expr* hx_new_node_expr ( hx_node* n ) {
	hx_expr* e		= (hx_expr*) calloc( 1, sizeof( hx_expr ) );
	e->type			= HX_EXPR_BUILTIN;
	e->subtype		= HX_EXPR_OP_NODE;
	e->arity		= 1;
	e->operands		= hx_node_copy(n);
	return e;
}

hx_expr* hx_new_builtin_expr1 ( hx_expr_subtype_t subtype, hx_expr* data ) {
	if (subtype > HX_EXPR_UNARY_MAX) {
		// wrong built-in type. this is for unary ops only.
		return NULL;
	}
	
	hx_expr* e		= (hx_expr*) calloc( 1, sizeof( hx_expr ) );
	e->type			= HX_EXPR_BUILTIN;
	e->subtype		= subtype;
	e->arity		= 1;
	if (subtype == HX_EXPR_OP_NODE) {
		e->operands	= data;
	} else {
		hx_expr** args	= (hx_expr**) calloc( 1, sizeof( hx_expr* ) );
		args[0]	= data;
		e->operands		= (void*) args;
	}
	return e;
}

hx_expr* hx_new_builtin_expr2 ( hx_expr_subtype_t type, hx_expr* data1, hx_expr* data2 ) {
	if (type <= HX_EXPR_UNARY_MAX) {
		// wrong built-in type. this is for unary ops only.
		return NULL;
	}
	if (type > HX_EXPR_BINARY_MAX) {
		// wrong built-in type. this is for unary ops only.
		return NULL;
	}
	
	hx_expr* e		= (hx_expr*) calloc( 1, sizeof( hx_expr ) );
	e->type			= HX_EXPR_BUILTIN;
	e->subtype		= type;
	e->arity		= 2;
	hx_expr** args	= (hx_expr**) calloc( 2, sizeof( hx_expr* ) );
	args[0]	= data1;
	args[1]	= data2;
	e->operands		= (void*) args;
	return e;
}

hx_expr* hx_new_builtin_expr3 ( hx_expr_subtype_t type, hx_expr* data1, hx_expr* data2, hx_expr* data3 ) {
	if (type <= HX_EXPR_BINARY_MAX) {
		// wrong built-in type. this is for unary ops only.
		return NULL;
	}
	
	hx_expr* e		= (hx_expr*) calloc( 1, sizeof( hx_expr ) );
	e->type			= HX_EXPR_BUILTIN;
	e->subtype		= type;
	e->arity		= 3;
	hx_expr** args	= (hx_expr**) calloc( 3, sizeof( hx_expr* ) );
	args[0]	= data1;
	args[1]	= data2;
	args[2]	= data3;
	e->operands		= (void*) args;
	return e;
}

int hx_free_expr ( hx_expr* e ) {
	if (e->type == HX_EXPR_BUILTIN) {
		if (e->subtype == HX_EXPR_OP_NODE) {
			hx_free_node( e->operands );
		} else {
			int i;
			for (i = 0; i < e->arity; i++) {
				hx_expr** args	= e->operands;
				hx_free_expr( args[i] );
			}
			free( e->operands );
		}
		free( e );
		return 0;
	} else {
		fprintf( stderr, "*** non-builtin exprs not implemented yet in hx_free_expr\n" );
		return 1;
	}
}

hx_expr* hx_copy_expr ( hx_expr* e ) {
	hx_expr* c		= (hx_expr*) calloc( 1, sizeof( hx_expr ) );
	c->type			= e->type;
	c->subtype		= e->subtype;
	c->arity		= e->arity;
	if (c->subtype == HX_EXPR_OP_NODE) {
		c->operands	= hx_node_copy( (hx_node*) e->operands );
	} else {
		int i;
		hx_expr** args	= (hx_expr**) calloc( c->arity, sizeof( hx_expr* ) );
		for (i = 0; i < c->arity; i++) {
			hx_expr** ops	= (hx_expr**) e->operands;
			args[i]	= hx_copy_expr( ops[i] );
		}
		c->operands	= args;
	}
	return c;
}

int hx_expr_type_arity ( hx_expr_subtype_t type ) {
	if (type <= HX_EXPR_UNARY_MAX) {
		return 1;
	} else if (type <= HX_EXPR_BINARY_MAX) {
		return 2;
	} else if (type <= HX_EXPR_TERNARY_MAX) {
		return 3;
	} else {
		return -1;
	}
}

int hx_expr_sse ( hx_expr* e, char** string, char* indent, int level ) {
	if (e->type == HX_EXPR_BUILTIN) {
		if (e->subtype == HX_EXPR_OP_NODE) {
			hx_node* n	= (hx_node*) e->operands;
			return hx_node_string( n, string );
		} else {
			char* name	= NULL;
			switch (e->subtype) {
				case HX_EXPR_BUILTIN_STR:
					name	= "sparql:str";
					break;
				case HX_EXPR_BUILTIN_LANG:
					name	= "sparql:lang";
					break;
				case HX_EXPR_BUILTIN_DATATYPE:
					name	= "sparql:datatype";
					break;
				case HX_EXPR_BUILTIN_BOUND:
					name	= "sparql:bound";
					break;
				case HX_EXPR_BUILTIN_ISIRI:
					name	= "sparql:isiri";
					break;
				case HX_EXPR_BUILTIN_ISURI:
					name	= "sparql:isuri";
					break;
				case HX_EXPR_BUILTIN_ISBLANK:
					name	= "sparql:isblank";
					break;
				case HX_EXPR_BUILTIN_ISLITERAL:
					name	= "sparql:isliteral";
					break;
				case HX_EXPR_BUILTIN_LANGMATCHES:
					name	= "sparql:langmatches";
					break;
				case HX_EXPR_BUILTIN_SAMETERM:
					name	= "sparql:sameterm";
					break;
				case HX_EXPR_BUILTIN_REGEX:
					name	= "regex";
					break;
				case HX_EXPR_BUILTIN_FUNCTION:
					name	= "function";
					break;
				case HX_EXPR_OP_EQUAL:
					name	= "eq";
					break;
				case HX_EXPR_OP_NEQUAL:
					name	= "neq";
					break;
				default:
					fprintf( stderr, "*** unrecognized builtin type %d\n", e->subtype );
			};
			int alloc	= 256 + strlen(name) + 3;
			char* str	= (char*) calloc( 1, alloc );
			snprintf( str, alloc, "(%s", name );
			
			hx_expr** args	= e->operands;
			int i;
			for (i = 0; i < e->arity; i++) {
				hx_expr* n	= args[i];
				char* nstring;
				hx_expr_sse( n, &nstring, indent, level+1 );
				if (_hx_expr_string_concat( &str, " ", &alloc )) {
					free( nstring );
					free( str );
					return 1;
				}
				if (_hx_expr_string_concat( &str, nstring, &alloc )) {
					free( nstring );
					free( str );
					return 1;
				}
				free( nstring );
			}
			if (_hx_expr_string_concat( &str, ")", &alloc )) {
				free( str );
				return 1;
			}
			*string	= str;
			return 0;
		}
	} else {
		fprintf( stderr, "*** non-builtin exprs not implemented yet in hx_expr_sse\n" );
		fprintf( stderr, "*** type == %d, subtype = %d\n", e->type, e->subtype );
		return 1;
	}
}

int _hx_expr_string_concat ( char** string, char* _new, int* alloc ) {
	int sl	= strlen(*string);
	int nl	= strlen(_new);
	while (sl + nl + 1 >= *alloc) {
		*alloc	*= 2;
		char* newstring	= (char*) malloc( *alloc );
		if (newstring == NULL) {
			fprintf( stderr, "*** malloc failed in _hx_expr_string_concat\n" );
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
