// lexer.h
// lexer for the guarded-command example language

#ifndef LEXER_H
#define LEXER_H

#include "lexerint.h"      // LexerInterface

// token codes (must agree with the parser)
enum TokenCode {
  TOK_EOF         = 0,     // end of file
  
  // minimal set of tokens for AExp
  TOK_LITERAL,             // integer literal
  TOK_IDENTIFIER,          // identifier like "x"
  TOK_PLUS,                // "+"
  TOK_MINUS,               // "-"
  TOK_TIMES,               // "*"
  TOK_LPAREN,              // "("
  TOK_RPAREN,              // ")"

//    // for BExp
//    TOK_TRUE,                // "true"
//    TOK_FALSE,               // "false"
//    TOK_EQUAL,               // "="
//    TOK_NOT,
};


// read characters from stdin, yield tokens for the parser
class Lexer : public LexerInterface {
public:
  // function that retrieves the next token from
  // the input stream
  static void nextToken(LexerInterface *lex);
  virtual NextTokenFunc getTokenFunc() const
    { return &Lexer::nextToken; }

  // debugging assistance functions
  string tokenDesc() const;
  string tokenKindDesc(int kind) const;
};


#endif // LEXER_H
