// cc2main.cc
// toplevel driver for cc2

#include <iostream.h>     // cout
#include <stdlib.h>       // exit

#include "trace.h"        // traceAddSys
#include "parssppt.h"     // ParseTreeAndTokens, treeMain
#include "cc_lang.h"      // CCLang


// no bison-parser present, so need to define this
Lexer2Token const *yylval = NULL;

// defined in cc2.gr
UserActions *makeUserActions();


void doit(int argc, char **argv)
{
  traceAddSys("progress");
  //traceAddSys("parse-tree");

  // parsing language options
  CCLang lang;
  lang.ANSI_Cplusplus();


  // --------------- parse --------------
  {
    SemanticValue treeTop;
    ParseTreeAndTokens tree(lang, treeTop);
    UserActions *user = makeUserActions();
    tree.userAct = user;
    if (!treeMain(tree, argc, argv,
          "  additional flags for cc2 (none of these work right now):\n"
          "    stopAfterParse     stop after parsing\n"
          "    printAST           print AST after parsing\n"
          "")) {
      // parse error
      exit(2);
    }

    traceProgress(2) << "final parse result: " << treeTop << endl;

    //unit->debugPrint(cout, 0);

    delete user;
  }

  traceRemoveAll();
}

int main(int argc, char **argv)
{
  doit(argc, argv);

  return 0;
}
