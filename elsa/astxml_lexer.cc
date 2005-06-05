// astxml_lexer.cc         see License.txt for copyright and terms of use

#include "astxml_lexer.h"


// ------------------------ AstXmlLexer -------------------
AstXmlLexer::AstXmlLexer
  (StringTable &strtable0, char const *fname0)
  : BaseLexer(strtable0, fname0)
  , prevIsNonsep(false)
{}
  
AstXmlLexer::AstXmlLexer
  (StringTable &strtable0, SourceLoc initLoc0, char const *buf0, int len0)
  : BaseLexer(strtable0, initLoc0, buf0, len0)
  , prevIsNonsep(false)
{}


void AstXmlLexer::checkForNonsep(ASTXMLTokenType t) 
{
  prevIsNonsep = false;   // degenerate; remove?
}


void AstXmlLexer::whitespace()
{
  BaseLexer::whitespace();

  // various forms of whitespace can separate nonseparating tokens
  prevIsNonsep = false;
}


// this, and 'svalTok', are out of line because I don't want the
// yylex() function to be enormous; I want that to just have a bunch
// of calls into these routines, which themselves can then have
// plenty of things inlined into them
int AstXmlLexer::tok(ASTXMLTokenType t)
{
  checkForNonsep(t);
  updLoc();
  sval = NULL_SVAL;     // catch mistaken uses of 'sval' for single-spelling tokens
  return t;
}


int AstXmlLexer::svalTok(ASTXMLTokenType t)
{
  checkForNonsep(t);
  updLoc();
  sval = (SemanticValue)addString(yytext, yyleng);
  return t;
}

string AstXmlLexer::tokenDesc() const
{
  return tokenKindDesc(type);
}


static char const * const tokenNames[] = {
  "XTOK_EOF",
  "XTOK_NAME",
  "XTOK_INT_LITERAL",
  "XTOK_FLOAT_LITERAL",
  "XTOK_HEX_LITERAL",
  "XTOK_STRING_LITERAL",
  "XTOK_TRANSLATION_UNIT",
  "XTOK_FOO",
  "XTOK_DOT_ID",
  "XTOK_TOPFORMS",
  "XTOK_LESSTHAN",
  "XTOK_GREATERTHAN",
  "XTOK_EQUAL",
  "XTOK_SLASH",
  "NUM_XML_TOKEN_TYPES",
};


string AstXmlLexer::tokenKindDesc(int kind) const
{
  // hack
  xassert(0 <= kind && kind < NUM_XML_TOKEN_TYPES);
  xassert(tokenNames[kind]);     // make sure the tokenNames array grows with the enum
  return tokenNames[kind];
}

