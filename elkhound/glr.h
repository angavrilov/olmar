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
#include "owner.h"       // Owner
#include "rcptr.h"       // RCPtr
#include "useract.h"     // SemanticValue


// fwds from other files
class Lexer2Token;       // lexer2.h
class Lexer2;            // lexer2.h

// forward decls for things declared below
class StackNode;         // unit of parse state
class SiblingLink;       // connections between stack nodes
class PendingShift;      // for postponing shifts.. may remove
class GLR;               // main class for GLR parsing


// the GLR parse state is primarily made up of a graph of these
// nodes, which play a role analogous to the stack nodes of a
// normal LR parser; GLR nodes form a graph instead of a linear
// stack because choice points (real or potential ambiguities)
// are represented as multiple left-siblings
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
  
  // so we can deallocate stack nodes earlier
  int referenceCount;

  // count and high-water for stack nodes
  static int numStackNodesAllocd;
  static int maxStackNodesAllocd;

public:     // funcs
  StackNode(int id, int tokenColumn, ItemSet const *state);
  ~StackNode();

  // add a new link with the given tree node; return the link
  SiblingLink *addSiblingLink(StackNode *leftSib, SemanticValue sval);

  // return the symbol represented by this stack node;  it's
  // the symbol shifted or reduced-to to get to this state
  // (this used to be a data member, but there are at least
  // two ways to compute it, so there's no need to store it)
  Symbol const *getSymbolC() const;

  // reference count stuff
  void incRefCt() { referenceCount++; }
  void decRefCt();

  // debugging
  static void printAllocStats();
};


// a pointer from a stacknode to one 'below' it (in the LR
// parse stack sense); also has a link to the parse graph
// we're constructing
class SiblingLink {
public:
  // the stack node being pointed-at; it was created eariler
  // than the one doing the pointing
  RCPtr<StackNode> sib;

  // this is the semantic value associated with this link
  // (parse tree nodes are *not* associated with stack nodes --
  // that's now it was originally, but I figured out the hard
  // way that's wrong (more info in compiler.notes.txt))
  SemanticValue sval;
  
  // every time I hand this value to a reduction function, I
  // increment the value (if the value is 0, then this object
  // is the only one which has a reference to it)
  int svalRefCt;

public:
  SiblingLink(StackNode *s, SemanticValue sv)
    : sib(s), sval(sv), svalRefCt(0) {}
  ~SiblingLink();
};


// during GLR parsing of a single token, we do all reductions before any
// shifts; thus, when we decide a shift is necessary, we need to save the
// relevant info somewhere so we can come back to it at the end
class PendingShift {
public:
  // which parser is this that's ready to shift?
  RCPtr<StackNode> parser;

  // which state is it ready to shift into?
  ItemSet const * const shiftDest;            // (serf)

public:
  PendingShift(StackNode *p, ItemSet const *s)
    : parser(p), shiftDest(s) {}
  ~PendingShift();
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
    RCPtr<StackNode> finalState;

    // the semantic value yielded by the reduction action
    SemanticValue sval;

  public:
    ReductionPath(StackNode *f, SemanticValue s)
      : finalState(f), sval(s) {}
    ~ReductionPath();
  };

public:	   // data
  // ---- stuff that changes ----
  // id of the state we started in (where the reduction was applied)
  int startStateId;

  // we collect paths into this array, which is maintained as we
  // enter/leave recursion; the 0th item is the leftmost, i.e.
  // the last one we collect when starting from the reduction state
  // and popping symbols as we move left
  SemanticValue *poppedSymbols;

  // also collect reference count pointers, so they can be incremented
  // each time the corresponding semantic value ends up being used
  int **svalRefCts;
  
  // yet more: collect the symbols associated with each of the
  // semantic values, as this info is needed to call the 'dup'
  // functions
  Symbol const **svalSymbols;

  // as reduction possibilities are encountered, we record them here
  ObjList<ReductionPath> paths;

  // ---- stuff constant across a series of DFSs ----
  // this is the production we're trying to reduce by
  Production const * const production;

public:	   // funcs
  PathCollectionState(Production const *p, int start);
  ~PathCollectionState();
};


// the GLR analyses are performed within this class; GLR
// differs from GrammarAnalysis in that the latter should have
// stuff useful across a wide range of possible analyses
class GLR : public GrammarAnalysis {
public:
  // ---- parser state between tokens ----
  // Every node in this set is (the top of) a parser that might
  // ultimately succeed to parse the input, or might reach a
  // point where it cannot proceed, and therefore dies.  (See
  // comments at top of glr.cc for more details.)
  SObjList<StackNode> activeParsers;        // (refct list)

  // this is for assigning unique ids to stack nodes
  int nextStackNodeId;
  enum { initialStackNodeId = 1 };

  // this is maintained for labeling stack nodes
  int currentTokenColumn;

  // ---- parser state during each token ----
  // the token we're trying to shift; any parser that fails to
  // shift this token (or reduce to one that can, recursively)
  // will "die"
  Terminal const *currentTokenClass;

  // the semantic value associated with that token
  SemanticValue currentTokenValue;

  // parsers that haven't yet had a chance to try to make progress
  // on this token
  SObjList<StackNode> parserWorklist;       // (refct list)


private:    // funcs
  // comments in glr.cc
  int glrParseAction(StackNode *parser,
                     ObjList<PendingShift> &pendingShifts);
  int postponeShift(StackNode *parser,
                    ObjList<PendingShift> &pendingShifts);
  int doAllPossibleReductions(StackNode *parser,
                              SiblingLink *mustUseLink);
  void collectReductionPaths(PathCollectionState &pcs, int popsRemaining,
                             StackNode *currentNode, SiblingLink *mustUseLink);
  void glrShiftNonterminal(StackNode *leftSibling, Production const *prod,
                           SemanticValue sval);
  void glrShiftTerminals(ObjList<PendingShift> &pendingShifts);
  StackNode *findActiveParser(ItemSet const *state);
  StackNode *makeStackNode(ItemSet const *state);
  void writeParseGraph(char const *input) const;
  void clearAllStackNodes();

public:     // funcs
  GLR();
  ~GLR();

  // 'main' for testing this class; returns false on error
  bool glrParseFrontEnd(Lexer2 &lexer2, SemanticValue &treeTop,
                        char const *grammarFname, char const *inputFname,
                        char const *symOfInterestName = NULL);
                         
  // parse, using the token stream in 'lexer2', and store the final
  // semantic value in 'treeTop'
  bool glrParse(Lexer2 const &lexer2, SemanticValue &treeTop);

  // parse a given input file, using the passed lexer to lex it,
  // and store the result in 'treeTop'
  bool glrParseNamedFile(Lexer2 &lexer2, SemanticValue &treeTop,
                         char const *input);

  // after parsing, retrieve the toplevel semantic value
  // (obsolete now; the parsing function returns it)
  //SemanticValue getParseResult();
};


#endif // __GLR_H
