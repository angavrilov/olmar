// ccgrmain.cc
// toplevel driver for ccgr

#include <iostream.h>     // cout
#include <stdlib.h>       // exit

#include "trace.h"        // traceAddSys
#include "parssppt.h"     // ParseTreeAndTokens, treeMain
#include "ckheap.h"       // malloc_stats
#include "grammar.h"      // grammarStringTable
#include "fileloc.h"      // sourceFileList
#include "c.ast.gen.h"    // C ast
#include "cc_env.h"       // Env
#include "aenv.h"         // AEnv

// defined by the user somewhere
UserActions *makeUserActions(StringTable &table);

void doit(int argc, char **argv)
{
  traceAddSys("progress");
  //traceAddSys("parse-tree");

  malloc_stats();

  // string table for storing parse tree identifiers
  StringTable strTable;


  // --------------- parse --------------
  TranslationUnit *unit;
  {
    SemanticValue treeTop;
    ParseTreeAndTokens tree(treeTop, strTable);
    UserActions *user = makeUserActions(tree.lexer2.idTable);
    tree.userAct = user;
    if (!treeMain(tree, argc, argv)) {
      // parse error
      exit(2);
    }

    cout << "final parse result: " << treeTop << endl;
    unit = (TranslationUnit*)treeTop;

    //unit->debugPrint(cout, 0);

    delete user;
    grammarStringTable.clear();
  }

  checkHeap();

  // print abstract syntax tree
  unit->debugPrint(cout, 0);


  // ---------------- typecheck -----------------
  {
    cout << "type checking...\n";
    Env env;
    unit->tcheck(env);
    cout << "done type checking\n";

    // print abstract syntax tree annotated with types
    unit->debugPrint(cout, 0);
  }


  // --------------- abstract interp ------------
  {
    cout << "abstract interpretation...\n";
    AEnv env(strTable);

    unit->vcgen(env);

    cout << "done with abs interp\n";
  }


  //malloc_stats();

  // delete the tree
  delete unit;
  strTable.clear();

  //checkHeap();
  //malloc_stats();

  sourceFileList.clear();
  traceRemoveAll();
}

int main(int argc, char **argv)
{
  doit(argc, argv);

  //malloc_stats();

  return 0;
}















// (had to move this down out of the code, because it's such an emotional
// sparkplug, I can't concentrate on coding with it up in the code ...)


          //                            ==============================
          //    *   *   *   *   *   *   ==============================
          //      *   *   *   *   *
          //    *   *   *   *   *   *   ==============================
          //      *   *   *   *   *     ==============================
          //    *   *   *   *   *   *
          //      *   *   *   *   *     ==============================
          //    *   *   *   *   *   *   ==============================
          //      *   *   *   *   *
          //    *   *   *   *   *   *   ==============================
          //                            ==============================
          //
          //   =======================================================
          //   =======================================================
          //
          //   =======================================================
          //   =======================================================
          //
          //   =======================================================
          //   =======================================================

          //   This symbol is *unique*: it stands, among other things,
          //   for the right to protest (burn) this symbol.

