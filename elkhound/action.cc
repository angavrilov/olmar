// action.cc
// code for action.h

#include "action.h"       // this module
#include "util.h"         // transferOwnership
#include "strutil.h"      // trimWhitespace
#include "exc.h"          // xBase
#include "flatutil.h"     // xfer helpers

#include <string.h>       // strchr


// ---------------------- Action --------------------
Action::~Action()
{}


// ---------------------- AttrAction --------------------
AttrAction::~AttrAction()
{}


AttrAction::AttrAction(Flatten &flat)
  : lvalue(flat),
    expr(NULL)
{}

void AttrAction::xfer(Flatten &flat)
{
  lvalue.xfer(flat);
  xferOwnerPtr_readObj(flat, expr);
}



void AttrAction::fire(AttrContext &actx) const
{
  // evaluate the expression
  int val = expr->eval(actx);

  // store it in the right place
  lvalue.storeInto(actx, val);
}


void AttrAction::check(Production const *ctx) const
{
  lvalue.check(ctx);
  expr->check(ctx);
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


Actions::Actions(Flatten&)
{}

void Actions::xfer(Flatten &flat)
{
  xferObjList(flat, actions);
}


void Actions::fire(AttrContext &actx) const
{
  FOREACH_OBJLIST(AttrAction, actions, iter) {
    iter.data()->fire(actx);
  }
}


void Actions::check(Production const *ctx) const
{
  FOREACH_OBJLIST(AttrAction, actions, iter) {
    try {
      iter.data()->check(ctx);
    }
    catch (xBase &x) {
      THROW(xBase(stringc << "in " << iter.data()->toString(ctx)
                          << ", " << x.why() ));
    }
  }
}


string Actions::toString(Production const *prod) const
{
  stringBuilder sb;
  FOREACH_OBJLIST(AttrAction, actions, iter) {
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


AttrAction const *Actions::getAttrActionFor(char const *attr) const
{
  FOREACH_OBJLIST(AttrAction, actions, iter) {
    AttrAction const *aa = iter.data();

    if (aa->lvalue.symbolIndex == 0 &&            // LHS
        0==strcmp(aa->lvalue.attrName, attr)) {   // matching name
      // found it
      return aa;
    }
  }

  return NULL;
}
