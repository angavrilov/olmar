// glrmain.cc
// simple 'main' to parse a file

#include <iostream.h>     // cout
#include "trace.h"        // traceAddSys
#include "parssppt.h"     // ParseTreeAndTokens, treeMain

int main(int argc, char **argv)
{
  traceAddSys("progress");
  //traceAddSys("parse-tree");

  SemanticValue treeTop;
  ParseTreeAndTokens tree(treeTop);
  treeMain(tree, argc, argv);

  cout << "final parse result: " << treeTop << endl;

  return 0;
}
