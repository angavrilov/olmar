// grampar.h
// declarations for bison-generated grammar parser

#ifndef __GRAMPAR_H
#define __GRAMPAR_H

#include "typ.h"          // NULL
#include "sobjlist.h"     // SObjList
#include "exc.h"          // xBase
#include "strsobjdict.h"  // StringSObjDict
#include "locstr.h"       // LocString

// fwd decl
class GrammarAST;
class GrammarLexer;
class TF_nonterminal;
class FormBodyElt;


// -------- rest of the program's view of Bison ------------
// name of extra parameter to yyparse (i.e. the context in
// which the parser operates, instead of that being stored
// in some collection of globals)
#define YYPARSE_PARAM parseParam

// type of thing extra param points at
struct ParseParams {
  GrammarAST *treeTop;    // set when parsing finishes; AST tree top
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

// when this is set to true, bison parser emits info about
// actions as it's taking them
extern int yydebug;


// ---------- Bison's view of the rest of the program --------
// Bison calls this to get each token; returns token code,
// or 0 for eof; semantic value for returned token can be
// put into '*lvalp'
int grampar_yylex(union YYSTYPE *lvalp, void *parseParam);

// error printer
void grampar_yyerror(char const *message, void *parseParam);


// ---------------- grampar's parsing structures ---------------
class Grammar;    // fwd

// while walking the AST, we do a kind of recursive evaluation
// to handle things like inherited actions and self-updating
// (eval'd at grammar parse time) action expressions
class Environment {
public:      // data
  // grammar we're playing with (stored here because it's
  // more convenient than passing it to every fn separately)
  Grammar &g;

  // env in which we're nested, if any
  Environment *prevEnv;      // (serf)

  // maps from a nonterminal name to its declaration, if that
  // nonterminal has in fact been declared already
  StringSObjDict<TF_nonterminal /*const*/> nontermDecls;

  // set of inherited actions and conditions; we simply
  // store pointers to the ASTs, and re-parse them in
  // the context where they are to be applied; I currently
  // store complete copies of all of 'prev's actions and
  // conditions, so I don't really need 'prevEnv' ...
  SObjList<FormBodyElt /*const*/> inherited;

  // current value of any sequence function (at this point,
  // sequencing is pretty much obsolete, because I botched
  // the first implementation and haven't fixed it)
  int sequenceVal;

public:
  Environment(Grammar &G);             // new env
  Environment(Environment &prevEnv);   // nested env
  ~Environment();
};


// miniature environment for tree names
class TreesContext {
private:    // data
  string t0Name, t1Name;

public:
  TreesContext(char const *t0n, char const *t1n)
    : t0Name(t0n), t1Name(t1n) {}
  TreesContext() {}       // dummy context where no names are defined
  ~TreesContext();

  // return which tree (0 or 1) the name matches,
  // or -1 if it doesn't match either
  int lookupTreeName(char const *tn) const;

  // return true if this context has no names defined
  bool dummyContext() const;
};


// --------------- grampar's external interface -----------
// parse grammar file 'fname' into grammar 'g', throwing exceptions
// if there are problems
void readGrammarFile(Grammar &g, char const *fname);


// thrown when there is an error parsing the AST
class XASTParse : public xBase {
public:    // data
  // token at or near failure
  LocString failToken;

  // what is wrong
  string message;

private:   // funcs
  static string constructMsg(LocString const &tok, char const *msg);

public:    // funcs
  XASTParse(LocString const &tok, char const *msg);
  XASTParse(XASTParse const &obj);
  ~XASTParse();
};


#endif // __GRAMPAR_H
