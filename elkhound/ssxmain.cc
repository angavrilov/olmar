// ssxmain.cc
// main() for use with SSx.tree.gr

#include "trivlex.h"   // trivialLexer
#include "parssppt.h"  // ParseTreeAndTokens
#include "test.h"      // ARGS_MAIN
#include "trace.h"     // TRACE_ARGS
#include "glr.h"       // GLR
#include "useract.h"   // UserActions
#include "ssxnode.h"   // Node


int count(Node *n)
{
  switch (n->type) {
    default:
      xfailure("bad code");

    case Node::MERGE:
      // a single tree can take either from the left or
      // the right, but not both simultaneously
      return count(n->left) + count(n->right);

    case Node::SSX:
      // a single tree can have any possibility from left
      // and any possibility from right
      return count(n->left) * count(n->right);

    case Node::X:
      return 1;
  }
}


// defined in the grammar file
UserActions *makeUserActions();

int entry(int argc, char *argv[])
{
  char const *progName = argv[0];
  TRACE_ARGS();

  if (argc < 2) {
    printf("usage: %s input-file\n", progName);
    return 0;
  }
  char const *inputFname = argv[1];

  // lex input
  Lexer2 lexer;
  trivialLexer(inputFname, lexer);

  // setup parser
  UserActions *user = makeUserActions();
  GLR glr(user);
  glr.readBinaryGrammar("SSx.tree.bin");

  // parse input
  SemanticValue treeTop;
  if (!glr.glrParse(lexer, treeTop)) {
    // glrParse prints the error itself
    return 2;
  }

  // count # of parses
  Node *top = (Node*)treeTop;
  int numParses = count(top);
  printf("num parses: %d\n", numParses);

  return 0;
}


ARGS_MAIN
