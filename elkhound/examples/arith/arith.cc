// arith.cc
// driver program for arithmetic evaluator
         
#include "arith.h"     // this module
#include "glr.h"       // GLR parser


// ------------------ ArithLexer ------------------
void ArithLexer::nextToken()
{                     
  // call underlying lexer; it will set 'sval' if necessary
  type = yylex();
}


string ArithLexer::tokenDesc()
{
  return tokenDescType(type);
}


string ArithLexer::tokenDescType(int t)
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

  if (t == TOK_NUMBER) {
    return stringc << "number(" << (int)sval << ")";
  }
  else {
    xassert((unsigned)t < (unsigned)TABLESIZE(names));
    return string(names[t]);
  }
}


// --------------------- main ----------------------
ArithLexer lexer;

int main()
{
  // initialize lexer by grabbing first token
  lexer.nextToken();
  
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
