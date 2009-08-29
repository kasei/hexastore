#ifndef _EXPR_H
#define _EXPR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include "hexastore_types.h"
#include "algebra/variablebindings.h"
#include "rdf/node.h"

static int hx_expr_debug	= 0;

#define HX_EXPR_UNARY_MAX	11
#define HX_EXPR_BINARY_MAX	25
#define HX_EXPR_TERNARY_MAX	26
#define HX_EXPR_ANY_MAX		27
typedef enum {
	HX_EXPR_OP_NODE				= 0,	/* unary (the no-op for wraping a node up as an expr) */
	HX_EXPR_OP_BANG				= 1,	/* unary */
	HX_EXPR_OP_UMINUS			= 2,	/* unary */
	HX_EXPR_OP_UPLUS			= 3,	/* unary */
	HX_EXPR_BUILTIN_STR			= 4,	/* unary */
	HX_EXPR_BUILTIN_LANG		= 5,	/* unary */
	HX_EXPR_BUILTIN_DATATYPE	= 6,	/* unary */
	HX_EXPR_BUILTIN_BOUND		= 7,	/* unary */
	HX_EXPR_BUILTIN_ISIRI		= 8,	/* unary */
	HX_EXPR_BUILTIN_ISURI		= 9,	/* unary */
	HX_EXPR_BUILTIN_ISBLANK		= 10,	/* unary */
	HX_EXPR_BUILTIN_ISLITERAL	= 11,	/* unary */
	
	HX_EXPR_OP_OR				= 12,	/* binary */
	HX_EXPR_OP_AND				= 13,	/* binary */
	HX_EXPR_OP_EQUAL			= 14,	/* binary */
	HX_EXPR_OP_NEQUAL			= 15,	/* binary */
	HX_EXPR_OP_LT				= 16,	/* binary */
	HX_EXPR_OP_GT				= 17,	/* binary */
	HX_EXPR_OP_LE				= 18,	/* binary */
	HX_EXPR_OP_GE				= 19,	/* binary */
	HX_EXPR_OP_PLUS				= 20,	/* binary */
	HX_EXPR_OP_MINUS			= 21,	/* binary */
	HX_EXPR_OP_MULT				= 22,	/* binary */
	HX_EXPR_OP_DIV				= 23,	/* binary */
	HX_EXPR_BUILTIN_LANGMATCHES	= 24,	/* binary */
	HX_EXPR_BUILTIN_SAMETERM	= 25,	/* binary */
	
	HX_EXPR_BUILTIN_REGEX		= 26,	/* ternary */
	
	HX_EXPR_BUILTIN_FUNCTION	= 27	/* any arity */
} hx_expr_subtype_t;

typedef enum {
	HX_EXPR_BUILTIN		= 1
} hx_expr_t;

typedef struct {
	hx_expr_t type;
	hx_expr_subtype_t subtype;
	int arity;
	void* operands;
} hx_expr;

#include "hexastore.h"
#include "misc/nodemap.h"

hx_expr* hx_new_node_expr ( hx_node* n );
hx_expr* hx_new_builtin_expr1 ( hx_expr_subtype_t type, hx_expr* data );
hx_expr* hx_new_builtin_expr2 ( hx_expr_subtype_t type, hx_expr* data1, hx_expr* data2 );
hx_expr* hx_new_builtin_expr3 ( hx_expr_subtype_t type, hx_expr* data1, hx_expr* data2, hx_expr* data3 );
hx_expr* hx_copy_expr ( hx_expr* e );
int hx_free_expr ( hx_expr* e );

hx_expr* hx_expr_substitute_variables ( hx_expr* orig, hx_variablebindings* b, hx_nodemap* map );

int hx_expr_type_arity ( hx_expr_subtype_t type );
int hx_expr_sse ( hx_expr* e, char** string, char* indent, int level );
int hx_expr_eval ( hx_expr* e, hx_variablebindings* b, hx_nodemap* map, hx_node** result );

#ifdef __cplusplus
}
#endif

#endif
