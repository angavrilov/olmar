// main_astxmlparse.cc          see license.txt for copyright and terms of use

#include "main_astxmlparse.h"   // this module
#include "fstream.h"            // ifstream
#include "astxml_lexer.h"       // AstXmlLexer

TranslationUnit *astxmlparse(StringTable &strTable, char const *inputFname)
{
  AstXmlLexer *lexer = new AstXmlLexer;

  ifstream in(inputFname);
  lexer->yyrestart(&in);
  while(1) {
    int ret = lexer->yylex();
    if (!ret) break;
    printf("\tret:%d\n", ret);
  }
  // fix report any errors

  TranslationUnit *unit = (TranslationUnit*) lexer->treeTop;

  delete lexer;
  return unit;
}
