// glrmain.cc
// simple 'main' to parse a file

#include <iostream.h>     // cout
#include <stdlib.h>       // exit

#include "trace.h"        // traceAddSys
#include "parssppt.h"     // ParseTreeAndTokens, treeMain
#include "ckheap.h"       // malloc_stats
#include "grammar.h"      // grammarStringTable
#include "fileloc.h"      // sourceFileList
#include "ccgrmain.h"     // makeUserActions
#include "cc_lang.h"      // CCLang

// no bison-parser present, so define it myself
Lexer2Token const *yylval = NULL;

void doit(int argc, char **argv)
{
  traceAddSys("progress");
  //traceAddSys("parse-tree");

  SemanticValue treeTop;
  CCLang lang;
  ParseTreeAndTokens tree(lang, treeTop);
  UserActions *user = makeUserActions(tree.lexer2.idTable, lang);
  tree.userAct = user;
  if (!treeMain(tree, argc, argv)) {
    // parse error
    exit(2);
  }

  cout << "final parse result: " << treeTop << endl;

  // global cleanup
  delete user;
  grammarStringTable.clear();
  sourceFileList.clear();
  traceRemoveAll();
}

int main(int argc, char **argv)
{
  doit(argc, argv);

  //malloc_stats();

  return 0;
}
