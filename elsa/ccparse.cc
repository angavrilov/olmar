// ccparse.cc            see license.txt for copyright and terms of use
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
#include "strutil.h"      // plural
#include "cc_lang.h"      // CCLang
#include "treeout.h"      // treeOut
#include "parsetables.h"  // ParseTables
#include "cc_print.h"     // PrintEnv
#include "ccparse.h"      // ParseEnv
      

// -------------------- ParseEnv ----------------
// UGLY HACK
// TODO: fix this by making a proper header during grammar analysis
ParseEnv *globalParseEnv = NULL;

SimpleTypeId ParseEnv::uberSimpleType(SourceLocation const &loc, UberModifiers m)
{
  m = (UberModifiers)(m & UM_TYPEKEYS);

  // implement cppstd Table 7, p.109
  switch (m) {
    case UM_CHAR:                         return ST_CHAR;
    case UM_UNSIGNED | UM_CHAR:           return ST_UNSIGNED_CHAR;
    case UM_SIGNED | UM_CHAR:             return ST_SIGNED_CHAR;
    case UM_BOOL:                         return ST_BOOL;
    case UM_UNSIGNED:                     return ST_UNSIGNED_INT;
    case UM_UNSIGNED | UM_INT:            return ST_UNSIGNED_INT;
    case UM_SIGNED:                       return ST_INT;
    case UM_SIGNED | UM_INT:              return ST_INT;
    case UM_INT:                          return ST_INT;
    case UM_UNSIGNED | UM_SHORT | UM_INT: return ST_UNSIGNED_SHORT_INT;
    case UM_UNSIGNED | UM_SHORT:          return ST_UNSIGNED_SHORT_INT;
    case UM_UNSIGNED | UM_LONG | UM_INT:  return ST_UNSIGNED_LONG_INT;
    case UM_UNSIGNED | UM_LONG:           return ST_UNSIGNED_LONG_INT;
    case UM_SIGNED | UM_LONG | UM_INT:    return ST_LONG_INT;
    case UM_SIGNED | UM_LONG:             return ST_LONG_INT;
    case UM_LONG | UM_INT:                return ST_LONG_INT;
    case UM_LONG:                         return ST_LONG_INT;
    case UM_SIGNED | UM_SHORT | UM_INT:   return ST_SHORT_INT;
    case UM_SIGNED | UM_SHORT:            return ST_SHORT_INT;
    case UM_SHORT | UM_INT:               return ST_SHORT_INT;
    case UM_SHORT:                        return ST_SHORT_INT;
    case UM_WCHAR_T:                      return ST_WCHAR_T;
    case UM_FLOAT:                        return ST_FLOAT;
    case UM_DOUBLE:                       return ST_DOUBLE;
    case UM_LONG | UM_DOUBLE:             return ST_LONG_DOUBLE;
    case UM_VOID:                         return ST_VOID;

    default:
      cout << loc.toString() << ": error: malformed type: "
           << toString(m) << endl;
      errors++;
      return ST_ERROR;
  }
}


UberModifiers ParseEnv
  ::uberCombine(SourceLocation const &loc, UberModifiers m1, UberModifiers m2)
{
  // for future reference: if I want to implement GNU long long, just
  // test for 'long' in each modifier set and if so set another flag,
  // e.g. UM_LONG_LONG; only if *that* one is already set would I complain

  // any duplicate flags?
  UberModifiers dups = (UberModifiers)(m1 & m2);
  if (dups) {
    cout << loc.toString() << ": error: duplicate modifier: "
         << toString(dups) << endl;
    errors++;
  }
  
  return (UberModifiers)(m1 | m2);
}


// ----------------- driver program ----------------
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

    if (!treeMain(tree, argc, argv,
          "  additional flags for ccgr:\n"
          "    malloc_stats       print malloc stats every so often\n"
          "    stopAfterParse     stop after parsing\n"
          "    printAST           print AST after parsing\n"
          "    stopAfterTCheck    stop after typechecking\n"
          "    printTypedAST      print AST with type info\n"
          "    tcheck             print typechecking info\n"
          "")) {
      // parse error
      exit(2);
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

  if (tracingSys("stopAfterParse")) {
    return;
  }


  // ---------------- typecheck -----------------
  {
    traceProgress() << "type checking...\n";
    Env env(strTable, lang);
    unit->tcheck(env);
    traceProgress(2) << "done type checking\n";
    
    int numErrors=0, numWarnings=0;
    FOREACH_OBJLIST(ErrorMsg, env.errors, iter) {
      if (iter.data()->isWarning) {
        numWarnings++;
      }
      else {
        numErrors++;
      }
    }

    cout << "typechecking results:\n"
         << "  errors:   " << numErrors << "\n"
         << "  warnings: " << numWarnings << "\n";

    // print errors and warnings in reverse order
    env.errors.reverse();
    FOREACH_OBJLIST(ErrorMsg, env.errors, iter) {
      cout << iter.data()->toString() << "\n";
    }

    if (numErrors != 0) {
      exit(4);
    }

    // print abstract syntax tree annotated with types
    if (tracingSys("printTypedAST")) {
      unit->debugPrint(cout, 0);
    }
                                  
    if (tracingSys("stopAfterTCheck")) {
      return;
    }
  }

  // dsw: Tree walk ****************
  if (tracingSys("prettyPrint")) {
      cout << endl;
      traceProgress() << "dsw tree walk...\n";
      PrintEnv env(cout);
      cout << "---- START ----" << endl;
      cout << "// -*-c++-*-" << endl;
      unit->print(env);
      env.finish();
      cout << "---- STOP ----" << endl;
      traceProgress() << "dsw tree walk... done\n";
      cout << endl;
  }

  // test AST cloning
  if (tracingSys("testClone")) {
    cout << "------- cloned tree --------\n";
    TranslationUnit *u2 = unit->clone();
    u2->debugPrint(cout, 0);
  }


  //malloc_stats();

  // delete the tree
  // (currently this doesn't do very much because FakeLists are
  // non-owning, so I won't pretend it does)
  //delete unit;

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
