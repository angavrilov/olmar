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


class GrammarAnalysis : public Grammar {
protected:  // data
  // if entry i,j is true, then nonterminal i can derive nonterminal j
  // (this is a graph, represented (for now) as an adjacency matrix)
  enum { emptyStringIndex = 0 };
  Bit2d *derivable;                     // (owner)

  // indexing structures
  Nonterminal **indexedNonterms;        // (owner * serfs) ntIndex -> Nonterminal
  Terminal **indexedTerms;              // (owner * serfs) termIndex -> Terminal
  int numNonterms;                      // length of 'indexedNonterms' array
  int numTerms;                         //   "     "         terms       "

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

  // ---- derivability ----
  // iteratively compute every pair A,B such that A can derive B
  void computeWhatCanDeriveWhat();
  void initDerivableRelation();

  // add a derivability relation; returns true if this makes a change
  bool addDerivable(Nonterminal const *left, Nonterminal const *right);
  bool addDerivable(int leftNtIndex, int rightNtIndex);

  // private derivability interface
  bool canDerive(int leftNtIndex, int rightNtIndex) const;
  bool sequenceCanDeriveEmpty(SymbolList const &list) const;
  bool iterSeqCanDeriveEmpty(SymbolListIter iter) const;

  // ---- First ----
  void computeFirst();
  bool addFirst(Nonterminal *NT, Terminal *term);
  void firstOfSequence(TerminalList &destList, SymbolList &sequence);
  void firstOfIterSeq(TerminalList &destList, SymbolListMutator sym);

  // ---- Follow ----
  void computeFollow();
  bool addFollow(Nonterminal *NT, Terminal *term);

  // ---- LR item sets ----
  void itemSetClosure(ItemSet &itemSet);
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
    
  // sample input helpers
  void leftContext(SymbolList &output, ItemSet const *state) const;
  bool rewriteAsTerminals(TerminalList &output, SymbolList const &input) const;
  bool rewriteAsTerminalsHelper(TerminalList &output, SymbolList const &input,
				NonterminalList &reducedStack) const;
  bool rewriteSingleNTAsTerminals(TerminalList &output, Nonterminal const *nonterminal,
				  NonterminalList &reducedStack) const;


public:	    // funcs
  GrammarAnalysis();
  ~GrammarAnalysis();

  // essentially, my 'main()' while experimenting
  void exampleGrammar();

  // overrides base class to add a little bit of the
  // annotated info
  void printProductions(ostream &os) const;

  // when grammar is built, this runs all analyses and stores
  // the results in this object's data fields
  void runAnalyses();


  // ---- grammar queries ----
  bool canDerive(Nonterminal const *lhs, Nonterminal const *rhs) const;
  bool canDeriveEmpty(Nonterminal const *lhs) const;

  bool firstIncludes(Nonterminal const *NT, Terminal const *term) const;
  bool followIncludes(Nonterminal const *NT, Terminal const *term) const;


  // ---- sample inputs and contexts ----
  string sampleInput(ItemSet const *state) const;
  string leftContextString(ItemSet const *state) const;
};


#endif // __GRAMANL_H
