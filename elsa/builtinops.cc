// builtinops.cc
// code for builtinops.h

#include "builtinops.h"    // this module
#include "cc_type.h"       // Type, etc.
#include "cc_env.h"        // Env
#include "overload.h"      // getConversionOperators, OverloadResolver


// ------------------ CandidateSet -----------------
CandidateSet::CandidateSet(Variable *v)
  : poly(v),
    pre(NULL),
    post(NULL)
{}

CandidateSet::CandidateSet(PreFilter r, PostFilter o)
  : poly(NULL),
    pre(r),
    post(o)
{}

Variable *CandidateSet::instantiatePattern(Env &env, BinaryOp op, Type *t)
{
  // PLAN:  Right now, I just leak a bunch of things.  To fix this, I
  // want maintain a pool of Variables for use as instantiated
  // candidates.  When I instantiate (say) operator-(T,T) with a
  // specific T, I rewrite an existing one from the pool, or else make
  // a new one if the pool is exhausted.  Later I put all the elements
  // back into the pool.
  return env.createBuiltinBinaryOp(op, t, t);
}


// get the set of types that 't' can be converted to via a
// user-defined conversion operator, or by the identity conversion
void getConversionOperatorResults(Env &env, SObjList<Type> &dest, Type *t)
{
  if (!t->isCompoundType()) {
    // this is for when one of the arguments to an operator is not
    // of class type; we treat it similarly to a class that can only
    // convert to one type
    dest.append(t);
  }
  else {
    // get conops
    SObjList<Variable> convs;
    getConversionOperators(convs, env, t->asCompoundType());

    // get return types
    dest.reverse();
    SFOREACH_OBJLIST_NC(Variable, convs, iter) {
      dest.prepend(iter.data()->type->asFunctionType()->retType);
    }
    dest.reverse();
  }
}

// add 't' to 'instTypes', but only if it isn't already present
void addTypeUniquely(SObjList<Type> &instTypes, Type *t)
{
  SFOREACH_OBJLIST(Type, instTypes, iter) {
    if (iter.data()->equals(t)) {
      return;   // already present
    }
  }
  instTypes.append(t);
}

bool CandidateSet::instantiateBinary(Env &env, OverloadResolver &resolver,
  BinaryOp op, Type *lhsType, Type *rhsType)
{
  if (poly) {
    // polymorphic candidates are easy
    resolver.processCandidate(poly);
    return true;    // no problem
  }

  // pattern candidate with correlated parameter types,
  // e.g. (13.6 para 4):
  //
  //   For every T, where T is a pointer to object type, there exist
  //   candidate operator functions of the form
  //     ptrdiff_t operator-(T, T);
  //
  // The essential idea here is to compute a set of types with
  // which to instantiate the above pattern.  Naively, we'd like
  // to instantiate it with all types, but of course that is not
  // practical.  Instead, we compute a finite set S of types such
  // that we get the same answer as we would have if we had
  // instantiated it with all types.  Note that if we compute S
  // correctly, it will be the case that any superset of S would
  // also work; we do not necessarily compute the minimal S.
  //
  // The core of the analysis is the least-upper-bound function
  // on types.  Given a pair of types, the LUB type
  // instantiation would win against all the other
  // instantiations the LUB dominates, in the final overload
  // analysis.  Thus we populate S with the LUBs of all pairs
  // of types the arguments' con-ops can yield.

  // set of types with which to instantiate T
  SObjList<Type> instTypes;      // called 'S' in the comments above

  // collect all of the operator functions rettypes from lhs and rhs
  SObjList<Type> lhsRets, rhsRets;
  getConversionOperatorResults(env, lhsRets, lhsType);
  getConversionOperatorResults(env, rhsRets, rhsType);

  // consider all pairs of conversion results
  SFOREACH_OBJLIST_NC(Type, lhsRets, lhsIter) {
    Type *lhsRet = pre(lhsIter.data());
    if (!lhsRet) continue;

    SFOREACH_OBJLIST_NC(Type, rhsRets, rhsIter) {
      Type *rhsRet = pre(rhsIter.data());
      if (!rhsRet) continue;

      // compute LUB
      bool wasAmbig;
      Type *lub = computeLUB(env, lhsRet, rhsRet, wasAmbig);
      if (wasAmbig) {
        // if any LUB is ambiguous, then the final overload analysis
        // over the infinite set of instantiations would also have
        // been ambiguous
        env.error(stringc
          << "In resolving operator" << toString(op)
          << ", LHS can convert to "
          << lhsRet->toString()
          << ", and RHS can convert to "
          << rhsRet->toString()
          << ", but their LUB is ambiguous");
        return false;
      }
      else if (lub && post(lub)) {
        // add the LUB to our list of to-instantiate types
        addTypeUniquely(instTypes, lub);
      }
    }
  }

  // instantiate T with all the types we've collected
  SFOREACH_OBJLIST_NC(Type, instTypes, iter) {
    Variable *v = instantiatePattern(env, op, iter.data());
    resolver.processCandidate(v);
  }

  return true;
}


// ------------------- filters -------------------
Type *rvalIsPointer(Type *t)
{
  // 13.3.1.5: functions that return 'T&' are regarded as
  // yielding 'T' for purposes of this analysis
  t = t->asRval();

  // only pointer types need consideration
  if (t->isPointer()) {
    return t;
  }
  else {
    return NULL;
  }
}

bool pointerToObject(Type *t)
{
  // the pre-filter already guarantees that only pointer types
  // can result from the LUB, but we need to ensure that
  // only pointer-to-object types are instantiated
  Type *at = t->asPointerType()->atType;
  if (at->isVoid() || at->isFunctionType()) {
    return false;    // don't instantiate with this one
  }
  else {
    return true;
  }
}


// EOF
