#include "expr.h"

int _true ( hx_node** result );
int _false ( hx_node** result );
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

hx_expr* hx_expr_substitute_variables ( hx_expr* orig, hx_variablebindings* b, hx_nodemap* map ) {
	if (orig->subtype == HX_EXPR_OP_NODE) {
		hx_node* n	= (hx_node*) orig->operands;
		if (hx_node_is_variable(n)) {
			char* name;
			hx_node_variable_name( n, &name );
			hx_node* m	= hx_variablebindings_node_for_binding_name( b, map, name );
			free(name);
			if (m != NULL) {
				n	= m;
			}
		}
		return hx_new_node_expr( n );
	} else {
		hx_expr** args;
		hx_expr *e, *e2, *e3;
		switch (orig->arity) {
			case 1:
				args	= (hx_expr**) orig->operands;
				e		= args[0];
				return hx_new_builtin_expr1( orig->subtype, hx_expr_substitute_variables(e,b,map) );
			case 2:
				args	= (hx_expr**) orig->operands;
				e		= args[0];
				e2		= args[1];
				return hx_new_builtin_expr2(
							orig->subtype,
							hx_expr_substitute_variables(e,b,map),
							hx_expr_substitute_variables(e2,b,map)
						);
			case 3:
				args	= (hx_expr**) orig->operands;
				e		= args[0];
				e2		= args[1];
				e3		= args[2];
				return hx_new_builtin_expr3(
							orig->subtype,
							hx_expr_substitute_variables(e,b,map),
							hx_expr_substitute_variables(e2,b,map),
							hx_expr_substitute_variables(e3,b,map)
						);
			default:
				fprintf( stderr, "*** Unrecognized or unimplemented expression type %d in hx_expr_substitute_variables\n", orig->type );
				return NULL;
		};
	}
	return NULL;
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
					free( str );
					return 1;
				}
				if (_hx_expr_string_concat( &str, nstring, &alloc )) {
					free( str );
					return 1;
				}
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

int hx_expr_eval ( hx_expr* e, hx_variablebindings* b, hx_nodemap* map, hx_node** result ) {
	if (e->type == HX_EXPR_BUILTIN) {
		if (e->subtype == HX_EXPR_OP_NODE) {
			hx_node* n	= (hx_node*) e->operands;
			if (hx_node_is_variable( n )) {
				char* vname;
				hx_node_variable_name( n, &vname );
				hx_node* v	= hx_variablebindings_node_for_binding_name( b, map, vname );
				if (v == NULL) {
					return 1;
				} else {
					*result	= v;
					return 0;
				}
			} else {
				*result	= hx_node_copy( e->operands );
				return 0;
			}
		} else {
			if (hx_expr_debug) {	// ---------------------------------------------
				char *e_sse, *bstring;
				hx_expr_sse( e, &e_sse, "  ", 0 );
				fprintf( stderr, "eval expression: %s\n", e_sse );
				free( e_sse );
				if (b != NULL) {
					hx_variablebindings_string( b, map, &bstring );
					fprintf( stderr, "- using bindings: %s\n", bstring );
					free( bstring );
				}
			}
		
			if (e->arity == 1) {
				hx_expr** args	= (hx_expr**) e->operands;
				hx_expr* child	= args[0];
				hx_node* value;
				int r	= hx_expr_eval( child, b, map, &value );
				if (r != 0) {
					if (hx_expr_debug) {	// ---------------------------------------------
						fprintf( stderr, "- error in sub-eval\n" );
					}
					return r;
				}
				switch (e->subtype) {
					case HX_EXPR_BUILTIN_ISIRI:
					case HX_EXPR_BUILTIN_ISURI:
						return (hx_node_is_resource(value))
							? _true( result )
							: _false( result );
					case HX_EXPR_BUILTIN_ISBLANK:
						return (hx_node_is_blank(value))
							? _true( result )
							: _false( result );
					case HX_EXPR_BUILTIN_ISLITERAL:
						return (hx_node_is_literal(value))
							? _true( result )
							: _false( result );
					default:
						break;
				};
				fprintf( stderr, "*** eval for expr subtype %d not implemented yet\n", e->subtype );
				return 1;
			} else if (e->arity == 2) {
				hx_expr** args	= (hx_expr**) e->operands;
				hx_expr* child1	= args[0];
				hx_expr* child2	= args[1];
				hx_node *value1, *value2;
				int r1	= hx_expr_eval( child1, b, map, &value1 );
				int r2	= hx_expr_eval( child2, b, map, &value2 );
				if (r1 != 0 || r2 != 0) {
					return r1 || r2;
				}
				
				if (hx_expr_debug) {	// ---------------------------------------------
					char * string;
					fprintf( stderr, "binary op in eval:\n" );
					hx_node_string( value1, &string );
					fprintf( stderr, "- %s\n", string );
					free(string);
					hx_node_string( value2, &string );
					fprintf( stderr, "- %s\n", string );
					free(string);
				}
				
				switch (e->subtype) {
					case HX_EXPR_OP_EQUAL:
						return (hx_node_cmp(value1, value2) == 0)
							? _true( result )
							: _false( result );
					case HX_EXPR_OP_NEQUAL:
						return (hx_node_cmp(value1, value2) != 0)
							? _true( result )
							: _false( result );
					default:
						break;
				};
				fprintf( stderr, "*** eval for expr subtype %d not implemented yet\n", e->subtype );
				return 1;
			} else {
				fprintf( stderr, "*** eval for expr subtype %d not implemented yet\n", e->subtype );
				return 1;
			}
		}
	} else {
		fprintf( stderr, "*** non-builtin exprs not implemented yet in hx_expr_eval\n" );
		return 1;
	}
}

int _true ( hx_node** result ) {
	if (hx_expr_debug) {	// ---------------------------------------------
		fprintf( stderr, "- expression evaluated to TRUE\n" );
	}
	*result	= (hx_node*) hx_new_node_dt_literal( "true", "http://www.w3.org/2001/XMLSchema#boolean" );
	return 0;
}

int _false ( hx_node** result ) {
	if (hx_expr_debug) {	// ---------------------------------------------
		fprintf( stderr, "- expression evaluated to FALSE\n" );
	}
	*result	= (hx_node*) hx_new_node_dt_literal( "false", "http://www.w3.org/2001/XMLSchema#boolean" );
	return 0;
}
