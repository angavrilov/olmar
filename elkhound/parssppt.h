// parssppt.h
// parser-support routines, for use at runtime while processing
// the generated Parse tree

#ifndef __PARSSPPT_H
#define __PARSSPPT_H

#include "lexer2.h"       // Lexer2
#include "useract.h"      // SemanticValue


// ----------------- helpers for analysis drivers ---------------
// a self-contained parse tree (or parse DAG, as the case may be)
class ParseTreeAndTokens {
public:
  // reference to place to store final semantic value
  SemanticValue &treeTop;

  // we need a place to put the ground tokens
  Lexer2 lexer2;

public:
  ParseTreeAndTokens(SemanticValue &top);
  ~ParseTreeAndTokens();
};


// given grammar and input, yield a parse tree
// returns false on error
bool toplevelParse(ParseTreeAndTokens &ptree, char const *grammarFname,
                   char const *inputFname, char const *symOfInterestName);

// useful for simple treewalkers; false on error
bool treeMain(ParseTreeAndTokens &ptree, int argc, char **argv);


#endif // __PARSSPPT_H
