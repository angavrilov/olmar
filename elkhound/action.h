// action.h
// actions that may be associated with grammar productions

#ifndef __ACTION_H
#define __ACTION_H

#include <iostream.h>     // ostream
#include "attrexpr.h"     // attribute expressions


// since I envision several possible kinds of actions, this
// serves as the basic interface to an action
class Action {
public:
  // an action is fired when a production is used to create a
  // parse tree node; the Reduction is that node
  virtual void fire(AttrContext &actx) const = 0;

  // check for reference legality; throw exception if not
  virtual void check(Production const *ctx) const = 0;

  // print something to represent this action; ideally, it is
  // syntax that could be parsed to produce this action again,
  // e.g. "a = (+ b c)"
  virtual string toString(Production const *prod) const = 0;

  // to allow internal destruction
  virtual ~Action();
};


// the particular action of interest first is attribute assignments
class AttrAction : public Action {
public:	  // data
  // attribute whose value will be changed/assigned
  AttrLvalue lvalue;

  // source of the value to put there
  AExprNode *expr;           // (owner)

public:	  // funcs
  AttrAction(AttrLvalue const &L, AExprNode *e)
    : lvalue(L), expr(e) {}
  ~AttrAction();

  // Action stuff
  virtual void fire(AttrContext &actx) const;
  virtual void check(Production const *ctx) const;
  virtual string toString(Production const *prod) const;
};


// a collection of actions associated with a production
class Actions {
public:	  // data
  ObjList<Action> actions;

public:	  // funcs
  Actions();
  ~Actions();

  Actions(Flatten&);
  void xfer(Flatten &flat);

  // fire all actions on an instantiated production
  void fire(AttrContext &actx) const;
  
  // check all actions for integrity
  void check(Production const *ctx) const;

  // print all the actions in syntax suitable for parsing, e.g.
  //   %action { a = b }
  //   %action { c = (+ d e) }
  string toString(Production const *prod) const;

  // parse an action string (the stuff inside "%action {...}") in the
  // context of the given production, and add the action to the list;
  // throw an exception on a parsing error
  void parse(Production const *prod, char const *actionsText);
  
  // find an action that sets the named attribute; return
  // NULL if none do
  Action const *getAttrActionFor(char const *attr) const;
};


#endif // __ACTION_H
