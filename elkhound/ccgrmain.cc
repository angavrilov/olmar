// ccgrmain.cc
// toplevel driver for ccgr

#include <iostream.h>     // cout
#include <stdlib.h>       // exit

#include "ccgrmain.h"     // this module, sorta..
#include "trace.h"        // traceAddSys
#include "parssppt.h"     // ParseTreeAndTokens, treeMain
#include "ckheap.h"       // malloc_stats
#include "grammar.h"      // grammarStringTable
#include "fileloc.h"      // sourceFileList
#include "c.ast.gen.h"    // C ast
#include "cc_env.h"       // Env
#include "aenv.h"         // AEnv
#include "strutil.h"      // plural
#include "factflow.h"     // factFlow
#include "cc_lang.h"      // CCLang
#include "treeout.h"      // treeOut


// globals that allow AEnv's to be created wherever ...
StringTable *globalStringTable;
Variable const *globalMemVariable;


void if_malloc_stats()
{
  if (tracingSys("malloc_stats")) {
    malloc_stats();
  }
}


void doit(int argc, char **argv)
{
  traceAddSys("progress");
  //traceAddSys("parse-tree");
                                    
  // this is useful for emacs' outline mode, because if the file
  // doesn't begin with a heading, it collapses the starting messages
  // and doesn't like to show them again
  treeOut(1) << "beginning of output      -*- outline -*-\n";

  if_malloc_stats();

  // string table for storing parse tree identifiers
  StringTable strTable;
  globalStringTable = &strTable;

  // parsing language options
  CCLang lang;
  lang.ANSI_Cplusplus();


  // --------------- parse --------------
  TranslationUnit *unit;
  {
    SemanticValue treeTop;
    ParseTreeAndTokens tree(treeTop, strTable);
    UserActions *user = makeUserActions(tree.lexer2.idTable, lang);
    tree.userAct = user;
    if (!treeMain(tree, argc, argv,
          "  additional flags for ccgr:\n"
          "    malloc_stats       print malloc stats every so often\n"
          "    stopAfterParse     stop after parsing\n"
          "    printAST           print AST after parsing\n"
          "    stopAfterTCheck    stop after typechecking\n"
          "    printTypedAST      print AST with type info\n"
          "    disableFactFlow    don't do invariant strengthening\n"
          "    factflow           print details of factflow computation\n"
          "    stopAfterVCGen     stop after vcgen\n"
          "    printAnalysisPath  print each path that is analyzed\n"
          "    predicates         print all predicates (proved or not)\n"
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

                 
  // --------- declarations provided automatically -------
  SourceLocation dummyLoc;
  Variable mem(dummyLoc, strTable.add("mem"),
               new PointerType(PO_POINTER, CV_NONE,
                 &CVAtomicType::fixed[ST_INT]), DF_NONE);
  globalMemVariable = &mem;

  // ---------------- typecheck -----------------
  {
    traceProgress() << "type checking...\n";
    Env env(strTable, lang);
    env.addVariable(mem.name, &mem);
    unit->tcheck(env);
    traceProgress(2) << "done type checking\n";

    // print abstract syntax tree annotated with types
    if (tracingSys("printTypedAST")) {
      unit->debugPrint(cout, 0);
    }

    if (env.getErrors() != 0) {
      int n = env.getErrors();
      cout << "there " << plural(n, "was") << " " << env.getErrors() 
           << " typechecking " << plural(n, "error") << "\n";
      exit(4);
    }

    if (tracingSys("stopAfterTCheck")) {
      return;
    }
  }

  
  // ------------- automatic invariant strengthening ---------
  traceProgress() << "automatic invariant strengthening...\n";
  if (!tracingSys("disableFactFlow")) {
    FOREACH_ASTLIST_NC(TopForm, unit->topForms, tf) {
      if (tf.data()->isTF_func()) {
        factFlow(*( tf.data()->asTF_func() ));
      }
    }
  }


  // --------------- abstract interp ------------
  {
    traceProgress() << "verification condition generation...\n";
    AEnv env(strTable, &mem);

    unit->vcgen(env);

    traceProgress(2) << "done with vcgen\n";

    if (env.failedProofs != 0) {
      treeOut(1) << "there were " << env.failedProofs << " failed proofs\n";
      exit(5);
    }

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
