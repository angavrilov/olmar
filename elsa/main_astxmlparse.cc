// main_astxmlparse.cc          see license.txt for copyright and terms of use

#include "main_astxmlparse.h"   // this module
#include "useract.h"            // SemanticValue, UserAction
#include "parssppt.h"           // ParseTreeAndTokens, treeMain
#include "cc_lang.h"            // CCLang
#include "astxml.gr.gen.h"      // AstXmlParse
#include "ptreeact.h"           // ParseTreeLexer, ParseTreeActions
#include "ptreenode.h"          // PTreeNode
#include "parsetables.h"        // ParseTables
#include "glr.h"                // GLR

#include "astxml.gr.gen.h"      // AstXmlParse
#include "astxml_lexer.h"       // AstXmlLexer

TranslationUnit *astxmlparse(StringTable &strTable, char const *inputFname)
{
  UserActions *userAct = new AstXmlParse; // parse parameter
  ParseTables *tables = userAct->makeTables(); // parse tables (or NULL)
  GLR glr(userAct, tables);
  
  // make and prime the lexer
  AstXmlLexer *astXmlLexer = new AstXmlLexer(strTable, inputFname);
  astXmlLexer->getTokenFunc()(astXmlLexer);

  SemanticValue treeTop;
  glrParseNamedFile(glr, *astXmlLexer, treeTop, inputFname);
  // treeTop is a TranslationUnit pointer
  TranslationUnit *unit = (TranslationUnit*)treeTop;
  //unit->debugPrint(cout, 0);
  delete userAct;
  delete tables;
  delete astXmlLexer;
  return unit;
}
