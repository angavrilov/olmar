// grampar.h
// declarations for bison-generated grammar parser

#ifndef __GRAMPAR_H
#define __GRAMPAR_H

// fwd decl
class ASTNode;


// -------- rest of the program's view of Bison ------------
// name of extra parameter to yyparse
#define YYPARSE_PARAM parseParam

// type of thing extra param points at
struct ParseParams {
  ASTNode *treeTop;    // set when parsing finishes; AST tree top
};

// caller interface to Bison-generated parser; starts parsing
// stdin (?) and returns 0 for success and 1 for error; the
// extra parameter is available to actions to use
extern "C" int yyparse(void *YYPARSE_PARAM);


// ---------- Bison's view of the rest of the program --------
// type of Bison semantic values
#define YYSTYPE ASTNode*

// Bison calls this to get each token; returns token code,
// or 0 for eof; semantic value for returned token can be
// put into '*lvalp'
extern "C" int yylex(YYSTYPE *lvalp);

// Bison calls this on error
extern "C" void yyerror(char const *message);



#endif // __GRAMPAR_H
