// glrmain.cc
// simple 'main' to parse a file

#include <iostream.h>     // cout
#include "trace.h"        // traceAddSys
#include "parssppt.h"     // ParseTreeAndTokens, treeMain
#include "ckheap.h"       // malloc_stats
#include "grampar.h"      // gramparStringTable
#include "fileloc.h"      // sourceFileList

void doit(int argc, char **argv)
{
  traceAddSys("progress");
  //traceAddSys("parse-tree");

  SemanticValue treeTop;
  ParseTreeAndTokens tree(treeTop);
  treeMain(tree, argc, argv);

  cout << "final parse result: " << treeTop << endl;

  // global cleanup
  gramparStringTable.clear();
  sourceFileList.clear();
  traceRemoveAll();
}

int main(int argc, char **argv)
{
  doit(argc, argv);

  malloc_stats();

  return 0;
}
