// useract.cc
// code for useract.h

#include "useract.h"     // this module
#include "typ.h"         // STATICDEF

#include <stdlib.h>      // NULL

UserActions::~UserActions()
{}


// ----------------- TrivialUserActions --------------------
UserActions::ReductionActionFunc TrivialUserActions::getReductionAction()
{
  return &TrivialUserActions::doReductionAction;
}

STATICDEF SemanticValue TrivialUserActions::doReductionAction(
  UserActions *, int , SemanticValue const *
  SOURCELOCARG( SourceLocation const & ) )
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


UserActions::ReclassifyFunc TrivialUserActions::getReclassifier()
{
  return &TrivialUserActions::reclassifyToken;
}

STATICDEF int TrivialUserActions::reclassifyToken(UserActions *,
  int oldTokenType, SemanticValue )
  { return oldTokenType; }
