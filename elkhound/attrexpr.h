// attrexpr.h
// expression tree for attribute expressions

#error This module is obsolete.

#ifndef __ATTREXPR_H
#define __ATTREXPR_H

#include "str.h"          // string
#include "objlist.h"      // ObjList
#include "attr.h"         // Attributes, AttrValue, AttrName
#include "macros.h"       // DMEMB

class Reduction;          // glrtree.h
class AttrContext;        // glrtree.h
class Production;         // grammar.h
class Attributes;         // attr.h
class Flatten;            // flatten.h


// we need a way to name an attribute, within the context of
// some production
class AttrLvalue {
public:	     // data
  int whichTree;        // which tree, if several are possible; 0 is typical
  int symbolIndex;      // 0 means LHS symbol, 1 is first RHS symbol, 2 is next, etc.
  AttrName attrName;    // name of attribute to change

public:	     // funcs
  AttrLvalue(int w, int s, AttrName a)
    : whichTree(w), symbolIndex(s), attrName(a) {}
  AttrLvalue(AttrLvalue const &obj)
    : DMEMB(whichTree), DMEMB(symbolIndex), DMEMB(attrName) {}
  ~AttrLvalue();

  AttrLvalue(Flatten&);
  void xfer(Flatten &flat);

  // create an AttrLvalue by parsing a textual reference; throw
  // exception on failure
  static AttrLvalue parseRef(Production const *prod, char const *refString);

  // return the attribute as it would have been supplied on input,
  // e.g. Expr.left
  string toString(Production const *prod) const;

  // check that the lvalue is a valid reference in the
  // context of the given production; throw exception if not
  void check(Production const *ctx) const;

  // given an instantiated production, yield the referred value
  AttrValue getFrom(AttrContext const &actx) const;

  // store a new value
  // (this fn is "const", meaning it does not modify the ref itself,
  // only the thing referred-to)
  void storeInto(AttrContext &actx, AttrValue newValue) const;
};


// I want to allow for some fairly sophisticated attribute functions,
// so I define an attribute expression tree hierarchy; this first one
// is the interface of a tree node
class AExprNode {
public:     // types
  enum Tag { T_LITERAL, T_ATTRREF, T_FUNC, NUM_TAGS };

public:
  AExprNode() {}
  virtual ~AExprNode();

  // get tag for the object; needed for flatten/unflatten
  virtual Tag getTag() const = 0;

  // evaluate the expression and return its value, within the
  // context of an instantiated production
  virtual int eval(AttrContext const &actx) const = 0;

  // check the expression for illegal references; throw an exception
  // if any are found (default impl. just returns)
  virtual void check(Production const *ctx) const;

  // print this node in syntax that might be parseable, e.g. "(+ a b)"
  virtual string toString(Production const *prod) const = 0;

  // read/write
  AExprNode(Flatten&) {}
  virtual void xfer(Flatten &flat);    // call me from derived
  static AExprNode *readObj(Flatten &flat);
};


// literal
class AExprLiteral : public AExprNode {
  int value;   	       	// literal value

public:
  AExprLiteral(int v) : value(v) {}

  AExprLiteral(Flatten&);
  virtual void xfer(Flatten &flat);

  // AExprNode stuff
  virtual Tag getTag() const { return T_LITERAL; }
  virtual int eval(AttrContext const &actx) const;
  virtual string toString(Production const *prod) const;
};


// reference to an existing attribute value
class AExprAttrRef : public AExprNode {
  AttrLvalue ref;	// reference to an attribute

public:
  AExprAttrRef(AttrLvalue const &r) : ref(r) {}
  virtual ~AExprAttrRef();

  AExprAttrRef(Flatten&);
  virtual void xfer(Flatten &flat);

  // AExprNode stuff
  virtual Tag getTag() const { return T_ATTRREF; }
  virtual int eval(AttrContext const &actx) const;
  virtual void check(Production const *ctx) const;
  virtual string toString(Production const *prod) const;
};



// function of several subexpressions; includes infix ops like addition
class AExprFunc : public AExprNode {
public:
  // all functions
  typedef int (*Func)(...);        // allow any number of integer arguments here
  struct FuncEntry {   	       	   // one per predefined func
    char const *name;                // name of fn (e.g. "+")
    int astTypeCode;                 // ast node type that represents this fn (see gramast.h)
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

  AExprFunc(Flatten&);
  virtual void xfer(Flatten &flat);

  static char const *lookupFunc(int code);

  // AExprNode stuff
  virtual Tag getTag() const { return T_FUNC; }
  virtual int eval(AttrContext const &actx) const;
  virtual void check(Production const *ctx) const;
  virtual string toString(Production const *prod) const;
};


// simple parser for expressions, in context of given production;
// throw exception on failure
// (returns an owner pointer)
AExprNode *parseAExpr(Production const *prod, char const *text);


#endif // __ATTREXPR_H
