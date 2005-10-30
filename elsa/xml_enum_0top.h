// xml_enum_0top.h            see license.txt for copyright and terms of use

// top boilerlate for token definitions for the ast xml lexer

// NOTE: the name of the guard does not match the name of the file
// because this is only the top of the generated .h file; Note that
// the bottom of the guard is in another file.
#ifndef XML_ENUM_GEN_H
#define XML_ENUM_GEN_H

enum XmlToken {
  XTOK_EOF,

  // non-keyword name
  XTOK_NAME,

  // literals
  XTOK_INT_LITERAL,
  XTOK_FLOAT_LITERAL,
  XTOK_HEX_LITERAL,
  XTOK_STRING_LITERAL,

  // punctuation
  XTOK_LESSTHAN,            // "<"
  XTOK_GREATERTHAN,         // ">"
  XTOK_EQUAL,               // "="
  XTOK_SLASH,               // "/"
