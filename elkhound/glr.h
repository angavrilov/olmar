// glr.h
// GLR parsing algorithm

/*
 * Author: Scott McPeak, April 2000
 *
 * The fundamental concept in Generalized LR (GLR) parsing
 * is to permit (at least local) ambiguity by "forking" the
 * parse stack.  If the input is actually unambiguous, then
 * all but one of the forked parsers will, at some point,
 * fail to shift a symbol, and die.  If the input is truly
 * ambiguous, forked parsers rejoin at some point, and the
 * parse tree becomes a parse DAG, representing all possible
 * parses.  (In fact, since cyclic grammars are supported,
 * which can have an infinite number of parse trees for
 * some inputs, we may end up with a cyclic parse *graph*.)
 *
 * In the larger scheme of things, this level of support for
 * ambiguity is useful because it lets us use simpler and
 * more intuitive grammars, more sophisticated disambiguation
 * techniques, and parsing in the presence of incomplete
 * or incorrect information (e.g. in an editor).
 *
 * The downside is that parsing is slower, and whatever tool
 * processes the parse graph needs to have ways of dealing
 * with the multiple parse interpretations.
 *
 * references:
 *
 *   [GLR]  J. Rekers.  Parser Generation for Interactive
 *          Environments.  PhD thesis, University of
 *          Amsterdam, 1992.  Available by ftp from
 *          ftp.cwi.nl:/pub/gipe/reports as Rek92.ps.Z.
 *          [Contains a good description of the Generalized
 *          LR (GLR) algorithm.]
 */

#ifndef __GLR_H
#define __GLR_H

#include "gramanl.h"     // basic grammar analyses, Grammar class, etc.
#include "glrtree.h"     // parse tree (graph) representation


// fwds from other files
class Lexer2Token;       // lexer2.h

// forward decls for things declared below
class StackNode;         // unit of parse state
class SiblingLink;       // connections between stack nodes
class PendingShift;      // for postponing shifts.. may remove
class GLR;               // main class for GLR parsing


// the GLR parse state is primarily made up of a graph of these
// nodes, which play a role analogous to the stack nodes of a
// normal LR parser; GLR nodes form a graph instead of a linear
// stack because choice points (real or potential ambiguities)
// are represented
class StackNode {
public:
  // it's convenient when printing diagnostic info to have
  // a unique integer id for these
  int const stackNodeId;

  // node layout is also important; I think this info will
  // be useful; it's the number of the input token that was
  // being processed when this node was created (first token
  // is column 1; initial stack node is column 0)
  int const tokenColumn;

  // the LR state the parser is in when this node is at the
  // top ("at the top" means nothing, besides perhaps itself,
  // is pointing to it)
  ItemSet const * const state;                 // (serf)

  // each leftSibling points to a stack node in one possible
  // LR stack.  if there is more than one, it means two or more
  // LR stacks have been joined at this point.  this is the
  // parse-time representation of ambiguity
  ObjList<SiblingLink> leftSiblings;           // this is a set

public:     // funcs
  StackNode(int id, int tokenColumn, ItemSet const *state);
  ~StackNode();
							      
  // add a new link with the given tree node; return the link
  SiblingLink *addSiblingLink(StackNode *leftSib, TreeNode *treeNode);
  
  // if 'leftSib' is one of our siblings, return the link that
  // makes that true
  SiblingLink *findSiblingLink(StackNode *leftSib);

  // return the symbol represented by this stack node;  it's
  // the symbol shifted or reduced-to to get to this state
  // (this used to be a data member, but there are at least
  // two ways to compute it, so there's no need to store it)
  Symbol const *getSymbolC() const;
};

                   
// a pointer from a stacknode to one 'below' it (in the LR
// parse stack sense); also has a link to the parse graph
// we're constructing
class SiblingLink {
public:
  // the stack node being pointed-at; it was created eariler
  // than the one doing the pointing
  StackNode * const sib;                       // (serf)

  // this is the parse tree node associated with this link
  // (parse tree nodes are *not* associated with stack nodes --
  // that's now it was originally, but I figured out the hard
  // way that's wrong (more info in compiler.notes.txt))
  TreeNode * const treeNode;                   // (serf)

  // the treeNode pointer is a serf because, while there is
  // a 1-1 relationship between siblinglinks and treenodes,
  // I want to be able to simply throw away the entire parse
  // state without disturbing the tree; tree nodes are instead
  // owned by a (more or less) global list

public:
  SiblingLink(StackNode *s, TreeNode *n)
    : sib(s), treeNode(n) {}
};


// during GLR parsing of a single token, we do all reductions before any
// shifts; thus, when we decide a shift is necessary, we need to save the
// relevant info somewhere so we can come back to it at the end
class PendingShift {
public:
  // which parser is this that's ready to shift?
  StackNode * const parser;                   // (serf)

  // which state is it ready to shift into?
  ItemSet const * const shiftDest;            // (serf)

public:
  PendingShift(StackNode *p, ItemSet const *s)
    : parser(p), shiftDest(s) {}
};


// when a reduction is performed, we do a DFS to find all the ways
// the reduction can happen; this structure keeps state persistent
// across the recursion (comments in code have some details, too)
class PathCollectionState {
public:    // types
  // each particular reduction possibility is one of these
  class ReductionPath {
  public:    
    // the state we end up in after reducing those symbols
    StackNode *finalState;        // (serf)

    // the reduction itself
    Reduction *reduction;         // (owner)

  public:
    ReductionPath(StackNode *f, Reduction *r)
      : finalState(f), reduction(r) {}
    ~ReductionPath();
  };

public:	   // data
  // ---- stuff that changes ----
  // we collect paths into this list, which is maintained as we
  // enter/leave recursion
  SObjList<TreeNode> poppedSymbols;

  // as reduction possibilities are encountered, we record them here
  ObjList<ReductionPath> paths;

  // ---- stuff constant across a series of DFSs ----
  // this is the production we're trying to reduce by
  Production const * const production;

public:	   // funcs
  PathCollectionState(Production const *p)
    : poppedSymbols(), paths(),      // both empty, initially
      production(p)
  {}

  ~PathCollectionState();
};


// the GLR analyses are performed within this class; GLR
// differs from GrammarAnalysis in that the latter should have
// stuff useful across a wide range of possible analyses
class GLR : public GrammarAnalysis {
public:
  // ---- state to keep after parsing is done ----
  // list of all parse tree (graph) nodes
  ObjList<TreeNode> treeNodes;

  // ---- parser state between tokens ----
  // Every node in this set is (the top of) a parser that might
  // ultimately succeed to parse the input, or might reach a
  // point where it cannot proceed, and therefore dies.  (See
  // comments at top of glr.cc for more details.)
  SObjList<StackNode> activeParsers;

  // after parsing I need an easy way to throw away the parse
  // state, so I keep all the nodes in an owner list
  ObjList<StackNode> allStackNodes;

  // this is for assigning unique ids to stack nodes
  int nextStackNodeId;
  enum { initialStackNodeId = 1 };

  // this is maintained for labeling stack nodes
  int currentTokenColumn;

  // ---- parser state during each token ----
  // the token we're trying to shift; any parser that fails to
  // shift this token (or reduce to one that can, recursively)
  // will "die"               
  Lexer2Token const *currentToken;
                                  
  // this is the class of the given token, i.e. its classification
  // in the grammar
  Terminal const *currentTokenClass;

  // parsers that haven't yet had a chance to try to make progress
  // on this token
  SObjList<StackNode> parserWorklist;


private:    // funcs
  // comments in glr.cc
  void glrParse(char const *input);
  void glrParseAction(StackNode *parser,
                      ObjList<PendingShift> &pendingShifts);
  void postponeShift(StackNode *parser,
                     ObjList<PendingShift> &pendingShifts);
  void doAllPossibleReductions(StackNode *parser,
                               SiblingLink *mustUseLink);
  void collectReductionPaths(PathCollectionState &pcs, int popsRemaining,
                             StackNode *currentNode, SiblingLink *mustUseLink);
  void glrShiftNonterminal(StackNode *leftSibling, Reduction *reduction);
  void mergeAlternativeParses(NonterminalNode &node, AttrContext &actx);
  void glrShiftTerminals(ObjList<PendingShift> &pendingShifts);
  StackNode *findActiveParser(ItemSet const *state);
  StackNode *makeStackNode(ItemSet const *state);
  void writeParseGraph(char const *input) const;
  void clearAllStackNodes();
  TerminalNode *makeTerminalNode(Lexer2Token const *tk, Terminal const *tc);
  NonterminalNode *makeNonterminalNode(AttrContext &actx);


public:     // funcs
  GLR();
  ~GLR();

  // 'main' for testing this class
  void glrTest(char const *grammarFname, char const *inputFname,
               char const *symOfInterestName);
};

#endif // __GLR_H
