// action.cc
// code for action.h

#include "action.h"       // this module
#include "util.h"         // transferOwnership

#include <string.h>       // strchr


// ---------------------- Action --------------------
Action::~Action()
{}


// ---------------------- AttrAction --------------------
AttrAction::~AttrAction()
{}


void AttrAction::fire(AttrContext &actx) const
{
  // evaluate the expression
  int val = expr->eval(actx);

  // store it in the right place
  lvalue.storeInto(actx, val);
}


string AttrAction::toString(Production const *prod) const
{
  return stringc << lvalue.toString(prod) << " := "
                 << expr->toString(prod);
}


// ---------------------- Actions --------------------
Actions::Actions()
{}

Actions::~Actions()
{}


void Actions::fire(AttrContext &actx) const
{
  FOREACH_OBJLIST(Action, actions, iter) {
    iter.data()->fire(actx);
  }
}


string Actions::toString(Production const *prod) const
{
  stringBuilder sb;
  FOREACH_OBJLIST(Action, actions, iter) {
    sb << "  %action { " << iter.data()->toString(prod) << " }\n";
  }
  return sb;
}


void Actions::parse(Production const *prod, char const *actionsText)
{
  // parse as <lvalue> := <expr>
  char const *colon = strstr(actionsText, ":=");
  if (!colon) {
    xfailure("action must contain the `:=' sequence\n");
  }

  // lvalue
  string lvalText(actionsText, colon-actionsText);
  AttrLvalue lval = AttrLvalue::parseRef(prod, trimWhitespace(lvalText));

  // expr
  AExprNode *expr = parseAExpr(prod, trimWhitespace(colon+2));    // (owner)

  // assemble into an action, stick it into the production
  actions.append(new AttrAction(lval, transferOwnership(expr)));
}
