// baselexer.cc            see license.txt for copyright and terms of use
// code for baselexer.h

#include "baselexer.h"   // this module
#include "strtable.h"    // StringTable
#include "exc.h"         // throw_XOpen

#include <fstream.h>     // ifstream


// this function effectively lets me initialize one of the
// members before initing a base class
istream *BaseLexer::openFile(char const *fname)
{
  this->inputStream = new ifstream(fname);
  if (!*inputStream) {
    throw_XOpen(fname);
  }
  return inputStream;
}

BaseLexer::BaseLexer(StringTable &s, char const *fname)
  : yyFlexLexer(openFile(fname)),

    // 'inputStream' is initialized by 'openFile'
    srcFile(NULL),           // changed below

    nextLoc(SL_UNKNOWN),     // changed below
    curLine(1),

    strtable(s),
    errors(0)
{
  srcFile = sourceLocManager->getInternalFile(fname);

  loc = sourceLocManager->encodeBegin(fname);
  nextLoc = loc;
}


BaseLexer::~BaseLexer()
{
  delete inputStream;
}


StringRef BaseLexer::addString(char *str, int len)
{
  // (copied from gramlex.cc, GrammarBaseLexer::addString)

  // write a null terminator temporarily
  char wasThere = str[len];
  if (wasThere) {
    str[len] = 0;
    StringRef ret = strtable.add(str);
    str[len] = wasThere;
    return ret;
  }
  else {
    return strtable.add(str);
  }
}


void BaseLexer::whitespace()
{
  updLoc();

  // scan for newlines
  char *p = yytext, *endp = yytext+yyleng;
  for (; p < endp; p++) {
    if (*p == '\n') {
      curLine++;
    }
  }
}


int BaseLexer::tok(int t)
{
  updLoc();
  sval = NULL_SVAL;     // catch mistaken uses of 'sval' for single-spelling tokens
  return t;
}


void BaseLexer::err(char const *msg)
{
  errors++;
  cerr << toString(loc) << ": error: " << msg << endl;
}


STATICDEF void BaseLexer::tokenFunc(LexerInterface *lex)
{
  BaseLexer *ths = static_cast<BaseLexer*>(lex);

  // call into the flex lexer; this updates 'loc' and sets
  // 'sval' as appropriate
  ths->type = ths->yylex();
}


BaseLexer::NextTokenFunc BaseLexer::getTokenFunc() const
{
  return &BaseLexer::tokenFunc;
}


// EOF
