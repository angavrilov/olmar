// useract.h
// declarations for user-supplied reduction action functions;
// the code appears in the .cc file generated by 'gramanl' from
// an associated .gr file

#ifndef USERACT_H
#define USERACT_H


// semantic values are represented as void* since their type is
// determined by the user; I use the name here instead of 'void*'
// is they're a little easier to recognize by visual inspection;
// the serf/owner status of these pointers is a design point yet to
// be fully worked out
typedef void *SemanticValue;          // (???)

// user-supplied reduction actions: production 'id' is being used, and
// 'svals' contains an array of semantic values yielded by the RHS
// symbols, such that the 0th element is the leftmost RHS element;
// this returns the semantic value for the reduction
SemanticValue doReductionAction(int productionId, SemanticValue *svals);

// a given semantic value is about to be passed a second (or third..)
// time to a reduction action function, and the user may need to
// copy it, or increment its refcount, etc.
SemanticValue duplicateTerminalValue(int termId, SemanticValue sval);
SemanticValue duplicateNontermValue(int nontermId, SemanticValue sval);
                                                             
// a semantic value didn't get passed to an action function, either
// because it was never used at all (e.g. a semantic value for a
// punctuator token, which the user can simply ignore), or because
// we duplicated it in response to a local ambiguity, but then that
// parse turned out not to be viable, so we're cancelling the dup now
void deallocateTerminalValue(int termId, SemanticValue sval);
void deallocateNontermValue(int nontermId, SemanticValue sval);


#endif // USERACT_H
