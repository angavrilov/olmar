// lexer.cc            see license.txt for copyright and terms of use
// code for lexer.h

#include "lexer.h"       // this module
#include "strtable.h"    // StringTable
#include "cc_lang.h"     // CCLang
#include "exc.h"         // throw_XOpen

#include <fstream.h>     // ifstream


/* 
 * Note about nonseparating tokens and the 'checkForNonsep' function:
 *
 * To diagnose and report erroneous syntax like "0x5g", which would
 * naively be parsed as "0x5" and "g" (two legal tokens), I divide
 * all tokens into two classes: separating and nonseparating.
 *
 * Separating tokens are allowed to be adjacent to each other and
 * to nonseparating tokens.  An example is "(".
 *
 * Nonseparating tokens are not allowed to be adjacent to each other.
 * They must be separated by either whitespace, or at least one
 * nonseparating token.  The nonseparating tokens are identifiers,
 * alphabetic keywords, and literals.  The lexer would of course never
 * yield two adjacent keywords, due to maximal munch, but classifying 
 * such an event as an error is harmless.
 * 
 * By keeping track of whether the last token yielded is separating or
 * not, we'll see (e.g.) "0x5g" as two consecutive nonseparating tokens,
 * and can report that as an error.
 *
 * The C++ standard is rather vague on this point as far as I can
 * tell.  I haven't checked the C standard.  In the C++ standard,
 * section 2.6 paragraph 1 states:
 *
 *  "There are five kinds of tokens: identifiers, keywords, literals,
 *   operators, and other separators.  Blanks, horizontal and
 *   vertical tabs, newlines, formfeeds, and comments (collectively,
 *   "whitespace"), as described below, are ignored except as they
 *   serve to separate tokens.  [Note: Some white space is required
 *   to separate otherwise adjacent identifiers, keywords, numeric
 *   literals, and alternative tokens containing alphabetic
 *   characters.]"
 *
 * The fact that the restriction is stated only in a parenthetical note
 * is of course nonideal.  I think the qualifier "numeric" on "literals"
 * is a mistake, otherwise "a'b'" would be a legal token sequence.  I
 * do not currently implement the "alternative tokens".
 */


// -------------------- TokenType ---------------------
// these aren't emitted into cc_tokens.cc because doing so would
// make that output dependent on smbase/xassert.h
char const *toString(TokenType type)
{
  xassert((unsigned)type < (unsigned)NUM_TOKEN_TYPES);
  return tokenNameTable[type];
}

TokenFlag tokenFlags(TokenType type)
{
  xassert((unsigned)type < (unsigned)NUM_TOKEN_TYPES);
  return (TokenFlag)tokenFlagTable[type];
}


// ------------------------ Lexer -------------------
// this function effectively lets me initialize one of the
// members before initing a base class
istream *Lexer::openFile(char const *fname)
{
  this->inputStream = new ifstream(fname);
  if (!*inputStream) {
    throw_XOpen(fname);
  }
  return inputStream;
}

Lexer::Lexer(StringTable &s, CCLang &L, char const *fname)
  : yyFlexLexer(openFile(fname)),

    // 'inputStream' is initialized by 'openFile'
    nextLoc(SL_UNKNOWN),     // changed below
    prevIsNonsep(false),

    strtable(s),
    lang(L),
    errors(0)
{
  loc = sourceLocManager->encodeBegin(fname);
  nextLoc = loc;

  // prime this with the first token
  tokenFunc(this);
}


Lexer::~Lexer()
{
  delete inputStream;
}


StringRef Lexer::addString(char *str, int len)
{
  // (copied from gramlex.cc, GrammarLexer::addString)

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


// this, and 'svalTok', are out of line because I don't want the
// yylex() function to be enormous; I want that to just have a bunch
// of calls into these routines, which themselves can then have
// plenty of things inlined into them
int Lexer::tok(TokenType t)
{
  checkForNonsep(t);
  updLoc();
  sval = NULL_SVAL;     // catch mistaken uses of 'sval' for single-spelling tokens
  return t;
}


int Lexer::svalTok(TokenType t) 
{
  checkForNonsep(t);
  updLoc();
  sval = (SemanticValue)addString(yytext, yyleng);
  return t;
}

  
void Lexer::err(char const *msg)
{
  errors++;
  cerr << toString(loc) << ": error: " << msg << endl;
}


STATICDEF void Lexer::tokenFunc(LexerInterface *lex)
{
  Lexer *ths = static_cast<Lexer*>(lex);
  
  // call into the flex lexer; this updates 'loc' and sets
  // 'sval' as appropriate
  ths->type = ths->yylex();
}


Lexer::NextTokenFunc Lexer::getTokenFunc() const
{
  return &Lexer::tokenFunc;
}

string Lexer::tokenDesc() const
{
  if (tokenFlags((TokenType)type) & TF_MULTISPELL) {
    // for tokens with multiple spellings, decode 'sval' as a
    // StringRef
    return string((StringRef)sval);
  }
  else {
    // for all others, consult the static table
    return string(toString(type));
  }
}






