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


string ArithLexer::tokenDesc() const
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

  if (type == TOK_NUMBER) {
    return stringc << "number(" << (int)sval << ")";
  }
  else {
    unsigned tableSize = sizeof(names) / sizeof(names[0]);
    assert((unsigned)type < tableSize);
    return string(names[type]);
  }
}


// --------------------- main ----------------------
ArithLexer lexer;

int main()
{
  // initialize lexer by grabbing first token
  lexer.nextToken(&lexer);
  
  // create parser; actions and tables not dealloc'd but who cares
  GLR glr(makeUserActions(), make_Arith_tables());
  
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
