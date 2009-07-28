/** \file SPARQLParser.yy Contains the Bison parser source */

/*** yacc/bison Declarations ***/

/* Require bison 2.3 or later */
%require "2.3"

/* add debug output code to generated parser. disable this for release
 * versions. */
%debug

/* start symbol is named "start" */
// XXX %start Query
%start myQuery

/* write out a header file containing the token defines */
%defines

/* keep track of the current position within the input */
%locations

/* verbose error messages */
%error-verbose

%{ /*** C/C++ Declarations ***/

#include <stdio.h>
#include "node.h"
#include "expr.h"
#include "graphpattern.h"

int yylex ( void );
void yyerror (char const *s);
#define YYSTYPE void*
#define YYSTYPE_IS_DECLARED

/**
Types:

	Nodes:
	- 'N' for a full node,
	- 'V' for a char* variable name
	- 'Q' for a QName (that still needs to be expanded)
	- 'L' for a Literal with Qname datatype
	
	Patterns:
	- 'T' Triple
	- 'B' BGP
	- 'G' GGP
	- 'O' Optional
	- 'U' Union
	
	- 'F' Filter
**/

typedef enum {
	TYPE_NODES		= 'n',
	TYPE_FULL_NODE	= 'N',
	TYPE_VARIABLE	= 'V',
	TYPE_QNAME		= 'Q',
	TYPE_DT_LITERAL	= 'L',
	TYPE_TRIPLE		= 'T',
	TYPE_BGP		= 'B',
	TYPE_GGP		= 'G',
	TYPE_OPT		= 'O',
	TYPE_UNION		= 'U',
	TYPE_EXPR		= 'E',
	TYPE_FILTER		= 'F'
} hx_sparqlparser_pattern_t;

typedef struct {
	hx_sparqlparser_pattern_t type;
	void* ptr;
	char* datatype;
} node_t;

typedef struct {
	hx_sparqlparser_pattern_t type;
	node_t* subject;
	node_t* predicate;
	node_t* object;
} triple_t;

typedef struct {
	hx_sparqlparser_pattern_t type;
	int allocated;
	int count;
	void** items;
} container_t;

typedef struct {
	char* language;
	node_t* datatype;
} string_attrs_t;

typedef struct {
	char* name;
	char* uri;
} namespace_t;

typedef struct {
	hx_expr_subtype_t op;
	container_t* args;
} expr_t;

typedef struct {
	int allocated;
	int namespace_count;
	namespace_t** namespaces;
} namespace_set_t;

typedef struct {
	namespace_set_t* ns;
	char* base;
} prologue_t;

typedef struct {
	prologue_t* prologue;
	container_t* bgp;
	expr_t* filter;
} bgp_query_t;

typedef struct {
	prologue_t* prologue;
	container_t* gp;
} gp_query_t;

extern void* parsedBGPPattern;
extern void* parsedPrologue;
extern void* parsedGP;
extern node_t* new_dt_literal_node ( char*, char* );
extern container_t* new_container ( char type, int size );
extern int free_container ( container_t* c, int free_contained_objects );
extern void container_push_item( container_t* set, void* t );
extern void container_unshift_item( container_t* set, void* t );
extern expr_t* new_expr_data ( hx_expr_subtype_t op, void* arg );
extern void XXXdebug_node( node_t*, prologue_t* );
extern void XXXdebug_triple( triple_t*, prologue_t* );

%}


%{
#include "SPARQLScanner.h"
%}
%token			__EOF__		 0	"end of file"

/* Terminals */
%token IT_BASE
%token IT_PREFIX
%token IT_SELECT
%token IT_DISTINCT
%token IT_REDUCED
%token GT_TIMES
%token IT_CONSTRUCT
%token IT_DESCRIBE
%token IT_ASK
%token IT_FROM
%token IT_NAMED
%token IT_WHERE
%token IT_ORDER
%token IT_BY
%token IT_ASC
%token IT_DESC
%token IT_LIMIT
%token IT_OFFSET
%token GT_LCURLEY
%token GT_RCURLEY
%token GT_DOT
%token IT_OPTIONAL
%token IT_GRAPH
%token IT_UNION
%token IT_FILTER
%token GT_COMMA
%token GT_LPAREN
%token GT_RPAREN
%token GT_SEMI
%token IT_a
%token GT_LBRACKET
%token GT_RBRACKET
%token GT_OR
%token GT_AND
%token GT_EQUAL
%token GT_NEQUAL
%token GT_LT
%token GT_GT
%token GT_LE
%token GT_GE
%token GT_PLUS
%token GT_MINUS
%token GT_DIVIDE
%token GT_NOT
%token IT_STR
%token IT_LANG
%token IT_LANGMATCHES
%token IT_DATATYPE
%token IT_BOUND
%token IT_sameTerm
%token IT_isIRI
%token IT_isURI
%token IT_isBLANK
%token IT_isLITERAL
%token IT_REGEX
%token GT_DTYPE
%token IT_true
%token IT_false
%token IRI_REF
%token PNAME_NS
%token PNAME_LN
%token BLANK_NODE_LABEL
%token VAR1
%token VAR2
%token LANGTAG
%token INTEGER
%token DECIMAL
%token DOUBLE
%token INTEGER_POSITIVE
%token DECIMAL_POSITIVE
%token DOUBLE_POSITIVE
%token INTEGER_NEGATIVE
%token DECIMAL_NEGATIVE
%token DOUBLE_NEGATIVE
%token STRING_LITERAL1
%token STRING_LITERAL2
%token STRING_LITERAL_LONG1
%token STRING_LITERAL_LONG2
%token NIL
%token ANON

/* END TokenBlock */

//%destructor { delete $$; } BlankNode

 /*** END SPARQL - Change the grammar's tokens above ***/

%{
#include <stdlib.h>
#include <stdarg.h>
#include "SPARQLScanner.h"
%}

%% /*** Grammar Rules ***/

 /*** BEGIN SPARQL - Change the grammar rules below ***/


myQuery:
	Prologue WhereClause {
		gp_query_t* q	= (gp_query_t*) calloc( 1, sizeof( gp_query_t ) );
		q->prologue		= (prologue_t*) $1;
		q->gp			= (container_t*) $2;
		parsedGP		= q;
		parsedPrologue	= (void*) $1; /** XXX should remove at some point... only here to support the parse_bgp functions **/
	}
	
// 	Prologue GT_LCURLEY _QTriplesBlock_E_Opt __Filter_Opt GT_RCURLEY {
// 		bgp_query_t* q	= (bgp_query_t*) calloc( 1, sizeof( bgp_query_t ) );
// 		q->prologue		= (prologue_t*) $1;
// 		q->bgp			= (container_t*) $3;
// 		q->filter		= $4;
// 		parsedBGPPattern		= (void*) q;
// 	}
;


Prologue:
	_QBaseDecl_E_Opt _QPrefixDecl_E_Star {
		prologue_t* p	= (prologue_t*) calloc( 1, sizeof( prologue_t ) );
		p->ns			= (namespace_set_t*) $2;
		p->base			= (char*) $1;
		$$				= (void*) p;
	}
;

_QBaseDecl_E_Opt:
	{
		$$	= NULL;
	}

	| BaseDecl	{
		$$	= $1;
	}
;

_QPrefixDecl_E_Star:
	{
		$$	= NULL;
	}

	| _QPrefixDecl_E_Star PrefixDecl {
		namespace_t* n;
		namespace_set_t* set	= (namespace_set_t*) $1;
		if (set == NULL) {
			set	= (namespace_set_t*) calloc( 1, sizeof( namespace_set_t ) );
			set->allocated			= 2;
			set->namespaces			= (namespace_t**) calloc( set->allocated, sizeof( namespace_t* ) );
			set->namespace_count	= 0;
		}
		
		if (set->allocated <= (set->namespace_count + 1)) {
			int i;
			namespace_t** old;
			namespace_t** newlist	= (namespace_t**) calloc( set->allocated, sizeof( namespace_t* ) );
			set->allocated	*= 2;
			for (i = 0; i < set->namespace_count; i++) {
				newlist[i]	= set->namespaces[i];
			}
			old		= set->namespaces;
			set->namespaces			= newlist;
			free( old );
		}
		
		n	= (namespace_t*) $2;
		set->namespaces[ set->namespace_count++ ]	= n;
		$$	= set;
	}
;

BaseDecl:
	IT_BASE IRI_REF {
		$$	= $2;
	}
;

PrefixDecl:
	IT_PREFIX PNAME_NS IRI_REF	{
		namespace_t* n	= (namespace_t*) calloc( 1, sizeof( namespace_t ) );
		n->name = (char*) $2;
		n->name[ strlen( n->name ) - 1 ]	= (char) 0;
		n->uri	= (char*) $3;
		$$	= n;
	}
;

TriplesBlock:
	TriplesSameSubject _Q_O_QGT_DOT_E_S_QTriplesBlock_E_Opt_C_E_Opt {
		if ($2 == NULL) {
			$$	= $1;
		} else {
			int i;
			container_t* triples1	= (container_t*) $1;
			container_t* triples2	= (container_t*) $2;
			for (i = 0; i < triples2->count; i++) {
				container_push_item( triples1, triples2->items[i] );
			}
			free_container( triples2, 0 );
			$$	= triples1;
		}
	}
;

_O_QGT_DOT_E_S_QTriplesBlock_E_Opt_C:
	GT_DOT _QTriplesBlock_E_Opt {
		$$	= $2;
	}
;

_Q_O_QGT_DOT_E_S_QTriplesBlock_E_Opt_C_E_Opt:
	{
		$$	= NULL;
	}

	| _O_QGT_DOT_E_S_QTriplesBlock_E_Opt_C	{
		$$	= $1;
	}
;

_QTriplesBlock_E_Opt:
	{
		$$	= NULL;
	}

	| TriplesBlock {
		$$	= $1;
	}
;

TriplesSameSubject:
	VarOrTerm PropertyListNotEmpty	{
		node_t* subject;
		container_t* triples;
		int i;
		container_t* subj_triples	= (container_t*) $1;
		if (subj_triples->count == 0) {
			fprintf( stderr, "uh oh. VarOrTerm didn't return any graph triples.\n" );
		}
		subject	= ((triple_t*) subj_triples->items[0])->subject;
		triples	= (container_t*) $2;
		for (i = 0; i < triples->count; i++) {
			triple_t* t	= (triple_t*) triples->items[i];
			t->subject	= subject;
		}
		$$	= triples;
	}

	| TriplesNode PropertyList	{
		fprintf( stderr, "TriplesSameSubject[2] not implemented\n" );
		$$	= NULL;
	}
;

PropertyListNotEmpty:
	Verb ObjectList _Q_O_QGT_SEMI_E_S_QVerb_E_S_QObjectList_E_Opt_C_E_Star	{
		int i;
		container_t* triples;
		node_t* predicate	= (node_t*) $1;
		triples	= (container_t*) $2;
		/* XXX */
		for (i = 0; i < triples->count; i++) {
			triple_t* t	= (triple_t*) triples->items[i];
			t->predicate	= predicate;
		}
		if ($3 != NULL) {
			int i;
			container_t* triples2	= (container_t*) $3;
			for (i = 0; i < triples2->count; i++) {
				container_push_item( triples, triples2->items[i] );
			}
		}
		$$	= (void*) triples;
	}
;

_O_QVerb_E_S_QObjectList_E_C:
	Verb ObjectList {
		container_t* triples	= (container_t*) $2;
		int i;
		node_t* predicate	= (node_t*) $1;
		for (i = 0; i < triples->count; i++) {
			triple_t* t	= (triple_t*) triples->items[i];
			t->predicate	= predicate;
		}
		$$	= (void*) triples;
	}
;

_Q_O_QVerb_E_S_QObjectList_E_C_E_Opt:
	{
		$$	= NULL;
	}

	| _O_QVerb_E_S_QObjectList_E_C	{
		$$	= $1;
	}
;

_O_QGT_SEMI_E_S_QVerb_E_S_QObjectList_E_Opt_C:
	GT_SEMI _Q_O_QVerb_E_S_QObjectList_E_C_E_Opt	{
		$$	= $2;
	}
;

_Q_O_QGT_SEMI_E_S_QVerb_E_S_QObjectList_E_Opt_C_E_Star:
	{
		$$	= NULL;
	}

	| _Q_O_QGT_SEMI_E_S_QVerb_E_S_QObjectList_E_Opt_C_E_Star _O_QGT_SEMI_E_S_QVerb_E_S_QObjectList_E_Opt_C	{
		container_t* triples;
		if ($1 == NULL) {
			triples	= (container_t*) $2;
		} else {
			triples	= (container_t*) $1;
			if ($2 != NULL) {
				int i;
				container_t* triples2	= (container_t*) $2;
				for (i = 0; i < triples2->count; i++) {
					container_push_item( triples, triples2->items[i] );
				}
				free_container( triples2, 0 );
			}
		}
		$$	= triples;
	}
;

PropertyList:
	_QPropertyListNotEmpty_E_Opt	{
		$$	= $1;
	}
;

_QPropertyListNotEmpty_E_Opt:
	{
		$$	= NULL;
	}

	| PropertyListNotEmpty	{
		$$	= $1;
	}
;

ObjectList:
	Object _Q_O_QGT_COMMA_E_S_QObject_E_C_E_Star	{
		triple_t* t;
		node_t* object;
		container_t* set;
		container_t* obj_triples;
		if ($2 == NULL) {
			set	= new_container( TYPE_BGP, 5 );
		} else {
			set	= (container_t*) $2;
		}
		
		obj_triples	= (container_t*) $1;
		object	= ((triple_t*) obj_triples->items[0])->subject;
		
		t			= (triple_t*) calloc( 1, sizeof( triple_t ) );
		t->type		= TYPE_TRIPLE;
		t->object	= object;
		container_push_item( set, t );
		$$	= (void*) set;
	}
;

_O_QGT_COMMA_E_S_QObject_E_C:
	GT_COMMA Object {
		$$	= $2;
	}
;

_Q_O_QGT_COMMA_E_S_QObject_E_C_E_Star:
	{
		$$	= NULL;
	}

	| _Q_O_QGT_COMMA_E_S_QObject_E_C_E_Star _O_QGT_COMMA_E_S_QObject_E_C	{
		triple_t* t;
		node_t* object;
		container_t* set;
		container_t* obj_triples;
		if ($1 == NULL) {
			set	= new_container( TYPE_BGP, 5 );
		} else {
			set	= (container_t*) $1;
		}
		
		obj_triples	= (container_t*) $2;
		object		= ((triple_t*) obj_triples->items[0])->subject;
		
		t			= (triple_t*) calloc( 1, sizeof( triple_t ) );
		t->type		= TYPE_TRIPLE;
		t->object	= object;
		
		free_container( obj_triples, 0 );
		container_push_item( set, t );
		
		$$	= set;
	}
;

Object:
	GraphNode	{
		$$	= $1;
	}
;

Verb:
	VarOrIRIref {
		$$	= $1;
	}

	| IT_a	{
		node_t* n	= (node_t*) calloc( 1, sizeof( node_t ) );
		hx_node* iri	= hx_new_node_resource( "http://www.w3.org/1999/02/22-rdf-syntax-ns#type" );
		n->type			= TYPE_FULL_NODE;
		n->ptr			= (void*) iri;
		$$	= n;
	}
;

TriplesNode:
	Collection	{}

	| BlankNodePropertyList {}
;

BlankNodePropertyList:
	GT_LBRACKET PropertyListNotEmpty GT_RBRACKET	{}
;

Collection:
	GT_LPAREN _QGraphNode_E_Plus GT_RPAREN	{}
;

_QGraphNode_E_Plus:
	GraphNode	{}

	| _QGraphNode_E_Plus GraphNode	{}
;

GraphNode:
	VarOrTerm	{
		$$	= $1;
	}

	| TriplesNode	{
		$$	= NULL;
	}
;

VarOrTerm:
	Var {
		container_t* set	= new_container( TYPE_BGP, 1 );
		triple_t* t			= (triple_t*) calloc( 1, sizeof( triple_t ) );
		t->type				= TYPE_TRIPLE;
		t->subject			= (node_t*) $1;
		t->predicate		= NULL;
		t->object			= NULL;
		container_push_item( set, t );
		$$	= set;
	}

	| GraphTerm {
		$$	= $1;
	}
;

VarOrIRIref:
	Var {
		$$	= $1;
	}

	| IRIref	{
		$$	= $1;
	}
;

Var:
	VAR1	{
		node_t* n	= (node_t*) calloc( 1, sizeof( node_t ) );
		n->type	= TYPE_VARIABLE;
		n->ptr	= (void*) $1;
		$$	= n;
	}

	| VAR2	{
		node_t* n	= (node_t*) calloc( 1, sizeof( node_t ) );
		n->type	= TYPE_VARIABLE;
		n->ptr	= (void*) $1;
		$$	= n;
	}
;

GraphTerm:
	IRIref	{
		container_t* set	= new_container( TYPE_BGP, 1 );
		triple_t* t			= (triple_t*) calloc( 1, sizeof( triple_t ) );
		t->type				= TYPE_TRIPLE;
		t->subject			= (node_t*) $1;
		t->predicate		= NULL;
		t->object			= NULL;
		container_push_item( set, t );
		$$	= set;
	}

	| RDFLiteral	{
		container_t* set	= new_container( TYPE_BGP, 1 );
		triple_t* t			= (triple_t*) calloc( 1, sizeof( triple_t ) );
		t->type				= TYPE_TRIPLE;
		t->subject			= (node_t*) $1;
		t->predicate		= NULL;
		t->object			= NULL;
		container_push_item( set, t );
		$$	= set;
	}

	| NumericLiteral	{
		container_t* set	= new_container( TYPE_BGP, 1 );
		triple_t* t			= (triple_t*) calloc( 1, sizeof( triple_t ) );
		t->type				= TYPE_TRIPLE;
		t->subject			= (node_t*) $1;
		t->predicate		= NULL;
		t->object			= NULL;
		container_push_item( set, t );
		$$	= set;
	}

	| BooleanLiteral	{
		container_t* set	= new_container( TYPE_BGP, 1 );
		triple_t* t			= (triple_t*) calloc( 1, sizeof( triple_t ) );
		t->type				= TYPE_TRIPLE;
		t->subject			= (node_t*) $1;
		t->predicate		= NULL;
		t->object			= NULL;
		container_push_item( set, t );
		$$	= set;
	}

	| BlankNode {
		container_t* set	= new_container( TYPE_BGP, 1 );
		triple_t* t			= (triple_t*) calloc( 1, sizeof( triple_t ) );
		t->type				= TYPE_TRIPLE;
		t->subject			= (node_t*) $1;
		t->predicate		= NULL;
		t->object			= NULL;
		container_push_item( set, t );
		$$	= set;
	}

	| NIL	{}
;

RDFLiteral:
	String _Q_O_QLANGTAG_E_Or_QGT_DTYPE_E_S_QIRIref_E_C_E_Opt	{
		string_attrs_t* attrs	= (string_attrs_t*) $2;
		node_t* node	= (node_t*) calloc( 1, sizeof( node_t ) );
		if (attrs == NULL) {
			hx_node* l	= hx_new_node_literal( (char*) $1 );
			node->type		= TYPE_FULL_NODE;
			node->ptr		= (void*) l;
		} else if (attrs->language != NULL) {
			hx_node* l	= (hx_node*) hx_new_node_lang_literal( (char*) $1, attrs->language );
			node->type		= TYPE_FULL_NODE;
			node->ptr		= (void*) l;
		} else {
			node_t* dt		= attrs->datatype;
			if (dt->type == TYPE_QNAME) {
				node->type		= TYPE_DT_LITERAL;
				node->ptr		= (char*) $1;
				node->datatype	= (char*) dt->ptr;
			} else {
				hx_node* l	= (hx_node*) hx_new_node_dt_literal( (char*) $1, (char*) dt->ptr );
				node->type		= TYPE_FULL_NODE;
				node->ptr		= (void*) l;
			}
		}
		
		$$	= (void*) node;
	}
;

_O_QGT_DTYPE_E_S_QIRIref_E_C:
	GT_DTYPE IRIref {
		$$	= $2;
	}
;

_O_QLANGTAG_E_Or_QGT_DTYPE_E_S_QIRIref_E_C:
	LANGTAG {
		string_attrs_t* attr	= (string_attrs_t*) calloc( 1, sizeof( string_attrs_t ) );
		attr->datatype	= NULL;
		attr->language	= (char*) $1;
		$$	= (void*) attr;
	}

	| _O_QGT_DTYPE_E_S_QIRIref_E_C	{
		string_attrs_t* attr	= (string_attrs_t*) calloc( 1, sizeof( string_attrs_t ) );
		attr->language	= NULL;
		attr->datatype	= (node_t*) $1;
		$$	= (void*) attr;
	}
;

_Q_O_QLANGTAG_E_Or_QGT_DTYPE_E_S_QIRIref_E_C_E_Opt:
	{
		$$	= NULL;
	}

	| _O_QLANGTAG_E_Or_QGT_DTYPE_E_S_QIRIref_E_C	{
		$$	= $1
	}
;

NumericLiteral:
	NumericLiteralUnsigned	{
		$$	= $1;
	}

	| NumericLiteralPositive	{
		$$	= $1;
	}

	| NumericLiteralNegative	{
		$$	= $1;
	}
;

NumericLiteralUnsigned:
	INTEGER {
		$$	= (void*) new_dt_literal_node( (char*) $1, "http://www.w3.org/2001/XMLSchema#integer" );
	}

	| DECIMAL	{
		$$	= (void*) new_dt_literal_node( (char*) $1, "http://www.w3.org/2001/XMLSchema#decimal" );
	}

	| DOUBLE	{
		$$	= (void*) new_dt_literal_node( (char*) $1, "http://www.w3.org/2001/XMLSchema#double" );
	}
;

NumericLiteralPositive:
	INTEGER_POSITIVE	{
		$$	= (void*) new_dt_literal_node( (char*) $1, "http://www.w3.org/2001/XMLSchema#integer" );
	}

	| DECIMAL_POSITIVE	{
		$$	= (void*) new_dt_literal_node( (char*) $1, "http://www.w3.org/2001/XMLSchema#decimal" );
	}

	| DOUBLE_POSITIVE	{
		$$	= (void*) new_dt_literal_node( (char*) $1, "http://www.w3.org/2001/XMLSchema#double" );
	}
;

NumericLiteralNegative:
	INTEGER_NEGATIVE	{
		$$	= (void*) new_dt_literal_node( (char*) $1, "http://www.w3.org/2001/XMLSchema#integer" );
	}

	| DECIMAL_NEGATIVE	{
		$$	= (void*) new_dt_literal_node( (char*) $1, "http://www.w3.org/2001/XMLSchema#decimal" );
	}

	| DOUBLE_NEGATIVE	{
		$$	= (void*) new_dt_literal_node( (char*) $1, "http://www.w3.org/2001/XMLSchema#double" );
	}
;

BooleanLiteral:
	IT_true {
		$$	= (void*) hx_new_node_dt_literal( "true", "http://www.w3.org/2001/XMLSchema#boolean" );
	}

	| IT_false	{
		$$	= (void*) hx_new_node_dt_literal( "false", "http://www.w3.org/2001/XMLSchema#boolean" );
	}
;

String:
	STRING_LITERAL1 {
		$$	= (void*) $1;
	}

	| STRING_LITERAL2	{
		$$	= (void*) $1;
	}

	| STRING_LITERAL_LONG1	{
		$$	= (void*) $1;
	}

	| STRING_LITERAL_LONG2	{
		$$	= (void*) $1;
	}
;

IRIref:
	IRI_REF {
		node_t* r;
		char* string;
		hx_node* iri	= hx_new_node_resource( (char*) $1 );
		hx_node_string( iri, &string );
		free( string );
		r	= (node_t*) calloc( 1, sizeof( node_t ) );
		r->type			= TYPE_FULL_NODE;
		r->ptr			= iri;
		$$	= (void*) r;
	}

	| PrefixedName	{
		node_t* r	= (node_t*) calloc( 1, sizeof( node_t ) );
		r->type			= TYPE_QNAME;
		r->ptr			= $1;
		$$	= (void*) r;
	}
;

PrefixedName:
	PNAME_LN	{
		$$	= $1;
	}

	| PNAME_NS	{
		$$	= $1;
	}
;

BlankNode:
	BLANK_NODE_LABEL {
		node_t* r;
		char* string;
		hx_node* b	= hx_new_node_blank( (char*) $1 );
		hx_node_string( b, &string );
		free( string );
		
		r	= (node_t*) calloc( 1, sizeof( node_t ) );
		r->type			= TYPE_FULL_NODE;
		r->ptr			= b;
		$$	= (void*) r;
	}

	| ANON	{}
;


Filter:
	IT_FILTER Constraint {
		container_t* set	= new_container( TYPE_FILTER, 1 );
		container_push_item( set, $2 );
		$$	= set;
	}
;

Constraint:
	BrackettedExpression {
		$$	= $1;
	}

	| BuiltInCall	{
		$$	= $1;
	}

	| FunctionCall	{
		$$	= $1;
	}
;

FunctionCall:
	IRIref ArgList	{}
;

ArgList:
	_O_QNIL_E_Or_QGT_LPAREN_E_S_QExpression_E_S_QGT_COMMA_E_S_QExpression_E_Star_S_QGT_RPAREN_E_C	{}
;

_O_QNIL_E_Or_QGT_LPAREN_E_S_QExpression_E_S_QGT_COMMA_E_S_QExpression_E_Star_S_QGT_RPAREN_E_C:
	NIL	{}

	| GT_LPAREN Expression _Q_O_QGT_COMMA_E_S_QExpression_E_C_E_Star GT_RPAREN	{}
;

_O_QGT_COMMA_E_S_QExpression_E_C:
	GT_COMMA Expression	{}
;

_Q_O_QGT_COMMA_E_S_QExpression_E_C_E_Star:
	{}

	| _Q_O_QGT_COMMA_E_S_QExpression_E_C_E_Star _O_QGT_COMMA_E_S_QExpression_E_C {}
;

Expression:
	ConditionalOrExpression	{
		$$	= $1;
	}
;

ConditionalOrExpression:
	ConditionalAndExpression _Q_O_QGT_OR_E_S_QConditionalAndExpression_E_C_E_Star	{
		/* XXX fill in opt star */
		$$	= $1;
	}
;

_O_QGT_OR_E_S_QConditionalAndExpression_E_C:
	GT_OR ConditionalAndExpression	{}
;

_Q_O_QGT_OR_E_S_QConditionalAndExpression_E_C_E_Star:
	{}

	| _Q_O_QGT_OR_E_S_QConditionalAndExpression_E_C_E_Star _O_QGT_OR_E_S_QConditionalAndExpression_E_C	{}
;

ConditionalAndExpression:
	ValueLogical _Q_O_QGT_AND_E_S_QValueLogical_E_C_E_Star	{
		/* XXX fill in opt star */
		$$	= $1;
	}
;

_O_QGT_AND_E_S_QValueLogical_E_C:
	GT_AND ValueLogical	{}
;

_Q_O_QGT_AND_E_S_QValueLogical_E_C_E_Star:
	{}

	| _Q_O_QGT_AND_E_S_QValueLogical_E_C_E_Star _O_QGT_AND_E_S_QValueLogical_E_C {}
;

ValueLogical:
	RelationalExpression {
		$$	= $1;
	}
;

RelationalExpression:
	NumericExpression _Q_O_QGT_EQUAL_E_S_QNumericExpression_E_Or_QGT_NEQUAL_E_S_QNumericExpression_E_Or_QGT_LT_E_S_QNumericExpression_E_Or_QGT_GT_E_S_QNumericExpression_E_Or_QGT_LE_E_S_QNumericExpression_E_Or_QGT_GE_E_S_QNumericExpression_E_C_E_Opt {
		if ($2 == NULL) {
			$$	= $1;
		} else {
			expr_t* e	= $2;
			container_unshift_item( e->args, $1 );
			$$	= e;
		}
	}
;

_O_QGT_EQUAL_E_S_QNumericExpression_E_Or_QGT_NEQUAL_E_S_QNumericExpression_E_Or_QGT_LT_E_S_QNumericExpression_E_Or_QGT_GT_E_S_QNumericExpression_E_Or_QGT_LE_E_S_QNumericExpression_E_Or_QGT_GE_E_S_QNumericExpression_E_C:
	GT_EQUAL NumericExpression	{
			$$	= new_expr_data( HX_EXPR_OP_EQUAL, $2 );
	}

	| GT_NEQUAL NumericExpression	{
			$$	= new_expr_data( HX_EXPR_OP_NEQUAL, $2 );
	}

	| GT_LT NumericExpression	{
			$$	= new_expr_data( HX_EXPR_OP_LT, $2 );
	}

	| GT_GT NumericExpression	{
			$$	= new_expr_data( HX_EXPR_OP_GT, $2 );
	}

	| GT_LE NumericExpression	{
			$$	= new_expr_data( HX_EXPR_OP_LE, $2 );
	}

	| GT_GE NumericExpression	{
			$$	= new_expr_data( HX_EXPR_OP_GE, $2 );
	}
;

_Q_O_QGT_EQUAL_E_S_QNumericExpression_E_Or_QGT_NEQUAL_E_S_QNumericExpression_E_Or_QGT_LT_E_S_QNumericExpression_E_Or_QGT_GT_E_S_QNumericExpression_E_Or_QGT_LE_E_S_QNumericExpression_E_Or_QGT_GE_E_S_QNumericExpression_E_C_E_Opt:
	{
		$$	= NULL;
	}

	| _O_QGT_EQUAL_E_S_QNumericExpression_E_Or_QGT_NEQUAL_E_S_QNumericExpression_E_Or_QGT_LT_E_S_QNumericExpression_E_Or_QGT_GT_E_S_QNumericExpression_E_Or_QGT_LE_E_S_QNumericExpression_E_Or_QGT_GE_E_S_QNumericExpression_E_C {
		$$	= $1;
	}
;

NumericExpression:
	AdditiveExpression	{
		$$	= $1;
	}
;

AdditiveExpression:
	MultiplicativeExpression _Q_O_QGT_PLUS_E_S_QMultiplicativeExpression_E_Or_QGT_MINUS_E_S_QMultiplicativeExpression_E_Or_QNumericLiteralPositive_E_Or_QNumericLiteralNegative_E_C_E_Star	{
		if ($2 == NULL) {
			$$	= $1;
		} else {
			expr_t* e	= $2;
			container_unshift_item( e->args, $1 );
			$$	= e;
		}
	}
;

_O_QGT_PLUS_E_S_QMultiplicativeExpression_E_Or_QGT_MINUS_E_S_QMultiplicativeExpression_E_Or_QNumericLiteralPositive_E_Or_QNumericLiteralNegative_E_C:
	GT_PLUS MultiplicativeExpression {
			$$	= new_expr_data( HX_EXPR_OP_PLUS, $2 );
	}

	| GT_MINUS MultiplicativeExpression	{
			$$	= new_expr_data( HX_EXPR_OP_MINUS, $2 );
	}

	| NumericLiteralPositive {
			$$	= new_expr_data( HX_EXPR_OP_NODE, $1 );
	}

	| NumericLiteralNegative {
			$$	= new_expr_data( HX_EXPR_OP_NODE, $1 );
	}
;

_Q_O_QGT_PLUS_E_S_QMultiplicativeExpression_E_Or_QGT_MINUS_E_S_QMultiplicativeExpression_E_Or_QNumericLiteralPositive_E_Or_QNumericLiteralNegative_E_C_E_Star:
	{
		$$	= NULL;
	}

	| _Q_O_QGT_PLUS_E_S_QMultiplicativeExpression_E_Or_QGT_MINUS_E_S_QMultiplicativeExpression_E_Or_QNumericLiteralPositive_E_Or_QNumericLiteralNegative_E_C_E_Star _O_QGT_PLUS_E_S_QMultiplicativeExpression_E_Or_QGT_MINUS_E_S_QMultiplicativeExpression_E_Or_QNumericLiteralPositive_E_Or_QNumericLiteralNegative_E_C {
		/* XXX */
	}
;

MultiplicativeExpression:
	UnaryExpression _Q_O_QGT_TIMES_E_S_QUnaryExpression_E_Or_QGT_DIVIDE_E_S_QUnaryExpression_E_C_E_Star	{
		if ($2 == NULL) {
			$$	= $1;
		} else {
			expr_t* e	= $2;
			container_unshift_item( e->args, $1 );
			$$	= e;
		}
	}
;

_O_QGT_TIMES_E_S_QUnaryExpression_E_Or_QGT_DIVIDE_E_S_QUnaryExpression_E_C:
	GT_TIMES UnaryExpression {
			$$	= new_expr_data( HX_EXPR_OP_MULT, $2 );
	}

	| GT_DIVIDE UnaryExpression	{
			$$	= new_expr_data( HX_EXPR_OP_DIV, $2 );
	}
;

_Q_O_QGT_TIMES_E_S_QUnaryExpression_E_Or_QGT_DIVIDE_E_S_QUnaryExpression_E_C_E_Star:
	{
		$$	= NULL;
	}

	| _Q_O_QGT_TIMES_E_S_QUnaryExpression_E_Or_QGT_DIVIDE_E_S_QUnaryExpression_E_C_E_Star _O_QGT_TIMES_E_S_QUnaryExpression_E_Or_QGT_DIVIDE_E_S_QUnaryExpression_E_C {
		/* XXX */
	}
;

UnaryExpression:
	GT_NOT PrimaryExpression {
		$$	= new_expr_data( HX_EXPR_OP_BANG, $2 );
	}

	| GT_PLUS PrimaryExpression	{
		$$	= new_expr_data( HX_EXPR_OP_UPLUS, $2 );
	}

	| GT_MINUS PrimaryExpression {
		$$	= new_expr_data( HX_EXPR_OP_UMINUS, $2 );
	}

	| PrimaryExpression	{
		$$	= $1;
	}
;

PrimaryExpression:
	BrackettedExpression {
		$$	= $1;
	}

	| BuiltInCall	{
		$$	= $1;
	}

	| IRIrefOrFunction	{
		$$	= $1;
	}

	| RDFLiteral {
			$$	= new_expr_data( HX_EXPR_OP_NODE, $1 );
	}

	| NumericLiteral {
			$$	= new_expr_data( HX_EXPR_OP_NODE, $1 );
	}

	| BooleanLiteral {
			$$	= new_expr_data( HX_EXPR_OP_NODE, $1 );
	}

	| Var	{
			$$	= new_expr_data( HX_EXPR_OP_NODE, $1 );
	}
;

BrackettedExpression:
	GT_LPAREN Expression GT_RPAREN	{
		$$	= $2;
	}
;

BuiltInCall:
	IT_STR GT_LPAREN Expression GT_RPAREN	{
		$$	= new_expr_data( HX_EXPR_BUILTIN_STR, $3 );
	}

	| IT_LANG GT_LPAREN Expression GT_RPAREN {
		$$	= new_expr_data( HX_EXPR_BUILTIN_LANG, $3 );
	}

	| IT_LANGMATCHES GT_LPAREN Expression GT_COMMA Expression GT_RPAREN	{
		$$	= new_expr_data( HX_EXPR_BUILTIN_LANGMATCHES, $3 );
	}

	| IT_DATATYPE GT_LPAREN Expression GT_RPAREN {
		$$	= new_expr_data( HX_EXPR_BUILTIN_DATATYPE, $3 );
	}

	| IT_BOUND GT_LPAREN Var GT_RPAREN	{
		expr_t* e	= new_expr_data( HX_EXPR_OP_NODE, $3 );
		$$	= new_expr_data( HX_EXPR_BUILTIN_BOUND, e );
	}

	| IT_sameTerm GT_LPAREN Expression GT_COMMA Expression GT_RPAREN {
		expr_t* e	= new_expr_data( HX_EXPR_BUILTIN_SAMETERM, $3 );
		container_push_item( e->args, $5 );
		$$	= e;
	}

	| IT_isIRI GT_LPAREN Expression GT_RPAREN	{
		$$	= new_expr_data( HX_EXPR_BUILTIN_ISIRI, $3 );
	}

	| IT_isURI GT_LPAREN Expression GT_RPAREN	{
		$$	= new_expr_data( HX_EXPR_BUILTIN_ISURI, $3 );
	}

	| IT_isBLANK GT_LPAREN Expression GT_RPAREN	{
		$$	= new_expr_data( HX_EXPR_BUILTIN_ISBLANK, $3 );
	}

	| IT_isLITERAL GT_LPAREN Expression GT_RPAREN	{
		$$	= new_expr_data( HX_EXPR_BUILTIN_ISLITERAL, $3 );
	}

	| RegexExpression	{
		$$	= $1;
	}
;

RegexExpression:
	IT_REGEX GT_LPAREN Expression GT_COMMA Expression _Q_O_QGT_COMMA_E_S_QExpression_E_C_E_Opt GT_RPAREN {
		expr_t* e	= new_expr_data( HX_EXPR_BUILTIN_REGEX, $3 );
		container_push_item( e->args, $5 );
		/* XXX handle optional regex flags */
		$$	= e;
	}
;

_Q_O_QGT_COMMA_E_S_QExpression_E_C_E_Opt:
	{
		$$	= NULL;
	}

	| _O_QGT_COMMA_E_S_QExpression_E_C	{
		$$	= $1;
	}
;

IRIrefOrFunction:
	IRIref _QArgList_E_Opt	{
		if ($2 == NULL) {
			$$	= new_expr_data( HX_EXPR_OP_NODE, $1 );
		} else {
			/* XXX handle function calls */
			$$	= NULL;
		}
	}
;

_QArgList_E_Opt:
	{
		$$	= NULL;
	}

	| ArgList	{
		/* arg lists... */
		$$	= NULL;
	}
;




/* ************************************************************************* */
/* my custum stuff: ******************************************************** */
__Filter_Opt:
	{
		$$	= NULL
	}
	
	| Filter {
		$$	= $1;
	}
;

/* ************************************************************************* */


// Query:
//	   Prologue _O_QSelectQuery_E_Or_QConstructQuery_E_Or_QDescribeQuery_E_Or_QAskQuery_E_C {
// //	 StupidGlobal = new Query($1, $2);
// }
// ;
// 
// _O_QSelectQuery_E_Or_QConstructQuery_E_Or_QDescribeQuery_E_Or_QAskQuery_E_C:
//	   SelectQuery	{
//	   $$ = new _O_QSelectQuery_E_Or_QConstructQuery_E_Or_QDescribeQuery_E_Or_QAskQuery_E_C_rule0($1);
// }
// 
//	   | ConstructQuery {
//	   $$ = new _O_QSelectQuery_E_Or_QConstructQuery_E_Or_QDescribeQuery_E_Or_QAskQuery_E_C_rule1($1);
// }
// 
//	   | DescribeQuery	{
//	   $$ = new _O_QSelectQuery_E_Or_QConstructQuery_E_Or_QDescribeQuery_E_Or_QAskQuery_E_C_rule2($1);
// }
// 
//	   | AskQuery	{
//	   $$ = new _O_QSelectQuery_E_Or_QConstructQuery_E_Or_QDescribeQuery_E_Or_QAskQuery_E_C_rule3($1);
// }
// ;
//

SelectQuery:
	IT_SELECT _Q_O_QIT_DISTINCT_E_Or_QIT_REDUCED_E_C_E_Opt _O_QVar_E_Plus_Or_QGT_TIMES_E_C _QDatasetClause_E_Star WhereClause SolutionModifier	{
		
	}
;

_O_QIT_DISTINCT_E_Or_QIT_REDUCED_E_C:
	IT_DISTINCT	{}

	| IT_REDUCED {}
;

_Q_O_QIT_DISTINCT_E_Or_QIT_REDUCED_E_C_E_Opt:
	{}

	| _O_QIT_DISTINCT_E_Or_QIT_REDUCED_E_C	{}
;

_QVar_E_Plus:
	Var	{
		container_t* set	= new_container( TYPE_NODES, 4 );
		node_t* n			= (node_t*) $1
		container_push_item( set, n );
		$$	= set;
	}

	| _QVar_E_Plus Var	{
		container_t* set	= $1;
		node_t* n			= (node_t*) $1
		container_push_item( set, n );
		$$	= set;
	}
;

_O_QVar_E_Plus_Or_QGT_TIMES_E_C:
		_QVar_E_Plus {
			$$	= $1;
		}

		| GT_TIMES	{
			$$	= new_container( TYPE_NODES, 1 );
		}
;

_QDatasetClause_E_Star:
		{
			$$	= NULL;
		}

		| _QDatasetClause_E_Star DatasetClause	{}
;

// ConstructQuery:
//	   IT_CONSTRUCT ConstructTemplate _QDatasetClause_E_Star WhereClause SolutionModifier	{}
// ;
// 
// DescribeQuery:
//	   IT_DESCRIBE _O_QVarOrIRIref_E_Plus_Or_QGT_TIMES_E_C _QDatasetClause_E_Star _QWhereClause_E_Opt SolutionModifier	{}
// ;
// 
// _QVarOrIRIref_E_Plus:
//	   VarOrIRIref	{}
// 
//	   | _QVarOrIRIref_E_Plus VarOrIRIref	{}
// ;
// 
// _O_QVarOrIRIref_E_Plus_Or_QGT_TIMES_E_C:
//	   _QVarOrIRIref_E_Plus {}
// 
//	   | GT_TIMES	{}
// ;
// 
// _QWhereClause_E_Opt:
//	   {}
// 
//	   | WhereClause	{}
// ;
// 
// AskQuery:
//	   IT_ASK _QDatasetClause_E_Star WhereClause	{}
// ;
// 
DatasetClause:
		IT_FROM _O_QDefaultGraphClause_E_Or_QNamedGraphClause_E_C	{}
;

_O_QDefaultGraphClause_E_Or_QNamedGraphClause_E_C:
		DefaultGraphClause	{}

		| NamedGraphClause	{}
;

DefaultGraphClause:
		SourceSelector	{}
;

NamedGraphClause:
		IT_NAMED SourceSelector	{}
;

SourceSelector:
		IRIref	{}
;

WhereClause:
		_QIT_WHERE_E_Opt GroupGraphPattern	{
			$$	= $2;
		}
;

_QIT_WHERE_E_Opt:
		{}
		| IT_WHERE	{}
;

SolutionModifier:
		_QOrderClause_E_Opt _QLimitOffsetClauses_E_Opt	{}
;

_QOrderClause_E_Opt:
		{
			$$	= NULL;
		}

		| OrderClause	{}
;

_QLimitOffsetClauses_E_Opt:
		{
			$$	= NULL;
		}

		| LimitOffsetClauses {}
;

LimitOffsetClauses:
		_O_QLimitClause_E_S_QOffsetClause_E_Opt_Or_QOffsetClause_E_S_QLimitClause_E_Opt_C	{}
;

_QOffsetClause_E_Opt:
		{
			$$	= NULL;
		}

		| OffsetClause	{}
;

_QLimitClause_E_Opt:
		{
			$$	= NULL;
		}

		| LimitClause	{}
;

_O_QLimitClause_E_S_QOffsetClause_E_Opt_Or_QOffsetClause_E_S_QLimitClause_E_Opt_C:
		LimitClause _QOffsetClause_E_Opt {}

		| OffsetClause _QLimitClause_E_Opt	{}
;

OrderClause:
		IT_ORDER IT_BY _QOrderCondition_E_Plus	{}
;

_QOrderCondition_E_Plus:
		OrderCondition	{}

		| _QOrderCondition_E_Plus OrderCondition {}
;

OrderCondition:
		_O_QIT_ASC_E_Or_QIT_DESC_E_S_QBrackettedExpression_E_C	{}

		| _O_QConstraint_E_Or_QVar_E_C	{}
;

_O_QIT_ASC_E_Or_QIT_DESC_E_C:
		IT_ASC	{}
		| IT_DESC	{}
;

_O_QIT_ASC_E_Or_QIT_DESC_E_S_QBrackettedExpression_E_C:
		_O_QIT_ASC_E_Or_QIT_DESC_E_C BrackettedExpression	{}
;

_O_QConstraint_E_Or_QVar_E_C:
		Constraint	{}
		| Var	{}
;

LimitClause:
		IT_LIMIT INTEGER {
			$$	= $2;
		}
;

OffsetClause:
		IT_OFFSET INTEGER	{
			$$	= $2;
		}
;

GroupGraphPattern:
		GT_LCURLEY _QTriplesBlock_E_Opt _Q_O_QGraphPatternNotTriples_E_Or_QFilter_E_S_QGT_DOT_E_Opt_S_QTriplesBlock_E_Opt_C_E_Star GT_RCURLEY	{
			container_t* bgp	= (container_t*) $2;
			container_t* set	= new_container( TYPE_GGP, 5 );
			container_push_item( set, bgp );
			
			$$	= set;
			if ($3 != NULL) {
				container_t* nottriples	= (container_t*) $3;
				if (nottriples->type == TYPE_FILTER) {
					container_t* filter	= nottriples;
					container_push_item( filter, set );
					$$	= filter;
				}
			}
			
			
			/** XXXXXXXXXXXXXXXXXXXX **/
			/** this is to set the BGP for the parse_bgp functions **/
			bgp_query_t* q	= (bgp_query_t*) calloc( 1, sizeof( bgp_query_t ) );
			q->prologue		= NULL;
			q->bgp			= bgp;
			q->filter		= NULL;
			parsedBGPPattern		= (void*) q;
			/** XXXXXXXXXXXXXXXXXXXX **/
			
			
			
			
			
			
		}
;

_O_QGraphPatternNotTriples_E_Or_QFilter_E_C:
		GraphPatternNotTriples	{
			$$	= $1;
		}

		| Filter {
			$$	= $1;
		}
;

_QGT_DOT_E_Opt:
		{}
		| GT_DOT {}
;

_O_QGraphPatternNotTriples_E_Or_QFilter_E_S_QGT_DOT_E_Opt_S_QTriplesBlock_E_Opt_C:
		_O_QGraphPatternNotTriples_E_Or_QFilter_E_C _QGT_DOT_E_Opt _QTriplesBlock_E_Opt	{
			$$	= $1;
			if ($3 != NULL) {
				/* XXX */
			}
		}
;

_Q_O_QGraphPatternNotTriples_E_Or_QFilter_E_S_QGT_DOT_E_Opt_S_QTriplesBlock_E_Opt_C_E_Star:
		{
			$$	= NULL;
		}

		| _Q_O_QGraphPatternNotTriples_E_Or_QFilter_E_S_QGT_DOT_E_Opt_S_QTriplesBlock_E_Opt_C_E_Star _O_QGraphPatternNotTriples_E_Or_QFilter_E_S_QGT_DOT_E_Opt_S_QTriplesBlock_E_Opt_C	{
			$$	= $2;
			/* XXX handle the stuff in $1 */
		}
;

GraphPatternNotTriples:
		OptionalGraphPattern {}

		| GroupOrUnionGraphPattern	{}

		| GraphGraphPattern	{}
;

OptionalGraphPattern:
		IT_OPTIONAL GroupGraphPattern	{}
;

GraphGraphPattern:
		IT_GRAPH VarOrIRIref GroupGraphPattern	{}
;

GroupOrUnionGraphPattern:
		GroupGraphPattern _Q_O_QIT_UNION_E_S_QGroupGraphPattern_E_C_E_Star	{}
;

_O_QIT_UNION_E_S_QGroupGraphPattern_E_C:
		IT_UNION GroupGraphPattern	{}
;

_Q_O_QIT_UNION_E_S_QGroupGraphPattern_E_C_E_Star:
		{
			$$	= NULL;
		}

		| _Q_O_QIT_UNION_E_S_QGroupGraphPattern_E_C_E_Star _O_QIT_UNION_E_S_QGroupGraphPattern_E_C	{}
;


// ConstructTemplate:
//	   GT_LCURLEY _QConstructTriples_E_Opt GT_RCURLEY	{}
// ;
// 
// _QConstructTriples_E_Opt:
//	   {}
// 
//	   | ConstructTriples	{}
// ;
// 
// ConstructTriples:
//	   TriplesSameSubject _Q_O_QGT_DOT_E_S_QConstructTriples_E_Opt_C_E_Opt	{}
// ;
// 
// _O_QGT_DOT_E_S_QConstructTriples_E_Opt_C:
//	   GT_DOT _QConstructTriples_E_Opt	{}
// ;
// 
// _Q_O_QGT_DOT_E_S_QConstructTriples_E_Opt_C_E_Opt:
//	   {}
// 
//	   | _O_QGT_DOT_E_S_QConstructTriples_E_Opt_C	{}
// ;
//


%%

 /*** END SPARQL - Change the grammar rules above ***/

/* START main */

#include <stdio.h>
#include <stdlib.h>
#include "triple.h"
#include "node.h"
#include "bgp.h"

typedef struct {
	char* name;
	int id;
	void* next;
} hx_sparqlparser_variable_map_list;

void* parsedBGPPattern	= NULL;
void* parsedGP			= NULL;
void* parsedPrologue	= NULL;
void free_prologue ( prologue_t* p );
char* prefix_uri ( prologue_t* p, char* ns );
char* qualify_qname ( prologue_t* p, char* qname );
hx_node* generate_node ( node_t* n, prologue_t* p, hx_sparqlparser_variable_map_list* vmap );
hx_expr* generate_expr ( expr_t* e, prologue_t* p, hx_sparqlparser_variable_map_list* vmap );
hx_bgp* generate_bgp ( container_t* bgp, prologue_t* p, hx_sparqlparser_variable_map_list* vmap );
hx_graphpattern* generate_graphpattern ( container_t* bgp, prologue_t* p, hx_sparqlparser_variable_map_list* vmap );

hx_bgp* parse_bgp_query ( void );
hx_bgp* parse_bgp_query_string ( char* string );

hx_graphpattern* parse_query ( void );
hx_graphpattern* parse_query_string ( char* string );

int free_container ( container_t* c, int free_contained_objects );
int free_node ( node_t* n );

void yyerror (char const *s) {
	fprintf (stderr, "*** %s\n", s);
}

hx_graphpattern* parse_query ( void ) {
	if (yyparse() == 0) {
		int i;
		hx_graphpattern* g;
		container_t* graphpattern;
		hx_triple** triples;
		
		hx_sparqlparser_variable_map_list* vmap	= (hx_sparqlparser_variable_map_list*) calloc( 1, sizeof(hx_sparqlparser_variable_map_list) );
		vmap->id	= 0;
		vmap->name	= "";
		vmap->next	= NULL;
		
		gp_query_t* q			= (gp_query_t*) parsedGP;
		prologue_t* prologue	= q->prologue;
		
		hx_graphpattern* pat	= generate_graphpattern( q->gp, prologue, vmap );
		free_prologue( prologue );
		
		return pat;
	}
	return NULL;
}

hx_graphpattern* parse_query_string ( char* string ) {
	yy_scan_string( string );
	return parse_query();
}

hx_bgp* parse_bgp_query_string ( char* string ) {
	yy_scan_string( string );
	return parse_bgp_query();
}

hx_bgp* parse_bgp_query ( void ) {
	if (yyparse() == 0) {
		hx_bgp* b;
		container_t* bgp;

		hx_sparqlparser_variable_map_list* vmap	= (hx_sparqlparser_variable_map_list*) calloc( 1, sizeof(hx_sparqlparser_variable_map_list) );
		vmap->id	= 0;
		vmap->name	= "";
		vmap->next	= NULL;
		
		bgp_query_t* q			= (bgp_query_t*) parsedBGPPattern;
		prologue_t* prologue	= parsedPrologue;
		if (q->filter != NULL) {
			hx_expr* e;
			char* string;
			fprintf( stderr, "got filter\n" );
			
			e	= generate_expr( q->filter, prologue, vmap );
			hx_expr_sse( e, &string, "  ", 9 );
			fprintf( stderr,         "filter expression: %s\n", string );
			free( string );
		}
		
		b	= generate_bgp( q->bgp, prologue, vmap );
		free_prologue( prologue );
		return b;
	}
	return NULL;
}

hx_graphpattern* generate_graphpattern ( container_t* gp, prologue_t* p, hx_sparqlparser_variable_map_list* vmap ) {
	int i;
	hx_bgp* b;
	hx_expr* e;
	hx_graphpattern** patterns;
	hx_graphpattern *pat;
	hx_sparqlparser_pattern_t type	= gp->type;

	switch (type) {
		case TYPE_GGP:
			patterns	= (hx_graphpattern**) calloc( gp->count, sizeof( hx_graphpattern* ) );
			for (i = 0; i < gp->count; i++) {
				patterns[i]	= generate_graphpattern( gp->items[i], p, vmap );
			}
			return hx_new_graphpattern_ptr( HX_GRAPHPATTERN_GROUP, gp->count, patterns );
		case TYPE_BGP:
			b	= generate_bgp( gp, p, vmap );
			return hx_new_graphpattern( HX_GRAPHPATTERN_BGP, b );
		case TYPE_FILTER:
			e	= generate_expr( gp->items[0], p, vmap );
			pat	= generate_graphpattern( gp->items[1], p, vmap );
			return hx_new_graphpattern( HX_GRAPHPATTERN_FILTER, e, pat );
		default:
			fprintf( stderr, "*** Unimplemented graphpattern type %c in generate_graphpattern\n", type );
			return NULL;
	};
	
	return NULL;
}

hx_bgp* generate_bgp ( container_t* bgp, prologue_t* prologue, hx_sparqlparser_variable_map_list* vmap ) {
	int i;
	hx_triple** triples	= (hx_triple**) calloc( bgp->count, sizeof( hx_triple* ) );
	for (i = 0; i < bgp->count; i++) {
		triple_t* t		= bgp->items[i];
		hx_node* s	= generate_node( t->subject, prologue, vmap );
		hx_node* p	= generate_node( t->predicate, prologue, vmap );
		hx_node* o	= generate_node( t->object, prologue, vmap );
		hx_triple* triple;
		if (s == NULL || p == NULL || o == NULL) {
			fprintf( stderr, "Got NULL node in triple: (%p, %p, %p)\n", (void*) s, (void*) p, (void*) o );
			return NULL;
		}
		triple	= hx_new_triple( s, p, o );
		triples[i]	= triple;
	}
	
	return hx_new_bgp( bgp->count, triples );
}

hx_expr* generate_expr ( expr_t* e, prologue_t* p, hx_sparqlparser_variable_map_list* vmap ) {
	container_t* c	= e->args;
	if (e->op == HX_EXPR_OP_NODE) {
		hx_node* n	= generate_node( c->items[0], p, vmap );
		return hx_new_node_expr( n );
	} else {
		int arity		= hx_expr_type_arity( e->op );
		if (arity == 1) {
			expr_t* d	= c->items[0];
			hx_expr* e1	= generate_expr( d, p, vmap );
			return hx_new_builtin_expr1( e->op, e1 );
		} else if (arity == 2) {
			expr_t* a	= c->items[0];
			expr_t* b	= c->items[1];
			hx_expr* e1	= generate_expr( a, p, vmap );
			hx_expr* e2	= generate_expr( b, p, vmap );
			return hx_new_builtin_expr2( e->op, e1, e2 );
		} else if (arity == 3) {
			expr_t* _a	= c->items[0];
			expr_t* _b	= c->items[1];
			expr_t* _c	= c->items[2];
			hx_expr* e1	= generate_expr( _a, p, vmap );
			hx_expr* e2	= generate_expr( _b, p, vmap );
			hx_expr* e3	= generate_expr( _c, p, vmap );
			return hx_new_builtin_expr3( e->op, e1, e2, e3 );
		} else {
			fprintf( stderr, "*** not an implemented arity size in generate_expr: %d\n", arity );
		}
	}
	fprintf( stderr, "expr type %d\n", e->op );
	return NULL;
}

int variable_id_with_name ( hx_sparqlparser_variable_map_list* vmap, char* name ) {
	int current_id	= 0;
	
	hx_sparqlparser_variable_map_list* p	= vmap;
	hx_sparqlparser_variable_map_list* last	= p;
	while (p != NULL) {
		current_id	= p->id;
		char* n		= p->name;
		if (strcmp(n,name) == 0) {
			return p->id;
		}
		last	= p;
		p		= (hx_sparqlparser_variable_map_list*) p->next;
	}
	
	--current_id;
	
	hx_sparqlparser_variable_map_list* new	= (hx_sparqlparser_variable_map_list*) calloc( 1, sizeof( hx_sparqlparser_variable_map_list ) );
	new->name	= name;
	new->id		= current_id;
	new->next	= NULL;
	last->next	= new;
	return current_id;
}

hx_node* generate_node ( node_t* n, prologue_t* p, hx_sparqlparser_variable_map_list* vmap ) {
	if (n->type == TYPE_FULL_NODE) {
		hx_node* node	= (hx_node*) n->ptr;
		return hx_node_copy( node );
	} else if (n->type == TYPE_VARIABLE) {
		char* name	= (char*) n->ptr;
		char* copy	= (char*) malloc( strlen( name ) + 1 );
		hx_node* v;
		strcpy( copy, name );
		int id	= variable_id_with_name( vmap, copy );
		v	= hx_new_node_named_variable( id, copy );
		return v;
	} else if (n->type == TYPE_QNAME) {
		hx_node* u;
		char* uri	= qualify_qname( p, (char*) n->ptr );
		if (uri == NULL)
			return NULL;
		u	= hx_new_node_resource( uri );
		return u;
	} else if (n->type == TYPE_DT_LITERAL) {
		char* value;
		hx_node* l;
		char* dt	= qualify_qname( p, n->datatype );
		if (dt == NULL)
			return NULL;
		value	= (char*) n->ptr;
		l	= (hx_node*) hx_new_node_dt_literal( value, dt );
		return l;
	} else {
		fprintf( stderr, "*** UNRECOGNIZED node type '%c' in generate_node()\n", n->type );
		return NULL;
	}
}

char* qualify_qname ( prologue_t* p, char* _qname ) {
	char* qname	= (char*) malloc( strlen( _qname ) + 1 );
	char *ns, *local, *ns_uri;
	
	strcpy( qname, _qname );
	ns	= qname;
	local	= strchr( ns, ':' );
	
	*(local++)	= (char) 0;
	ns_uri	= prefix_uri( p, ns );
	if (ns_uri == NULL) {
		char* uri	= (char*) calloc( strlen( local ) + 1, sizeof( char ) );
		strcpy( uri, local );
		free( qname );
		return uri;
	} else {
		char* uri	= (char*) calloc( strlen( ns_uri ) + strlen( local ) + 1, sizeof( char ) );
		strcat( uri, ns_uri );
		strcat( uri, local );
		free( qname );
		return uri;
	}
}

char* prefix_uri ( prologue_t* p, char* ns ) {
	if (p->ns != NULL) {
		int i;
		namespace_set_t* s	= p->ns;
		for (i = 0; i < s->namespace_count; i++) {
			namespace_t* n	= s->namespaces[i];
			if (strcmp( n->name, ns ) == 0) {
				return n->uri;
			}
		}
	}
	return NULL;
}

void free_prologue ( prologue_t* p ) {
	if (p->base != NULL) {
		free( p->base );
	}
	if (p->ns != NULL) {
		int i;
		namespace_set_t* s	= p->ns;
		for (i = 0; i < s->namespace_count; i++) {
			namespace_t* n	= s->namespaces[i];
			free( n->name );
			free( n->uri );
			free( n );
		}
		free( s );
	}
	free( p );
}

extern node_t* new_dt_literal_node ( char* string, char* dt ) {
	hx_node* n		= (hx_node*) hx_new_node_dt_literal( string, dt );
	node_t* node	= (node_t*) calloc( 1, sizeof( node_t ) );
	node->type		= TYPE_FULL_NODE;
	node->ptr		= (void*) n;
	return node;
}


container_t* new_container ( char type, int size ) {
	container_t* container	= (container_t*) calloc( 1, sizeof( container_t ) );
	container->type			= type;
	container->allocated	= size;
	container->count		= 0;
	container->items		= (void**) calloc( container->allocated, sizeof( void* ) );
	return container;
}

int free_container ( container_t* c, int free_contained_objects ) {
	int i;
	switch (c->type) {
		case TYPE_BGP:
			if (free_contained_objects) {
				for (i = 0; i < c->count; i++) {
					triple_t* t	= (triple_t*) c->items[i];
					free_node( t->subject );
					free_node( t->predicate );
					free_node( t->object );
					free(t);
				}
			}
			free(c->items);
			break;
		default:
			fprintf( stderr, "*** unrecognized container type '%c' in free_container()\n", c->type );
			return 1;
	};
	free(c);
	return 0;
}

int free_node ( node_t* n ) {
	if (n->type == TYPE_FULL_NODE) {
		hx_node* node	= (hx_node*) n->ptr;
		hx_free_node( node );
		free(n);
	} else if (n->type == TYPE_VARIABLE) {
		char* name	= (char*) n->ptr;
		if (name != NULL)
			free(name);
		free(n);
	} else if (n->type == TYPE_QNAME) {
		char* qname	= (char*) n->ptr;
		free( qname );
		free(n);
	} else if (n->type == TYPE_DT_LITERAL) {
		char* qname	= (char*) n->ptr;
		char* dt	= (char*) n->datatype;
		free( qname );
		free( dt );
		free(n);
	} else {
		fprintf( stderr, "*** UNRECOGNIZED node type '%c' in free_node()\n", n->type );
		return 1;
	}
	return 0;
}

void container_push_item( container_t* set, void* t ) {
	if (set->allocated <= (set->count + 1)) {
		int i;
		void** old;
		void** newlist;
		set->allocated	*= 2;
		newlist	= (void**) calloc( set->allocated, sizeof( void* ) );
		for (i = 0; i < set->count; i++) {
			newlist[i]	= set->items[i];
		}
		old	= set->items;
		set->items	= newlist;
		free( old );
	}
	
	set->items[ set->count++ ]	= t;
}

void container_unshift_item( container_t* set, void* t ) {
	if (set->allocated <= (set->count + 1)) {
		int i;
		void** old;
		void** newlist;
		set->allocated	*= 2;
		newlist	= (void**) calloc( set->allocated, sizeof( void* ) );
		for (i = 0; i < set->count; i++) {
			newlist[i+1]	= set->items[i];
		}
		old	= set->items;
		set->items	= newlist;
		free( old );
	} else {
		int i;
		for (i = set->count; i > 0; i--) {
			set->items[i]	= set->items[i-1];
		}
	}
	
	set->count++;
	set->items[ 0 ]	= t;
}

expr_t* new_expr_data ( hx_expr_subtype_t op, void* arg ) {
	expr_t* d	= (expr_t*) calloc( 1, sizeof( expr_t ) );
	d->op	= op;
	d->args	= new_container( TYPE_EXPR, 3 );
	container_push_item( d->args, arg );
	return d;
}

void XXXdebug_triple( triple_t* t, prologue_t* p ) {
	fprintf( stderr, "- subject: " ); XXXdebug_node( t->subject, p );
	fprintf( stderr, "- predicate: " ); XXXdebug_node( t->predicate, p );
	fprintf( stderr, "- object: " ); XXXdebug_node( t->object, p );
}

void XXXdebug_node( node_t* n, prologue_t* p ) {
	hx_sparqlparser_variable_map_list* vmap	= (hx_sparqlparser_variable_map_list*) calloc( 1, sizeof(hx_sparqlparser_variable_map_list) );
	vmap->id	= 0;
	vmap->name	= "";
	vmap->next	= NULL;
	
	hx_node* node	= generate_node( n, p, vmap );
	if (node != NULL) {
		char* string;
		hx_node_string( node, &string );
		fprintf( stderr, "Node: %s\n", string );
		free( string );
		hx_free_node( node );
	}
}

