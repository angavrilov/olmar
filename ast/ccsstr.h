// ccsstr.h            see license.txt for copyright and terms of use
// C++ substrate for my parser

#ifndef __CCSSTR_H
#define __CCSSTR_H

#include "embedded.h"      // EmbeddedLang

class CCSubstrate : public EmbeddedLang {
public:
  // these are public so the test code can access them,
  // and because it doesn't hurt much
  enum State {
    ST_NORMAL,       // normal text
    ST_STRING,       // inside a string literal
    ST_CHAR,         // inside a char literal
    ST_SLASH,        // from ST_NORMAL, just saw a slash
    ST_C_COMMENT,    // inside a C comment
    ST_CC_COMMENT,   // inside a C++ comment
    NUM_STATES
  } state;
  int nesting;       // depth of paren/bracket/brace nesting
  bool backslash;    // in ST_{STRING,CHAR}, just seen backslash?
  bool star;         // in ST_C_COMMENT, just seen '*'?

public:
  CCSubstrate(ReportError &err);
  virtual ~CCSubstrate();

  // EmbeddedLang entry points (see gramlex.h for description
  // of each function)
  virtual void reset(int initNest = 0);
  virtual void handle(char const *str, int len, char finalDelim);
  virtual bool zeroNesting() const;
  virtual string getFuncBody() const;
  virtual string getDeclName() const;
};

#endif // __CCSSTR_H
