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

int yylex ( void );
void yyerror (char const *s);
#define YYSTYPE void*
#define YYSTYPE_IS_DECLARED

typedef struct {
	/**
	// type is either:
	// - 'N' for a full node,
	// - 'V' for a char* variable name
	// - 'Q' for a QName (that still needs to be expanded)
	// - 'L' for a Literal with Qname datatype
	**/
	char type;
	void* ptr;
	char* datatype;
} node_t;

typedef struct {
	node_t* subject;
	node_t* predicate;
	node_t* object;
} triple_t;

typedef struct {
	int allocated;
	int triple_count;
	triple_t** triples;
} triple_set_t;

typedef struct {
	char* language;
	node_t* datatype;
} string_attrs_t;

typedef struct {
	char* name;
	char* uri;
} namespace_t;

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
	triple_set_t* bgp;
} query_t;

extern void* parsedPattern;
extern triple_set_t* new_triple_set ( int size );
extern node_t* new_dt_literal_node ( char*, char* );
extern void add_triple_to_set( triple_set_t* set, triple_t* t );

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
	Prologue GT_LCURLEY _QTriplesBlock_E_Opt GT_RCURLEY {
		query_t* q		= (query_t*) calloc( 1, sizeof( query_t ) );
		q->prologue		= (prologue_t*) $1;
		q->bgp			= (triple_set_t*) $3;
		parsedPattern		= (void*) q;
	}
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
			/* XXX */
			int i;
			triple_set_t* triples1	= (triple_set_t*) $1;
			triple_set_t* triples2	= (triple_set_t*) $2;
			for (i = 0; i < triples2->triple_count; i++) {
				add_triple_to_set( triples1, triples2->triples[i] );
			}
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
		int i;
		triple_set_t* subj_triples	= (triple_set_t*) $1;
		if (subj_triples->triple_count == 0) {
			fprintf( stderr, "uh oh. VarOrTerm didn't return any graph triples.\n" );
		}
		node_t* subject	= subj_triples->triples[0]->subject;
		triple_set_t* triples	= (triple_set_t*) $2;
		for (i = 0; i < triples->triple_count; i++) {
			triples->triples[i]->subject	= subject;
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
		triple_set_t* triples;
		triples	= (triple_set_t*) $2;
		/* XXX */
		int i;
		node_t* predicate	= (node_t*) $1;
		for (i = 0; i < triples->triple_count; i++) {
			triples->triples[i]->predicate	= predicate;
		}
		if ($3 != NULL) {
			int i;
			triple_set_t* triples2	= (triple_set_t*) $3;
			for (i = 0; i < triples2->triple_count; i++) {
				add_triple_to_set( triples, triples2->triples[i] );
			}
		}
		$$	= (void*) triples;
	}
;

_O_QVerb_E_S_QObjectList_E_C:
	Verb ObjectList {
		triple_set_t* triples	= (triple_set_t*) $2;
		int i;
		node_t* predicate	= (node_t*) $1;
		for (i = 0; i < triples->triple_count; i++) {
			triples->triples[i]->predicate	= predicate;
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
		triple_set_t* triples;
		if ($1 == NULL) {
			triples	= (triple_set_t*) $2;
		} else {
			triples	= (triple_set_t*) $1;
			if ($2 != NULL) {
				int i;
				triple_set_t* triples2	= (triple_set_t*) $2;
				for (i = 0; i < triples2->triple_count; i++) {
					add_triple_to_set( triples, triples2->triples[i] );
				}
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
		triple_set_t* set;
		triple_set_t* obj_triples;
		if ($2 == NULL) {
			set	= new_triple_set( 5 );
		} else {
			set	= (triple_set_t*) $2;
		}
		
		obj_triples	= (triple_set_t*) $1;
		object	= obj_triples->triples[0]->subject;
		
		t			= (triple_t*) calloc( 1, sizeof( triple_t ) );
		t->object			= object;
		add_triple_to_set( set, t );
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
		triple_set_t* set;
		triple_set_t* obj_triples;
		if ($1 == NULL) {
			set	= new_triple_set( 5 );
		} else {
			set	= (triple_set_t*) $1;
		}
		
		obj_triples	= (triple_set_t*) $2;
		object	= obj_triples->triples[0]->subject;
		
		t			= (triple_t*) calloc( 1, sizeof( triple_t ) );
		t->object			= object;
		add_triple_to_set( set, t );
		
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
		n->type			= 'N';
		hx_node* iri	= hx_new_node_resource( "http://www.w3.org/1999/02/22-rdf-syntax-ns#type" );
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
		triple_set_t* set	= new_triple_set( 1 );
		triple_t* t			= (triple_t*) calloc( 1, sizeof( triple_t ) );
		t->subject			= (node_t*) $1;
		t->predicate		= NULL;
		t->object			= NULL;
		add_triple_to_set( set, t );
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
		n->type	= 'V';
		n->ptr	= (void*) $1;
		$$	= n;
	}

	| VAR2	{
		node_t* n	= (node_t*) calloc( 1, sizeof( node_t ) );
		n->type	= 'V';
		n->ptr	= (void*) $1;
		$$	= n;
	}
;

GraphTerm:
	IRIref	{
		triple_set_t* set	= new_triple_set( 1 );
		triple_t* t			= (triple_t*) calloc( 1, sizeof( triple_t ) );
		t->subject			= (node_t*) $1;
		t->predicate		= NULL;
		t->object			= NULL;
		add_triple_to_set( set, t );
		$$	= set;
	}

	| RDFLiteral	{
		triple_set_t* set	= new_triple_set( 1 );
		triple_t* t			= (triple_t*) calloc( 1, sizeof( triple_t ) );
		t->subject			= (node_t*) $1;
		t->predicate		= NULL;
		t->object			= NULL;
		add_triple_to_set( set, t );
		$$	= set;
	}

	| NumericLiteral	{
		triple_set_t* set	= new_triple_set( 1 );
		triple_t* t			= (triple_t*) calloc( 1, sizeof( triple_t ) );
		t->subject			= (node_t*) $1;
		t->predicate		= NULL;
		t->object			= NULL;
		add_triple_to_set( set, t );
		$$	= set;
	}

	| BooleanLiteral	{
		triple_set_t* set	= new_triple_set( 1 );
		triple_t* t			= (triple_t*) calloc( 1, sizeof( triple_t ) );
		t->subject			= (node_t*) $1;
		t->predicate		= NULL;
		t->object			= NULL;
		add_triple_to_set( set, t );
		$$	= set;
	}

	| BlankNode {
		triple_set_t* set	= new_triple_set( 1 );
		triple_t* t			= (triple_t*) calloc( 1, sizeof( triple_t ) );
		t->subject			= (node_t*) $1;
		t->predicate		= NULL;
		t->object			= NULL;
		add_triple_to_set( set, t );
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
			node->type		= 'N';
			node->ptr		= (void*) l;
		} else if (attrs->language != NULL) {
			hx_node* l	= (hx_node*) hx_new_node_lang_literal( (char*) $1, attrs->language );
			node->type		= 'N';
			node->ptr		= (void*) l;
		} else {
			node_t* dt		= attrs->datatype;
			if (dt->type == 'Q') {
				node->type		= 'L';
				node->ptr		= (char*) $1;
				node->datatype	= (char*) dt->ptr;
			} else {
				hx_node* l	= (hx_node*) hx_new_node_dt_literal( (char*) $1, (char*) dt->ptr );
				node->type		= 'N';
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
		r->type			= 'N';
		r->ptr			= iri;
		$$	= (void*) r;
	}

	| PrefixedName	{
		node_t* r	= (node_t*) calloc( 1, sizeof( node_t ) );
		r->type			= 'Q';
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
		r->type			= 'N';
		r->ptr			= b;
		$$	= (void*) r;
	}

	| ANON	{}
;









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
// SelectQuery:
//	   IT_SELECT _Q_O_QIT_DISTINCT_E_Or_QIT_REDUCED_E_C_E_Opt _O_QVar_E_Plus_Or_QGT_TIMES_E_C _QDatasetClause_E_Star WhereClause SolutionModifier	{
//	   $$ = new SelectQuery($1, $2, $3, $4, $5, $6);
// }
// ;
// 
// _O_QIT_DISTINCT_E_Or_QIT_REDUCED_E_C:
//	   IT_DISTINCT	{
//	   $$ = new _O_QIT_DISTINCT_E_Or_QIT_REDUCED_E_C_rule0($1);
// }
// 
//	   | IT_REDUCED {
//	   $$ = new _O_QIT_DISTINCT_E_Or_QIT_REDUCED_E_C_rule1($1);
// }
// ;
// 
// _Q_O_QIT_DISTINCT_E_Or_QIT_REDUCED_E_C_E_Opt:
//	   {
//	   $$ = new _Q_O_QIT_DISTINCT_E_Or_QIT_REDUCED_E_C_E_Opt_rule0();
// }
// 
//	   | _O_QIT_DISTINCT_E_Or_QIT_REDUCED_E_C	{
//	   $$ = new _Q_O_QIT_DISTINCT_E_Or_QIT_REDUCED_E_C_E_Opt_rule1($1);
// }
// ;
// 
// _QVar_E_Plus:
//	   Var	{
//	   $$ = new _QVar_E_Plus_rule0($1);
// }
// 
//	   | _QVar_E_Plus Var	{
//	   $$ = new _QVar_E_Plus_rule1($1, $2);
// }
// ;
// 
// _O_QVar_E_Plus_Or_QGT_TIMES_E_C:
//	   _QVar_E_Plus {
//	   $$ = new _O_QVar_E_Plus_Or_QGT_TIMES_E_C_rule0($1);
// }
// 
//	   | GT_TIMES	{
//	   $$ = new _O_QVar_E_Plus_Or_QGT_TIMES_E_C_rule1($1);
// }
// ;
// 
// _QDatasetClause_E_Star:
//	   {
//	   $$ = new _QDatasetClause_E_Star_rule0();
// }
// 
//	   | _QDatasetClause_E_Star DatasetClause	{
//	   $$ = new _QDatasetClause_E_Star_rule1($1, $2);
// }
// ;
// 
// ConstructQuery:
//	   IT_CONSTRUCT ConstructTemplate _QDatasetClause_E_Star WhereClause SolutionModifier	{
//	   $$ = new ConstructQuery($1, $2, $3, $4, $5);
// }
// ;
// 
// DescribeQuery:
//	   IT_DESCRIBE _O_QVarOrIRIref_E_Plus_Or_QGT_TIMES_E_C _QDatasetClause_E_Star _QWhereClause_E_Opt SolutionModifier	{
//	   $$ = new DescribeQuery($1, $2, $3, $4, $5);
// }
// ;
// 
// _QVarOrIRIref_E_Plus:
//	   VarOrIRIref	{
//	   $$ = new _QVarOrIRIref_E_Plus_rule0($1);
// }
// 
//	   | _QVarOrIRIref_E_Plus VarOrIRIref	{
//	   $$ = new _QVarOrIRIref_E_Plus_rule1($1, $2);
// }
// ;
// 
// _O_QVarOrIRIref_E_Plus_Or_QGT_TIMES_E_C:
//	   _QVarOrIRIref_E_Plus {
//	   $$ = new _O_QVarOrIRIref_E_Plus_Or_QGT_TIMES_E_C_rule0($1);
// }
// 
//	   | GT_TIMES	{
//	   $$ = new _O_QVarOrIRIref_E_Plus_Or_QGT_TIMES_E_C_rule1($1);
// }
// ;
// 
// _QWhereClause_E_Opt:
//	   {
//	   $$ = new _QWhereClause_E_Opt_rule0();
// }
// 
//	   | WhereClause	{
//	   $$ = new _QWhereClause_E_Opt_rule1($1);
// }
// ;
// 
// AskQuery:
//	   IT_ASK _QDatasetClause_E_Star WhereClause	{
//	   $$ = new AskQuery($1, $2, $3);
// }
// ;
// 
// DatasetClause:
//	   IT_FROM _O_QDefaultGraphClause_E_Or_QNamedGraphClause_E_C	{
//	   $$ = new DatasetClause($1, $2);
// }
// ;
// 
// _O_QDefaultGraphClause_E_Or_QNamedGraphClause_E_C:
//	   DefaultGraphClause	{
//	   $$ = new _O_QDefaultGraphClause_E_Or_QNamedGraphClause_E_C_rule0($1);
// }
// 
//	   | NamedGraphClause	{
//	   $$ = new _O_QDefaultGraphClause_E_Or_QNamedGraphClause_E_C_rule1($1);
// }
// ;
// 
// DefaultGraphClause:
//	   SourceSelector	{
//	   $$ = new DefaultGraphClause($1);
// }
// ;
// 
// NamedGraphClause:
//	   IT_NAMED SourceSelector	{
//	   $$ = new NamedGraphClause($1, $2);
// }
// ;
// 
// SourceSelector:
//	   IRIref	{
//	   $$ = new SourceSelector($1);
// }
// ;
// 
// WhereClause:
//	   _QIT_WHERE_E_Opt GroupGraphPattern	{
//	   $$ = new WhereClause($1, $2);
// }
// ;
// 
// _QIT_WHERE_E_Opt:
//	   {
//	   $$ = new _QIT_WHERE_E_Opt_rule0();
// }
// 
//	   | IT_WHERE	{
//	   $$ = new _QIT_WHERE_E_Opt_rule1($1);
// }
// ;
// 
// SolutionModifier:
//	   _QOrderClause_E_Opt _QLimitOffsetClauses_E_Opt	{
//	   $$ = new SolutionModifier($1, $2);
// }
// ;
// 
// _QOrderClause_E_Opt:
//	   {
//	   $$ = new _QOrderClause_E_Opt_rule0();
// }
// 
//	   | OrderClause	{
//	   $$ = new _QOrderClause_E_Opt_rule1($1);
// }
// ;
// 
// _QLimitOffsetClauses_E_Opt:
//	   {
//	   $$ = new _QLimitOffsetClauses_E_Opt_rule0();
// }
// 
//	   | LimitOffsetClauses {
//	   $$ = new _QLimitOffsetClauses_E_Opt_rule1($1);
// }
// ;
// 
// LimitOffsetClauses:
//	   _O_QLimitClause_E_S_QOffsetClause_E_Opt_Or_QOffsetClause_E_S_QLimitClause_E_Opt_C	{
//	   $$ = new LimitOffsetClauses($1);
// }
// ;
// 
// _QOffsetClause_E_Opt:
//	   {
//	   $$ = new _QOffsetClause_E_Opt_rule0();
// }
// 
//	   | OffsetClause	{
//	   $$ = new _QOffsetClause_E_Opt_rule1($1);
// }
// ;
// 
// _QLimitClause_E_Opt:
//	   {
//	   $$ = new _QLimitClause_E_Opt_rule0();
// }
// 
//	   | LimitClause	{
//	   $$ = new _QLimitClause_E_Opt_rule1($1);
// }
// ;
// 
// _O_QLimitClause_E_S_QOffsetClause_E_Opt_Or_QOffsetClause_E_S_QLimitClause_E_Opt_C:
//	   LimitClause _QOffsetClause_E_Opt {
//	   $$ = new _O_QLimitClause_E_S_QOffsetClause_E_Opt_Or_QOffsetClause_E_S_QLimitClause_E_Opt_C_rule0($1, $2);
// }
// 
//	   | OffsetClause _QLimitClause_E_Opt	{
//	   $$ = new _O_QLimitClause_E_S_QOffsetClause_E_Opt_Or_QOffsetClause_E_S_QLimitClause_E_Opt_C_rule1($1, $2);
// }
// ;
// 
// OrderClause:
//	   IT_ORDER IT_BY _QOrderCondition_E_Plus	{
//	   $$ = new OrderClause($1, $2, $3);
// }
// ;
// 
// _QOrderCondition_E_Plus:
//	   OrderCondition	{
//	   $$ = new _QOrderCondition_E_Plus_rule0($1);
// }
// 
//	   | _QOrderCondition_E_Plus OrderCondition {
//	   $$ = new _QOrderCondition_E_Plus_rule1($1, $2);
// }
// ;
// 
// OrderCondition:
//	   _O_QIT_ASC_E_Or_QIT_DESC_E_S_QBrackettedExpression_E_C	{
//	   $$ = new OrderCondition_rule0($1);
// }
// 
//	   | _O_QConstraint_E_Or_QVar_E_C	{
//	   $$ = new OrderCondition_rule1($1);
// }
// ;
// 
// _O_QIT_ASC_E_Or_QIT_DESC_E_C:
//	   IT_ASC	{
//	   $$ = new _O_QIT_ASC_E_Or_QIT_DESC_E_C_rule0($1);
// }
// 
//	   | IT_DESC	{
//	   $$ = new _O_QIT_ASC_E_Or_QIT_DESC_E_C_rule1($1);
// }
// ;
// 
// _O_QIT_ASC_E_Or_QIT_DESC_E_S_QBrackettedExpression_E_C:
//	   _O_QIT_ASC_E_Or_QIT_DESC_E_C BrackettedExpression	{
//	   $$ = new _O_QIT_ASC_E_Or_QIT_DESC_E_S_QBrackettedExpression_E_C($1, $2);
// }
// ;
// 
// _O_QConstraint_E_Or_QVar_E_C:
//	   Constraint	{
//	   $$ = new _O_QConstraint_E_Or_QVar_E_C_rule0($1);
// }
// 
//	   | Var	{
//	   $$ = new _O_QConstraint_E_Or_QVar_E_C_rule1($1);
// }
// ;
// 
// LimitClause:
//	   IT_LIMIT INTEGER {
//	   $$ = new LimitClause($1, $2);
// }
// ;
// 
// OffsetClause:
//	   IT_OFFSET INTEGER	{
//	   $$ = new OffsetClause($1, $2);
// }
// ;
// 
// GroupGraphPattern:
//	   GT_LCURLEY _QTriplesBlock_E_Opt _Q_O_QGraphPatternNotTriples_E_Or_QFilter_E_S_QGT_DOT_E_Opt_S_QTriplesBlock_E_Opt_C_E_Star GT_RCURLEY	{
//	   $$ = new GroupGraphPattern($1, $2, $3, $4);
// }
// ;
// 
// _O_QGraphPatternNotTriples_E_Or_QFilter_E_C:
//	   GraphPatternNotTriples	{
//	   $$ = new _O_QGraphPatternNotTriples_E_Or_QFilter_E_C_rule0($1);
// }
// 
//	   | Filter {
//	   $$ = new _O_QGraphPatternNotTriples_E_Or_QFilter_E_C_rule1($1);
// }
// ;
// 
// _QGT_DOT_E_Opt:
//	   {
//	   $$ = new _QGT_DOT_E_Opt_rule0();
// }
// 
//	   | GT_DOT {
//	   $$ = new _QGT_DOT_E_Opt_rule1($1);
// }
// ;
// 
// _O_QGraphPatternNotTriples_E_Or_QFilter_E_S_QGT_DOT_E_Opt_S_QTriplesBlock_E_Opt_C:
//	   _O_QGraphPatternNotTriples_E_Or_QFilter_E_C _QGT_DOT_E_Opt _QTriplesBlock_E_Opt	{
//	   $$ = new _O_QGraphPatternNotTriples_E_Or_QFilter_E_S_QGT_DOT_E_Opt_S_QTriplesBlock_E_Opt_C($1, $2, $3);
// }
// ;
// 
// _Q_O_QGraphPatternNotTriples_E_Or_QFilter_E_S_QGT_DOT_E_Opt_S_QTriplesBlock_E_Opt_C_E_Star:
//	   {
//	   $$ = new _Q_O_QGraphPatternNotTriples_E_Or_QFilter_E_S_QGT_DOT_E_Opt_S_QTriplesBlock_E_Opt_C_E_Star_rule0();
// }
// 
//	   | _Q_O_QGraphPatternNotTriples_E_Or_QFilter_E_S_QGT_DOT_E_Opt_S_QTriplesBlock_E_Opt_C_E_Star _O_QGraphPatternNotTriples_E_Or_QFilter_E_S_QGT_DOT_E_Opt_S_QTriplesBlock_E_Opt_C	{
//	   $$ = new _Q_O_QGraphPatternNotTriples_E_Or_QFilter_E_S_QGT_DOT_E_Opt_S_QTriplesBlock_E_Opt_C_E_Star_rule1($1, $2);
// }
// ;
//
// GraphPatternNotTriples:
//	   OptionalGraphPattern {
//	   $$ = new GraphPatternNotTriples_rule0($1);
// }
// 
//	   | GroupOrUnionGraphPattern	{
//	   $$ = new GraphPatternNotTriples_rule1($1);
// }
// 
//	   | GraphGraphPattern	{
//	   $$ = new GraphPatternNotTriples_rule2($1);
// }
// ;
// 
// OptionalGraphPattern:
//	   IT_OPTIONAL GroupGraphPattern	{
//	   $$ = new OptionalGraphPattern($1, $2);
// }
// ;
// 
// GraphGraphPattern:
//	   IT_GRAPH VarOrIRIref GroupGraphPattern	{
//	   $$ = new GraphGraphPattern($1, $2, $3);
// }
// ;
// 
// GroupOrUnionGraphPattern:
//	   GroupGraphPattern _Q_O_QIT_UNION_E_S_QGroupGraphPattern_E_C_E_Star	{
//	   $$ = new GroupOrUnionGraphPattern($1, $2);
// }
// ;
// 
// _O_QIT_UNION_E_S_QGroupGraphPattern_E_C:
//	   IT_UNION GroupGraphPattern	{
//	   $$ = new _O_QIT_UNION_E_S_QGroupGraphPattern_E_C($1, $2);
// }
// ;
// 
// _Q_O_QIT_UNION_E_S_QGroupGraphPattern_E_C_E_Star:
//	   {
//	   $$ = new _Q_O_QIT_UNION_E_S_QGroupGraphPattern_E_C_E_Star_rule0();
// }
// 
//	   | _Q_O_QIT_UNION_E_S_QGroupGraphPattern_E_C_E_Star _O_QIT_UNION_E_S_QGroupGraphPattern_E_C	{
//	   $$ = new _Q_O_QIT_UNION_E_S_QGroupGraphPattern_E_C_E_Star_rule1($1, $2);
// }
// ;
// 
// Filter:
//	   IT_FILTER Constraint {
//	   $$ = new Filter($1, $2);
// }
// ;
// 
// Constraint:
//	   BrackettedExpression {
//	   $$ = new Constraint_rule0($1);
// }
// 
//	   | BuiltInCall	{
//	   $$ = new Constraint_rule1($1);
// }
// 
//	   | FunctionCall	{
//	   $$ = new Constraint_rule2($1);
// }
// ;
// 
// FunctionCall:
//	   IRIref ArgList	{
//	   $$ = new FunctionCall($1, $2);
// }
// ;
// 
// ArgList:
//	   _O_QNIL_E_Or_QGT_LPAREN_E_S_QExpression_E_S_QGT_COMMA_E_S_QExpression_E_Star_S_QGT_RPAREN_E_C	{
//	   $$ = new ArgList($1);
// }
// ;
// 
// _O_QGT_COMMA_E_S_QExpression_E_C:
//	   GT_COMMA Expression	{
//	   $$ = new _O_QGT_COMMA_E_S_QExpression_E_C($1, $2);
// }
// ;
// 
// _Q_O_QGT_COMMA_E_S_QExpression_E_C_E_Star:
//	   {
//	   $$ = new _Q_O_QGT_COMMA_E_S_QExpression_E_C_E_Star_rule0();
// }
// 
//	   | _Q_O_QGT_COMMA_E_S_QExpression_E_C_E_Star _O_QGT_COMMA_E_S_QExpression_E_C {
//	   $$ = new _Q_O_QGT_COMMA_E_S_QExpression_E_C_E_Star_rule1($1, $2);
// }
// ;
// 
// _O_QNIL_E_Or_QGT_LPAREN_E_S_QExpression_E_S_QGT_COMMA_E_S_QExpression_E_Star_S_QGT_RPAREN_E_C:
//	   NIL	{
//	   $$ = new _O_QNIL_E_Or_QGT_LPAREN_E_S_QExpression_E_S_QGT_COMMA_E_S_QExpression_E_Star_S_QGT_RPAREN_E_C_rule0($1);
// }
// 
//	   | GT_LPAREN Expression _Q_O_QGT_COMMA_E_S_QExpression_E_C_E_Star GT_RPAREN	{
//	   $$ = new _O_QNIL_E_Or_QGT_LPAREN_E_S_QExpression_E_S_QGT_COMMA_E_S_QExpression_E_Star_S_QGT_RPAREN_E_C_rule1($1, $2, $3, $4);
// }
// ;
// 
// ConstructTemplate:
//	   GT_LCURLEY _QConstructTriples_E_Opt GT_RCURLEY	{
//	   $$ = new ConstructTemplate($1, $2, $3);
// }
// ;
// 
// _QConstructTriples_E_Opt:
//	   {
//	   $$ = new _QConstructTriples_E_Opt_rule0();
// }
// 
//	   | ConstructTriples	{
//	   $$ = new _QConstructTriples_E_Opt_rule1($1);
// }
// ;
// 
// ConstructTriples:
//	   TriplesSameSubject _Q_O_QGT_DOT_E_S_QConstructTriples_E_Opt_C_E_Opt	{
//	   $$ = new ConstructTriples($1, $2);
// }
// ;
// 
// _O_QGT_DOT_E_S_QConstructTriples_E_Opt_C:
//	   GT_DOT _QConstructTriples_E_Opt	{
//	   $$ = new _O_QGT_DOT_E_S_QConstructTriples_E_Opt_C($1, $2);
// }
// ;
// 
// _Q_O_QGT_DOT_E_S_QConstructTriples_E_Opt_C_E_Opt:
//	   {
//	   $$ = new _Q_O_QGT_DOT_E_S_QConstructTriples_E_Opt_C_E_Opt_rule0();
// }
// 
//	   | _O_QGT_DOT_E_S_QConstructTriples_E_Opt_C	{
//	   $$ = new _Q_O_QGT_DOT_E_S_QConstructTriples_E_Opt_C_E_Opt_rule1($1);
// }
// ;
//
// Expression:
//	   ConditionalOrExpression	{
//	   $$ = new Expression($1);
// }
// ;
// 
// ConditionalOrExpression:
//	   ConditionalAndExpression _Q_O_QGT_OR_E_S_QConditionalAndExpression_E_C_E_Star	{
//	   $$ = new ConditionalOrExpression($1, $2);
// }
// ;
// 
// _O_QGT_OR_E_S_QConditionalAndExpression_E_C:
//	   GT_OR ConditionalAndExpression	{
//	   $$ = new _O_QGT_OR_E_S_QConditionalAndExpression_E_C($1, $2);
// }
// ;
// 
// _Q_O_QGT_OR_E_S_QConditionalAndExpression_E_C_E_Star:
//	   {
//	   $$ = new _Q_O_QGT_OR_E_S_QConditionalAndExpression_E_C_E_Star_rule0();
// }
// 
//	   | _Q_O_QGT_OR_E_S_QConditionalAndExpression_E_C_E_Star _O_QGT_OR_E_S_QConditionalAndExpression_E_C	{
//	   $$ = new _Q_O_QGT_OR_E_S_QConditionalAndExpression_E_C_E_Star_rule1($1, $2);
// }
// ;
// 
// ConditionalAndExpression:
//	   ValueLogical _Q_O_QGT_AND_E_S_QValueLogical_E_C_E_Star	{
//	   $$ = new ConditionalAndExpression($1, $2);
// }
// ;
// 
// _O_QGT_AND_E_S_QValueLogical_E_C:
//	   GT_AND ValueLogical	{
//	   $$ = new _O_QGT_AND_E_S_QValueLogical_E_C($1, $2);
// }
// ;
// 
// _Q_O_QGT_AND_E_S_QValueLogical_E_C_E_Star:
//	   {
//	   $$ = new _Q_O_QGT_AND_E_S_QValueLogical_E_C_E_Star_rule0();
// }
// 
//	   | _Q_O_QGT_AND_E_S_QValueLogical_E_C_E_Star _O_QGT_AND_E_S_QValueLogical_E_C {
//	   $$ = new _Q_O_QGT_AND_E_S_QValueLogical_E_C_E_Star_rule1($1, $2);
// }
// ;
// 
// ValueLogical:
//	   RelationalExpression {
//	   $$ = new ValueLogical($1);
// }
// ;
// 
// RelationalExpression:
//	   NumericExpression _Q_O_QGT_EQUAL_E_S_QNumericExpression_E_Or_QGT_NEQUAL_E_S_QNumericExpression_E_Or_QGT_LT_E_S_QNumericExpression_E_Or_QGT_GT_E_S_QNumericExpression_E_Or_QGT_LE_E_S_QNumericExpression_E_Or_QGT_GE_E_S_QNumericExpression_E_C_E_Opt {
//	   $$ = new RelationalExpression($1, $2);
// }
// ;
// 
// _O_QGT_EQUAL_E_S_QNumericExpression_E_Or_QGT_NEQUAL_E_S_QNumericExpression_E_Or_QGT_LT_E_S_QNumericExpression_E_Or_QGT_GT_E_S_QNumericExpression_E_Or_QGT_LE_E_S_QNumericExpression_E_Or_QGT_GE_E_S_QNumericExpression_E_C:
//	   GT_EQUAL NumericExpression	{
//	   $$ = new _O_QGT_EQUAL_E_S_QNumericExpression_E_Or_QGT_NEQUAL_E_S_QNumericExpression_E_Or_QGT_LT_E_S_QNumericExpression_E_Or_QGT_GT_E_S_QNumericExpression_E_Or_QGT_LE_E_S_QNumericExpression_E_Or_QGT_GE_E_S_QNumericExpression_E_C_rule0($1, $2);
// }
// 
//	   | GT_NEQUAL NumericExpression	{
//	   $$ = new _O_QGT_EQUAL_E_S_QNumericExpression_E_Or_QGT_NEQUAL_E_S_QNumericExpression_E_Or_QGT_LT_E_S_QNumericExpression_E_Or_QGT_GT_E_S_QNumericExpression_E_Or_QGT_LE_E_S_QNumericExpression_E_Or_QGT_GE_E_S_QNumericExpression_E_C_rule1($1, $2);
// }
// 
//	   | GT_LT NumericExpression	{
//	   $$ = new _O_QGT_EQUAL_E_S_QNumericExpression_E_Or_QGT_NEQUAL_E_S_QNumericExpression_E_Or_QGT_LT_E_S_QNumericExpression_E_Or_QGT_GT_E_S_QNumericExpression_E_Or_QGT_LE_E_S_QNumericExpression_E_Or_QGT_GE_E_S_QNumericExpression_E_C_rule2($1, $2);
// }
// 
//	   | GT_GT NumericExpression	{
//	   $$ = new _O_QGT_EQUAL_E_S_QNumericExpression_E_Or_QGT_NEQUAL_E_S_QNumericExpression_E_Or_QGT_LT_E_S_QNumericExpression_E_Or_QGT_GT_E_S_QNumericExpression_E_Or_QGT_LE_E_S_QNumericExpression_E_Or_QGT_GE_E_S_QNumericExpression_E_C_rule3($1, $2);
// }
// 
//	   | GT_LE NumericExpression	{
//	   $$ = new _O_QGT_EQUAL_E_S_QNumericExpression_E_Or_QGT_NEQUAL_E_S_QNumericExpression_E_Or_QGT_LT_E_S_QNumericExpression_E_Or_QGT_GT_E_S_QNumericExpression_E_Or_QGT_LE_E_S_QNumericExpression_E_Or_QGT_GE_E_S_QNumericExpression_E_C_rule4($1, $2);
// }
// 
//	   | GT_GE NumericExpression	{
//	   $$ = new _O_QGT_EQUAL_E_S_QNumericExpression_E_Or_QGT_NEQUAL_E_S_QNumericExpression_E_Or_QGT_LT_E_S_QNumericExpression_E_Or_QGT_GT_E_S_QNumericExpression_E_Or_QGT_LE_E_S_QNumericExpression_E_Or_QGT_GE_E_S_QNumericExpression_E_C_rule5($1, $2);
// }
// ;
// 
// _Q_O_QGT_EQUAL_E_S_QNumericExpression_E_Or_QGT_NEQUAL_E_S_QNumericExpression_E_Or_QGT_LT_E_S_QNumericExpression_E_Or_QGT_GT_E_S_QNumericExpression_E_Or_QGT_LE_E_S_QNumericExpression_E_Or_QGT_GE_E_S_QNumericExpression_E_C_E_Opt:
//	   {
//	   $$ = new _Q_O_QGT_EQUAL_E_S_QNumericExpression_E_Or_QGT_NEQUAL_E_S_QNumericExpression_E_Or_QGT_LT_E_S_QNumericExpression_E_Or_QGT_GT_E_S_QNumericExpression_E_Or_QGT_LE_E_S_QNumericExpression_E_Or_QGT_GE_E_S_QNumericExpression_E_C_E_Opt_rule0();
// }
// 
//	   | _O_QGT_EQUAL_E_S_QNumericExpression_E_Or_QGT_NEQUAL_E_S_QNumericExpression_E_Or_QGT_LT_E_S_QNumericExpression_E_Or_QGT_GT_E_S_QNumericExpression_E_Or_QGT_LE_E_S_QNumericExpression_E_Or_QGT_GE_E_S_QNumericExpression_E_C {
//	   $$ = new _Q_O_QGT_EQUAL_E_S_QNumericExpression_E_Or_QGT_NEQUAL_E_S_QNumericExpression_E_Or_QGT_LT_E_S_QNumericExpression_E_Or_QGT_GT_E_S_QNumericExpression_E_Or_QGT_LE_E_S_QNumericExpression_E_Or_QGT_GE_E_S_QNumericExpression_E_C_E_Opt_rule1($1);
// }
// ;
// 
// NumericExpression:
//	   AdditiveExpression	{
//	   $$ = new NumericExpression($1);
// }
// ;
// 
// AdditiveExpression:
//	   MultiplicativeExpression _Q_O_QGT_PLUS_E_S_QMultiplicativeExpression_E_Or_QGT_MINUS_E_S_QMultiplicativeExpression_E_Or_QNumericLiteralPositive_E_Or_QNumericLiteralNegative_E_C_E_Star	{
//	   $$ = new AdditiveExpression($1, $2);
// }
// ;
// 
// _O_QGT_PLUS_E_S_QMultiplicativeExpression_E_Or_QGT_MINUS_E_S_QMultiplicativeExpression_E_Or_QNumericLiteralPositive_E_Or_QNumericLiteralNegative_E_C:
//	   GT_PLUS MultiplicativeExpression {
//	   $$ = new _O_QGT_PLUS_E_S_QMultiplicativeExpression_E_Or_QGT_MINUS_E_S_QMultiplicativeExpression_E_Or_QNumericLiteralPositive_E_Or_QNumericLiteralNegative_E_C_rule0($1, $2);
// }
// 
//	   | GT_MINUS MultiplicativeExpression	{
//	   $$ = new _O_QGT_PLUS_E_S_QMultiplicativeExpression_E_Or_QGT_MINUS_E_S_QMultiplicativeExpression_E_Or_QNumericLiteralPositive_E_Or_QNumericLiteralNegative_E_C_rule1($1, $2);
// }
// 
//	   | NumericLiteralPositive {
//	   $$ = new _O_QGT_PLUS_E_S_QMultiplicativeExpression_E_Or_QGT_MINUS_E_S_QMultiplicativeExpression_E_Or_QNumericLiteralPositive_E_Or_QNumericLiteralNegative_E_C_rule2($1);
// }
// 
//	   | NumericLiteralNegative {
//	   $$ = new _O_QGT_PLUS_E_S_QMultiplicativeExpression_E_Or_QGT_MINUS_E_S_QMultiplicativeExpression_E_Or_QNumericLiteralPositive_E_Or_QNumericLiteralNegative_E_C_rule3($1);
// }
// ;
// 
// _Q_O_QGT_PLUS_E_S_QMultiplicativeExpression_E_Or_QGT_MINUS_E_S_QMultiplicativeExpression_E_Or_QNumericLiteralPositive_E_Or_QNumericLiteralNegative_E_C_E_Star:
//	   {
//	   $$ = new _Q_O_QGT_PLUS_E_S_QMultiplicativeExpression_E_Or_QGT_MINUS_E_S_QMultiplicativeExpression_E_Or_QNumericLiteralPositive_E_Or_QNumericLiteralNegative_E_C_E_Star_rule0();
// }
// 
//	   | _Q_O_QGT_PLUS_E_S_QMultiplicativeExpression_E_Or_QGT_MINUS_E_S_QMultiplicativeExpression_E_Or_QNumericLiteralPositive_E_Or_QNumericLiteralNegative_E_C_E_Star _O_QGT_PLUS_E_S_QMultiplicativeExpression_E_Or_QGT_MINUS_E_S_QMultiplicativeExpression_E_Or_QNumericLiteralPositive_E_Or_QNumericLiteralNegative_E_C {
//	   $$ = new _Q_O_QGT_PLUS_E_S_QMultiplicativeExpression_E_Or_QGT_MINUS_E_S_QMultiplicativeExpression_E_Or_QNumericLiteralPositive_E_Or_QNumericLiteralNegative_E_C_E_Star_rule1($1, $2);
// }
// ;
// 
// MultiplicativeExpression:
//	   UnaryExpression _Q_O_QGT_TIMES_E_S_QUnaryExpression_E_Or_QGT_DIVIDE_E_S_QUnaryExpression_E_C_E_Star	{
//	   $$ = new MultiplicativeExpression($1, $2);
// }
// ;
// 
// _O_QGT_TIMES_E_S_QUnaryExpression_E_Or_QGT_DIVIDE_E_S_QUnaryExpression_E_C:
//	   GT_TIMES UnaryExpression {
//	   $$ = new _O_QGT_TIMES_E_S_QUnaryExpression_E_Or_QGT_DIVIDE_E_S_QUnaryExpression_E_C_rule0($1, $2);
// }
// 
//	   | GT_DIVIDE UnaryExpression	{
//	   $$ = new _O_QGT_TIMES_E_S_QUnaryExpression_E_Or_QGT_DIVIDE_E_S_QUnaryExpression_E_C_rule1($1, $2);
// }
// ;
// 
// _Q_O_QGT_TIMES_E_S_QUnaryExpression_E_Or_QGT_DIVIDE_E_S_QUnaryExpression_E_C_E_Star:
//	   {
//	   $$ = new _Q_O_QGT_TIMES_E_S_QUnaryExpression_E_Or_QGT_DIVIDE_E_S_QUnaryExpression_E_C_E_Star_rule0();
// }
// 
//	   | _Q_O_QGT_TIMES_E_S_QUnaryExpression_E_Or_QGT_DIVIDE_E_S_QUnaryExpression_E_C_E_Star _O_QGT_TIMES_E_S_QUnaryExpression_E_Or_QGT_DIVIDE_E_S_QUnaryExpression_E_C {
//	   $$ = new _Q_O_QGT_TIMES_E_S_QUnaryExpression_E_Or_QGT_DIVIDE_E_S_QUnaryExpression_E_C_E_Star_rule1($1, $2);
// }
// ;
// 
// UnaryExpression:
//	   GT_NOT PrimaryExpression {
//	   $$ = new UnaryExpression_rule0($1, $2);
// }
// 
//	   | GT_PLUS PrimaryExpression	{
//	   $$ = new UnaryExpression_rule1($1, $2);
// }
// 
//	   | GT_MINUS PrimaryExpression {
//	   $$ = new UnaryExpression_rule2($1, $2);
// }
// 
//	   | PrimaryExpression	{
//	   $$ = new UnaryExpression_rule3($1);
// }
// ;
// 
// PrimaryExpression:
//	   BrackettedExpression {
//	   $$ = new PrimaryExpression_rule0($1);
// }
// 
//	   | BuiltInCall	{
//	   $$ = new PrimaryExpression_rule1($1);
// }
// 
//	   | IRIrefOrFunction	{
//	   $$ = new PrimaryExpression_rule2($1);
// }
// 
//	   | RDFLiteral {
//	   $$ = new PrimaryExpression_rule3($1);
// }
// 
//	   | NumericLiteral {
//	   $$ = new PrimaryExpression_rule4($1);
// }
// 
//	   | BooleanLiteral {
//	   $$ = new PrimaryExpression_rule5($1);
// }
// 
//	   | Var	{
//	   $$ = new PrimaryExpression_rule6($1);
// }
// ;
// 
// BrackettedExpression:
//	   GT_LPAREN Expression GT_RPAREN	{
//	   $$ = new BrackettedExpression($1, $2, $3);
// }
// ;
// 
// BuiltInCall:
//	   IT_STR GT_LPAREN Expression GT_RPAREN	{
//	   $$ = new BuiltInCall_rule0($1, $2, $3, $4);
// }
// 
//	   | IT_LANG GT_LPAREN Expression GT_RPAREN {
//	   $$ = new BuiltInCall_rule1($1, $2, $3, $4);
// }
// 
//	   | IT_LANGMATCHES GT_LPAREN Expression GT_COMMA Expression GT_RPAREN	{
//	   $$ = new BuiltInCall_rule2($1, $2, $3, $4, $5, $6);
// }
// 
//	   | IT_DATATYPE GT_LPAREN Expression GT_RPAREN {
//	   $$ = new BuiltInCall_rule3($1, $2, $3, $4);
// }
// 
//	   | IT_BOUND GT_LPAREN Var GT_RPAREN	{
//	   $$ = new BuiltInCall_rule4($1, $2, $3, $4);
// }
// 
//	   | IT_sameTerm GT_LPAREN Expression GT_COMMA Expression GT_RPAREN {
//	   $$ = new BuiltInCall_rule5($1, $2, $3, $4, $5, $6);
// }
// 
//	   | IT_isIRI GT_LPAREN Expression GT_RPAREN	{
//	   $$ = new BuiltInCall_rule6($1, $2, $3, $4);
// }
// 
//	   | IT_isURI GT_LPAREN Expression GT_RPAREN	{
//	   $$ = new BuiltInCall_rule7($1, $2, $3, $4);
// }
// 
//	   | IT_isBLANK GT_LPAREN Expression GT_RPAREN	{
//	   $$ = new BuiltInCall_rule8($1, $2, $3, $4);
// }
// 
//	   | IT_isLITERAL GT_LPAREN Expression GT_RPAREN	{
//	   $$ = new BuiltInCall_rule9($1, $2, $3, $4);
// }
// 
//	   | RegexExpression	{
//	   $$ = new BuiltInCall_rule10($1);
// }
// ;
// 
// RegexExpression:
//	   IT_REGEX GT_LPAREN Expression GT_COMMA Expression _Q_O_QGT_COMMA_E_S_QExpression_E_C_E_Opt GT_RPAREN {
//	   $$ = new RegexExpression($1, $2, $3, $4, $5, $6, $7);
// }
// ;
// 
// _Q_O_QGT_COMMA_E_S_QExpression_E_C_E_Opt:
//	   {
//	   $$ = new _Q_O_QGT_COMMA_E_S_QExpression_E_C_E_Opt_rule0();
// }
// 
//	   | _O_QGT_COMMA_E_S_QExpression_E_C	{
//	   $$ = new _Q_O_QGT_COMMA_E_S_QExpression_E_C_E_Opt_rule1($1);
// }
// ;
// 
// IRIrefOrFunction:
//	   IRIref _QArgList_E_Opt	{
//	   $$ = new IRIrefOrFunction($1, $2);
// }
// ;
// 
// _QArgList_E_Opt:
//	   {
//	   $$ = new _QArgList_E_Opt_rule0();
// }
// 
//	   | ArgList	{
//	   $$ = new _QArgList_E_Opt_rule1($1);
// }
// ;



%%

 /*** END SPARQL - Change the grammar rules above ***/

/* START main */

#include <stdio.h>
#include <stdlib.h>
#include "triple.h"
#include "node.h"
#include "bgp.h"

void* parsedPattern	= NULL;

void free_prologue ( prologue_t* p );
char* prefix_uri ( prologue_t* p, char* ns );
char* qualify_qname ( prologue_t* p, char* qname );
hx_node* generate_node ( node_t* n, prologue_t* p, int* counter );
hx_bgp* parse_bgp_query ( void );
hx_bgp* parse_bgp_query_string ( char* string );

void yyerror (char const *s) {
	fprintf (stderr, "*** %s\n", s);
}

hx_bgp* parse_bgp_query_string ( char* string ) {
	yy_scan_string( string );
	return parse_bgp_query();
}

hx_bgp* parse_bgp_query ( void ) {
	if (yyparse() == 0) {
		int i;
		hx_bgp* b;
		triple_set_t* bgp;
		hx_triple** triples;
		int counter	= -1;
		query_t* q		= (query_t*) parsedPattern;
		prologue_t* prologue	= q->prologue;
		bgp	= q->bgp;
		triples	= (hx_triple**) calloc( bgp->triple_count, sizeof( hx_triple* ) );
		for (i = 0; i < bgp->triple_count; i++) {
			triple_t* t		= bgp->triples[i];
			hx_node* s	= generate_node( t->subject, prologue, &counter );
			hx_node* p	= generate_node( t->predicate, prologue, &counter );
			hx_node* o	= generate_node( t->object, prologue, &counter );
			hx_triple* triple;
			if (s == NULL || p == NULL || o == NULL) {
				fprintf( stderr, "Got NULL node in triple: (%p, %p, %p)\n", (void*) s, (void*) p, (void*) o );
				return NULL;
			}
			triple	= hx_new_triple( s, p, o );
			triples[i]	= triple;
		}
		free_prologue( prologue );
		
		b	= hx_new_bgp( bgp->triple_count, triples );
		return b;
	}
	return NULL;
}

/* END main */

void XXXdebug_triple( triple_t* t, prologue_t* p ) {
	fprintf( stderr, "- subject: " ); XXXdebug_node( t->subject, p );
	fprintf( stderr, "- predicate: " ); XXXdebug_node( t->predicate, p );
	fprintf( stderr, "- object: " ); XXXdebug_node( t->object, p );
}

hx_node* generate_node ( node_t* n, prologue_t* p, int* counter ) {
	if (n->type == 'N') {
		hx_node* node	= (hx_node*) n->ptr;
		return hx_node_copy( node );
	} else if (n->type == 'V') {
		char* name	= (char*) n->ptr;
		char* copy	= (char*) malloc( strlen( name ) + 1 );
		hx_node* v;
		strcpy( copy, name );
		v	= hx_new_node_named_variable( (*counter)--, copy );
		return v;
	} else if (n->type == 'Q') {
		char* uri	= qualify_qname( p, (char*) n->ptr );
		if (uri == NULL)
			return NULL;
		hx_node* u	= hx_new_node_resource( uri );
		return u;
	} else if (n->type == 'L') {
		char* dt	= qualify_qname( p, n->datatype );
		if (dt == NULL)
			return NULL;
		char* value	= (char*) n->ptr;
		hx_node* l	= (hx_node*) hx_new_node_dt_literal( value, dt );
		return l;
	} else {
		fprintf( stderr, "*** UNRECOGNIZED node type '%c'\n", n->type );
		return NULL;
	}
}

void XXXdebug_node( node_t* n, prologue_t* p ) {
	int counter	= -1;
	hx_node* node	= generate_node( n, p, &counter );
	if (node != NULL) {
		char* string;
		hx_node_string( node, &string );
		fprintf( stderr, "Node: %s\n", string );
		free( string );
		hx_free_node( node );
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

triple_set_t* new_triple_set ( int size ) {
	triple_set_t* triples	= (triple_set_t*) calloc( 1, sizeof( triple_set_t ) );
	triples->allocated		= size;
	triples->triple_count	= 0;
	triples->triples		= (triple_t**) calloc( triples->allocated, sizeof( triple_t* ) );
	return triples;
}

void add_triple_to_set( triple_set_t* set, triple_t* t ) {
	if (set->allocated <= (set->triple_count + 1)) {
		int i;
		triple_t** old;
		triple_t** newlist;
		set->allocated	*= 2;
		newlist	= (triple_t**) calloc( set->allocated, sizeof( triple_t* ) );
		for (i = 0; i < set->triple_count; i++) {
			newlist[i]	= set->triples[i];
		}
		old	= set->triples;
		set->triples	= newlist;
		free( old );
	}
	
	set->triples[ set->triple_count++ ]	= t;
}

extern node_t* new_dt_literal_node ( char* string, char* dt ) {
	hx_node* n		= (hx_node*) hx_new_node_dt_literal( string, dt );
	node_t* node	= (node_t*) calloc( 1, sizeof( node_t ) );
	node->type		= 'N';
	node->ptr		= (void*) n;
	return node;
}

















/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

