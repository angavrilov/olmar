// useract.h
// declarations for user-supplied reduction action functions

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


#endif // USERACT_H
