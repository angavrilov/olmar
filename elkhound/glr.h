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


// forward decls for things declared below
class StackNode;
class RuleNode;
class PendingShift;


// the GLR parse state is primarily made up of a graph of these
// nodes, which play a role analogous to the stack nodes of a
// normal LR parser; GLR nodes form a graph instead of a linear
// stack because choice points (real or potential ambiguities)
// are represented
class StackNode {
public:
  // it's convenient when printing diagnostic info to have
  // a unique integer id for these
  int stackNodeId;		  
  
  // node layout is also important; I think this info will
  // be useful; it's the number of the input token that was
  // being processed when this node was created (first token
  // is column 1; initial stack node is column 0)
  int tokenColumn;

  // the LR state the parser is in when this node is at the
  // top ("at the top" means nothing, besides perhaps itself,
  // is pointing to it)
  ItemSet const *state;                        // (serf)

  // the symbol that was shifted, or the LHS of the production
  // reduced, to arrive at this state
  Symbol const *symbol;                        // (serf)

  // each leftAdjState represents a symbol that might be immediately
  // to the left of this symbol in a derivation; multiple links
  // here represent choice points (ambiguities) during parsing; these
  // links are only important at parse time -- they are ignored
  // once parsing is complete
  SObjList<StackNode> leftAdjStates;           // this is a set

  // Each RuleNode is an ordered list of the children
  // of a production (symbols for the RHS elements).  Multiple
  // lists here represent choice points in the parse graph.
  // These links are the parse graph's links -- they are built,
  // but otherwise ignored, during parsing.
  ObjList<RuleNode> rules;                     // this is a set


public:     // funcs
  StackNode(int id, int tokenColumn,
            ItemSet const *state, Symbol const *symbol);
  ~StackNode();
  
  void printParseTree(int indent) const;
};


// for a particular production, this contains the pointers to
// the representatives of the RHS elements; it also identifies
// the production
class RuleNode {
public:
  // the production that generated this node
  Production const *production;                // (serf)

  // for each RHS member of 'production', a pointer to the thing
  // that matches that symbol (terminal or nonterminal)
  SObjList<StackNode> children;                // this is a list

public:
  RuleNode(Production const *prod);
  ~RuleNode();

  void printParseTree(int indent) const;
};


// during GLR parsing of a single token, we do all reductions before any
// shifts; thus, when we decide a shift is necessary, we need to save the
// relevant info somewhere so we can come back to it at the end
class PendingShift {
public:
  // which parser is this that's ready to shift?
  StackNode *parser;                   // (serf)

  // which state is it ready to shift into?
  ItemSet const *shiftDest;            // (serf)

public:
  PendingShift(StackNode *p, ItemSet const *s)
    : parser(p), shiftDest(s) {}
};


// name a 'leftAdjStates' link
class SiblingLinkDesc {
public:
  StackNode *left;     	// (serf) thing pointed-at
  StackNode *right;	// (serf) thing with the pointer

public:
  SiblingLinkDesc(StackNode *L, StackNode *R)
    : left(L), right(R) {}
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
  SObjList<StackNode> activeParsers;

  // after parsing I need an easy way to throw away the parse
  // state, so I keep all the nodes in an owner list
  ObjList<StackNode> allStackNodes;

  // this is for assigning unique ids to stack nodes
  int nextStackNodeId;
  enum { initialStackNodeId = 1 };

  // ---- parser state during each token ----
  // the token we're trying to shift; any parser that fails to
  // shift this token (or reduce to one that can, recursively)
  // will "die"
  Terminal const *currentToken;
  
  // this is maintained for labelling stack nodes
  int currentTokenColumn;

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
                               SiblingLinkDesc *mustUseLink);
  void popStackSearch(int popsRemaining, SObjList<StackNode> &poppedSymbols,
                      StackNode *currentNode, Production const *production,
                      SiblingLinkDesc *mustUseLink);
  void glrShiftRule(StackNode *leftSibling, RuleNode *ruleNode);
  void glrShiftTerminals(ObjList<PendingShift> &pendingShifts);
  StackNode *findActiveParser(ItemSet const *state);
  StackNode *makeStackNode(ItemSet const *state, Symbol const *symbol);
  void writeParseGraph(char const *input) const;
  void clearAllStackNodes();

public:     // funcs
  GLR();
  ~GLR();

  // 'main' for testing this class
  void glrTest();
};

#endif // __GLR_H
