// grampar.cc
// additional C++ code for the grammar parser

#include "grampar.h"     // this module
#include "gramlex.h"     // GrammarLexer
#include "trace.h"       // tracing debug functions
#include "gramast.h"     // grammar AST nodes

// Bison parser calls this to get a token
GrammarLexer lexer;
int yylex()
{
  int code = lexer.yylex();
  trace("yylex") << "yielding token (" << code << ") "
                 << lexer.curToken() << " at "
                 << lexer.curLoc() << endl;

  // yield semantic values for some things
  switch (code) {
    case TOK_INTEGER:
      yylval = new ASTIntLeaf(lexer.integerLiteral);
      break;

    case TOK_STRING:
      yylval = new ASTStringLeaf(lexer.stringLiteral);
      break;

    case TOK_NAME:
      yylval = new ASTNameLeaf(lexer.curToken());
      break;

    default:
      yylval = NULL;
  }

  return code;
}


void yyerror(char const *message)
{
  cout << message << " at " << lexer.curLoc() << endl;
}


#ifdef TEST_GRAMPAR

int main(int argc, char **argv)
{
  TRACE_ARGS();

  ASTNode::typeToString = astTypeToString;

  cout << "go!\n";

  if (yyparse() == 0) {
    cout << "parsing finished successfully.\n";
    return 0;
  }
  else {
    cout << "parsing finished with an error.\n";
    return 1;
  }
}

#endif // TEST_GRAMPAR
