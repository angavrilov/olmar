// ccwrite.h
// emit C++ code to implement semantic functions

#error This module is obsolete; gramanl emits the C++ code now.

#ifndef __CCWRITE_H
#define __CCWRITE_H

class GrammarAnalysis;
class NonterminalNode;
class Reduction;


// for generating C++ code; see ccwrite.cc for comments
void emitSemFunImplFile(char const *fname, char const *headerFname,
                        GrammarAnalysis const *g);
void emitSemFunDeclFile(char const *fname, GrammarAnalysis const *g);


#endif // __CCWRITE_H
