// parsetables.h            see license.txt for copyright and terms of use
// ParseTables, a class to contain the tables need by the
// LR/GLR parsing algorithm

#ifndef PARSETABLES_H
#define PARSETABLES_H

#include "array.h"        // ArrayStack
#include "bit2d.h"        // Bit2d

class Flatten;            // flatten.h
class EmitCode;           // emitcode.h
class Symbol;             // grammar.h


// integer id for an item-set DFA state; I'm using an 'enum' to
// prevent any other integers from silently flowing into it
enum StateId { STATE_INVALID=-1 };


// encodes an action in 'action' table; see 'actionTable'
typedef signed short ActionEntry;

// encodes a destination state in 'gotoTable'
typedef unsigned short GotoEntry;

// names a nonterminal using an index
typedef unsigned char NtIndex;


// encodes either terminal index N (as N+1) or
// nonterminal index N (as -N-1), or 0 for no-symbol
typedef signed short SymbolId;
inline bool symIsTerm(SymbolId id) { return id > 0; }
inline int symAsTerm(SymbolId id) { return id-1; }
inline bool symIsNonterm(SymbolId id) { return id < 0; }
inline NtIndex symAsNonterm(SymbolId id) { return (NtIndex)(-(id+1)); }
SymbolId encodeSymbolId(Symbol const *sym);       // gramanl.cc


// the parse tables are the traditional action/goto, plus the list
// of ambiguous actions, plus any more auxilliary tables useful during
// run-time parsing; the eventual goal is to be able to keep *only*
// this structure around for run-time parsing; most things in this
// class are public because it's a high-traffic access point, so I
// expose interpretation information and the raw data itself instead
// of an abstract interface
class ParseTables {
public:     // types
  // per-production info
  struct ProdInfo {
    unsigned char rhsLen;                // # of RHS symbols
    NtIndex lhsIndex;                    // 'ntIndex' of LHS
  };

private:    // data
  // when this is false, all of the below "(owner)" annotations are
  // actually "(serf)", i.e. this object does *not* own any of the
  // tables (see emitConstructionCode())
  bool owning;

public:     // data
  // # terminals, nonterminals in grammar
  int numTerms;
  int numNonterms;

  // # of parse states
  int numStates;

  // # of productions in the grammar
  int numProds;

  // action table, indexed by (state*numTerms + lookahead),
  // each entry is one of:
  //   +N+1, 0 <= N < numStates:         shift, and go to state N
  //   -N-1, 0 <= N < numProds:          reduce using production N
  //   numStates+N+1, 0 <= N < numAmbig: ambiguous, use ambigAction N
  //   0:                                error
  // (there is no 'accept', acceptance is handled outside this table)
  ActionEntry *actionTable;              // (owner)

  // goto table, indexed by (state*numNonterms + nontermId),
  // each entry is N, the state to go to after having reduced by
  // 'nontermId'
  GotoEntry *gotoTable;                  // (owner)

  // map production id to information about that production
  ProdInfo *prodInfo;                    // (owner ptr to array)

  // map a state id to the symbol (terminal or nonterminal) which is
  // shifted to arrive at that state
  SymbolId *stateSymbol;                 // (owner)

  // ambiguous action table, indexed by ambigActionId; each entry is
  // a pointer to an array of signed short; the first number is the
  // # of actions, and is followed by that many actions, each
  // interpreted the same way ordinary 'actionTable' entries are
  ArrayStack<ActionEntry*> ambigAction;  // (array of owner ptrs)
  int numAmbig() const { return ambigAction.length(); }

  // start state id
  StateId startState;

  // index of the production which will finish a parse; it's the
  // final reduction executed
  int finalProductionIndex;

  // per-state bit flag: eager or delayed?
  unsigned char *delayedStates;          // (owner)

  // derivability relation among nonterminals
  //Bit2d *derivability;                   // (owner)
  
  // total order on nonterminals for use in choosing which to
  // reduce to in the RWL algorithm; index into this using a
  // nonterminal index, and it yields the ordinal for that
  // nonterminal (so these aren't really NtIndex's, but they're
  // exactly as wide, so I use NtIndex anyway)
  //
  // The order is consistent with the requirement that if
  //   A ->+ B
  // then B will be earlier in the order (assuming acyclicity).
  // That way, we'll do all reductions to B before any to A (for
  // reductions spanning the same set of ground terminals), and
  // therefore will merge all alternatives for B before reducing
  // any of them to A.
  NtIndex *nontermOrder;                 // (owner)

private:    // funcs
  void alloc(int numTerms, int numNonterms, int numStates, int numProds,
             StateId start, int finalProd);

public:     // funcs
  ParseTables(int numTerms, int numNonterms, int numStates, int numProds,
              StateId start, int finalProd);
  ParseTables(bool owning);    // only legal when owning==false
  ~ParseTables();

  ParseTables(Flatten&);
  void xfer(Flatten &flat);
  
  // see emittables.cc
  void emitConstructionCode(EmitCode &out, char const *funcName);

  // index tables
  ActionEntry &actionEntry(int stateId, int termId)
    { return actionTable[stateId*numTerms + termId]; }
  int actionTableSize() const
    { return numStates * numTerms; }
  GotoEntry &gotoEntry(int stateId, int nontermId)
    { return gotoTable[stateId*numNonterms + nontermId]; }
  int gotoTableSize() const
    { return numStates * numNonterms; }

  // encode actions
  ActionEntry encodeShift(StateId stateId) const
    { return validateAction(+stateId+1); }
  ActionEntry encodeReduce(int prodId) const
    { return validateAction(-prodId-1); }
  ActionEntry encodeAmbig(int ambigId) const
    { return validateAction(numStates+ambigId+1); }
  ActionEntry encodeError() const
    { return validateAction(0); }
  ActionEntry validateAction(int code) const;

  // decode actions
  bool isShiftAction(ActionEntry code) const
    { return code > 0 && code <= numStates; }
  static StateId decodeShift(ActionEntry code)
    { return (StateId)(code-1); }
  static bool isReduceAction(ActionEntry code)
    { return code < 0; }
  static int decodeReduce(ActionEntry code)
    { return -(code+1); }
  static bool isErrorAction(ActionEntry code)
    { return code == 0; }
  // ambigAction is only other choice
  int decodeAmbigAction(ActionEntry code) const
    { return code-1-numStates; }

  // encode gotos
  GotoEntry encodeGoto(StateId stateId) const
    { return validateGoto(stateId); }
  GotoEntry encodeGotoError() const
    { return validateGoto(numStates); }
  GotoEntry validateGoto(int code) const;

  // decode gotos
  static StateId decodeGoto(GotoEntry code)
    { return (StateId)code; }

  // delayed flag
  int delayedTableSize() const
    { return (numStates+7) / 8; }
  bool isDelayed(StateId stateId) const
    { return (delayedStates[stateId >> 3] >> (stateId & 7)) & 1; }
  void markDelayed(StateId stateId)
    { delayedStates[stateId >> 3] |= (1 << (stateId & 7)); }
    
  // derivability
  //bool canDerive(int ntIndex1, int ntIndex2) const
  //  { return derivability->get(point(ntIndex1, ntIndex2)); }
  
  // nonterminal order
  int nontermOrderSize() const
    { return numNonterms; }
  NtIndex getNontermOrdinal(NtIndex idx) const
    { return nontermOrder[idx]; }
};


// read a parse table from a file
ParseTables *readParseTablesFile(char const *fname);

// similarly for writing
void writeParseTablesFile(ParseTables const *tables, char const *fname);


#endif // PARSETABLES_H
