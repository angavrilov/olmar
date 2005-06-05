// astxml_tokens.h            see license.txt for copyright and terms of use
// token definitions for the ast xml lexer

#ifndef ASTXML_TOKENS_H
#define ASTXML_TOKENS_H

enum ASTXMLTokenType {
  XTOK_EOF,

  // non-keyword name
  XTOK_NAME,

  // classified name (for e.g. cdecl2)
  //  TOK_TYPE_NAME,           "<type name>",               : m  n
  //  TOK_VARIABLE_NAME,       "<variable name>",           : m  n

  // literals
  XTOK_INT_LITERAL,
  XTOK_FLOAT_LITERAL,
  XTOK_HEX_LITERAL,
  XTOK_STRING_LITERAL,
  //  TOK_CHAR_LITERAL,        "<char literal>",            : m

  // keywords
  //  TOK_ASM,                 "asm",                       :    n
  //  TOK_AUTO,                "auto",                      :    n
  //  TOK_BREAK,               "break",                     :    n
  //  TOK_BOOL,                "bool",                      :    n  p
  XTOK_TRANSLATION_UNIT,    // "TranslationUnit"
  XTOK_FOO,                 // "Foo"

  // names
  XTOK_DOT_ID,              // ".id"
  XTOK_TOPFORMS,            // "topForms"

  // operators (I don't identify C++ operators because in C they're not identifiers)
  //  TOK_LPAREN,              "(",                         :
  //  TOK_RPAREN,              ")",                         :
  XTOK_LESSTHAN,            // "<"
  XTOK_GREATERTHAN,         // ">"

  XTOK_EQUAL,               // "="
  XTOK_SLASH,               // "/"

  // dummy terminals used for precedence games
  //  TOK_PREFER_REDUCE,       "<prefer reduce>",           :
  //  TOK_PREFER_SHIFT,        "<prefer shift>",            :
  NUM_XML_TOKEN_TYPES,

};  // enum TokenType

#endif // ASTXML_TOKENS_H
