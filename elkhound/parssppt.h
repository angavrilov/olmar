// parssppt.h
// parser-support routines, for use at runtime while processing
// the generated Parse tree

#ifndef __PARSSPPT_H
#define __PARSSPPT_H
               
#include "glrtree.h"      // NonterminalNode, etc.
#include "glr.h"          // GLR
#include "lexer2.h"       // Lexer2


// ----------------- helpers for analysis drivers ---------------
// a self-contained parse tree (or parse DAG, as the case may be)
struct ParseTree {
public:
  // we need a place to put the ground tokens
  Lexer2 lexer2;

  // this holds the grammar and the parse tree
  GLR glr;

public:
  ParseTree();
  ~ParseTree();

  // get the tree top
  TreeNode const *getTop() const;
};

// given grammar and input, yield a parse tree
ParseTree * /*owner*/ toplevelParse(char const *grammarFname,
                                    char const *inputFname,
                                    char const *symOfInterestName = NULL);

// useful for simple treewalkers
ParseTree * /*owner*/ treeMain(int argc, char **argv);


// ------------- interface to create nodes -------------
// constructs a tree node of some type; signature matches
// NonterminalNode's ctor
typedef NonterminalNode* (*TreeNodeCtorFn)(Reduction *red);

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


// these two are in the emitted C++ code:
// map from nonterminal indices to ctor/name
extern NonterminalInfo nontermMap[];

// length of this map; again, mostly as a consistency check
extern int nontermMapLength;


#endif // __PARSSPPT_H
