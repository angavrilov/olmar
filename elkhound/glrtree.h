// glrtree.h
// parse graph yielded by GLR parsing algorithm

// the name of this file suggests parse tree, which is
// how it's usually thought of, but representing
// ambiguities means it is actually a graph (possibly with
// cycles too, as I understand it)

// this has been separated from glr.h because the parse
// tree really should be a separate entity from the other
// data structures used during parsing

// see glr.h for references and more info

#ifndef __GLRTREE_H
#define __GLRTREE_H

#include "grammar.h"     // Symbol, Production, etc.
#include "attr.h"        // Attributes
#include "lexer2.h"      // Lexer2, Lexer2Token
#include "owner.h"       // Owner
#include "exc.h"         // xBase


// forward decls for things declared below
class TreeNode;          // abstract parse tree (graph) node
class TerminalNode;      // tree node for terminals; derived from TreeNode
class NonterminalNode;   // tree node for nonterminals; derived from TreeNode
class Reduction;         // inside NonterminalNode, reduction choices


// abstract constituent of a parse graph
// ([GLR] calls these "tree nodes" also)
class TreeNode {
public:     // types                                        
  // called with each node; returns false to continue
  // walking or true to stop
  typedef bool (*WalkFn)(TreeNode const *node, void *extra);

public:	    // data
  // count and high-water for tree nodes
  static int numTreeNodesAllocd;
  static int maxTreeNodesAllocd;

public:	    // funcs
  TreeNode();
  virtual ~TreeNode();

  // returns the representative symbol (terminal or nonterminal)
  virtual Symbol const *getSymbolC() const = 0;

  // type determination
  virtual bool isTerm() const = 0;
  bool isNonterm() const { return !isTerm(); }

  // type-safe casts (exception thrown if bad)
  TerminalNode const &asTermC() const;
  TerminalNode &asTerm() { return const_cast<TerminalNode&>(asTermC()); }

  NonterminalNode const &asNontermC() const;
  NonterminalNode &asNonterm() { return const_cast<NonterminalNode&>(asNontermC()); }

  // attribute access
  virtual AttrValue getAttrValue(AttrName name) const = 0;
  virtual void setAttrValue(AttrName name, AttrValue value) = 0;

  // walk the tree, calling 'func' on each node encountered (in order),
  // until it returns true; returns the tree node at which 'func' retruned
  // true, or NULL if it never did
  virtual TreeNode const *walkTree(WalkFn func, void *extra=NULL) const;

  // ambiguity report
  virtual TerminalNode const *getLeftmostTerminalC() const = 0;
  virtual void ambiguityReport(ostream &os) const = 0;

  // get list of tree leaves
  virtual void getGroundTerms(SObjList<TerminalNode> &dest) const = 0;

  // simple unparse: yield string of tokens in this tree, separated by spaces
  string unparseString() const;

  // debugging
  virtual void printParseTree(ostream &os, int indent) const = 0;
  string locString() const;
  static void printAllocStats();
};


// a leaf of a parse graph: where the source language's
// symbols are referenced
// ([GLR] calls these "terminal nodes" also)
class TerminalNode : public TreeNode {
public:     // data
  // which token we're talking about
  Lexer2Token const * const token;

  // and also its class; as far as I can tell this isn't
  // really used, and it is redundant if one has the
  // Grammar to query (since token->type is the same info)
  Terminal const * const terminalClass;

public:     // funcs
  TerminalNode(Lexer2Token const *tk, Terminal const *tc);
  virtual ~TerminalNode();

  // TreeNode stuff
  virtual bool isTerm() const { return true; }
  virtual Symbol const *getSymbolC() const;
  virtual AttrValue getAttrValue(AttrName name) const;
  virtual void setAttrValue(AttrName name, AttrValue value);
  virtual TerminalNode const *getLeftmostTerminalC() const;
  virtual void ambiguityReport(ostream &os) const;
  virtual void getGroundTerms(SObjList<TerminalNode> &dest) const;
  virtual void printParseTree(ostream &os, int indent) const;
};


// an internal node in the parse graph; it has a list of all possible
// reductions (i.e. ambiguities) at this point in the parse
// ([GLR] calls these "symbol nodes")
class NonterminalNode : public TreeNode {
public:     // data
  // Each Reduction is an ordered list of the children
  // of a production (symbols for the RHS elements).  Multiple
  // rules here represent choice points (ambiguities) in the
  // parse graph.  These links are the parse graph's links --
  // they are built, but otherwise ignored, during parsing.
  ObjList<Reduction> reductions;               // this is a set

public:
  // it must be given the first reduction at creation time
  NonterminalNode(Reduction *red);             // (transfer owner)
  virtual ~NonterminalNode();

  // add a new reduction; use this instead of adding directly
  // (other issues constrain me from making 'reductions' private)
  void addReduction(Reduction *red);           // (transfer owner)

  // get the symbol that is the LHS of all reductions here
  Nonterminal const *getLHS() const;

  // assuming there is no ambiguity, get the single Reduction;
  // if there is not 1 reduction, xassert
  Reduction const *only() const;

  // same assumption; nice pre-chewed calls for
  // emitted code
  int onlyProductionIndex() const;
  TreeNode const *getOnlyChild(int childNum) const;
  Lexer2Token const &getOnlyChildToken(int childNum) const;

  // TreeNode stuff
  virtual bool isTerm() const { return false; }
  virtual Symbol const *getSymbolC() const;
  virtual TreeNode const *walkTree(WalkFn func, void *extra=NULL) const;
  virtual AttrValue getAttrValue(AttrName name) const;
  virtual void setAttrValue(AttrName name, AttrValue value);
  virtual TerminalNode const *getLeftmostTerminalC() const;
  virtual void ambiguityReport(ostream &os) const;
  virtual void getGroundTerms(SObjList<TerminalNode> &dest) const;
  virtual void printParseTree(ostream &os, int indent) const;
};


// for a particular production, this contains the pointers to
// the representatives of the RHS elements; it also identifies
// the production
// ([GLR] calls these "rule nodes")
class Reduction {
private:     // data
  // attributes associated with the node
  Attributes attr;

public:      // data
  // the production that generated this node
  Production const * const production;         // (serf)

  // for each RHS member of 'production', a pointer to the thing
  // that matches that symbol (terminal or nonterminal)
  SObjList<TreeNode> children;                 // this is a list

public:      // funcs
  Reduction(Production const *prod);
  ~Reduction();

  AttrValue getAttrValue(AttrName name) const;
  void setAttrValue(AttrName name, AttrValue value);

  void printParseTree(ostream &os, int indent) const;
  TreeNode const *walkTree(TreeNode::WalkFn func, void *extra=NULL) const;
  void ambiguityReport(ostream &os) const;

  void getGroundTerms(SObjList<TerminalNode> &dest) const;
};


// OLD: during attribution, a context for evaluation is provided by a
// list of children (a Reduction) and the attributes for the parent;
// this structure is created at that time to carry around that
// context, though it is not stored anywhere in the tree
class AttrContext {
  Reduction *red;      	   // (owner)

public:
  AttrContext(Reduction *r)
    : red(r) {}
  ~AttrContext();

  // access without modification
  Reduction const &reductionC() const { return *red; }

  // access with modification
  Reduction &reduction() { return *red; }

  // transfer of ownership (nullifies 'red')
  Reduction *grabReduction();
};


// carry information about an unhandled ambiguity
class XAmbiguity : public xBase {
public:     // data
  // node that has more than one possible reduction, but
  // some code expected only one
  // NOTE: since this is a pointer into the tree, this
  // exception must be caught *before* the tree is destroyed
  NonterminalNode const *node;

  // additional info what/why about the ambiguity
  string message;

private:    // funcs
  static string makeWhy(NonterminalNode const *n, char const *m);

public:     // funcs
  XAmbiguity(NonterminalNode const *n, char const *msg);
  XAmbiguity(XAmbiguity const &obj);
  ~XAmbiguity();
};


#endif // __GLRTREE_H
