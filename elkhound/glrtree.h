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

#error I do not intend to use this right now.

#ifndef __GLRTREE_H
#define __GLRTREE_H

#include "grammar.h"     // Symbol, Production, etc.
#include "attr.h"        // Attributes
#include "lexer2.h"      // Lexer2, Lexer2Token
#include "owner.h"       // Owner
#include "exc.h"         // xBase
#include "trdelete.h"    // TRASHINGDELETE

#if 0     // sm: I think we don't need this
  #ifdef WES_OCAML_LINKAGE
    extern "C" {
    #include "caml/mlvalues.h"
    #include "caml/alloc.h"
    #include "caml/memory.h"
    }
  #else
    // provide a dummy decl so the fn prototypes don't
    // need to be protected by ugly #ifdefs
    struct wes_ast_node { int x; };
  #endif
#endif // 0


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

public:     // data
  // count and high-water for tree nodes
  static int numTreeNodesAllocd;
  static int maxTreeNodesAllocd;

public:     // funcs
  TreeNode();
  virtual ~TreeNode();
  TRASHINGDELETE

  // essentially a "prepare to delete" function .. there's some
  // conceptual inelegance having this here, but let's go with
  // it for the moment
  virtual void killParentLink();

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
  virtual int numGroundTerms() const = 0;

  // simple unparse: yield string of tokens in this tree, separated by spaces
  string unparseString() const;
  
  // location of source that generated this subtree
  string locString() const;               // may be "(?loc)"
  SourceLocation const *loc() const;      // may be NULL if no leaves

  // debugging
  virtual void printParseTree(ostream &os, int indent,
                              bool asSexp) const = 0;
  static void printAllocStats();

  // tree walk and gather an ast to export to ocaml
  virtual struct wes_ast_node * camlAST() const = 0;  // wes

  // selfOnly: check this node's internal invariants
  // !selfOnly: also check this node and its children for a valid tree
  virtual void selfCheck(bool selfOnly) const = 0;
  void treeCheck() const { selfCheck(false); }
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
  virtual int numGroundTerms() const;
  virtual void printParseTree(ostream &os, int indent, bool asSexp) const;
  virtual struct wes_ast_node * camlAST() const;  // wes
  virtual void selfCheck(bool selfOnly) const;
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
  Reduction const *onlyC() const;
  Reduction *only() { return const_cast<Reduction*>(onlyC()); }

  // same assumption; nice pre-chewed calls for
  // emitted code
  int onlyProductionIndex() const;
  TreeNode const *getOnlyChild(int childNum) const;
  Lexer2Token const &getOnlyChildToken(int childNum) const;

  // little hack to get info from CCTreeNode
  virtual bool getIsJustInt() const { return false; }
  virtual int getTheInt() const { return 0; }

  // TreeNode stuff
  virtual bool isTerm() const { return false; }
  virtual Symbol const *getSymbolC() const;
  virtual TreeNode const *walkTree(WalkFn func, void *extra=NULL) const;
  virtual AttrValue getAttrValue(AttrName name) const;
  virtual void setAttrValue(AttrName name, AttrValue value);
  virtual TerminalNode const *getLeftmostTerminalC() const;
  virtual void ambiguityReport(ostream &os) const;
  virtual void getGroundTerms(SObjList<TerminalNode> &dest) const;
  virtual int numGroundTerms() const;
  virtual void printParseTree(ostream &os, int indent, bool asSexp) const;
  virtual struct wes_ast_node * camlAST() const;  // wes
  virtual void selfCheck(bool selfOnly) const;
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
  TRASHINGDELETE

  AttrValue getAttrValue(AttrName name) const;
  void setAttrValue(AttrName name, AttrValue value);

  bool attrsAreEqual(Reduction const &obj) const
    { return attr == obj.attr; }

  void printParseTree(ostream &os, int indent, bool asSexp) const;
  struct wes_ast_node * camlAST() const;  // wes
  TreeNode const *walkTree(TreeNode::WalkFn func, void *extra=NULL) const;
  void ambiguityReport(ostream &os) const;

  string locString() const;
  TerminalNode const *getLeftmostTerminalC() const;
  void getGroundTerms(SObjList<TerminalNode> &dest) const;
  int numGroundTerms() const;
  void selfCheck(bool selfOnly) const;
};


// container to carry an entire parse tree
class ParseTree {
private:     // data
  // top of the tree; set after parsing is complete
  TreeNode *treeTop;              // (serf)

public:      // data
  // list of all parse tree (graph) nodes
  ObjList<TreeNode> treeNodes;

  // additional data for use in semantic functions
  void *extra;                    // (serf)

public:      // funcs
  ParseTree();
  ~ParseTree();

  // thin level of indirection..
  void setTop(TreeNode *t) { treeTop = t; }
  TreeNode *getTop() { return treeTop; }
};


// context which attributes are evaluated.. typically, this is a
// single reduction, but the treeCompare expressions have two
// reductions they're comparing, so there are two
class AttrContext {
  Reduction *red[2];           // one or two interesting reductions

public:
  AttrContext(Reduction *r1, Reduction *r2=NULL);

  // access without modification
  Reduction const &reductionC(int which) const;

  // access with modification
  Reduction &reduction(int which)
    { return const_cast<Reduction&>(reductionC(which)); }
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
