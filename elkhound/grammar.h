// grammar.h
// representation and algorithms for context-free grammars

#ifndef __GRAMMAR_H
#define __GRAMMAR_H

#include "str.h"       // string
#include "objlist.h"   // ObjList
#include "sobjlist.h"  // SObjList

// forward decls
class Bit2d;           // bit2d.h


// either a nonterminal or terminal symbol
class Symbol {
public:
  // key attributes
  string name;        // symbol's name in grammar
  bool isTerminal;    // true: terminal (only on right-hand sides of productions)
                      // false: nonterminal (can appear on left-hand sides)

  // annotations used and computed by auxilliary algorithms
  bool canDeriveEmpty;   // true if this symbol can (directly or indirectly) derive emptyString
  int ntindex;           // nonterminal index; see ICFR::computeWhatCanDeriveWhat

public:
  Symbol(char const *n, bool t)
    : name(n), isTerminal(t),
      canDeriveEmpty(false), ntindex(0) {}

  // debugging
  void print() const;    // print as 'emptyString: isTerminal=true, canDeriveEmpty=true' (no newline)
};


// I have several needs for serf lists of symbols, so let's use this for now
typedef SObjList<Symbol> SymbolList;
typedef SObjListIter<Symbol> SymbolListIter;


// a rewrite rule
class Production {
public:
  Symbol *left;                 // (serf) left hand side; must be nonterminal
  SymbolList right;         	// (serf) right hand side; terminals & nonterminals

public:
  Production(Symbol *L)
    : left(L) {}    // have to call right.append manually

  // print to cout as 'A -> B c D' (no newline)
  void print() const;
};


// a production, with an indicator that says how much of this
// production has been matched by some part of the input string
// (exactly which part of the input depends on where this appears
// in the algorithm's data structures)
class DottedProduction {
public:
  Production *prod;        // (serf) the base production
  int dot;                 // 0 means it's before all RHS symbols, 1 means after first, etc.

public:
  DottedProduction(Production *p, int d)
    : prod(p), dot(d) {}

  // print to cout as 'A -> B . c D' (no newline)
  void print() const;
};


// (serf) lists of dotted productions
typedef SObjList<DottedProduction> DProductionList;
typedef SObjListIter<DottedProduction> DProductionListIter;


// represent a grammar: nonterminals, terminals, productions, and start-symbol
class Grammar {
  // computed from key attributes:
  // if entry i,j is true, then nonterminal i can derive nonterminal j
  // (this is a graph, represented (for now) as an adjacency matrix)
  enum { emptyStringIndex = 0 };
  Bit2d *derivable;    	       	  // (owner)

public:
  // ---- key attributes ----
  ObjList<Symbol> nonterminals;	        // (owner list)
  ObjList<Symbol> terminals;   		// (owner list)
  ObjList<Production> productions;     	// (owner list)
  Symbol *startSymbol;                  // (serf) a nonterminal

  // the special terminal for the empty string; does not appear in
  // the list of nonterminals or terminals for a grammar, but can
  // be referenced by productions, etc.
  Symbol *emptyString;                  // (owner)
    // I had 'const' here, but since I annotate nonterminals, that
    // isn't right.. which then calls into question having a single
    // emptyString instead of one per grammar... will leave as-is for now
    // update: ok, moved into the grammar


  // ---- computed from the above ----
  // every production, with a dot in every possible place
  ObjList<DottedProduction> dottedProductions;

private:
  // iteratively compute, for every nonterminal, whether it can
  // derive emptyString (worst-case O(n^2) in size of grammar)
  void computeWhichCanDeriveEmpty();

  // iteratively compute every pair A,B such that A can derive B
  void computeWhatCanDeriveWhat();


public:
  Grammar();				// set everything manually (except emptyString)
  ~Grammar();

  // print the current list of productions
  void printProductions() const;

  // add a new production; the rhs arg list must be terminated with a NULL
  void addProduction(Symbol *lhs, Symbol *rhs, ...);


  // essentially, my 'main()' while experimenting
  void exampleGrammar();


  // ---------------- grammar algorithms --------------------
  // true if the lhs can be rewritten as the rhs in zero
  // or more rewrite steps (the list versions are symbol
  // sequences to be matched)
  // [I need algorithms for these.. maybe dragon book?]
  bool canDerive(Symbol const *lhs, Symbol const *rhs) const;
  bool canDerive(Symbol const *lhs, SymbolList const &rhs) const;
  bool canDerive(SymbolList const &lhs, Symbol const *rhs) const;
  bool canDerive(SymbolList const &lhs, SymbolList const &rhs) const;


  // we move the dot across all the symbols in 'symbols' (or the set of
  //   lhs symbols in 'symProds' with dots at the rightmost position),
  //   and across any symbols that can derive emptyString
  // this is the x operator in [GHR80]
  void advanceDot(DProductionList &final,             // out
                  DProductionList const &initial,
                  SymbolList const &symbols) const;
  void advanceDot(DProductionList &final,             // out
                  DProductionList const &initial,
                  DProductionList const &symProds) const;

  // similar to advanceDot, but we also advance past symbols that can
  //   derive symbols in 'symbols' (or lhs of 'symProds' with dots in
  //   rightmost position)
  // this is the * operator in [GHR80]
  void advanceDotStar(DProductionList &final,         // out
                      DProductionList const &initial,
                      SymbolList const &symbols) const;
  void advanceDotStar(DProductionList &final,         // out
                      DProductionList const &initial,
                      DProductionList const &symProds) const;

  // yield those nonterminals (as dotted productions with the dot moved
  //   beyond any initial nonterminals that can yield emptyString) that
  //   can be the first nonterminal of a string derived from something in
  //   'symbols' (or those nonterminals that follow dots in 'symProds')
  // this is the PREDICT function in [GHR80]
  void predict(DProductionList &prods,                // out
               SymbolList const &symbols) const;
  void predict(DProductionList &prods,                // out
               DProductionList const &symProds) const;
               
};


#endif // __GRAMMAR_H

