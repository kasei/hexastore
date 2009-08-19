/* $Id: Langname_Scanner.ll,v 1.1 2008/04/06 17:10:46 eric Exp SPARQLScanner.ll 28 2007-08-20 10:27:39Z tb $ -*- mode: c++ -*- */
/** \file SPARQLScanner.ll Define the Flex lexical scanner */

%{ /*** C/C++ Declarations ***/

#define YYSTYPE void*
#define YYSTYPE_IS_DECLARED

#include "SPARQLParser.h"
#include "SPARQLScanner.h"

/* This disables inclusion of unistd.h, which is not available under Visual C++
 * on Win32. The C++ scanner uses STL streams instead. */
#define YY_NO_UNISTD_H

#define YY_SKIP_YYWRAP
# undef yywrap
#define yywrap() 1

char* string_copy( char* text );
void unescape( char* text );

%}

/*** Flex Declarations and Options ***/

/* enable scanner to generate debug output. disable this for release
 * versions. */
/* %option debug */

/* The following paragraph suffices to track locations accurately. Each time
 * yylex is invoked, the begin position is moved onto the end position. */
/* %{ */
/* #define YY_USER_ACTION  yylloc->columns(yyleng); */
/* %} */

/* START patterns for SPARQL terminals */
pat_IT_BASE		"BASE"
pat_IT_PREFIX		"PREFIX"
pat_IT_SELECT		"SELECT"
pat_IT_DISTINCT		"DISTINCT"
pat_IT_REDUCED		"REDUCED"
pat_GT_TIMES		"*"
pat_IT_CONSTRUCT		"CONSTRUCT"
pat_IT_DESCRIBE		"DESCRIBE"
pat_IT_ASK		"ASK"
pat_IT_FROM		"FROM"
pat_IT_NAMED		"NAMED"
pat_IT_WHERE		"WHERE"
pat_IT_ORDER		"ORDER"
pat_IT_BY		"BY"
pat_IT_ASC		"ASC"
pat_IT_DESC		"DESC"
pat_IT_LIMIT		"LIMIT"
pat_IT_OFFSET		"OFFSET"
pat_GT_LCURLEY		"{"
pat_GT_RCURLEY		"}"
pat_GT_DOT		"."
pat_IT_OPTIONAL		"OPTIONAL"
pat_IT_GRAPH		"GRAPH"
pat_IT_UNION		"UNION"
pat_IT_FILTER		"FILTER"
pat_GT_COMMA		","
pat_GT_LPAREN		"("
pat_GT_RPAREN		")"
pat_GT_SEMI		";"
pat_IT_a		"a"
pat_GT_LBRACKET		"\["
pat_GT_RBRACKET		"\]"
pat_GT_OR		"||"
pat_GT_AND		"&&"
pat_GT_EQUAL		"="
pat_GT_NEQUAL		"!="
pat_GT_LT		"<"
pat_GT_GT		">"
pat_GT_LE		"<="
pat_GT_GE		">="
pat_GT_PLUS		"+"
pat_GT_MINUS		"-"
pat_GT_DIVIDE		"/"
pat_GT_NOT		"!"
pat_IT_STR		"STR"|"str"
pat_IT_LANG		"LANG"|"lang"
pat_IT_LANGMATCHES		"LANGMATCHES"|"langmatches"
pat_IT_DATATYPE		"DATATYPE"|"datatype"
pat_IT_BOUND		"BOUND"|"bound"
pat_IT_sameTerm		"sameTerm"|"SAMETERM"|"sameterm"
pat_IT_isIRI		"isIRI"|"ISIRI"|"isiri"
pat_IT_isURI		"isURI"|"ISURI"|"isuri"
pat_IT_isBLANK		"isBLANK"|"ISBLANK"|"isblank"
pat_IT_isLITERAL		"isLITERAL"
pat_IT_REGEX		"REGEX"
pat_GT_DTYPE		"^^"
pat_IT_true		"true"
pat_IT_false		"false"
pat_IRI_REF		"<"(([#-;=?-\[\]_a-z~-\x7F]|([\xC2-\xDF][\x80-\xBF])|(\xE0([\xA0-\xBF][\x80-\xBF]))|([\xE1-\xEC][\x80-\xBF][\x80-\xBF])|([\xE1-\xEC][\x80-\xBF][\x80-\xBF])|(\xED([\x80-\x9F][\x80-\xBF]))|([\xEE-\xEF][\x80-\xBF][\x80-\xBF])|(\xF0([\x90-\xBF][\x80-\xBF][\x80-\xBF]))|([\xF1-\xF3][\x80-\xBF][\x80-\xBF][\x80-\xBF])|(\xF4([\x80-\x8E][\x80-\xBF][\x80-\xBF])|(\x8F([\x80-\xBE][\x80-\xBF])|(\xBF[\x80-\xBD])))]))*">"
pat_LANGTAG		"@"([A-Za-z])+(("-"([0-9A-Za-z])+))*
pat_INTEGER		([0-9])+
pat_DECIMAL		(([0-9])+"."([0-9])*)|("."([0-9])+)
pat_INTEGER_POSITIVE		"+"({pat_INTEGER})
pat_DECIMAL_POSITIVE		"+"({pat_DECIMAL})
pat_INTEGER_NEGATIVE		"-"({pat_INTEGER})
pat_DECIMAL_NEGATIVE		"-"({pat_DECIMAL})
pat_EXPONENT		[Ee]([+-])?([0-9])+
pat_DOUBLE		(([0-9])+"."([0-9])*({pat_EXPONENT}))|(("."(([0-9]))+({pat_EXPONENT}))|((([0-9]))+({pat_EXPONENT})))
pat_DOUBLE_NEGATIVE		"-"({pat_DOUBLE})
pat_DOUBLE_POSITIVE		"+"({pat_DOUBLE})
pat_ECHAR		"\\"[\"'\\bfnrt]
pat_STRING_LITERAL_LONG2		"\"\"\""((((("\"")|("\"\"")))?(([\x00-!#-\[\]-\x7F]|([\xC2-\xDF][\x80-\xBF])|(\xE0([\xA0-\xBF][\x80-\xBF]))|([\xE1-\xEC][\x80-\xBF][\x80-\xBF])|([\xE1-\xEC][\x80-\xBF][\x80-\xBF])|(\xED([\x80-\x9F][\x80-\xBF]))|([\xEE-\xEF][\x80-\xBF][\x80-\xBF])|(\xF0([\x90-\xBF][\x80-\xBF][\x80-\xBF]))|([\xF1-\xF3][\x80-\xBF][\x80-\xBF][\x80-\xBF])|(\xF4([\x80-\x8E][\x80-\xBF][\x80-\xBF])|(\x8F([\x80-\xBE][\x80-\xBF])|(\xBF[\x80-\xBD])))])|(({pat_ECHAR})))))*"\"\"\""
pat_STRING_LITERAL_LONG1		"'''"((((("'")|("''")))?(([\x00-&(-\[\]-\x7F]|([\xC2-\xDF][\x80-\xBF])|(\xE0([\xA0-\xBF][\x80-\xBF]))|([\xE1-\xEC][\x80-\xBF][\x80-\xBF])|([\xE1-\xEC][\x80-\xBF][\x80-\xBF])|(\xED([\x80-\x9F][\x80-\xBF]))|([\xEE-\xEF][\x80-\xBF][\x80-\xBF])|(\xF0([\x90-\xBF][\x80-\xBF][\x80-\xBF]))|([\xF1-\xF3][\x80-\xBF][\x80-\xBF][\x80-\xBF])|(\xF4([\x80-\x8E][\x80-\xBF][\x80-\xBF])|(\x8F([\x80-\xBE][\x80-\xBF])|(\xBF[\x80-\xBD])))])|(({pat_ECHAR})))))*"'''"
pat_STRING_LITERAL2		"\""(((([\x00-\t\x0B-\x0C\x0E-!#-\[\]-\x7F]|([\xC2-\xDF][\x80-\xBF])|(\xE0([\xA0-\xBF][\x80-\xBF]))|([\xE1-\xEC][\x80-\xBF][\x80-\xBF])|([\xE1-\xEC][\x80-\xBF][\x80-\xBF])|(\xED([\x80-\x9F][\x80-\xBF]))|([\xEE-\xEF][\x80-\xBF][\x80-\xBF])|(\xF0([\x90-\xBF][\x80-\xBF][\x80-\xBF]))|([\xF1-\xF3][\x80-\xBF][\x80-\xBF][\x80-\xBF])|(\xF4([\x80-\x8E][\x80-\xBF][\x80-\xBF])|(\x8F([\x80-\xBE][\x80-\xBF])|(\xBF[\x80-\xBD])))]))|(({pat_ECHAR}))))*"\""
pat_STRING_LITERAL1		"'"(((([\x00-\t\x0B-\x0C\x0E-&(-\[\]-\x7F]|([\xC2-\xDF][\x80-\xBF])|(\xE0([\xA0-\xBF][\x80-\xBF]))|([\xE1-\xEC][\x80-\xBF][\x80-\xBF])|([\xE1-\xEC][\x80-\xBF][\x80-\xBF])|(\xED([\x80-\x9F][\x80-\xBF]))|([\xEE-\xEF][\x80-\xBF][\x80-\xBF])|(\xF0([\x90-\xBF][\x80-\xBF][\x80-\xBF]))|([\xF1-\xF3][\x80-\xBF][\x80-\xBF][\x80-\xBF])|(\xF4([\x80-\x8E][\x80-\xBF][\x80-\xBF])|(\x8F([\x80-\xBE][\x80-\xBF])|(\xBF[\x80-\xBD])))]))|(({pat_ECHAR}))))*"'"
pat_WS		(" ")|(("\t")|(("\r")|("\n")))
pat_NIL		"("(({pat_WS}))*")"
pat_ANON		"\["(({pat_WS}))*"\]"
pat_PN_CHARS_BASE		([A-Z])|(([a-z])|(((\xC3[\x80-\x96]))|(((\xC3[\x98-\xB6]))|(((\xC3[\xB8-\xBF])|([\xC4-\xCB][\x80-\xBF]))|(((\xCD[\xB0-\xBD]))|(((\xCD\xBF)|([\xCE-\xDF][\x80-\xBF])|(\xE0([\xA0-\xBF][\x80-\xBF]))|(\xE1([\x80-\xBF][\x80-\xBF])))|(((\xE2(\x80[\x8C-\x8D])))|(((\xE2(\x81[\xB0-\xBF])|([\x82-\x85][\x80-\xBF])|(\x86[\x80-\x8F])))|(((\xE2([\xB0-\xBE][\x80-\xBF])|(\xBF[\x80-\xAF])))|(((\xE3(\x80[\x81-\xBF])|([\x81-\xBF][\x80-\xBF]))|([\xE4-\xEC][\x80-\xBF][\x80-\xBF])|([\xE1-\xEC][\x80-\xBF][\x80-\xBF])|(\xED([\x80-\x9F][\x80-\xBF])))|(((\xEF([\xA4-\xB6][\x80-\xBF])|(\xB7[\x80-\x8F])))|(((\xEF(\xB7[\xB0-\xBF])|([\xB8-\xBE][\x80-\xBF])|(\xBF[\x80-\xBD])))|((\xF0([\x90-\xBF][\x80-\xBF][\x80-\xBF]))|([\xF1-\xF3][\x80-\xBF][\x80-\xBF][\x80-\xBF]))))))))))))))
pat_PN_CHARS_U		(({pat_PN_CHARS_BASE}))|("_")
pat_VARNAME		((({pat_PN_CHARS_U}))|([0-9]))(((({pat_PN_CHARS_U}))|(([0-9])|((\xC2\xB7)|(((\xCD[\x80-\xAF]))|((\xE2(\x80\xBF)|(\x81\x80))))))))*
pat_VAR2		"$"({pat_VARNAME})
pat_VAR1		"?"({pat_VARNAME})
pat_PN_CHARS		(({pat_PN_CHARS_U}))|(("-")|(([0-9])|((\xC2\xB7)|(((\xCD[\x80-\xAF]))|((\xE2(\x80\xBF)|(\x81\x80)))))))
pat_PN_PREFIX		({pat_PN_CHARS_BASE})(((((({pat_PN_CHARS}))|(".")))*({pat_PN_CHARS})))?
pat_PNAME_NS		(({pat_PN_PREFIX}))?":"
pat_PN_LOCAL		((({pat_PN_CHARS_U}))|([0-9]))(((((({pat_PN_CHARS}))|(".")))*({pat_PN_CHARS})))?
pat_BLANK_NODE_LABEL		"_:"({pat_PN_LOCAL})
pat_PNAME_LN		({pat_PNAME_NS})({pat_PN_LOCAL})
pat_PASSED_TOKENS		(([\t\n\r ])+)|("#"([\x00-\t\x0B-\x0C\x0E-\x7F]|([\xC2-\xDF][\x80-\xBF])|(\xE0([\xA0-\xBF][\x80-\xBF]))|([\xE1-\xEC][\x80-\xBF][\x80-\xBF])|([\xE1-\xEC][\x80-\xBF][\x80-\xBF])|(\xED([\x80-\x9F][\x80-\xBF]))|([\xEE-\xEF][\x80-\xBF][\x80-\xBF])|(\xF0([\x90-\xBF][\x80-\xBF][\x80-\xBF]))|([\xF1-\xF3][\x80-\xBF][\x80-\xBF][\x80-\xBF])|(\xF4([\x80-\x8E][\x80-\xBF][\x80-\xBF])|(\x8F([\x80-\xBE][\x80-\xBF])|(\xBF[\x80-\xBD])))])*)

/* END patterns for SPARQL terminals */

/* START semantic actions for SPARQL terminals */
%%
{pat_PASSED_TOKENS}		{ /* yylloc->step(); @@ needed? useful? */ }
{pat_IT_BASE}		{return IT_BASE;}
{pat_IT_PREFIX}		{return IT_PREFIX;}
{pat_IT_SELECT}		{return IT_SELECT;}
{pat_IT_DISTINCT}		{return IT_DISTINCT;}
{pat_IT_REDUCED}		{return IT_REDUCED;}
{pat_GT_TIMES}		{return GT_TIMES;}
{pat_IT_CONSTRUCT}		{return IT_CONSTRUCT;}
{pat_IT_DESCRIBE}		{return IT_DESCRIBE;}
{pat_IT_ASK}		{return IT_ASK;}
{pat_IT_FROM}		{return IT_FROM;}
{pat_IT_NAMED}		{return IT_NAMED;}
{pat_IT_WHERE}		{return IT_WHERE;}
{pat_IT_ORDER}		{return IT_ORDER;}
{pat_IT_BY}		{return IT_BY;}
{pat_IT_ASC}		{return IT_ASC;}
{pat_IT_DESC}		{return IT_DESC;}
{pat_IT_LIMIT}		{return IT_LIMIT;}
{pat_IT_OFFSET}		{return IT_OFFSET;}
{pat_GT_LCURLEY}		{return GT_LCURLEY;}
{pat_GT_RCURLEY}		{return GT_RCURLEY;}
{pat_GT_DOT}		{return GT_DOT;}
{pat_IT_OPTIONAL}		{return IT_OPTIONAL;}
{pat_IT_GRAPH}		{return IT_GRAPH;}
{pat_IT_UNION}		{return IT_UNION;}
{pat_IT_FILTER}		{return IT_FILTER;}
{pat_GT_COMMA}		{return GT_COMMA;}
{pat_GT_LPAREN}		{return GT_LPAREN;}
{pat_GT_RPAREN}		{return GT_RPAREN;}
{pat_GT_SEMI}		{return GT_SEMI;}
{pat_IT_a}		{return IT_a;}
{pat_GT_LBRACKET}		{return GT_LBRACKET;}
{pat_GT_RBRACKET}		{return GT_RBRACKET;}
{pat_GT_OR}		{return GT_OR;}
{pat_GT_AND}		{return GT_AND;}
{pat_GT_EQUAL}		{return GT_EQUAL;}
{pat_GT_NEQUAL}		{return GT_NEQUAL;}
{pat_GT_LT}		{return GT_LT;}
{pat_GT_GT}		{return GT_GT;}
{pat_GT_LE}		{return GT_LE;}
{pat_GT_GE}		{return GT_GE;}
{pat_GT_PLUS}		{return GT_PLUS;}
{pat_GT_MINUS}		{return GT_MINUS;}
{pat_GT_DIVIDE}		{return GT_DIVIDE;}
{pat_GT_NOT}		{return GT_NOT;}
{pat_IT_STR}		{return IT_STR;}
{pat_IT_LANG}		{return IT_LANG;}
{pat_IT_LANGMATCHES}		{return IT_LANGMATCHES;}
{pat_IT_DATATYPE}		{return IT_DATATYPE;}
{pat_IT_BOUND}		{return IT_BOUND;}
{pat_IT_sameTerm}		{return IT_sameTerm;}
{pat_IT_isIRI}		{return IT_isIRI;}
{pat_IT_isURI}		{return IT_isURI;}
{pat_IT_isBLANK}		{return IT_isBLANK;}
{pat_IT_isLITERAL}		{return IT_isLITERAL;}
{pat_IT_REGEX}		{return IT_REGEX;}
{pat_GT_DTYPE}		{return GT_DTYPE;}
{pat_IT_true}		{return IT_true;}
{pat_IT_false}		{return IT_false;}
{pat_IRI_REF}		{
						char* copy	= string_copy( &( ((char*) yytext)[1] ) );
						copy[ strlen(copy) - 1 ]	= (char) 0;
						yylval	= (void*) copy;
						return IRI_REF;
					}
{pat_PNAME_NS}		{yylval = (void*) string_copy( yytext ); return PNAME_NS;}
{pat_PNAME_LN}		{
						char* ln	= string_copy( yytext );
						yylval = (void*) ln;
						
						return PNAME_LN;
					}
{pat_BLANK_NODE_LABEL}		{
								yylval = (void*) string_copy( &( ((char*) yytext)[2] ) );
								return BLANK_NODE_LABEL;
							}
{pat_VAR1}		{
					yylval = (void*) string_copy( &( ((char*) yytext)[1] ) );
					return VAR1;
				}
{pat_VAR2}		{
					yylval = (void*) string_copy( &( ((char*) yytext)[1] ) );
					return VAR1;
				}
{pat_LANGTAG}		{
						yylval = (void*) string_copy( &( ((char*) yytext)[1] ) );
						return LANGTAG;
					}
{pat_INTEGER}		{yylval = (void*) string_copy( yytext ); return INTEGER;}
{pat_DECIMAL}		{yylval = (void*) string_copy( yytext ); return DECIMAL;}
{pat_DOUBLE}		{yylval = (void*) string_copy( yytext ); return DOUBLE;}
{pat_INTEGER_POSITIVE}		{yylval = (void*) string_copy( yytext ); return INTEGER_POSITIVE;}
{pat_DECIMAL_POSITIVE}		{yylval = (void*) string_copy( yytext ); return DECIMAL_POSITIVE;}
{pat_DOUBLE_POSITIVE}		{yylval = (void*) string_copy( yytext ); return DOUBLE_POSITIVE;}
{pat_INTEGER_NEGATIVE}		{yylval = (void*) string_copy( yytext ); return INTEGER_NEGATIVE;}
{pat_DECIMAL_NEGATIVE}		{yylval = (void*) string_copy( yytext ); return DECIMAL_NEGATIVE;}
{pat_DOUBLE_NEGATIVE}		{yylval = (void*) string_copy( yytext ); return DOUBLE_NEGATIVE;}
{pat_STRING_LITERAL1}		{
								char* literal	= string_copy( &( ((char*) yytext)[1] ) );
								literal[ strlen(literal) - 1 ]	= (char) 0;
								unescape( literal );
								yylval = (void*) literal;
								return STRING_LITERAL1;
							}
{pat_STRING_LITERAL2}		{
								char* literal	= string_copy( &( ((char*) yytext)[1] ) );
								literal[ strlen(literal) - 1 ]	= (char) 0;
								unescape( literal );
								yylval = (void*) literal;
								return STRING_LITERAL1;
							}
{pat_STRING_LITERAL_LONG1}		{
								char* literal	= string_copy( &( ((char*) yytext)[3] ) );
								literal[ strlen(literal) - 3 ]	= (char) 0;
								unescape( literal );
								yylval = (void*) literal;
								return STRING_LITERAL1;
							}
{pat_STRING_LITERAL_LONG2}		{
								char* literal	= string_copy( &( ((char*) yytext)[3] ) );
								literal[ strlen(literal) - 3 ]	= (char) 0;
								unescape( literal );
								yylval = (void*) literal;
								return STRING_LITERAL1;
							}
{pat_NIL}		{return NIL;}
{pat_ANON}		{return ANON;}

<<EOF>>			{ yyterminate();}
%%
/* END semantic actions for SPARQL terminals */

/* This implementation of SPARQLFlexLexer::yylex() is required to fill the
 * vtable of the class SPARQLFlexLexer. We define the scanner's main yylex
 * function via YY_DECL to reside in the SPARQLScanner class instead. */

char* string_copy( char* text ) {
	char* p	= (char*) malloc( strlen( text ) + 1 );
	strcpy( p, text );
	return p;
}

void unescape( char* text ) {
	char* p	= text;
	char* q	= text;
	while (*p) {
		if (*p == '\\') {
			p++;
			switch (*p) {
				case 't':
					*(q++)	= '\t';
					break;
				case 'n':
					*(q++)	= '\n';
					break;
				case 'r':
					*(q++)	= '\r';
					break;
				case 'b':
					*(q++)	= '\b';
					break;
				case 'f':
					*(q++)	= '\f';
					break;
				default:
					*(q++)	= *p;
			};
			p++;
		} else {
			*(q++)	= *(p++);
		}
	}
	*q	= (char) 0;
}
