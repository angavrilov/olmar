// glrmain.cc
// simple 'main' to parse a file

#include <iostream.h>     // cout
#include <stdlib.h>       // exit

#include "trace.h"        // traceAddSys
#include "parssppt.h"     // ParseTreeAndTokens, treeMain
#include "ckheap.h"       // malloc_stats
#include "grammar.h"      // grammarStringTable
#include "fileloc.h"      // sourceFileList

// defined by the user somewhere
UserActions *makeUserActions(StringTable &table);

void doit(int argc, char **argv)
{
  traceAddSys("progress");
  //traceAddSys("parse-tree");

  SemanticValue treeTop;
  ParseTreeAndTokens tree(treeTop);
  UserActions *user = makeUserActions(tree.lexer2.idTable);
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
