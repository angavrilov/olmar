// attrexpr.h
// expression tree for attribute expressions

#ifndef __ATTREXPR_H
#define __ATTREXPR_H

#include "str.h"          // string
#include "objlist.h"      // ObjList

class Reduction;          // glrtree.h
class AttrContext;        // glrtree.h
class Production;         // grammar.h
class Attributes;         // attr.h


// we need a way to name an attribute, within the context of
// some production
class AttrLvalue {
public:	     // data
  int symbolIndex;      // 0 means LHS symbol, 1 is first RHS symbol, 2 is next, etc.
  string attrName;      // name of attribute to change

private:     // funcs
  Attributes const &getNamedAttrC(AttrContext const &actx) const;
  Attributes &getNamedAttr(AttrContext &actx) const
    { return const_cast<Attributes&>(getNamedAttrC(actx)); }

public:	     // funcs
  AttrLvalue(int s, string a)
    : symbolIndex(s), attrName(a) {}
  AttrLvalue(AttrLvalue const &obj)
    : symbolIndex(obj.symbolIndex), attrName(obj.attrName) {}
  ~AttrLvalue();

  // create an AttrLvalue by parsing a textual reference; throw
  // exception on failure
  static AttrLvalue parseRef(Production const *prod, char const *refString);

  // return the attribute as it would have been supplied on input,
  // e.g. Expr.left
  string toString(Production const *prod) const;

  // given an instantiated production, yield the referred value
  int getFrom(AttrContext const &actx) const;

  // store a new value
  // (this fn is "const", meaning it does not modify the ref itself,
  // only the thing referred-to)
  void storeInto(AttrContext &actx, int newValue) const;
};


// I want to allow for some fairly sophisticated attribute functions,
// so I define an attribute expression tree hierarchy; this first one
// is the interface of a tree node
class AExprNode {
public:
  // evaluate the expression and return its value, within the
  // context of an instantiated production
  virtual int eval(AttrContext const &actx) const = 0;

  // print this node in syntax that might be parseable, e.g. "(+ a b)"
  virtual string toString(Production const *prod) const = 0;

  // delete children (if any)
  virtual ~AExprNode();
};


// literal
class AExprLiteral : public AExprNode {
  int value;   	       	// literal value

public:
  AExprLiteral(int v) : value(v) {}

  // AExprNode stuff
  virtual int eval(AttrContext const &actx) const;
  virtual string toString(Production const *prod) const;
};


// reference to an existing attribute value
class AExprAttrRef : public AExprNode {
  AttrLvalue ref;	// reference to an attribute

public:
  AExprAttrRef(AttrLvalue const &r) : ref(r) {}
  virtual ~AExprAttrRef();

  // AExprNode stuff
  virtual int eval(AttrContext const &actx) const;
  virtual string toString(Production const *prod) const;
};



// function of several subexpressions; includes infix ops like addition
class AExprFunc : public AExprNode {
public:
  // all functions
  typedef int (*Func)(...);        // allow any number of integer arguments here
  struct FuncEntry {   	       	   // one per predefined func
    char const *name;                // name of fn (e.g. "+")
    int numArgs;                     // # of arguments it requires
    Func eval;                       // code to evaluate
  };
  static FuncEntry const funcEntries[];         // all predefined fns
  static int const numFuncEntries;              // size of this array

  // this function
  FuncEntry const *func;           // (serf) how to evaluate this fn
  ObjList<AExprNode> args;         // arguments to that function, in order

public:
  AExprFunc(char const *name);     // the name must match something predefined
  virtual ~AExprFunc();

  // AExprNode stuff
  virtual int eval(AttrContext const &actx) const;
  virtual string toString(Production const *prod) const;
};


// simple parser for expressions, in context of given production;
// throw exception on failure
// (returns an owner pointer)
AExprNode *parseAExpr(Production const *prod, char const *text);


#endif // __ATTREXPR_H
