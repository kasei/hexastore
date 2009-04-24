// $Id: Langname_Scanner.hh,v 1.1 2008/04/06 17:10:46 eric Exp $

#ifndef SPARQLScanner_H
#define SPARQLScanner_H

#ifndef __FLEX_LEXER_H
#define yyFlexLexer SPARQLFlexLexer
#include "FlexLexer.h"
#undef yyFlexLexer
#endif

#include "SPARQLParser.hh"

#endif // SPARQLScanner_H
