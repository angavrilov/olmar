// grammar.h
// representation and algorithms for context-free grammars

// Author: Scott McPeak, April 2000

// Unfortunately, representation and algorithm tend to get
// mixed together.  Separating them entirely is possible,
// but syntactically inconvenient.  So, instead, I try to
// document the separation in comments.  Specifically,
// sections beginning with ---- representation ---- are data
// for representation of the underlying concept, while
// sections with ---- annotation ---- are data created by
// algorithms manipulating the data.

// Another measure is I've split all grammar-wide algorithm
// stuff into GrammarAnalysis (gramanl.h).  Things should
// only be put into Grammar if they are directly related
// to the grammar representation.  (However, constitutent
// objects like Production will continue to be a mix.)

#ifndef __GRAMMAR_H
#define __GRAMMAR_H

#include <iostream.h>  // ostream

#include "str.h"       // string
#include "objlist.h"   // ObjList
#include "sobjlist.h"  // SObjList

// fwds defined below
class Symbol;
class Terminal;
class Nonterminal;
class Production;
class DottedProduction;
class Grammar;

// this really should be in a more general place..
#define OSTREAM_OPERATOR(MyClass)                                \
  friend ostream &operator << (ostream &os, MyClass const &ths)  \
    { ths.print(os); return os; }

// this too
#define INTLOOP(var, start, maxPlusOne) \
  for (int var = start; var < maxPlusOne; var++)


// ---------------- Symbol --------------------
// either a nonterminal or terminal symbol
class Symbol {
// ------ representation ------
public:
  string const name;        // symbol's name in grammar
  bool const isTerm;        // true: terminal (only on right-hand sides of productions)
                            // false: nonterminal (can appear on left-hand sides)
  bool isEmptyString;       // true only for the emptyString nonterminal

public:
  Symbol(char const *n, bool t)
    : name(n), isTerm(t), isEmptyString(false) {}
  virtual ~Symbol();

  // uniform selectors
  bool isTerminal() const { return isTerm; }
  bool isNonterminal() const { return !isTerm; }

  // casting
  Terminal const &asTerminalC() const;       // checks 'isTerminal' for cast safety
  Terminal &asTerminal()
    { return const_cast<Terminal&>(asTerminalC()); }

  Nonterminal const &asNonterminalC() const;
  Nonterminal &asNonterminal()
    { return const_cast<Nonterminal&>(asNonterminalC()); }

  // debugging
  virtual void print(ostream &os) const;
  OSTREAM_OPERATOR(Symbol)
    // print as '$name: isTerminal=$isTerminal' (no newline)
};

// I have several needs for serf lists of symbols, so let's use this for now
typedef SObjList<Symbol> SymbolList;
typedef SObjListIter<Symbol> SymbolListIter;
typedef SObjListMutator<Symbol> SymbolListMutator;

#define FOREACH_SYMBOL(list, iter) FOREACH_OBJLIST(Symbol, list, iter)
#define MUTATE_EACH_SYMBOL(list, iter) MUTATE_EACH_OBJLIST(Symbol, list, iter)
#define SFOREACH_SYMBOL(list, iter) SFOREACH_OBJLIST(Symbol, list, iter)
#define SMUTATE_EACH_SYMBOL(list, iter) SMUTATE_EACH_OBJLIST(Symbol, list, iter)


// ---------------- Terminal --------------------
// something that only appears on the right-hand side of
// productions, and is an element of the source language
class Terminal : public Symbol {
// ------ annotation ------
public:     // data
  int termIndex;         // terminal index - this terminal's id

public:     // funcs
  Terminal(char const *name)
    : Symbol(name, true /*terminal*/),
      termIndex(-1) {}

  virtual void print(ostream &os) const;
  OSTREAM_OPERATOR(Terminal)
};

typedef SObjList<Terminal> TerminalList;
typedef SObjListIter<Terminal> TerminalListIter;

#define FOREACH_TERMINAL(list, iter) FOREACH_OBJLIST(Terminal, list, iter)
#define MUTATE_EACH_TERMINAL(list, iter) MUTATE_EACH_OBJLIST(Terminal, list, iter)
#define SFOREACH_TERMINAL(list, iter) SFOREACH_OBJLIST(Terminal, list, iter)
#define SMUTATE_EACH_TERMINAL(list, iter) SMUTATE_EACH_OBJLIST(Terminal, list, iter)

// casting aggregates
inline ObjList<Symbol> const &toObjList(ObjList<Terminal> const &list)
  { return reinterpret_cast< ObjList<Symbol>const& >(list); }


// ---------------- Nonterminal --------------------
// something that can appear on the left-hand side of a production
// (or, emptyString, since we classify that as a nonterminal also)
class Nonterminal : public Symbol {
// ------ annotation ------
public:     // funcs
  int ntIndex;           // nonterminal index; see Grammar::computeWhatCanDeriveWhat
  bool cyclic;           // true if this can derive itself in 1 or more steps
  TerminalList first;    // set of terminals that can be start of a string derived from 'this'
  TerminalList follow;   // set of terminals that can follow a string derived from 'this'

public:     // funcs
  Nonterminal(char const *name);
  virtual ~Nonterminal();

  virtual void print(ostream &os) const;
  OSTREAM_OPERATOR(Nonterminal)
};

typedef SObjList<Nonterminal> NonterminalList;
typedef SObjListIter<Nonterminal> NonterminalListIter;

#define FOREACH_NONTERMINAL(list, iter) FOREACH_OBJLIST(Nonterminal, list, iter)
#define MUTATE_EACH_NONTERMINAL(list, iter) MUTATE_EACH_OBJLIST(Nonterminal, list, iter)
#define SFOREACH_NONTERMINAL(list, iter) SFOREACH_OBJLIST(Nonterminal, list, iter)
#define SMUTATE_EACH_NONTERMINAL(list, iter) SMUTATE_EACH_OBJLIST(Nonterminal, list, iter)

// casting aggregates
inline ObjList<Symbol> const &toObjList(ObjList<Nonterminal> const &list)
  { return reinterpret_cast< ObjList<Symbol>const& >(list); }


// ---------------- Production --------------------
// a rewrite rule
class Production {
// ------ representation ------
public:	    // data
  Nonterminal *left;            // (serf) left hand side; must be nonterminal
  SymbolList right;             // (serf) right hand side; terminals & nonterminals

public:	    // data
  Production(Nonterminal *L);   // you have to call append manually
  ~Production();

  // queries
  int rhsLength() const;        // length *not* including emptySymbol, if present

  // append a RHS symbol
  void append(Symbol *sym);

  // call this when production is built, so it can compute dprods
  void finished();


// ------ annotation ------
private:    // data
  int numDotPlaces;             // after finished(): equals rhsLength()+1
  DottedProduction *dprods;     // (owner) array of dotted productions

public:     // funcs
  // retrieve an item
  DottedProduction const *getDProdC(int dotPlace) const;
  DottedProduction *getDProd(int dotPlace)
    { return const_cast<DottedProduction*>(getDProdC(dotPlace)); }

  // print to cout as 'A -> B c D' (no newline)
  string toString() const;
  void print(ostream &os) const;
  OSTREAM_OPERATOR(Production)
};

typedef SObjList<Production> ProductionList;
typedef SObjListIter<Production> ProductionListIter;

#define FOREACH_PRODUCTION(list, iter) FOREACH_OBJLIST(Production, list, iter)
#define MUTATE_EACH_PRODUCTION(list, iter) MUTATE_EACH_OBJLIST(Production, list, iter)
#define SFOREACH_PRODUCTION(list, iter) SFOREACH_OBJLIST(Production, list, iter)
#define SMUTATE_EACH_PRODUCTION(list, iter) SMUTATE_EACH_OBJLIST(Production, list, iter)


// ---------------- DottedProduction --------------------
// a production, with an indicator that says how much of this
// production has been matched by some part of the input string
// (exactly which part of the input depends on where this appears
// in the algorithm's data structures)
class DottedProduction {
// ------ representation ------
public:	    // data
  Production *prod;        // (serf) the base production
  int dot;                 // 0 means it's before all RHS symbols, 1 means after first, etc.

public:	    // funcs
  DottedProduction()       // for later filling-in
    : prod(NULL), dot(-1) {}
  DottedProduction(Production *p, int d)
    : prod(p), dot(d) {}
			     
  bool isDotAtStart() const { return dot==0; }
  bool isDotAtEnd() const;

  // dot must not be at the start (left edge)
  Symbol const *symbolBeforeDotC() const;
  Symbol *symbolBeforeDot() { return const_cast<Symbol*>(symbolBeforeDotC()); }

  // dot must not be at the end (right edge)
  Symbol const *symbolAfterDotC() const;
  Symbol *symbolAfterDot() { return const_cast<Symbol*>(symbolAfterDotC()); }

  // print to cout as 'A -> B . c D' (no newline)
  void print(ostream &os) const;
  OSTREAM_OPERATOR(DottedProduction)
};

// (serf) lists of dotted productions
typedef SObjList<DottedProduction> DProductionList;
typedef SObjListIter<DottedProduction> DProductionListIter;

#define FOREACH_DOTTEDPRODUCTION(list, iter) FOREACH_OBJLIST(DottedProduction, list, iter)
#define MUTATE_EACH_DOTTEDPRODUCTION(list, iter) MUTATE_EACH_OBJLIST(DottedProduction, list, iter)
#define SFOREACH_DOTTEDPRODUCTION(list, iter) SFOREACH_OBJLIST(DottedProduction, list, iter)
#define SMUTATE_EACH_DOTTEDPRODUCTION(list, iter) SMUTATE_EACH_OBJLIST(DottedProduction, list, iter)


// ---------------- ItemSet -------------------
// a set of dotted productions, and the transitions between
// item sets, as in LR(0) set-of-items construction
class ItemSet {
public:
  // numerical state id, should be unique among item sets
  // in a particular grammar's sets
  int id;

  // the items (kernel items are recognized as the ones
  // with the dot *not* at the left edge)
  DProductionList items;

  // transition function (where we go on shifts)
  //   Map : (Terminal id or Nonterminal id)  -> ItemSet*
  ItemSet **termTransition;
  ItemSet **nontermTransition;

  // bounds for above
  int terms;
  int nonterms;

private:    // funcs
  int bcheckTerm(int index);
  int bcheckNonterm(int index);
  ItemSet *&refTransition(Symbol const *sym);

public:     // funcs
  ItemSet(int id, int numTerms, int numNonterms);
  ~ItemSet();

  // the set of items names a symbol as the symbol used
  // to reach this state -- namely, the symbol that appears
  // to the left of a dot.  this fn retrieves that symbol
  // (if all items have dots at left edge, returns NULL)
  Symbol const *getStateSymbolC() const;

  // query transition fn for an arbitrary symbol; returns
  // NULL if no transition is defined
  ItemSet const *transitionC(Symbol const *sym) const;
  ItemSet *transition(Symbol const *sym)
    { return const_cast<ItemSet*>(transitionC(sym)); }

  // set transition on 'sym' to be 'dest'
  void setTransition(Symbol const *sym, ItemSet *dest);

  // get the list of productions that are ready to reduce, given
  // that the next input symbol is 'lookahead' (i.e. in the follow
  // of a production's LHS)
  void getPossibleReductions(ProductionList &reductions,
                             Terminal const *lookahead) const;

  // debugging
  void writeGraph(ostream &os) const;
  void print(ostream &os) const;
  OSTREAM_OPERATOR(ItemSet)
};


// ---------------- Grammar --------------------
// represent a grammar: nonterminals, terminals, productions, and start-symbol
class Grammar {
// ------ representation ------
public:	    // data
  ObjList<Nonterminal> nonterminals;    // (owner list)
  ObjList<Terminal> terminals;          // (owner list)
  ObjList<Production> productions;      // (owner list)
  Nonterminal *startSymbol;             // (serf) a particular nonterminal

  // the special terminal for the empty string; does not appear in
  // the list of nonterminals or terminals for a grammar, but can
  // be referenced by productions, etc.
  Nonterminal emptyString;

public:     // funcs
  Grammar();                            // set everything manually
  ~Grammar();

  // simple queries
  int numTerminals() const;
  int numNonterminals() const;


  // ---- building a grammar ----
  // add a new production; the rhs arg list must be terminated with a NULL
  void addProduction(Nonterminal *lhs, Symbol *rhs, ...);

  // add a pre-constructed production
  void addProduction(Production *prod);

  // print the current list of productions
  void printProductions(ostream &os) const;


  // ---- grammar parsing ----
  void readFile(char const *fname);

  // parse a line like "LHS -> R1 R2 R3", return false on parse error
  bool parseLine(char const *grammarLine);


  // ---- symbol access ----
  #define SYMBOL_ACCESS(Thing)                              \
    /* retrieve, return NULL if not there */                \
    Thing const *find##Thing##C(char const *name) const;    \
    Thing *find##Thing(char const *name)                    \
      { return const_cast<Thing*>(find##Thing##C(name)); }  \
                                                            \
    /* retrieve, or create it if not already there */       \
    Thing *getOrMake##Thing(char const *name);

  SYMBOL_ACCESS(Symbol)        // findSymbolC, findSymbol, getOrMakeSymbol
  SYMBOL_ACCESS(Terminal)      //   likewise
  SYMBOL_ACCESS(Nonterminal)   //   ..
  #undef SYMBOL_ACCESS
};


#endif // __GRAMMAR_H

