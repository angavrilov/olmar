// main.cc            see license.txt for copyright and terms of use
// entry-point module for a program that parses C++

#include <iostream.h>     // cout
#include <stdlib.h>       // exit, getenv, abort

#include "trace.h"        // traceAddSys
#include "parssppt.h"     // ParseTreeAndTokens, treeMain
#include "srcloc.h"       // SourceLocManager
#include "ckheap.h"       // malloc_stats
#include "cc_env.h"       // Env
#include "cc_ast.h"       // C++ AST (r)
#include "cc_ast_aux.h"   // class ASTTemplVisitor
#include "cc_lang.h"      // CCLang
#include "parsetables.h"  // ParseTables
#include "cc_print.h"     // PrintEnv
#include "cc.gr.gen.h"    // CCParse
#include "nonport.h"      // getMilliseconds
#include "ptreenode.h"    // PTreeNode
#include "ptreeact.h"     // ParseTreeLexer, ParseTreeActions
#include "sprint.h"       // structurePrint
#include "strtokp.h"      // StrtokParse
#include "smregexp.h"     // regexpMatch
#include "cc_elaborate.h" // ElabVisitor


// little check: is it true that only global declarators
// ever have Declarator::type != Declarator::var->type?
// .. no, because it's true for class members too ..
// .. it's also true of arrays declared [] but then inited ..
// .. finally, it's true of parameters whose types get
//    normalized as per cppstd 8.3.5 para 3; I didn't include
//    that case below because there's no easy way to test for it ..
class DeclTypeChecker : public ASTTemplVisitor {
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


// this scans the AST for E_variables, and writes down the locations
// to which they resolved; it's helpful for writing tests of name and
// overload resolution
class NameChecker : public ASTTemplVisitor {
public:            
  // accumulates the results
  stringBuilder sb;
  
public:
  NameChecker() {}
  virtual ~NameChecker() {}    // gcc idiocy

  virtual bool visitExpression(Expression *obj)
  {
    Variable *v = NULL;
    if (obj->isE_variable()) {
      v = obj->asE_variable()->var;
    }
    else if (obj->isE_fieldAcc()) {
      v = obj->asE_fieldAcc()->field;
    }
    
    // this output format is designed to minimize the effect of
    // changes to unrelated details
    if (v
        && 0!=strcmp("__testOverload", v->name)
        && 0!=strcmp("dummy",          v->name)
        && 0!=strcmp("__other",        v->name) // "__other": for inserted elaboration code
        && 0!=strcmp("this",           v->name) // dsw: not sure why "this" is showing up
        && 0!=strcmp("operator=",      v->name) // an implicitly defined member of every class
        && v->name[0]!='~'                      // don't print dtors
        ) {
      sb << " " << v->name << "=" << sourceLocManager->getLine(v->loc);
    }

    return true;
  }
};


void if_malloc_stats()
{
  if (tracingSys("malloc_stats")) {
    malloc_stats();
  }
}


void doit(int argc, char **argv)
{
  // I think this is more noise than signal at this point
  xBase::logExceptions = false;

  traceAddSys("progress");
  //traceAddSys("parse-tree");

  if_malloc_stats();

  SourceLocManager mgr;

  // string table for storing parse tree identifiers
  StringTable strTable;

  // parsing language options
  CCLang lang;
  lang.GNU_Cplusplus();


  // ------------- process command-line arguments ---------
  char const *inputFname = processArgs
    (argc, argv,
     "\n"
     "  general behavior flags:\n"
     "    c_lang             use C language rules (default is C++)\n"
     "    nohashline         ignore #line when reporting locations\n"
     "    doOverload         do named function overload resolution\n"
     "    doOperatorOverload do operator overload resolution\n"
     "\n"
     "  options to stop after a certain stage:\n"
     "    stopAfterParse     stop after parsing\n"
     "    stopAfterTCheck    stop after typechecking\n"
     "    stopAfterElab      stop after semantic elaboration\n"
     "\n"
     "  output options:\n"
     "    parseTree          make a parse tree and print that, only\n"
     "    printAST           print AST after parsing\n"
     "    printTypedAST      print AST with type info\n"
     "    printElabAST       print AST after semantic elaboration\n"
     "    prettyPrint        echo input as pretty-printed (but sometimes invalid) C++\n"
     "\n"
     "  debugging output:\n"
     "    malloc_stats       print malloc stats every so often\n"
     "    env                print as variables are added to the environment\n"
     "    error              print as errors are accumulated\n"
     "    overload           print details of overload resolution\n"
     "\n"
     "  (grep in source for \"trace\" to find more obscure flags)\n"
     "");

  if (tracingSys("printAsML")) {
    Type::printAsML = true;
  }

  if (tracingSys("nohashline")) {
    sourceLocManager->useHashLines = false;
  }

  if (tracingSys("strict")) {
    lang.ANSI_Cplusplus();
  }

  if (tracingSys("c_lang")) {
    lang.GNU_C();
  }

  if (tracingSys("gnu_kandr_c_lang")) {
    lang.GNU_KandR_C();
    #ifndef KANDR_EXTENSION
      xbase("gnu_kandr_c_lang option requires the K&R module (./configure -kandr=yes)");
    #endif
  }

  if (tracingSys("gnu2_kandr_c_lang")) {
    lang.GNU2_KandR_C();
    #ifndef KANDR_EXTENSION
      xbase("gnu2_kandr_c_lang option requires the K&R module (./configure -kandr=yes)");
    #endif
  }

  if (tracingSys("templateDebug")) {
    // predefined set of tracing flags I've been using while debugging
    // the new templates implementation
    traceAddSys("template");
    traceAddSys("error");
    traceAddSys("scope");
    traceAddSys("templateParams");
    traceAddSys("templateXfer");
    traceAddSys("prettyPrint");
    traceAddSys("topform");
  }


  // --------------- parse --------------
  TranslationUnit *unit;
  {
    SemanticValue treeTop;
    ParseTreeAndTokens tree(lang, treeTop, strTable, inputFname);
    
    // grab the lexer so we can check it for errors (damn this
    // 'tree' thing is stupid..)
    Lexer *lexer = dynamic_cast<Lexer*>(tree.lexer);
    xassert(lexer);

    CCParse *parseContext = new CCParse(strTable, lang);
    tree.userAct = parseContext;

    traceProgress(2) << "building parse tables from internal data\n";
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

  if (tracingSys("stopAfterParse")) {
    return;
  }


  // ---------------- typecheck -----------------
  BasicTypeFactory tfac;
  {

    traceProgress(2) << "type checking...\n";
    long tcheckStart = getMilliseconds();
    Env env(strTable, lang, tfac, unit);
    try {
      unit->tcheck(env);
      xassert(env.scope()->isGlobalScope());
    }
    catch (xBase &x) {
      // typically an assertion failure from the tchecker; catch it here
      // so we can print the errors, and something about the location
      env.errors.print(cout);
      cout << x << endl;
      cout << "Failure probably related to code near " << env.locStr() << endl;
      
      // print all the locations on the scope stack; this is sometimes
      // useful when the env.locStr refers to some template code that
      // was instantiated from somewhere else
      //
      // (unfortunately, env.instantiationLocStack isn't an option b/c
      // it will have been cleared by the automatic invocation of
      // destructors unwinding the stack...)
      cout << "current location stack:\n";
      cout << env.locationStackString();

      // I changed from using exit(4) here to using abort() because
      // that way the multitest.pl script can distinguish them; the
      // former is reserved for orderly exits, and signals (like
      // SIGABRT) mean that something went really wrong
      abort();
    }
    traceProgress() << "done type checking ("
                    << (getMilliseconds() - tcheckStart)
                    << " ms)\n";

    int numErrors = env.errors.numErrors();
    int numWarnings = env.errors.numWarnings();

    // do this now so that 'printTypedAST' will include CFG info
    #ifdef CFG_EXTENSION
    // A possible TODO is to do this right after each function is type
    // checked.  However, in the current design, that means physically
    // inserting code into Function::tcheck (ifdef'd of course).  A way
    // to do it better would be to have a general post-function-tcheck
    // interface that analyses could hook in to.  That could be augmented
    // by a parsing mode that parsed each function, analyzed it, and then
    // immediately discarded its AST.
    if (numErrors == 0) {
      numErrors += computeUnitCFG(unit);
    }
    #endif // CFG_EXTENSION

    // print abstract syntax tree annotated with types
    if (tracingSys("printTypedAST")) {
      unit->debugPrint(cout, 0);
    }

    // structural delta thing
    if (tracingSys("structure")) {
      structurePrint(unit);
    }

    if (numErrors==0 && tracingSys("secondTcheck")) {
      // this is useful to measure the cost of disambiguation, since
      // now the tree is entirely free of ambiguities
      traceProgress() << "beginning second tcheck...\n";
      Env env2(strTable, lang, tfac, unit);
      unit->tcheck(env2);
      traceProgress() << "end of second tcheck\n";
    }

    // print errors and warnings
    env.errors.print(cout);

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

    // lookup diagnostic
    if (env.collectLookupResults) {     
      // scan AST
      NameChecker nc;
      nc.sb << "\"collectLookupResults";
      unit->traverse(nc);
      nc.sb << "\"";

      // compare to given text
      if (0==strcmp(env.collectLookupResults, nc.sb)) {
        // ok
      }
      else {
        cout << "collectLookupResults do not match:\n"
             << "  source: " << env.collectLookupResults << "\n"
             << "  tcheck: " << nc.sb << "\n"
             ;
        exit(4);
      }
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

  // semantic elaboration
  if (lang.isCplusplus) {
    long start = getMilliseconds();

    ElabVisitor vis(strTable, tfac, unit);
    
    // if we are going to pretty print, then we need to retain defunct children
    if (tracingSys("prettyPrint")) {
      vis.cloneDefunctChildren = true;
    }

    unit->traverse(vis);

    traceProgress() << "done elaborating ("
                    << (getMilliseconds() - start)
                    << " ms)\n";

    // print abstract syntax tree annotated with types
    if (tracingSys("printElabAST")) {
      unit->debugPrint(cout, 0);
    }
    if (tracingSys("stopAfterElab")) {
      return;
    }
  }

  // dsw: pretty printing
  if (tracingSys("prettyPrint")) {
    traceProgress() << "dsw pretty print...\n";
    PrintEnv env(cout);
    cout << "---- START ----" << endl;
    cout << "// -*-c++-*-" << endl;
    unit->print(env);
    env.finish();
    cout << "---- STOP ----" << endl;
    traceProgress() << "dsw pretty print... done\n";
  }

  // test AST cloning
  if (tracingSys("testClone")) {
    TranslationUnit *u2 = unit->clone();

    if (tracingSys("cloneAST")) {
      cout << "------- cloned AST --------\n";
      u2->debugPrint(cout, 0);
    }

    if (tracingSys("cloneCheck")) {
      // dsw: I hope you intend that I should use the cloned TranslationUnit
      Env env3(strTable, lang, tfac, u2);
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


  //traceProgress() << "cleaning up...\n";

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
  try {
    doit(argc, argv);
  }
  catch (xBase &x) {
    cout << x << endl;
    abort();
  }

  //malloc_stats();

  return 0;
}
