// grampar.cc
// additional C++ code for the grammar parser

#include "grampar.h"     // this module
#include "gramlex.h"     // GrammarLexer
#include "trace.h"       // tracing debug functions
#include "gramast.h"     // grammar AST nodes

// Bison parser calls this to get a token
GrammarLexer lexer;
int yylex(ASTNode **lvalp)
{
  int code = lexer.yylex();
  trace("yylex") << "yielding token (" << code << ") "
                 << lexer.curToken() << " at "
                 << lexer.curLoc() << endl;

  // yield semantic values for some things
  switch (code) {
    case TOK_INTEGER:
      *lvalp = new ASTIntLeaf(lexer.integerLiteral);
      break;

    case TOK_STRING:
      *lvalp = new ASTStringLeaf(lexer.stringLiteral);
      break;

    case TOK_NAME:
      *lvalp = new ASTNameLeaf(lexer.curToken());
      break;

    default:
      *lvalp = NULL;
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

  ParseParams params;
  if (yyparse(&params) == 0) {
    cout << "parsing finished successfully.\n";
                                        
    cout << "AST:\n";
    params.treeTop->debugPrint(cout, 2);
    delete params.treeTop;

    cout << "leaked " << ASTNode::nodeCount << " AST nodes\n";
    return 0;
  }
  else {
    cout << "parsing finished with an error.\n";
    return 1;
  }
}

#endif // TEST_GRAMPAR
