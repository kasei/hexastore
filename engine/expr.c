#include "engine/expr.h"

int _true ( hx_node** result );
int _false ( hx_node** result );

hx_expr* hx_expr_substitute_variables ( hx_expr* orig, hx_variablebindings* b, hx_store* store ) {
	if (orig->subtype == HX_EXPR_OP_NODE) {
		hx_node* n	= (hx_node*) orig->operands;
		if (hx_node_is_variable(n)) {
			char* name;
			hx_node_variable_name( n, &name );
			hx_node* m	= hx_variablebindings_node_for_binding_name( b, store, name );
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
				return hx_new_builtin_expr1( orig->subtype, hx_expr_substitute_variables(e,b,store) );
			case 2:
				args	= (hx_expr**) orig->operands;
				e		= args[0];
				e2		= args[1];
				return hx_new_builtin_expr2(
							orig->subtype,
							hx_expr_substitute_variables(e,b,store),
							hx_expr_substitute_variables(e2,b,store)
						);
			case 3:
				args	= (hx_expr**) orig->operands;
				e		= args[0];
				e2		= args[1];
				e3		= args[2];
				return hx_new_builtin_expr3(
							orig->subtype,
							hx_expr_substitute_variables(e,b,store),
							hx_expr_substitute_variables(e2,b,store),
							hx_expr_substitute_variables(e3,b,store)
						);
			default:
				fprintf( stderr, "*** Unrecognized or unimplemented expression type %d in hx_expr_substitute_variables\n", orig->type );
				return NULL;
		};
	}
	return NULL;
}

int hx_expr_eval ( hx_expr* e, hx_variablebindings* b, hx_store* store, hx_node** result ) {
	if (e->type == HX_EXPR_BUILTIN) {
		if (e->subtype == HX_EXPR_OP_NODE) {
			hx_node* n	= (hx_node*) e->operands;
			if (hx_node_is_variable( n )) {
				char* vname;
				hx_node_variable_name( n, &vname );
				hx_node* v	= hx_variablebindings_node_for_binding_name( b, store, vname );
				free( vname );
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
					hx_store_variablebindings_string( store, b, &bstring );
					fprintf( stderr, "- using bindings: %s\n", bstring );
					free( bstring );
				}
			}
		
			if (e->arity == 1) {
				hx_expr** args	= (hx_expr**) e->operands;
				hx_expr* child	= args[0];
				hx_node* value;
				int r	= hx_expr_eval( child, b, store, &value );
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
				int r1	= hx_expr_eval( child1, b, store, &value1 );
				int r2	= hx_expr_eval( child2, b, store, &value2 );
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
