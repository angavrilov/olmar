// gramlex.cc
// code for gramlex.h

#include "gramlex.h"     // this module
#include <iostream.h>    // cout


GrammarLexer::GrammarLexer()
  : yyFlexLexer(),
    line(firstLine),
    column(firstColumn),
    commentStartLine(0),
    integerLiteral(0),
    stringLiteral(""),
    includeFileName("")
{}

GrammarLexer::~GrammarLexer()
{}


void GrammarLexer::newLine()
{
  line++;
  column = firstColumn;
}


string GrammarLexer::curToken() const
{
  // hack around bad decl in FlexLexer.h
  GrammarLexer *ths = const_cast<GrammarLexer*>(this);

  return string(ths->YYText(), ths->YYLeng());
}


int GrammarLexer::curCol() const
{
  // we want to report the *start* column, not the column
  // after the last character
  return column - curLen();
}


string GrammarLexer::curLoc() const
{
  return stringc << "line " << curLine() << ", col " << curCol();
}


void GrammarLexer::err(char const *msg)
{
  cerr << "lexer error at " << curLoc() << ": " << msg << endl;
}


void GrammarLexer::errorUnterminatedComment()
{
  err(stringc << "unterminated comment, beginning on line " << commentStartLine);
}

void GrammarLexer::errorMalformedInclude()
{
  err(stringc << "malformed #include");
}

void GrammarLexer::errorIllegalCharacter(char ch)
{
  err(stringc << "illegal character: `" << ch << "'");
}


#ifdef TEST_GRAMLEX

int main()
{
  GrammarLexer lexer;

  cout << "go!\n";

  while (1) {
    int code = lexer.yylex();
    if (code == 0) {  // eof
      break;
    }

    if (code != TOK_INCLUDE) {
      cout << "token at " << lexer.curLoc()
           << ": code=" << code
           << ", text: " << lexer.curToken().pcharc()
           << endl;
    }
    else {
      cout << "include at " << lexer.curLoc()
           << ": filename is \"" << lexer.includeFileName.pcharc()
           << "\"\n";
    }
  }

  return 0;
}

#endif // TEST_GRAMLEX
