// gramanl.h
// grammar analysis module; separated from grammar.h to
//   reduce mixing of representation and algorithm; this
//   module should be entirely algorithm

// Author: Scott McPeak, April 2000

// references:
//
//   [ASU]  Aho, Sethi Ullman.  Compilers: Principles,
//          Techniques, and Tools.  Addison-Wesley,
//          Reading, MA.  1986.  Second printing (3/88).
//          [A classic reference for LR parsing.]


#ifndef __GRAMANL_H
#define __GRAMANL_H

#include "grammar.h"    // Grammar and friends

// forward decls
class Bit2d;            // bit2d.h
class EmitCode;         // emitcode.h

// this file
class GrammarAnalysis;


// ---------------- ItemSet -------------------
// a set of dotted productions, and the transitions between
// item sets, as in LR(0) set-of-items construction
class ItemSet {
private:    // data
  // kernel items: the items that define the set; except for
  // the special case of the initial item in the initial state,
  // the kernel items are distinguished by having the dot *not*
  // at the left edge
  DProductionList kernelItems;

  // nonkernel items: those derived as the closure of the kernel
  // items by expanding symbols to the right of dots; here I am
  // making the choice to materialize them, rather than derive
  // them on the spot as needed (and may change this decision)
  DProductionList nonkernelItems;

  // transition function (where we go on shifts)
  //   Map : (Terminal id or Nonterminal id)  -> ItemSet*
  ItemSet **termTransition;		     // (owner ptr to array of serf ptrs)
  ItemSet **nontermTransition;		     // (owner ptr to array of serf ptrs)

  // bounds for above
  int terms;
  int nonterms;

  // profiler reports I'm spending significant time rifling through
  // the items looking for those that have the dot at the end; so this
  // array will point to all such items
  DottedProduction const **dotsAtEnd;        // (owner ptr to array of serf ptrs)
  int numDotsAtEnd;                          // number of elements in 'dotsAtEnd'
  
  // profiler also reports I'm still spending time comparing item sets; this
  // stores a CRC of the numerically sorted kernel item pointer addresses,
  // concatenated into a buffer of sufficient size
  unsigned long kernelItemsCRC;

public:	    // data
  // numerical state id, should be unique among item sets
  // in a particular grammar's sets
  int id;

  // it's useful to have a BFS tree superimposed on the transition
  // graph; for example, it makes it easy to generate sample inputs
  // for each state.  so we store the parent pointer; we can derive
  // child pointers by looking at all outgoing transitions, and
  // filtering for those whose targets' parent pointers equal 'this'.
  // the start state's parent is NULL, since it is the root of the
  // BFS tree
  ItemSet *BFSparent;                        // (serf)

private:    // funcs
  int bcheckTerm(int index);
  int bcheckNonterm(int index);
  ItemSet *&refTransition(Symbol const *sym);

  // computes things derived from the item set lists:
  // dotsAtEnd, numDotsAtEnd, kernelItemsCRC
  void changedItems();
  
  void allocateTransitionFunction();

public:     // funcs
  ItemSet(int id, int numTerms, int numNonterms);
  ~ItemSet();

  ItemSet(Flatten&);
  void xfer(Flatten &flat);
  void xferSerfs(Flatten &flat, GrammarAnalysis &g);

  // ---- item queries ----
  // the set of items names a symbol as the symbol used
  // to reach this state -- namely, the symbol that appears
  // to the left of a dot.  this fn retrieves that symbol
  // (if all items have dots at left edge, returns NULL; this
  // would be true only for the initial state)
  Symbol const *getStateSymbolC() const;

  // equality is defined as having the same items (basic set equality)
  bool operator== (ItemSet const &obj) const;

  // sometimes it's convenient to have all items mixed together
  // (CONSTNESS: allows modification of items...)
  void getAllItems(DProductionList &dest) const;

  // ---- transition queries ----
  // query transition fn for an arbitrary symbol; returns
  // NULL if no transition is defined
  ItemSet const *transitionC(Symbol const *sym) const;
  ItemSet *transition(Symbol const *sym)
    { return const_cast<ItemSet*>(transitionC(sym)); }

  // get the list of productions that are ready to reduce, given
  // that the next input symbol is 'lookahead' (i.e. in the follow
  // of a production's LHS); parsing=true means we are actually
  // parsing input, so certain tracing output is appropriate
  void getPossibleReductions(ProductionList &reductions,
                             Terminal const *lookahead,
                             bool parsing) const;

  // ---- item mutations ----
  // add a kernel item; used while constructing the state
  void addKernelItem(DottedProduction *item);

  // add a nonkernel item; used while computing closure; this
  // item must not already be in the item set
  void addNonkernelItem(DottedProduction *item);

  // experiment: do I need them during parsing?
  void throwAwayItems();

  // ---- transition mutations ----
  // set transition on 'sym' to be 'dest'
  void setTransition(Symbol const *sym, ItemSet *dest);

  // ---- debugging ----
  void writeGraph(ostream &os) const;
  void print(ostream &os, GrammarAnalysis const &g) const;
};


// ---------------------- GrammarAnalysis -------------------
class GrammarAnalysis : public Grammar {
protected:  // data
  // if entry i,j is true, then nonterminal i can derive nonterminal j
  // (this is a graph, represented (for now) as an adjacency matrix)
  enum { emptyStringIndex = 0 };
  Bit2d *derivable;                     // (owner)

  // index the symbols on their integer ids
  Nonterminal **indexedNonterms;        // (owner -> serfs) ntIndex -> Nonterminal
  Terminal **indexedTerms;              // (owner -> serfs) termIndex -> Terminal
  int numNonterms;                      // length of 'indexedNonterms' array
  int numTerms;                         //   "     "         terms       "

  // during itemSetClosure, profiling reports we spend a lot of time
  // walking the list of productions looking for those that have a given
  // symbol on the LHS; so let's index produtions by LHS symbol index;
  // this array has 'numNonterms' elements, mapping each nonterminal to
  // the list of productions with that nonterminal on the LHS
  SObjList<Production> *productionsByLHS;    // (owner ptr to array)

  // only true after initializeAuxData has been called
  bool initialized;

  // used to assign itemsets ids
  int nextItemSetId;

  // the LR parsing tables
  ObjList<ItemSet> itemSets;
  ItemSet *startState;			// (serf) distinguished start state

public:	    // data
  // true if any nonterminal can derive itself (with no extra symbols
  // surrounding it) in 1 or more steps
  bool cyclic;

  // symbol of interest; various diagnostics are printed when
  // certain things happen with it (e.g. the first application
  // is to print whenever something is added to this sym's
  // follow)
  Symbol const *symOfInterest;


private:    // funcs
  // ---- analyis init ----
  // call this after grammar is completely built
  void initializeAuxData();
  void computeIndexedNonterms();
  void computeIndexedTerms();
  void computeProductionsByLHS();

  // ---- derivability ----
  // iteratively compute every pair A,B such that A can derive B
  void computeWhatCanDeriveWhat();
  void initDerivableRelation();

  // add a derivability relation; returns true if this makes a change
  bool addDerivable(Nonterminal const *left, Nonterminal const *right);
  bool addDerivable(int leftNtIndex, int rightNtIndex);

  // private derivability interface
  bool canDerive(int leftNtIndex, int rightNtIndex) const;
  bool sequenceCanDeriveEmpty(RHSEltList const &list) const;
  bool iterSeqCanDeriveEmpty(RHSEltListIter iter) const;

  // ---- First ----
  void computeFirst();
  bool addFirst(Nonterminal *NT, Terminal *term);
  void firstOfSequence(TerminalList &destList, RHSEltList &sequence);
  void firstOfIterSeq(TerminalList &destList, RHSEltListMutator sym);

  // ---- Follow ----
  void computeFollow();
  bool addFollow(Nonterminal *NT, Terminal *term);

  // ---- LR item sets ----
  ItemSet *makeItemSet();
  void disposeItemSet(ItemSet *is);
  ItemSet *moveDotNoClosure(ItemSet const *source, Symbol const *symbol);
  ItemSet *findItemSetInList(ObjList<ItemSet> &list,
                             ItemSet const *itemSet);
  static bool itemSetsEqual(ItemSet const *is1, ItemSet const *is2);

  void constructLRItemSets();
  void lrParse(char const *input);

  void findSLRConflicts() const;
  bool checkSLRConflicts(ItemSet const *state, Terminal const *sym,
                         bool conflictAlready) const;

  void computeBFSTree();

  // misc
  void computePredictiveParsingTable();
    // non-const because have to add productions to lists

  // the inverse of transition: map a target state to the symbol that
  // would transition to that state (from the given source state)
  Symbol const *inverseTransitionC(ItemSet const *source,
                                   ItemSet const *target) const;

  // sample input helpers
  void leftContext(SymbolList &output, ItemSet const *state) const;
  bool rewriteAsTerminals(TerminalList &output, SymbolList const &input) const;
  bool rewriteAsTerminalsHelper(TerminalList &output, SymbolList const &input,
				NonterminalList &reducedStack) const;
  bool rewriteSingleNTAsTerminals(TerminalList &output, Nonterminal const *nonterminal,
				  NonterminalList &reducedStack) const;

  // let's try this .. it needs to access 'itemSets'
  friend void ItemSet::xferSerfs(Flatten &flat, GrammarAnalysis &g);

public:	    // funcs
  GrammarAnalysis();
  ~GrammarAnalysis();

  // access symbols by index
  Terminal const *getTerminal(int index) const;
  Nonterminal const *getNonterminal(int index) const;

  // binary read/write
  void xfer(Flatten &flat);

  // essentially, my 'main()' while experimenting
  void exampleGrammar();

  // overrides base class to add a little bit of the
  // annotated info
  void printProductions(ostream &os, bool printCode=true) const;

  // print lots of stuff
  void printProductionsAndItems(ostream &os, bool printCode=true) const;

  // when grammar is built, this runs all analyses and stores
  // the results in this object's data fields
  void runAnalyses();

  // really should be in ccwrite.h, but it's here
  // because the implementation needed private data..
  void emitTypeCtorMap(EmitCode &os) const;

  // ---- grammar queries ----
  bool canDerive(Nonterminal const *lhs, Nonterminal const *rhs) const;
  bool canDeriveEmpty(Nonterminal const *lhs) const;

  bool firstIncludes(Nonterminal const *NT, Terminal const *term) const;
  bool followIncludes(Nonterminal const *NT, Terminal const *term) const;

  // ---- sample inputs and contexts ----
  string sampleInput(ItemSet const *state) const;
  string leftContextString(ItemSet const *state) const;
  
  // ---- moved out of private ----
  void itemSetClosure(ItemSet &itemSet);
};


#endif // __GRAMANL_H
