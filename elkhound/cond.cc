// cond.cc
// code for cond.h

#include "cond.h"      // this module
#include "trace.h"     // trace
#include "glrtree.h"   // AttrContext
#include "util.h"      // TVAL
#include "exc.h"       // xBase
#include "flatutil.h"  // xferOwnerPtr_readObj


// ----------------------- Condition --------------------
Condition::~Condition()
{}


// --------------------- ExprCondition ---------------------
ExprCondition::~ExprCondition()
{
  delete expr;
}


ExprCondition::ExprCondition(Flatten&)
  : expr(NULL)
{}

void ExprCondition::xfer(Flatten &flat)
{
  xferOwnerPtr_readObj(flat, expr);
}


bool ExprCondition::test(AttrContext const &actx) const
{
  int val = expr->eval(actx);
  if (val != 0 && val != 1) {
    xfailure(stringb("ExprCondition value must be 0 or 1 (was: " << val << ")"));
  }

  return val==1;
}


void ExprCondition::check(Production const *ctx) const
{
  expr->check(ctx);
}

string ExprCondition::toString(Production const *prod) const
{
  return expr->toString(prod);
}


// -------------------- Conditions -------------------------
Conditions::Conditions()
{}

Conditions::~Conditions()
{}


Conditions::Conditions(Flatten&)
{}

void Conditions::xfer(Flatten &flat)
{
  xferObjList(flat, conditions);
}


bool Conditions::test(AttrContext const &actx) const
{
  FOREACH_OBJLIST(ExprCondition, conditions, iter) {
    try {
      if (!iter.data()->test(actx)) {
	trace("conditions")
	  << "condition " << iter.data()->toString(actx.reductionC(0).production)
	  << " not satisfied\n";
	return false;
      }
    }
    catch (xBase &x) {
      cout << "condition " << iter.data()->toString(actx.reductionC(0).production)
           << " failed with exception: " << x << endl;
      return false;
    }
  }
  return true;
}


void Conditions::check(Production const *ctx) const
{
  FOREACH_OBJLIST(ExprCondition, conditions, iter) {
    try {
      iter.data()->check(ctx);
    }
    catch (xBase &x) {
      THROW(xBase(stringc << "in " << iter.data()->toString(ctx)
                          << ", " << x.why() ));
    }
  }
}


string Conditions::toString(Production const *prod) const
{
  stringBuilder sb;
  FOREACH_OBJLIST(ExprCondition, conditions, iter) {
    sb << "  %condition { " << iter.data()->toString(prod) << " }\n";
  }
  return sb;
}


void Conditions::parse(Production const *prod, char const *conditionText)
{
  TVAL(conditionText);

  // it's just an expression
  AExprNode *expr = parseAExpr(prod, conditionText);    // (owner)

  conditions.append(new ExprCondition(transferOwnership(expr)));
}
