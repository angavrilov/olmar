// arith.cc
// driver program for arithmetic evaluator
         
#include "arith.h"     // this module
#include "glr.h"       // GLR parser

#include <assert.h>    // assert


// ------------------ ArithLexer ------------------
/*static*/ void ArithLexer::nextToken(ArithLexer *ths)
{
  // call underlying lexer; it will set 'sval' if necessary
  ths->type = yylex();
}

LexerInterface::NextTokenFunc ArithLexer::getTokenFunc() const
{
  return (NextTokenFunc)&ArithLexer::nextToken;
}

  
char const *toString(ArithTokenCodes code)
{
  char const * const names[] = {
    "EOF",
    "number",
    "+",
    "-",
    "*",
    "/",
    "(",
    ")",
  };

  unsigned tableSize = sizeof(names) / sizeof(names[0]);
  assert((unsigned)code < tableSize);
  return names[code];
}

string ArithLexer::tokenDesc() const
{
  if (type == TOK_NUMBER) {
    return stringc << "number(" << (int)sval << ")";
  }
  else {
    return toString((ArithTokenCodes)type);
  }
}

string ArithLexer::tokenKindDesc(int kind) const
{
  return toString((ArithTokenCodes)kind);
}


// --------------------- main ----------------------
ArithLexer lexer;

int main()
{
  // initialize lexer by grabbing first token
  lexer.nextToken(&lexer);
  
  // create parser; actions and tables not dealloc'd but who cares
  Arith *arith = new Arith;
  GLR glr(arith, arith->makeTables());
  
  // start parsing         
  SemanticValue result;
  if (!glr.glrParse(lexer, result)) {
    printf("parse error\n");
    return 2;
  }
  
  // print result
  printf("result: %d\n", (int)result);
  
  return 0;
}
