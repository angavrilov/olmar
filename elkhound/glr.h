// glr.h
// GLR parsing algorithm

// references:
//
//   [GLR]  J. Rekers.  Parser Generation for Interactive
//          Environments.  PhD thesis, University of
//          Amsterdam, 1992.  Available by ftp from
//          ftp.cwi.nl:/pub/gipe/reports as Rek92.ps.Z.
//          [Contains a good description of the Generalized
//          LR (GLR) algorithm.]

#ifndef __GLR_H
#define __GLR_H

#include "gramanl.h"     // basic grammar analyses, Grammar, etc.
   
               
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
  // the LR state the parser is in when this node is at the
  // top ("at the top" means nothing, besides perhaps itself,
  // is pointing to it)
  ItemSet *state;                              // (serf)

  // the symbol that was shifted, or the LHS of the production
  // reduced, to arrive at this state
  Symbol *symbol;                              // (serf)

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
}


// for a particular production, this contains the pointers to
// the representatives of the RHS elements; it also identifies
// the production
class RuleNode {
public:
  // the production that generated this node
  Production *production;                      // (serf)

  // for each RHS member of 'production', a pointer to the thing
  // that matches that symbol (terminal or nonterminal)
  SObjList<StackNode> children;                // this is a list
};


// during GLR parsing of a single token, we do all reductions before any
// shifts; thus, when we decide a shift is necessary, we need to save the
// relevant info somewhere so we can come back to it at the end
class PendingShift {
public:             
  // which parser is this that's ready to shift
  StackNode *parser;                   // (serf)

  // which state it is ready to shift into
  ItemSet *shiftDest;                  // (serf)
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

  // ---- parser state during each token ----
  // the token we're trying to shift; any parser that fails to
  // shift this token (or reduce to one that can, recursively)
  // will "die"
  Terminal *currentToken;

  // parsers that haven't had a chance to try to make progress
  // on this token
  SObjList<StackNode> parserWorklist;





#endif // __GLR_H
