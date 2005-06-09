// astxml_lexer.h           see license.txt for copyright and terms of use

#ifndef ASTXML_LEXER_H
#define ASTXML_LEXER_H

#include <stdio.h>

#include "baselexer.h"          // FLEX_OUTPUT_METHOD_DECLS
#include "sm_flexlexer.h"       // yyFlexLexer
#include "str.h"                // string
#include "astxml_tokens.h"

class AstXmlLexer : public yyFlexLexer {
  public:
  int linenumber;

  AstXmlLexer()
    : linenumber(1)             // file line counting traditionally starts at 1
  {}

  int tok(ASTXMLTokenType kind);
  int svalTok(ASTXMLTokenType t);
  void err(char const *msg);
  
  string tokenKindDesc(int kind) const;

  FLEX_OUTPUT_METHOD_DECLS
};

#endif // ASTXML_LEXER_H
