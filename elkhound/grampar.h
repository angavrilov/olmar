// grampar.h
// declarations for bison-generated grammar parser

#ifndef __GRAMPAR_H
#define __GRAMPAR_H

#include "typ.h"        // NULL

// fwd decl
class ASTNode;
class GrammarLexer;


// -------- rest of the program's view of Bison ------------
// name of extra parameter to yyparse
#define YYPARSE_PARAM parseParam

// type of thing extra param points at
struct ParseParams {
  ASTNode *treeTop;       // set when parsing finishes; AST tree top
  GrammarLexer &lexer;    // lexer we're using

public:
  ParseParams(GrammarLexer &L) :
    treeTop(NULL),
    lexer(L)
  {}
};

// caller interface to Bison-generated parser; starts parsing
// (whatever stream lexer is reading) and returns 0 for success and
// 1 for error; the extra parameter is available to actions to use
int yyparse(void *YYPARSE_PARAM);


// ---------- Bison's view of the rest of the program --------
// type of Bison semantic values
#define YYSTYPE ASTNode*

// name of extra parameter to yylex; is of type ParseParams also
#define YYLEX_PARAM parseParam

// Bison calls this to get each token; returns token code,
// or 0 for eof; semantic value for returned token can be
// put into '*lvalp'
int yylex(YYSTYPE *lvalp, void *YYLEX_PARAM);

// Bison calls yyerror(msg) on error; we need the extra
// parameter too
#define yyerror(msg) my_yyerror(msg, YYPARSE_PARAM)
void my_yyerror(char const *message, void *YYPARSE_PARAM);


// ---------------- grampar's parsing structures ---------------
class Environment {
  // empty for now
};


#endif // __GRAMPAR_H
