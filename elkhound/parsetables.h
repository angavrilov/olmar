// parsetables.h            see license.txt for copyright and terms of use
// ParseTables, a class to contain the tables need by the
// LR/GLR parsing algorithm

#ifndef PARSETABLES_H
#define PARSETABLES_H

#include "array.h"        // ArrayStack
#include "glrconfig.h"    // compression options
#include <iostream.h>     // ostream

class Flatten;            // flatten.h
class EmitCode;           // emitcode.h
class Symbol;             // grammar.h
class Bit2d;              // bit2d.h


// integer id for an item-set DFA state; I'm using an 'enum' to
// prevent any other integers from silently flowing into it
enum StateId { STATE_INVALID=-1 };

inline ostream& operator<< (ostream &os, StateId id)
  { return os << (int)id; }


// encodes an action in 'action' table; see 'actionTable'
#if ENABLE_CRS_COMPRESSION
  // high bits: 00 = shift, 10 = reduce, 01 = ambiguous, 11 = error
  // (if EEF is off)
  //
  // remaining 6 bits:
  //
  //   shift: desination state, encoded as an offset from the
  //   first state that that terminal can reach
  //
  //   reduce: production, encoded as an index into a per-state
  //   array of distinct production indices
  //
  //   ambiguous: for each state, have an array of ActionEntries.
  //   ambiguous entries index into this array.  first indexed
  //   entry is the count of how many actions follow
  typedef unsigned char ActionEntry;
#else
  // each entry is one of:
  //   +N+1, 0 <= N < numStates:         shift, and go to state N
  //   -N-1, 0 <= N < numProds:          reduce using production N
  //   numStates+N+1, 0 <= N < numAmbig: ambiguous, use ambigAction N
  //   0:                                error
  // (there is no 'accept', acceptance is handled outside this table)
  typedef signed short ActionEntry;
#endif


// encodes a destination state in 'gotoTable'
typedef unsigned short GotoEntry;

// name a terminal using an index
typedef unsigned char TermIndex;

// name a nonterminal using an index
typedef unsigned char NtIndex;

// name a production using an index
typedef unsigned short ProdIndex;

// an addressed cell in the 'errorBits' table
typedef unsigned char ErrorBitsEntry;


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
  // when this is false, all of the below "(owner*)" annotations are
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

  // action table, indexed by (state*actionCols + lookahead)
  int actionCols;
  ActionEntry *actionTable;              // (owner*)

  // goto table, indexed by (state*numNonterms + nontermId),
  // each entry is N, the state to go to after having reduced by
  // 'nontermId'
  GotoEntry *gotoTable;                  // (owner*)

  // map production id to information about that production
  ProdInfo *prodInfo;                    // (owner*)

  // map a state id to the symbol (terminal or nonterminal) which is
  // shifted to arrive at that state
  SymbolId *stateSymbol;                 // (owner*)

  // ambiguous actions: one big list, for allocation purposes; then
  // the actions encode indices into this table; the first indexed
  // entry gives the # of actions, and is followed by that many
  // actions, each interpreted the same way ordinary 'actionTable'
  // entries are
  int ambigTableSize;
  ActionEntry *ambigTable;               // (nullable owner*)

  //ArrayStack<ActionEntry*> ambigAction;  // (array of owner ptrs)
  //int numAmbig() const { return ambigAction.length(); }

  // start state id
  StateId startState;

  // index of the production which will finish a parse; it's the
  // final reduction executed
  int finalProductionIndex;

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
  NtIndex *nontermOrder;                 // (owner*)

  // --------------------- table compression ----------------------

  // table compression techniques taken from:
  //    [DDH] Peter Dencker, Karl Dürre, and Johannes Heuft.
  //    Optimization of Parser Tables for Portable Compilers.
  //    In ACM TOPLAS, 6, 4 (1984) 546-572.
  //    http://citeseer.nj.nec.com/context/27540/0 (not in database)
  //    ~/doc/papers/p546-dencker.pdf (from ACM DL)

  #if 0    // still working
  // Code Reduction Scheme (CRS):
  //
  // Part (a):  The states are numbered such that all states that
  // are reached by transitions on a given symbol are contiguous.
  // See gramanl.cc, GrammarAnalysis::renumberStates().  Then, we
  // simply need a map from the symbol index to the first state
  // that is reached along that symbol.
  StateId *firstWithTerminal;            // (nullable owner*) termIndex -> state
  StateId *firstWithNonterminal;         // (nullable owner*) ntIndex -> state
  //
  // Part (b):  The production indices that appear on a given row
  // are collected together.  (This is called (c) by [DDH]; I don't
  // have a counterpart to their (b).)
  ProdIndex *bigProductionList;          // (nullable owner*) array into which 'productionsForState' points
  ProdIndex **productionsForState;       // (nullable owner to serf) state -> prod
  //
  // Part (c):  The ambiguous actions are collected together in
  // par-state lists as well.  But that's done regardless of whether
  // other CRS activities are done.  The encoding is explained above.
  // (THIS IS WRONG)
  #endif // 0

  // Error Entry Factoring (EEF):
  //
  // Factor out all the error entries into their own bitmap.  Then
  // regard error entries in the original tables as "insignificant".
  //
  // 'errorBits' is a map of where the error actions are in the action
  // table.  It is indexed through 'errorBitsPointers':
  //   byte = errorBitsPointers[stateId][lookahead >> 3];
  //   if ((byte >> (lookahead & 7)) & 1) then ERROR
  int errorBitsRowSize;                  // bytes per row
  int uniqueErrorRows;                   // distinct rows
  ErrorBitsEntry *errorBits;             // (nullable owner*)
  ErrorBitsEntry **errorBitsPointers;    // (nullable owner ptr to serfs)

  // Graph Coloring Scheme (GCS):
  //
  // Merge lines and columns that have identical significant entries.
  // This is done as two-pass graph coloring.  They give a specific
  // heuristic.
  //
  // this is a map to be applied to terminal indiced before being
  // used to access the compressed action table; it maps the terminal
  // id (as reported by the lexer) to the proper action table column
  TermIndex *actionIndexMap;             // (nullable owner*)
  //
  // this is a map from states to the beginning of the action table
  // row that pertains to that state; it effectively factors the
  // states into equivalence classes
  int actionRows;                        // rows in actionTable[]
  ActionEntry **actionRowPointers;       // (nullable owner ptr to serfs)

private:    // funcs
  void alloc(int numTerms, int numNonterms, int numStates, int numProds,
             StateId start, int finalProd);

  void fillInErrorBits(bool setPointers);
  int colorTheGraph(int *color, Bit2d &graph);

public:     // funcs
  ParseTables(int numTerms, int numNonterms, int numStates, int numProds,
              StateId start, int finalProd);
  ParseTables(bool owning);    // only legal when owning==false
  ~ParseTables();

  ParseTables(Flatten&);
  void xfer(Flatten &flat);
  
  // write the tables out as C++ source that can be compiled into
  // the program that will ultimately do the parsing
  void emitConstructionCode(EmitCode &out, char const *funcName);

  // index tables
  ActionEntry &actionEntry(int stateId, int termId)
    { return actionTable[stateId*actionCols + termId]; }
  int actionTableSize() const
    { return actionRows * actionCols; }
  GotoEntry &gotoEntry(int stateId, int nontermId)
    { return gotoTable[stateId*numNonterms + nontermId]; }
  int gotoTableSize() const
    { return numStates * numNonterms; }

  // return true if the action is an error
  bool actionEntryIsError(int stateId, int termId) {
    #if ENABLE_EEF_COMPRESSION
      // check with the error table
      return ( errorBitsPointers[stateId][termId >> 3]
                 >> (termId & 7) ) & 1;
    #else
      return isErrorAction(actionEntry(stateId, termId));
    #endif
  }

  // query action table, without checking the error bitmap
  ActionEntry getActionEntry_noError(int stateId, int termId) {
    #if ENABLE_GCS_COMPRESSION
      return actionRowPointers[stateId][actionIndexMap[termId]];
    #else
      return actionEntry(stateId, termId);
    #endif
  }

  // query the action table in a way compatible with various
  // compression schemes
  ActionEntry getActionEntry(int stateId, int termId) {
    #if ENABLE_EEF_COMPRESSION
      if (actionEntryIsError(stateId, termId)) {
        return 0;       // error
      }
    #endif

    return getActionEntry_noError(stateId, termId);
  }

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

  // ambigAction is only other choice; this yields a pointer to
  // an array of actions, the first of which says how many actions
  // there are
  ActionEntry *decodeAmbigAction(ActionEntry code) const
    { return ambigTable + (code-1-numStates); }

  // encode gotos
  GotoEntry encodeGoto(StateId stateId) const
    { return validateGoto(stateId); }
  GotoEntry encodeGotoError() const
    { return validateGoto(numStates); }
  GotoEntry validateGoto(int code) const;

  // decode gotos
  static StateId decodeGoto(GotoEntry code)
    { return (StateId)code; }

  // nonterminal order
  int nontermOrderSize() const
    { return numNonterms; }
  NtIndex getNontermOrdinal(NtIndex idx) const
    { return nontermOrder[idx]; }
    
    
  // ----------- table compressors -------------
  // scrape all the error entries from the action table into the
  // 'errorBits' bitmap
  void computeErrorBits();
  void mergeActionColumns();
  void mergeActionRows();
};


// read a parse table from a file
ParseTables *readParseTablesFile(char const *fname);

// similarly for writing
void writeParseTablesFile(ParseTables const *tables, char const *fname);


#endif // PARSETABLES_H
