// cond.h
// conditions that must be true for a grammar rule to apply

#ifndef __COND_H
#define __COND_H

#include "attrexpr.h"       // AExprNode


// the interface to a condition
class Condition {
public:
  // test the condition; a true return value means the
  // condition is satisfied
  virtual bool test(AttrContext const &actx) const = 0;

  // check for reference integrity
  virtual void check(Production const *ctx) const = 0;

  // print the condition in a parseable format, e.g. "(< a b)"
  virtual string toString(Production const *prod) const = 0;

  virtual ~Condition();
};


// first condition kind: boolean expression over attribute values;
// I'm going to use ints with the convention that 0=false and 1=true
class ExprCondition : public Condition {
  AExprNode *expr;               // (owner) expression to evaluate

public:
  ExprCondition(AExprNode *e) : expr(e) {}
  virtual ~ExprCondition();

  // Condition stuff
  virtual bool test(AttrContext const &actx) const;
  virtual void check(Production const *ctx) const;
  virtual string toString(Production const *prod) const;
};


// (possibly empty) collection of conditions
// (question of whether to derive this from Condition is still open)
class Conditions {
public:
  ObjList<Condition> conditions;

public:
  Conditions();
  ~Conditions();

  // test all conditions
  bool test(AttrContext const &actx) const;

  // check all conditions for referential integrity
  void check(Production const *ctx) const;

  // print all conditions in parseable format, e.g.:
  //   %condition { (< a b) }
  //   %condition { (== c d) }
  string toString(Production const *prod) const;

  // parse a condition string (the stuff inside "%condition {...}") in
  // the context of the given production, and add the condition to the
  // list; throw an exception on a parsing error
  void parse(Production const *prod, char const *conditionText);
};


#endif // __COND_H
