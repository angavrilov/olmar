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


void if_malloc_stats()
{
  if (tracingSys("malloc_stats")) {
    malloc_stats();
  }
}


// defined by the user somewhere
UserActions *makeUserActions(StringTable &table);

void doit(int argc, char **argv)
{
  traceAddSys("progress");
  //traceAddSys("parse-tree");

  if_malloc_stats();

  // string table for storing parse tree identifiers
  StringTable strTable;


  // --------------- parse --------------
  TranslationUnit *unit;
  {
    SemanticValue treeTop;
    ParseTreeAndTokens tree(treeTop, strTable);
    UserActions *user = makeUserActions(tree.lexer2.idTable);
    tree.userAct = user;
    if (!treeMain(tree, argc, argv,
          "  additional flags for ccgr:\n"
          "    malloc_stats       print malloc stats every so often\n"
          "    stopAfterParse     stop after parsing\n"
          "    printAST           print AST after parsing\n"
          "    stopAfterTCheck    stop after typechecking\n"
          "    printTypedAST      print AST with type info\n"
          "    stopAfterVCGen     stop after vcgen\n"
          "    printPredicates    print all predicates proved\n"
          "    absInterp          print results of abstract interpretation\n"
          "    tcheck             print typechecking info\n"
          "")) {
      // parse error
      exit(2);
    }

    traceProgress(2) << "final parse result: " << treeTop << endl;
    unit = (TranslationUnit*)treeTop;

    //unit->debugPrint(cout, 0);

    delete user;
    grammarStringTable.clear();
  }

  checkHeap();

  // print abstract syntax tree
  if (tracingSys("printAST")) {
    unit->debugPrint(cout, 0);
  }

  if (tracingSys("stopAfterParse")) {
    return;
  }


  // ---------------- typecheck -----------------
  {
    traceProgress() << "type checking...\n";
    Env env(strTable);
    unit->tcheck(env);
    traceProgress(2) << "done type checking\n";

    // print abstract syntax tree annotated with types
    if (tracingSys("printTypedAST")) {
      unit->debugPrint(cout, 0);
    }

    if (env.getErrors() != 0) {
      cout << "there were " << env.getErrors() << " typechecking errors\n";
      exit(2);
    }

    if (tracingSys("stopAfterTCheck")) {
      return;
    }
  }


  // --------------- abstract interp ------------
  {
    traceProgress() << "verification condition generation...\n";
    AEnv env(strTable);

    unit->vcgen(env);

    traceProgress(2) << "done with vcgen\n";

    if (tracingSys("stopAfterVCGen")) {
      return;
    }
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

