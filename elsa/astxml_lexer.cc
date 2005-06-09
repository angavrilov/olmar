// astxml_lexer.cc         see License.txt for copyright and terms of use

#include "astxml_lexer.h"
#include "xassert.h"


// ------------------------ AstXmlLexer -------------------
static char const * const tokenNames[] = {
  "XTOK_EOF",

  "XTOK_NAME",

  "XTOK_INT_LITERAL",
  "XTOK_FLOAT_LITERAL",
  "XTOK_HEX_LITERAL",
  "XTOK_STRING_LITERAL",

  "XTOK_LESSTHAN",
  "XTOK_GREATERTHAN",
  "XTOK_EQUAL",
  "XTOK_SLASH",

#include "astxml_lexer1_mid.gen.cc"

  "NUM_XML_TOKEN_TYPES",
};


string AstXmlLexer::tokenKindDesc(int kind) const
{
  // hack
  xassert(0 <= kind && kind < NUM_XML_TOKEN_TYPES);
  xassert(tokenNames[kind]);     // make sure the tokenNames array grows with the enum
  return tokenNames[kind];
}

int AstXmlLexer::tok(ASTXMLTokenType kind) {
  printf("%s\n", tokenKindDesc(kind).c_str());
  fflush(stdout);
  return kind;
}

int AstXmlLexer::svalTok(ASTXMLTokenType kind)
{
  printf("%s '%s'\n", tokenKindDesc(kind).c_str(), yytext);
  fflush(stdout);
  return kind;
}

void AstXmlLexer::err(char const *msg)
{
//    errors++;
  // FIX: do locations
//    cerr << toString(loc) << ": error: " << msg << endl;
}
