// ccwrite.h
// emit C++ code to implement semantic functions

#ifndef __CCWRITE_H
#define __CCWRITE_H

class GrammarAnalysis;
class NonterminalNode;
class Reduction;


// for generating C++ code; see ccwrite.cc for comments
void emitSemFunImplFile(char const *fname, GrammarAnalysis const *g);
void emitSemFunDeclFile(char const *fname, GrammarAnalysis const *g);


// -------- for using the generated code ----------
// constructs a tree node of some type; signature matches
// NonterminalNode's ctor
typedef NonterminalNode (*TreeNodeCtorFn)(Reduction *red);

// describes a nonterminal, for use at runtime by the
// user of the generated code
struct NonterminalInfo {
  // call this fn to make a tree node of the right type
  TreeNodeCtorFn ctor;

  // this is here mainly to provide a consistency check
  // between the nonterminal set before and after the
  // C++ code is emitted and compiled
  char const *nontermName;
};

// map from nonterminal indices to ctor/name
extern NonterminalInfo nontermMap[];

// length of this map; again, mostly as a consistency check
extern int nontermMapLength;



#endif // __CCWRITE_H
