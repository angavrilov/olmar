// builtinops.h
// data structures to represent cppstd 13.6

#ifndef BUILTINOPS_H
#define BUILTINOPS_H

#include "cc_flags.h"      // BinaryOp

class Type;                // cc_type.h
class Variable;            // variable.h
class Env;                 // cc_env.h
class OverloadResolver;    // overload.h

// a set of candidates, usually one line of 13.6; since many of the
// sets in 13.6 are infinite, and the finite ones are large, the
// purpose of this class is to represent those sets in a way that
// allows overload resolution to act *as if* it used the full sets
class CandidateSet {
public:      // types
  // each potential argument type is passed through this filter before
  // being evaluated as part of a pair; it can return a different
  // type, and it can also return NULL to indicate that the type
  // shouldn't be considered; 'isLeft' says whether this is the left
  // arg or right arg type
  typedef Type* (*PreFilter)(Type *t, bool isLeft);

  // after computing a pairwise LUB, the LUB type is passed
  // through this filter; if it returns false, then the type
  // is not used to instantiate the pattern
  typedef bool (*PostFilter)(Type *t);

public:      // data
  // if this is non-NULL, the candidate is represented by
  // a single polymorphic function
  Variable *poly;          // (serf)

  // if poly==NULL, then this pair of functions filters the argument
  // types to a binary operator in a pairwise analysis to instantiate
  // a pattern rule
  PreFilter pre;
  PostFilter post;
  
  // if true, then this is a pattern rule for an assignment
  // operator; this affects how instantiation is done
  bool isAssignment;

public:      // funcs
  CandidateSet(Variable *v);
  CandidateSet(PreFilter pre, PostFilter post, bool isAssignment);

  // instantiate the pattern as many times as necessary, given the
  // argument types 'lhsType' and 'rhsType'
  void instantiateBinary(Env &env, OverloadResolver &resolver,
    OverloadableOp op, Type *lhsType, Type *rhsType);
};


// some pre filters
Type *rvalFilter(Type *t, bool);
Type *rvalIsPointer(Type *t, bool);
Type *rvalIsPointer_leftIsRef(Type *t, bool isLeft);

// some post filters
bool pointerToObject(Type *t);
bool pointerOrEnum(Type *t);
bool pointerOrEnumOrPTM(Type *t);
bool pointerToAny(Type *t);


#endif // BUILTINOPS_H
