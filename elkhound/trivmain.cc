// trivmain.cc
// main() for use with trivlex, and grammars which just test
// parsing properties
//

#ifndef GRAMMAR_NAME
  #error set preprocessor symbol GRAMMAR_NAME to the name of the .bin grammar file
#endif

#include "trivlex.h"   // trivialLexer
#include "useract.h"   // TrivialUserActions
#include "parssppt.h"  // ParseTreeAndTokens
#include "test.h"      // ARGS_MAIN
#include "trace.h"     // TRACE_ARGS
#include "glr.h"       // GLR
#include "useract.h"   // UserActions
#include "ptreenode.h" // PTreeNode


// defined in the grammar file
UserActions *makeUserActions();

int entry(int argc, char *argv[])
{
  char const *progName = argv[0];
  TRACE_ARGS();

  if (argc < 2) {
    printf("usage: %s [-tr flags] [-count] input-file\n", progName);
    return 0;
  }
                                     
  bool count = false;
  if (0==strcmp(argv[1], "-count")) {
    count = true;
    argv++;    // shift
    argc--;
  }

  char const *inputFname = argv[1];

  // lex input
  Lexer2 lexer;
  trivialLexer(inputFname, lexer);

  // setup parser
  UserActions *user = makeUserActions();
  GLR glr(user);
  glr.readBinaryGrammar(GRAMMAR_NAME);

  // parse input
  SemanticValue treeTop;
  if (!glr.glrParse(lexer, treeTop)) {
    // glrParse prints the error itself
    return 2;
  }

  // count # of parses
  if (count) {
    PTreeNode *top = (PTreeNode*)treeTop;
    TreeCount numParses = top->countTrees();
    cout << "num parses: " << numParses << endl;
  }

  return 0;
}


ARGS_MAIN
