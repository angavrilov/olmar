// main.cc            see license.txt for copyright and terms of use
// entry-point module for a program that parses C++

#include <iostream.h>     // cout
#include <stdlib.h>       // exit

#include "ccgrmain.h"     // some random prototypes..
#include "trace.h"        // traceAddSys
#include "parssppt.h"     // ParseTreeAndTokens, treeMain
#include "fileloc.h"      // sourceFileList (r)
#include "ckheap.h"       // malloc_stats
#include "cc_env.h"       // Env
#include "cc.ast.gen.h"   // C++ AST (r)
#include "cc_lang.h"      // CCLang
#include "treeout.h"      // treeOut
#include "parsetables.h"  // ParseTables
#include "cc_print.h"     // PrintEnv
#ifdef CC_QUAL
  #include "cc_qual/cc_qual_walk.h"
  #include "cc_qual/cqual_iface.h"
#else
  #include "cc_qual_walk_dummy.h"
#endif
//  #include "cc_flatten.h"   // FlattenEnv
#include "ccparse.h"      // ParseEnv


// no bison-parser present, so need to define this
Lexer2Token const *yylval = NULL;

// how to get the parse tables
// linkdepend: cc.gr.gen.cc
ParseTables *make_CCGr_tables();


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

  // parsing language options
  CCLang lang;
  lang.ANSI_Cplusplus();


  // --------------- parse --------------
  TranslationUnit *unit;
  {
    SemanticValue treeTop;
    ParseTreeAndTokens tree(lang, treeTop, strTable);

    UserActions *user = makeUserActions(tree.lexer2.idTable, lang);
    tree.userAct = user;

    traceProgress() << "building parse tables from internal data\n";
    ParseTables *tables = make_CCGr_tables();
    tree.tables = tables;

    char const *positionalArg = processArgs
      (argc, argv, 
       "  additional flags for ccgr:\n"
       "    malloc_stats       print malloc stats every so often\n"
       "    stopAfterParse     stop after parsing\n"
       "    printAST           print AST after parsing\n"
       "    stopAfterTCheck    stop after typechecking\n"
       "    printTypedAST      print AST with type info\n"
       "    tcheck             print typechecking info\n"
       "");
    maybeUseTrivialActions(tree);

    if (tracingSys("cc_qual")) {
      // FIX: pass this in on the command line; wrote Scott
      init_cc_qual("cc_qual/cqual/config/lattice");
      cc_qual_flag = 1;
#if CC_QUAL
      if (tracingSys("matt")) matt_flag = 1;
#endif // CC_QUAL
    } 
    else {
      cc_qual_flag = 0;
    }

    if (!toplevelParse(tree, positionalArg)) {
      exit(2); // parse error
    }

    // check for parse errors detected by the context class
    if (globalParseEnv->errors) {    // HACK!!
      exit(2);
    }

    traceProgress(2) << "final parse result: " << treeTop << endl;
    unit = (TranslationUnit*)treeTop;

    //unit->debugPrint(cout, 0);

    delete user;
    delete tables;
  }

  checkHeap();

  // print abstract syntax tree
  if (tracingSys("printAST")) {
    unit->debugPrint(cout, 0);
  }

  if (unit) {     // when "-tr trivialActions" it's NULL...
    cout << "ambiguous nodes: " << numAmbiguousNodes(unit) << endl;
  }

  // dsw:print abstract syntax tree as XML
  if (tracingSys("xmlPrintAST")) {
    unit->xmlPrint(cout, 0);
  }

  if (tracingSys("stopAfterParse")) {
    return;
  }


  // ---------------- typecheck -----------------
  {
    traceProgress() << "type checking...\n";
    BasicTypeFactory tfac;
    Env env(strTable, lang, tfac);
    unit->tcheck(env);
    traceProgress() << "done type checking\n";

    if (tracingSys("secondTcheck")) {
      // this is useful to measure the cost of disambiguation, since
      // now the tree is entirely free of ambiguities
      traceProgress() << "beginning second tcheck...\n";
      Env env2(strTable, lang, tfac);
      unit->tcheck(env2);
      traceProgress() << "end of second tcheck\n";
    }

    int numErrors=0, numWarnings=0;
    FOREACH_OBJLIST(ErrorMsg, env.errors, iter) {
      if (iter.data()->isWarning) {
        numWarnings++;
      }
      else {
        numErrors++;
      }
    }

    // print errors and warnings in reverse order
    env.errors.reverse();
    FOREACH_OBJLIST(ErrorMsg, env.errors, iter) {
      cout << iter.data()->toString() << "\n";
    }

    cout << "typechecking results:\n"
         << "  errors:   " << numErrors << "\n"
         << "  warnings: " << numWarnings << "\n";

    if (numErrors != 0) {
      exit(4);
    }

    // verify the tree now has no ambiguities
    if (unit && numAmbiguousNodes(unit) != 0) {
      cout << "UNEXPECTED: ambiguities remain after type checking!\n";
      if (tracingSys("mustBeUnambiguous")) {
        exit(2);
      }
    }

    // print abstract syntax tree annotated with types
    if (tracingSys("printTypedAST")) {
      unit->debugPrint(cout, 0);
    }
                                  
//      if (tracingSys("flattenTemplates")) {
//        traceProgress() << "dsw flatten...\n";
//        FlattenEnv env(cout);
//        unit->flatten(env);
//        traceProgress() << "dsw flatten... done\n";
//        cout << endl;
//      }
                                  
    if (tracingSys("stopAfterTCheck")) {
      return;
    }

    #if 0
    // C++Qual-aware type checker
    traceProgress() << "C++Qual-aware type checker...\n";

    // little bit of a hack: I need to make a Variable_Q to
    // correspond to env.dependentTypeVar
    try {
      unit->qualTcheck(env.madeUpVariables);
    }
    catch (XNotSupported &x) {
      cout << x.why() << endl;
    }    
    #endif // 0
  }

  // dsw: cc_qual
#ifdef CC_QUAL
  if (tracingSys("cc_qual")) {
    xassert(cc_qual_flag);       // should have been set above
    traceProgress() << "dsw cc_qual...\n";
    // done above
    //        init_cc_qual("cc_qual/cqual/config/lattice");
    QualEnv env;
    // insert the names into all the Qualifiers objects
    printf("** insert the names into all the Qualifiers objects\n");
    SFOREACH_OBJLIST_NC(Variable, Variable::instances, variable_iter) {
      nameSubtypeQualifiers(variable_iter.data());
    }
    printf("** Qualifiers::global_attach_funky_set_qualifiers()\n");
    Qualifiers::global_attach_funky_set_qualifiers();
    printf("** unit->qual(env)\n");
    unit->qual(env);
    printf("** Qualifiers::global_attach_funky_set_qualifiers()\n");
    Qualifiers::global_attach_funky_set_qualifiers();
    printf("** Qualifiers::global_insert_instances_into_graph()\n");
    Qualifiers::global_insert_instances_into_graph();
    printf("** finish_quals_CQUAL()\n");
    finish_quals_CQUAL();
    printf("** qual done\n");
    traceProgress() << "dsw cc_qual... done\n";
  }
#endif

  // dsw: pretty printing
  if (tracingSys("prettyPrint")) {
    cout << endl;
    traceProgress() << "dsw pretty print...\n";
    PrintEnv env(cout);
    cout << "---- START ----" << endl;
    cout << "// -*-c++-*-" << endl;
    unit->print(env);
    env.finish();
    cout << "---- STOP ----" << endl;
    traceProgress() << "dsw pretty print... done\n";
    cout << endl;
  }

  // test AST cloning
  if (tracingSys("testClone")) {
    cout << "------- cloned tree --------\n";
    TranslationUnit *u2 = unit->clone();
    u2->debugPrint(cout, 0);
  }

  traceProgress() << "cleaning up...\n";

  //malloc_stats();

  // delete the tree
  // (currently this doesn't do very much because FakeLists are
  // non-owning, so I won't pretend it does)
  //delete unit;

  strTable.clear();

  //checkHeap();
  //malloc_stats();

  sourceFileList.clear();
}

int main(int argc, char **argv)
{
  doit(argc, argv);

  //malloc_stats();

  return 0;
}
