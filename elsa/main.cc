// main.cc            see license.txt for copyright and terms of use
// entry-point module for a program that parses C++

#include <iostream.h>     // cout
#include <stdlib.h>       // exit, getenv

#include "trace.h"        // traceAddSys
#include "parssppt.h"     // ParseTreeAndTokens, treeMain
#include "srcloc.h"       // SourceLocManager
#include "ckheap.h"       // malloc_stats
#include "cc_env.h"       // Env
#include "cc.ast.gen.h"   // C++ AST (r)
#include "cc_lang.h"      // CCLang
#include "parsetables.h"  // ParseTables
#include "cc_print.h"     // PrintEnv
#include "cc.gr.gen.h"    // CCParse
#include "nonport.h"      // getMilliseconds
#include "ptreenode.h"    // PTreeNode
#include "ptreeact.h"     // ParseTreeLexer, ParseTreeActions


// little check: is it true that only global declarators
// ever have Declarator::type != Declarator::var->type?
// .. no, because it's true for class members too ..
// .. it's also true of arrays declared [] but then inited ..
// .. finally, it's true of parameters whose types get
//    normalized as per cppstd 8.3.5 para 3; I didn't include
//    that case below because there's no easy way to test for it ..
class DeclTypeChecker : public ASTVisitor {
public:
  int instances;

public:
  DeclTypeChecker() : instances(0) {}
  virtual bool visitDeclarator(Declarator *obj);
};

bool DeclTypeChecker::visitDeclarator(Declarator *obj)
{
  if (obj->type != obj->var->type &&
      !(obj->var->flags & (DF_GLOBAL | DF_MEMBER)) &&
      !obj->type->isArrayType()) {
    instances++;
    cout << toString(obj->var->loc) << ": " << obj->var->name
         << " has type != var->type, but is not global or member or array\n";
  }
  return true;
}


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

  if_malloc_stats();

  SourceLocManager mgr;

  // string table for storing parse tree identifiers
  StringTable strTable;

  // parsing language options
  CCLang lang;
  lang.ANSI_Cplusplus();


  // --------------- parse --------------
  TranslationUnit *unit;
  {
    char const *inputFname = processArgs
      (argc, argv,
       "  additional flags for ccparse:\n"
       "    malloc_stats       print malloc stats every so often\n"
       "    parseTree          make a parse tree and print that, only\n"
       "    stopAfterParse     stop after parsing\n"
       "    printAST           print AST after parsing\n"
       "    stopAfterTCheck    stop after typechecking\n"
       "    printTypedAST      print AST with type info\n"
       "    tcheck             print typechecking info\n"
       "    nohashline         ignore #line when reporting locations\n"
       "");

    if (tracingSys("nohashline")) {
      sourceLocManager->useHashLines = false;
    }

    if (tracingSys("c_lang")) {
      lang.ANSI_C();
    }

    SemanticValue treeTop;
    ParseTreeAndTokens tree(lang, treeTop, strTable, inputFname);
    
    // grab the lexer so we can check it for errors (damn this
    // 'tree' thing is stupid..)
    Lexer *lexer = dynamic_cast<Lexer*>(tree.lexer);
    xassert(lexer);

    CCParse *parseContext = new CCParse(strTable, lang);
    tree.userAct = parseContext;

    traceProgress() << "building parse tables from internal data\n";
    ParseTables *tables = parseContext->makeTables();
    tree.tables = tables;

    maybeUseTrivialActions(tree);

    if (tracingSys("parseTree")) {
      // make some helpful aliases
      LexerInterface *underLexer = tree.lexer;
      UserActions *underAct = parseContext;

      // replace the lexer and parser with parse-tree-building versions
      tree.lexer = new ParseTreeLexer(underLexer, underAct);
      tree.userAct = new ParseTreeActions(underAct, tables);

      // 'underLexer' and 'tree.userAct' will be leaked.. oh well
    }

    if (!toplevelParse(tree, inputFname)) {
      exit(2); // parse error
    }

    // check for parse errors detected by the context class
    if (parseContext->errors || lexer->errors) {
      exit(2);
    }

    traceProgress(2) << "final parse result: " << treeTop << endl;

    if (tracingSys("parseTree")) {
      // the 'treeTop' is actually a PTreeNode pointer; print the
      // tree and bail
      PTreeNode *ptn = (PTreeNode*)treeTop;
      ptn->printTree(cout, PTreeNode::PF_EXPAND);
      return;
    }

    // treeTop is a TranslationUnit pointer
    unit = (TranslationUnit*)treeTop;

    //unit->debugPrint(cout, 0);

    delete parseContext;
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
  BasicTypeFactory tfac;
  {

    traceProgress() << "type checking...\n";
    long tcheckStart = getMilliseconds();
    Env env(strTable, lang, tfac);
    unit->tcheck(env);
    traceProgress() << "done type checking (" 
                    << (getMilliseconds() - tcheckStart) 
                    << " ms)\n";

    int numErrors=0, numWarnings=0;
    FOREACH_OBJLIST(ErrorMsg, env.errors, iter) {
      if (iter.data()->isWarning) {
        numWarnings++;
      }
      else {
        numErrors++;
      }
    }

    // do this now so that 'printTypedAST' will include CFG info
    #ifdef CFG_EXTENSION
    // A possible TODO is to do this right after each function is type
    // checked.  However, in the current design, that means physically
    // inserting code into Function::tcheck (ifdef'd of course).  A way
    // to do it better would be to have a general post-function-tcheck
    // interface that analyses could hook in to.  That could be augmented
    // by a parsing mode that parsed each function, analyzed it, and then
    // immediately discarded its AST.
    numErrors += computeUnitCFG(unit);
    #endif // CFG_EXTENSION

    // print abstract syntax tree annotated with types
    if (tracingSys("printTypedAST")) {
      unit->debugPrint(cout, 0);
    }

    if (tracingSys("secondTcheck")) {
      // this is useful to measure the cost of disambiguation, since
      // now the tree is entirely free of ambiguities
      traceProgress() << "beginning second tcheck...\n";
      Env env2(strTable, lang, tfac);
      unit->tcheck(env2);
      traceProgress() << "end of second tcheck\n";
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

    // check an expected property of the annotated AST
    if (tracingSys("declTypeCheck") || getenv("declTypeCheck")) {
      DeclTypeChecker vis;
      unit->traverse(vis);
      cout << "instances of type != var->type: " << vis.instances << endl;
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
  }

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
    TranslationUnit *u2 = unit->clone();

    if (tracingSys("cloneAST")) {
      cout << "------- cloned AST --------\n";
      u2->debugPrint(cout, 0);
    }

    if (tracingSys("cloneCheck")) {
      Env env3(strTable, lang, tfac);
      u2->tcheck(env3);

      if (tracingSys("cloneTypedAST")) {
        cout << "------- cloned typed AST --------\n";
        u2->debugPrint(cout, 0);
      }

      if (tracingSys("clonePrint")) {
        PrintEnv penv(cout);
        cout << "---- cloned pretty print ----" << endl;
        u2->print(penv);
        penv.finish();
      }
    }
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
}

int main(int argc, char **argv)
{
  doit(argc, argv);

  //malloc_stats();

  return 0;
}
