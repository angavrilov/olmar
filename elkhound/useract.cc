// useract.cc
// code for useract.h

#include "useract.h"     // this module

#include <stdlib.h>      // NULL

UserActions::~UserActions()
{}


// ----------------- TrivialUserActions --------------------
SemanticValue TrivialUserActions::doReductionAction(
  int , SemanticValue const *, SourceLocation const &)
  { return NULL; }

SemanticValue TrivialUserActions::duplicateTerminalValue(
  int , SemanticValue sval)
  { return sval; }

SemanticValue TrivialUserActions::duplicateNontermValue(
  int , SemanticValue sval)
  { return sval; }


void TrivialUserActions::deallocateTerminalValue(
  int , SemanticValue )
  {}

void TrivialUserActions::deallocateNontermValue(
  int , SemanticValue )
  {}

SemanticValue TrivialUserActions::mergeAlternativeParses(
  int , SemanticValue left, SemanticValue )
  { return left; }

bool TrivialUserActions::keepNontermValue(int , SemanticValue )
  { return true; }     // do not cancel

int TrivialUserActions::reclassifyToken(int oldTokenType, SemanticValue )
  { return oldTokenType; }
