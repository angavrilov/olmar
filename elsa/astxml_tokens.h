// astxml_tokens.h            see license.txt for copyright and terms of use
// token definitions for the ast xml lexer

#ifndef ASTXML_TOKENS_H
#define ASTXML_TOKENS_H

enum ASTXMLTokenType {
  XTOK_EOF,

  // non-keyword name
  XTOK_NAME,

  // literals
  XTOK_INT_LITERAL,
  XTOK_FLOAT_LITERAL,
  XTOK_HEX_LITERAL,
  XTOK_STRING_LITERAL,

  // AST nodes
  XTOK_TRANSLATION_UNIT,    // "TranslationUnit"
  XTOK_FOO,                 // "Foo"

  // child attribute names
  XTOK_DOT_ID,              // ".id"
  XTOK_TOPFORMS,            // "topForms"

  // operators
  XTOK_LESSTHAN,            // "<"
  XTOK_GREATERTHAN,         // ">"

  XTOK_EQUAL,               // "="
  XTOK_SLASH,               // "/"

  // dummy terminals
  NUM_XML_TOKEN_TYPES,

};  // enum TokenType

#endif // ASTXML_TOKENS_H
