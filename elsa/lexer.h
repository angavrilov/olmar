// lexer.h            see license.txt for copyright and terms of use
// lexer for C and C++ source files

#ifndef LEXER_H
#define LEXER_H

// This included file is part of the Flex distribution.  It defines
// the base class yyFlexLexer.
#include <FlexLexer.h>

#include "lexerint.h"       // LexerInterface
#include "cc_tokens.h"      // TokenType
#include "strtable.h"       // StringRef, StringTable

// fwd decls
class CCLang;               // cc_lang.h


// bounds-checking functional interfaces to tables declared in cc_tokens.h
char const *toString(TokenType type);
TokenFlag tokenFlags(TokenType type);


// lexer object
class Lexer : public yyFlexLexer, public LexerInterface {
private:    // data
  istream *inputStream;            // (owner) file from which we're reading
  SourceLoc nextLoc;               // location of *next* token
  bool prevIsNonsep;               // true if last-yielded token was nonseparating

public:     // data
  StringTable &strtable;           // string table
  CCLang &lang;                    // language options
  int errors;                      // count of errors encountered

private:    // funcs
  Lexer(Lexer&);                   // disallowed

  // advance source location
  void updLoc() {
    loc = nextLoc;                 // location of *this* token
    nextLoc = advText(nextLoc, yytext, yyleng);
  }

  // adds a string with only the specified # of chars; writes (but
  // then restores) a null terminator if necessary, so 'str' isn't const
  StringRef addString(char *str, int len);

  // see comments at top of lexer.cc
  void checkForNonsep(TokenType t) {
    if (tokenFlags(t) & TF_NONSEPARATOR) {
      if (prevIsNonsep) {
        err("two adjacent nonseparating tokens");
      }
      prevIsNonsep = true;
    }
    else {
      prevIsNonsep = false;
    }
  }

  // various forms of whitespace can separate nonseparating tokens
  void whitespace() {
    updLoc();
    prevIsNonsep = false;
  }

  // do everything for a single-spelling token
  int tok(TokenType t);

  // do everything for a multi-spelling token
  int svalTok(TokenType t);

  // report an error
  void err(char const *msg);

  // part of the constructor
  istream *openFile(char const *fname);

  // read the next token and return its code; returns TOK_EOF for end of file;
  // this function is defined in flex's output source code
  virtual int yylex();

public:     // funcs
  // make a lexer to scan the given file
  Lexer(StringTable &strtable, CCLang &lang, char const *fname);
  ~Lexer();

  // LexerInterface funcs
  static void tokenFunc(LexerInterface *lex);
  virtual NextTokenFunc getTokenFunc() const;
  virtual string tokenDesc() const;
};


#endif // LEXER_H
