// iptparse.cc
// (some) code for iptparse.h

#include "iptparse.h"      // this module
#include "iptree.h"        // IPTree
#include "autofile.h"      // AutoFILE
#include "exc.h"           // xfatal


TokenType lookahead = TOK_EOF;


// get the next token and stash it in 'lookahead'
void getToken()
{
  lookahead = getNextToken();
}
 

// string representation of current token
string tokenToString()
{
  if (lookahead == TOK_INTLIT) {
    return stringc << "intlit(" << lexerSval << ")";
  }
  else {
    static char const * const map[] = {
      "EOF",
      "INTLIT",
      "COMMA",
      "SEMICOLON"
    };

    xassert(TABLESIZE(map) == NUM_TOKENTYPES);
    return string(map[lookahead]);
  }
}


// report parse error at current token
void parseError()
{
  xfatal("parse error at " << tokenToString());
}


// get token and expect it to be a specific kind
void expect(TokenType t)
{
  getToken();
  if (lookahead != t) {
    parseError();
  }
}


void parseBracePair(IPTree &dest)
{
  xassert(lookahead == TOK_INTLIT);
  int lo = lexerSval;
  
  expect(TOK_COMMA);

  expect(TOK_INTLIT);
  int hi = lexerSval;
  
  expect(TOK_SEMICOLON);
  
  dest.insert(lo, hi);
}


void parseFile(IPTree &dest, rostring fname)
{
  AutoFILE fp(toCStr(fname), "r");
  yyrestart(fp);

  for (;;) {
    getToken();
    switch (lookahead) {
      case TOK_EOF:
        return;

      case TOK_INTLIT:
        parseBracePair(dest);
        break;

      default:
        parseError();
    }
  }
}


// EOF
