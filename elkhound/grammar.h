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

#include <iostream.h>    // ostream

#include "str.h"         // string
#include "objlist.h"     // ObjList
#include "sobjlist.h"    // SObjList
#include "util.h"        // OSTREAM_OPERATOR, INTLOOP
#include "locstr.h"      // LocString, StringRef
#include "strobjdict.h"  // StringObjDict
#include "owner.h"       // Owner

class StrtokParse;       // strtokp.h

// fwds defined below
class Symbol;
class Terminal;
class Nonterminal;
class Production;
class DottedProduction;
class Grammar;

// transitional definitions
typedef StringObjDict<LocString> LitCodeDict;
typedef LocString LiteralCode;


// ---------------- Symbol --------------------
// either a nonterminal or terminal symbol
class Symbol {
// ------ representation ------
public:
  string const name;        // symbol's name in grammar
  bool const isTerm;        // true: terminal (only on right-hand sides of productions)
                            // false: nonterminal (can appear on left-hand sides)
  bool const isEmptyString; // true only for the emptyString nonterminal

  StringRef type;           // C type of semantic value

public:
  Symbol(char const *n, bool t, bool e = false)
    : name(n), isTerm(t), isEmptyString(e), type(NULL) {}
  virtual ~Symbol();

  Symbol(Flatten&);
  void xfer(Flatten &flat);

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

// format: "s1 s2 s3"
string symbolSequenceToString(SymbolList const &list);


// ---------------- Terminal --------------------
// something that only appears on the right-hand side of
// productions, and is an element of the source language
// NOTE:  This is really a terminal *class*, in that it's possible
// for several different tokens to be classified into the same
// terminal class (e.g. "foo" and "bar" are both identifiers)
class Terminal : public Symbol {
// ------ annotation ------
public:     // data
  // terminal class index - this terminal's id; -1 means unassigned
  int termIndex;

  // whereas 'name' is the canonical name for the terminal class,
  // this field is an alias; for example, if the canonical name is
  // L2_EQUALEQUAL, the alias might be "==" (i.e. the alias
  // should include quotes if the grammar should have them too);
  // if the alias is "", there is no alias
  string alias;

public:     // funcs
  Terminal(char const *name)        // canonical name for terminal class
    : Symbol(name, true /*terminal*/),
      termIndex(-1),
      alias() {}

  Terminal(Flatten &flat);
  void xfer(Flatten &flat);

  virtual void print(ostream &os) const;
  OSTREAM_OPERATOR(Terminal)
                          
  // return alias if defined, name otherwise
  string toString() const;
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

// format: "t1 t2 t3"
string terminalSequenceToString(TerminalList const &list);


// ---------------- Nonterminal --------------------
// something that can appear on the left-hand side of a production
// (or, emptyString, since we classify that as a nonterminal also)
class Nonterminal : public Symbol {
// ------ representation ------
public:     // data
  #if 0
    // names of attributes that are associated with this nonterminal;
    // every production with this NT on its LHS must specify values
    // for all attributes
    ObjList<string> attributes;

    // inheritance relationships
    SObjList<Nonterminal> superclasses;

    // declarations of functions, as a dictionary: name -> declBody; the
    // text of the declaration is stored because it is needed when
    // emitting substrate code
    LitCodeDict funDecls;

    // for each function, we can optionally have prefix code which is run
    // at the start of that function, regardless of the production in
    // which it appears
    LitCodeDict funPrefixes;

    // declarations of things (data and fns) that are *not* implemented
    // in generated code
    ObjList<LocString> declarations;

    // definitions of disambiguation routines
    LitCodeDict disambFuns;

    // con/destructor functions
    LocString constructor;
    LocString destructor;
  #endif // 0

public:     // funcs
  Nonterminal(char const *name, bool isEmptyString=false);
  virtual ~Nonterminal();

  Nonterminal(Flatten &flat);
  void xfer(Flatten &flat);
  void xferSerfs(Flatten &flat, Grammar &g);

  #if 0
    // return true if 'attr' is among 'attributes'
    // (by 0==strcmp comparison)
    bool hasAttribute(char const *attr) const;

    // true if the given nonterminal is a superclass (transitively)
    bool hasSuperclass(Nonterminal const *nt) const;

    // true if the named function has a declaration here
    bool hasFunDecl(char const *name) const
      { return funDecls.isMapped(name); }
  #endif // 0

  virtual void print(ostream &os) const;
  OSTREAM_OPERATOR(Nonterminal)

// ------ annotation ------
public:     // data
  int ntIndex;           // nonterminal index; see Grammar::computeWhatCanDeriveWhat
  bool cyclic;           // true if this can derive itself in 1 or more steps
  TerminalList first;    // set of terminals that can be start of a string derived from 'this'
  TerminalList follow;   // set of terminals that can follow a string derived from 'this'
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
public:     // types
  class RHSElt {
  public:
    Symbol const *sym;          // (serf) rhs element symbol

    // tags applied to the symbols for purposes of unambiguous naming in
    // actions, and for self-commenting value as role indicators; an
    // empty tag ("") is allowed and means there is no tag
    string tag;                 // tag for this symbol; can be ""

  public:
    RHSElt(Symbol const *s, char const *t) : sym(s), tag(t) {}
    ~RHSElt();
    
    RHSElt(Flatten&);
    void xfer(Flatten &flat);
    void xferSerfs(Flatten &flat, Grammar &g);
  };

public:	    // data
  // fundamental context-free grammar (CFG) component
  Nonterminal * const left;     // (serf) left hand side; must be nonterminal
  string leftTag;      	       	// tag for LHS symbol
  ObjList<RHSElt> right;        // right hand side; terminals & nonterminals

  // user-supplied reduction action code
  LocString action;

public:	    // funcs
  Production(Nonterminal *left, char const *leftTag);
  ~Production();

  Production(Flatten &flat);
  void xfer(Flatten &flat);
  void xferSerfs(Flatten &flat, Grammar &g);

  // length *not* including emptySymbol, if present
  // UPDATE: I'm now disallowing emptySymbol from ever appearing in 'right'
  int rhsLength() const;

  // number of nonterminals on RHS
  int numRHSNonterminals() const;

  #if 0
    // find an action that sets the named attribute; return
    // NULL if none do
    Action const *getAttrActionFor(char const *attr) const
      { return actions.getAttrActionFor(attr); }

    // true if the named function has an implementation here
    bool hasFunction(char const *name) const
      { return functions.isMapped(name); }

    // check for referential integrity in actions and conditions
    void checkRefs() const;
  #endif // 0

  // append a RHS symbol
  void append(Symbol *sym, char const *tag);

  // call this when production is built, so it can compute dprods
  // (this is called by GrammarAnalysis::initializeAuxData, from
  // inside runAnalyses)
  void finished();

  // find a symbol by tag; returns 0 to identify LHS symbol, 1 for
  // first RHS symbol, 2 for second, etc.; returns -1 if the tag
  // doesn't match anything
  int findTag(char const *tag) const;

  // given an index as returned by 'findTaggedSymbol', translate that
  // back into a tag
  string symbolTag(int symbolIndex) const;

  // or translate a symbol index into a symbol
  Symbol const *symbolByIndexC(int symbolIndex) const;
  Symbol *symbolByIndex(int symbolIndex)
    { return const_cast<Symbol*>(symbolByIndexC(symbolIndex)); }

  // retrieve an item
  DottedProduction const *getDProdC(int dotPlace) const;
  DottedProduction *getDProd(int dotPlace)
    { return const_cast<DottedProduction*>(getDProdC(dotPlace)); }

  // print 'A -> B c D' (no newline)
  string toString() const;
  string rhsString() const;       // 'B c D' for above example rule
  void print(ostream &os) const;
  OSTREAM_OPERATOR(Production)

  // print entire input syntax, with newlines, e.g.
  //   A -> B c D { return foo; }
  string toStringMore(bool printCode) const;

// ------ annotation ------
private:    // data
  int numDotPlaces;             // after finished(): equals rhsLength()+1
  DottedProduction *dprods;     // (owner) array of dotted productions
  
public:     // data
  int prodIndex;                // unique production id
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
  Production * const prod;       // (serf) the base production
  int const dot;                 // 0 means it's before all RHS symbols, 1 means after first, etc.
  bool const dotAtEnd;           // performance optimization

public:	    // funcs
  DottedProduction()       // for later filling-in
    : prod(NULL), dot(-1), dotAtEnd(false) {}
  DottedProduction(Production *p, int d)
    : prod(NULL), dot(-1), dotAtEnd(false)     // silence warning about const init
    { setProdAndDot(p, d); }

  bool isDotAtStart() const { return dot==0; }
  bool isDotAtEnd() const { return dotAtEnd; }

  // call this to change prod and dot; don't change them directly
  // (I didn't make them private because of the syntactic hassle
  // of accessing them.  Instead I hacked them as 'const'.)
  void setProdAndDot(Production *p, int d) /*mutable*/;

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


// ---------------- Grammar --------------------
// represent a grammar: nonterminals, terminals, productions, and start-symbol
class Grammar {
// ------ representation ------
public:	    // data
  ObjList<Nonterminal> nonterminals;    // (owner list)
  ObjList<Terminal> terminals;          // (owner list)
  ObjList<Production> productions;      // (owner list)
  Nonterminal *startSymbol;             // (serf) a particular nonterminal

  // the special terminal for the empty string; does not appear in the
  // list of nonterminals or terminals for a grammar, but can be
  // referenced by productions, etc.; the decision to explicitly have
  // such a symbol, instead of letting it always be implicit, is
  // motivated by things like the derivability relation, where it's
  // nice to treat empty like any other symbol
  Nonterminal emptyString;

  #if 0
    // ---- stuff for emitting treewalk code ----
    // extra user-supplied source in the embedded language,
    // meant to appear in the generated semantic-functions files
    LocString semanticsPrologue;          // top of .h file
    LocString semanticsEpilogue;          // bottom of .cc file

    // name of base class for tree nodes; defaults to "NonterminalNode"
    string treeNodeBaseClass;
  #endif // 0

private:    // funcs
  #if 0
    // obsolete parsing functions
    bool parseAnAction(char const *keyword, char const *insideBraces,
                       Production *lastProduction);

    Symbol *parseGrammarSymbol(char const *token, string &tag);
    bool parseProduction(ProductionList &prods, StrtokParse const &tok);
  #endif // 0

public:     // funcs
  Grammar();                            // set everything manually
  ~Grammar();

  // read/write as binary file
  void xfer(Flatten &flat);

  // simple queries
  int numTerminals() const;
  int numNonterminals() const;


  // ---- building a grammar ----
  // declare a new token exists, with name and optional alias;
  // return false if it's already declared
  bool declareToken(char const *symbolName, int code, char const *alias);

  // add a new production; the rhs arg list must be terminated with a NULL
  void addProduction(Nonterminal *lhs, Symbol *rhs, ...);

  // add a pre-constructed production
  void addProduction(Production *prod);

  // ---------- outputting a grammar --------------
  // print the current list of productions
  void printProductions(ostream &os, bool printCode=true) const;

  // emit C++ code to construct this grammar later
  void emitSelfCC(ostream &os) const;

  // ---- whole-grammar stuff ----
  // after adding all rules, check that all nonterminals have
  // at least one rule; also checks referential integrity
  // in actions and conditions; throw exception if there is a
  // problem
  void checkWellFormed() const;

  // output grammar in Bison's syntax
  // (coincidentally, when bison dumps its table with '-v', its table
  // dump syntax is identical to my (current) input syntax!)
  void printAsBison(ostream &os) const;

  #if 0
    // ---- grammar parsing (obsolete) ----
    // these are retained because a few test codes use them
    bool readFile(char const *fname);

    // parse a line like "LHS -> R1 R2 R3", return false on parse error
    bool parseLine(char const *grammarLine);
    bool parseLine(char const *preLine, SObjList<Production> &lastProductions);
  #endif // 0


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

  // map a production to a unique index
  int getProductionIndex(Production const *prod) const;
};


#endif // __GRAMMAR_H

