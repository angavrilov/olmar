// lexer1.cc
// non-parser code for Lexer 1, declared in lexer1.h

#include "lexer1.h"       // this module
#include "typ.h"          // staticAssert, TABLESIZE
#include "trace.h"        // tracing stuff
#include "strutil.h"      // encodeWithEscapes

#include <stdio.h>        // printf
#include <assert.h>       // assert
#include <ctype.h>        // isprint


// -------------------- FileLocation -----------------------------
void FileLocation::advance(char const *text, int length)
{
  char const *p = text;
  char const *endp = text + length;
  for (; p < endp; p++) {
    if (*p == '\n') {      // new line
      line++;
      col = 1;
    }
    else {	      	   // ordinary character
      col++;
    }
  }
}


// -------------------- Lexer1Token -----------------------------
Lexer1Token::Lexer1Token(Lexer1TokenType aType, char const *aText,
               	         int aLength, FileLocation const &aLoc)
  : type(aType),
    text(aText, aLength),      // makes a copy 
    length(aLength),
    loc(aLoc)
{}

Lexer1Token::~Lexer1Token()
{
  // 'text' deallocates its string
}


// map Lexer1TokenType to a string
char const *l1Tok2String(Lexer1TokenType tok)
{
  char const *map[] = {
    "L1_IDENTIFIER",
    "L1_INT_LITERAL",
    "L1_FLOAT_LITERAL",
    "L1_STRING_LITERAL",
    "L1_CHAR_LITERAL",
    "L1_OPERATOR",
    "L1_PREPROCESSOR",
    "L1_WHITESPACE",
    "L1_COMMENT",
    "L1_ILLEGAL"
  };
  assert(TABLESIZE(map) == NUM_L1_TOKENS);

  assert(tok >= L1_IDENTIFIER && tok < NUM_L1_TOKENS);
  return map[tok];
}


void Lexer1Token::print() const
{
  printf("[L1] Token at line %d, col %d: %s \"%s\"\n",
         loc.line, loc.col, l1Tok2String(type),
         encodeWithEscapes(text, length).pcharc());
}


// -------------------- Lexer1 -----------------------------
Lexer1::Lexer1()
  : loc(),
    errors(0),
    tokens(),
    tokensMut(tokens)
{}

Lexer1::~Lexer1()
{
  // tokens list is deallocated
}


// eventually I want this to store the errors in a list of objects...
void Lexer1::error(char const *msg)
{
  printf("[L1] Error at line %d, col %d: %s\n", loc.line, loc.col, msg);
  errors++;
}


void Lexer1::emit(Lexer1TokenType toktype, char const *tokenText, int length)
{
  // construct object to represent this token
  Lexer1Token *tok = new Lexer1Token(toktype, tokenText, length, loc);

  // (debugging) print it
  if (tracingSys("lexer1")) {
    tok->print();
  }

  // illegal tokens should be noted
  if (toktype == L1_ILLEGAL) {
    error(stringb("illegal token: `" << tokenText << "'"));
  }

  // add it to our running list of tokens
  tokensMut.append(tok);

  // update line and column counters
  loc.advance(tokenText, length);
}


// ------------------- testing -----------------------
#ifdef TEST_LEXER1

int main(int argc, char **argv)
{
  while (traceProcessArg(argc, argv)) {}

  Lexer1 lexer;
  lexer1_lex(lexer, stdin);

  printf("%d token(s), %d error(s)\n",
         lexer.tokens.count(), lexer.errors);

  return 0;
}

#endif // TEST_LEXER1


