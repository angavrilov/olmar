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


// forward decls for things declared below
class TreeNode;          // abstract parse tree (graph) node
class TerminalNode;      // tree node for terminals; derived from TreeNode
class NonterminalNode;   // tree node for nonterminals; derived from TreeNode
class Reduction;         // inside NonterminalNode, reduction choices


// abstract constituent of a parse graph
// ([GLR] calls these "tree nodes" also)
class TreeNode {
public:	    // data
  // kind of tree node
  enum NodeType { TERMINAL, NONTERMINAL };
  NodeType const type;

public:	    // funcs
  TreeNode(NodeType t) : type(t) {}
  virtual ~TreeNode();
				   
  // returns the representative symbol (terminal or nonterminal)
  virtual Symbol const *getSymbolC() const = 0;

  // type determination
  bool isTerm() const { return type == TERMINAL; }
  bool isNonterm() const { return type == NONTERMINAL; }

  // type-safe casts (exception thrown if bad)
  TerminalNode const &asTermC() const;
  TerminalNode &asTerm() { return const_cast<TerminalNode&>(asTermC()); }

  NonterminalNode const &asNontermC() const;
  NonterminalNode &asNonterm() { return const_cast<NonterminalNode&>(asNontermC()); }

  // debugging
  virtual void printParseTree(ostream &os, int indent) const = 0;
};


// a leaf of a parse graph: where the source language's
// symbols are referenced
// ([GLR] calls these "terminal nodes" also)
class TerminalNode : public TreeNode {
public:     // data
  // which terminal we're talking about
  Terminal const * const terminal;

  // at some point, I will store information about where in
  // the source text this symbol came from

public:     // funcs
  TerminalNode(Terminal const *t);
  virtual ~TerminalNode();

  // TreeNode stuff
  virtual Symbol const *getSymbolC() const;
  virtual void printParseTree(ostream &os, int indent) const;
};


// an internal node in the parse graph; it has a list of all possible
// reductions at this point in the parse.
// ([GLR] calls these "symbol nodes")
class NonterminalNode : public TreeNode {
public:     // data
  // Each Reduction is an ordered list of the children
  // of a production (symbols for the RHS elements).  Multiple
  // rules here represent choice points (ambiguities) in the
  // parse graph.  These links are the parse graph's links --
  // they are built, but otherwise ignored, during parsing.
  ObjList<Reduction> reductions;               // this is a set

  // for a variety of reasons, I store the nonterminal itself
  // also, even though I can get it from the reductions
  Nonterminal const * const nonterminal;

public:
  // it must be given the first reduction at creation time
  NonterminalNode(Reduction *red);             // (transfer owner)
  virtual ~NonterminalNode();

  // add a new reduction; use this instead of adding directly
  // (other issues constrain me from making 'reductions' private)
  void addReduction(Reduction *red);           // (transfer owner)

  // TreeNode stuff
  virtual Symbol const *getSymbolC() const;
  virtual void printParseTree(ostream &os, int indent) const;
};


// for a particular production, this contains the pointers to
// the representatives of the RHS elements; it also identifies
// the production
// ([GLR] calls these "rule nodes")
class Reduction {
public:
  // the production that generated this node
  Production const * const production;         // (serf)

  // for each RHS member of 'production', a pointer to the thing
  // that matches that symbol (terminal or nonterminal)
  SObjList<TreeNode> children;                 // this is a list

public:
  Reduction(Production const *prod);
  ~Reduction();

  void printParseTree(ostream &os, int indent) const;
};


#endif // __GLRTREE_H
