// ocsstr.h            see license.txt for copyright and terms of use
// Ocaml substrate for the ast file parser

// This only implements a limited ocaml grammar, which is sufficient
// for type declarations. It supports normal ocaml text and comments.
// However, it does not support
// - strings
// - strings in comments
// - character constants

#ifndef OCSSTR_H
#define OCSSTR_H

#include "embedded.h"      // EmbeddedLang

class OCSubstrateTest;

class OCSubstrate : public EmbeddedLang {
private:
  enum State {
    ST_NORMAL,       // normal text for comment_nesting == 0
                     // comment for comment_nesting > 0
    ST_TICK,         // after single tick ('), expecting a type variable
    ST_TICK_NEXT,    // after tick and either one letter or one white space
    NUM_STATES
  } state;
  int nesting;       // depth of paren/bracket/brace nesting
  int comment_nesting; // comment nesting depth
  // bool backslash;    // in ST_{STRING,CHAR}, just seen backslash?
  bool star;         // in ST_COMMENT, just seen '*'?
  bool oparen;       // in ST_NORMAL/ST_COMMENT, just seen '('?

  // only implemented in the test frame 
  // if TEST_OCSSTR is on
  void print_state();

  // so test code can interrogate internal state
  friend class OCSubstrateTest;

public:
  OCSubstrate(ReportError *err = NULL);
  virtual ~OCSubstrate();

  // EmbeddedLang entry points (see embedded.h for description
  // of each function)
  virtual void reset(int initNest = 0);
  virtual void handle(char const *str, int len, char finalDelim);
  virtual bool zeroNesting() const;
  virtual string getFuncBody() const; // not supported
  virtual string getDeclName() const; // not supported
};

#endif // OCSSTR_H
