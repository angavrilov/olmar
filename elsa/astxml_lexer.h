// astxml_lexer.h           see license.txt for copyright and terms of use

#ifndef ASTXML_LEXER_H
#define ASTXML_LEXER_H

#include "baselexer.h"      // BaseLexer
#include "astxml_tokens.h"

class AstXmlLexer : public BaseLexer {
  bool prevIsNonsep;               // true if last-yielded token was nonseparating

  public:
  AstXmlLexer(StringTable &strtable0, char const *fname0);
  AstXmlLexer(StringTable &strtable0, SourceLoc initLoc0, char const *buf0, int len0);

  protected:
  // see comments at top of lexer.cc (er, this should probably be removed)
  void checkForNonsep(ASTXMLTokenType t);

  // consume whitespace
  void whitespace();

  // do everything for a single-spelling token
  int tok(ASTXMLTokenType t);

  // do everything for a multi-spelling token
  int svalTok(ASTXMLTokenType t);

  virtual string tokenDesc() const;
  virtual string tokenKindDesc(int kind) const;

  FLEX_OUTPUT_METHOD_DECLS
};

#endif // ASTXML_LEXER_H
