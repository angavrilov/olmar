// gramanl.h
// grammar analysis module; separated from grammar.h to
//   reduce mixing of representation and algorithm; this
//   module should be entirely algorithm

// references:
//
//   [ASU]  Aho, Sethi Ullman.  Compilers: Principles,
//          Techniques, and Tools.  Addison-Wesley,
//          Reading, MA.  1986.  Second printing (3/88).
//          [A classic reference for LR parsing.]
//
//   [GLR]  J. Rekers.  Parser Generation for Interactive
//          Environments.  PhD thesis, University of
//          Amsterdam, 1992.  Available by ftp from
//          ftp.cwi.nl:/pub/gipe/reports as Rek92.ps.Z.
//          [Contains a good description of the Generalized
//          LR (GLR) algorithm.]


#ifndef __GRAMANL_H
#define __GRAMANL_H

#include "grammar.h"    // Grammar and friends

// forward decls
class Bit2d;            // bit2d.h


class GrammarAnalysis : public Grammar {
private:    // data
  // if entry i,j is true, then nonterminal i can derive nonterminal j
  // (this is a graph, represented (for now) as an adjacency matrix)
  enum { emptyStringIndex = 0 };
  Bit2d *derivable;                     // (owner)

  // indexing structures
  Nonterminal **indexedNonterms;        // (owner) ntIndex -> Nonterminal
  Terminal **indexedTerms;              // (owner) termIndex -> Terminal

  // only true after initializeAuxData has been called
  bool initialized;

  // used to assign itemsets ids
  int nextItemSetId;

public:	    // data
  // true if any nonterminal can derive itself in 1 or more steps
  bool cyclic;


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
  void itemSetClosure(DProductionList &itemSet);
    // non-const because have to add dotted productions to the list
  ItemSet *makeItemSet();
  void disposeItemSet(ItemSet *is);
  ItemSet *moveDot(ItemSet const *source, Symbol const *symbol);
  ItemSet *findItemSetInList(ObjList<ItemSet> &list,
                             ItemSet const *itemSet);
  bool itemSetContainsItemSet(ItemSet const *big,
                              ItemSet const *small);
  bool itemSetsEqual(ItemSet const *is1, ItemSet const *is2);
  void constructLRItemSets(ObjList<ItemSet> &itemSetsDone);
  void lrParse(ObjList<ItemSet> &itemSets, char const *input);

  // misc
  void computePredictiveParsingTable();
    // non-const because have to add productions to lists


public:	    // funcs
  GrammarAnalysis();
  ~GrammarAnalysis();

  // essentially, my 'main()' while experimenting
  void exampleGrammar();

  // overrides base class to add a little bit of the
  // annotated info
  void printProductions(ostream &os) const;


  // ---- grammar queries ----
  bool canDerive(Nonterminal const *lhs, Nonterminal const *rhs) const;
  bool canDeriveEmpty(Nonterminal const *lhs) const;

  bool firstIncludes(Nonterminal const *NT, Terminal const *term) const;
  bool followIncludes(Nonterminal const *NT, Terminal const *term) const;
};


#endif // __GRAMANL_H
