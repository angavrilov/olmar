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
    post(NULL),
    isAssignment(false)
{}

CandidateSet::CandidateSet(PreFilter r, PostFilter o, bool a)
  : poly(NULL),
    pre(r),
    post(o),
    isAssignment(a)
{}


// get the set of types that 't' can be converted to via a
// user-defined conversion operator, or by the identity conversion
void getConversionOperatorResults(Env &env, SObjList<Type> &dest, Type *t)
{
  if (!t->asRval()->isCompoundType()) {
    // this is for when one of the arguments to an operator is not
    // of class type; we treat it similarly to a class that can only
    // convert to one type
    dest.append(t);
  }
  else {
    t = t->asRval();

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

void CandidateSet::instantiateBinary(Env &env, OverloadResolver &resolver,
  OverloadableOp op, Type *lhsType, Type *rhsType)
{
  if (poly) {
    // polymorphic candidates are easy
    resolver.processCandidate(poly);
    return;
  }

  if (op == OP_ARROW_STAR) {
    instantiateArrowStar(env, resolver, lhsType, rhsType);
    return;
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
  //
  // See also: convertibility.txt

  // set of types with which to instantiate T
  SObjList<Type> instTypes;      // called 'S' in the comments above

  // collect all of the operator functions rettypes from lhs and rhs
  SObjList<Type> lhsRets, rhsRets;
  getConversionOperatorResults(env, lhsRets, lhsType);
  getConversionOperatorResults(env, rhsRets, rhsType);

  // consider all pairs of conversion results
  SFOREACH_OBJLIST_NC(Type, lhsRets, lhsIter) {
    Type *lhsRet = pre(lhsIter.data(), true /*isLeft*/);
    if (!lhsRet) continue;

    if (isAssignment) {
      // the way the assignment built-ins work, we only need to
      // instantiate with the types to which the LHS convert, to
      // get a complete set (i.e. no additional instantiations
      // would change the answer)
      addTypeUniquely(instTypes, lhsRet);
      continue;
    }

    SFOREACH_OBJLIST_NC(Type, rhsRets, rhsIter) {
      Type *rhsRet = pre(rhsIter.data(), false /*isLeft*/);
      if (!rhsRet) continue;

      // compute LUB
      bool wasAmbig;
      Type *lub = computeLUB(env, lhsRet, rhsRet, wasAmbig);
      if (wasAmbig) {
        addAmbigCandidate(env, resolver, op);
      }
      else if (lub && post(lub)) {
        // add the LUB to our list of to-instantiate types
        addTypeUniquely(instTypes, lub);
      }
    }
  }

  // instantiate T with all the types we've collected
  SFOREACH_OBJLIST_NC(Type, instTypes, iter) {
    // PLAN:  Right now, I just leak a bunch of things.  To fix this, I
    // want maintain a pool of Variables for use as instantiated
    // candidates.  When I instantiate (say) operator-(T,T) with a
    // specific T, I rewrite an existing one from the pool, or else make
    // a new one if the pool is exhausted.  Later I put all the elements
    // back into the pool.

    Type *T = iter.data();

    if (!isAssignment) {
      // parameters are symmetric
      Variable *v = env.createBuiltinBinaryOp(op, T, T);
      resolver.processCandidate(v);
    }

    else {
      // left parameter is a reference, and we need two versions,
      // one with volatile and one without
      //
      // update: now that I directly instantiate the LHS types,
      // without first stripping their volatile-ness, I don't need
      // to add volatile versions of types that don't already have
      // volatile versions present
      //Type *Tv = env.tfac.applyCVToType(SL_INIT, CV_VOLATILE, T, NULL /*syntax*/);

      Type *Tr = env.tfac.makeRefType(SL_INIT, T);
      //Type *Tvr = env.tfac.makeRefType(SL_INIT, Tv);

      resolver.processCandidate(env.createBuiltinBinaryOp(op, Tr, T));
      //resolver.processCandidate(env.createBuiltinBinaryOp(op, Tvr, T));
    }
  }
}


// If any LUB is ambiguous, then the final overload analysis over the
// infinite set of instantiations would also have been ambiguous.
//
// But it's not so simple as yielding an error; see in/t0140.cc for an
// example of why.  What we need to do is yield a candidate that will
// be ambiguous unless it's beaten by a candidate that is better than
// any of the built-in instantiations (e.g. a member operator).
//
// I defer to the overload module to make such a candidate; here I
// just say what I want.
void CandidateSet::addAmbigCandidate(Env &env, OverloadResolver &resolver,
  OverloadableOp op)
{
  Type *t_void = env.getSimpleType(SL_INIT, ST_VOID);
  Variable *v = env.createBuiltinBinaryOp(op, t_void, t_void);
  resolver.addAmbiguousBinaryCandidate(v);
}


// goal: instantiate pattern
//   CV12 T& operator->* (CV1 C1 *, CV2 T C2::*);
// by finding instantiation pairs (C1,C2)
void CandidateSet::instantiateArrowStar(Env &env,
  OverloadResolver &resolver, Type *lhsType, Type *rhsType)
{
  // these arrays record all pairs of types used to instantiate
  // the pattern, so that we ensure that no pair is instantiated
  // more than once
  ArrayStack<Type*> lhsInst, rhsInst;

  // collect all of the operator functions rettypes from lhs and rhs
  SObjList<Type> lhsRets, rhsRets;
  getConversionOperatorResults(env, lhsRets, lhsType);
  getConversionOperatorResults(env, rhsRets, rhsType);

  // consider all pairs of conversion results
  SFOREACH_OBJLIST_NC(Type, lhsRets, lhsIter) {
    Type *lhsRet = lhsIter.data();

    // expect 'lhsRet' to be of form 'class U cv1 *'; extract cv1 and U
    if (!lhsRet->isPointer()) continue;
    Type *lhsUnder = lhsRet->asPointerType()->atType;
    CVFlags cv1 = lhsUnder->getCVFlags();
    if (!lhsUnder->isCompoundType()) continue;
    CompoundType *U = lhsUnder->asCompoundType();

    SFOREACH_OBJLIST_NC(Type, rhsRets, rhsIter) {
      Type *rhsRet = rhsIter.data();

      // expect 'rhsRet' to be of form 'T cv2 V::*'; extract V
      if (!rhsRet->isPointerToMemberType()) continue;
      PointerToMemberType *rhsPtm = rhsRet->asPointerToMemberType();
      CompoundType *V = rhsPtm->inClass;

      // Now, any possible instantiation C1 and C2 must have an
      // inheritance relation like
      //
      //                   V
      //                  /
      //                C2
      //               /
      //             C1
      //            /
      //           U
      //
      // Furthermore, the order on conversions will push C1 towards U
      // and C2 towards V.  Hence the only instantiation necessary
      // for the (U,V) pair is (U,V) itself; and that's only if U and
      // V are related by inheritance.
      
      if (!U->hasBaseClass(V)) {
        // necessary relationship between U and V doesn't hold
        continue;
      }

      // build operator's parameter types
      Type *lhsParam = env.makePtrType(SL_INIT,
                         env.makeCVAtomicType(SL_INIT, U, cv1));
      Type *rhsParam = env.tfac.makePointerToMemberType(SL_INIT,
        V, CV_NONE /*pointer itself is CV_NONE*/, rhsPtm->atType);

      // have we instantiated this pair already?
      for (int i=0; i < lhsInst.length(); i++) {
        if (lhsInst[i]->equals(lhsParam) &&
            rhsInst[i]->equals(rhsParam)) {
          // this pair has already been instantiated
          goto after_instantiation;
        }
      }
      
      // instantiate the operator
      resolver.processCandidate(
        env.createBuiltinBinaryOp(OP_ARROW_STAR, lhsParam, rhsParam));

      // remember that this pair was instantiated
      lhsInst.push(lhsParam);
      rhsInst.push(rhsParam);

    after_instantiation:
      ;
    }
  }
}


// ------------------- filters -------------------
Type *rvalFilter(Type *t, bool)
{
  return t->asRval();
}

Type *rvalIsPointer(Type *t, bool)
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

// we admit the pattern
//   T VQ &
// where T is a pointer to any type (para 19), or
// is an enumeration or pointer-to-member (para 20)
//
// we yield the T for instantiation
Type *para19_20filter(Type *t, bool)
{
  if (!t->isReference()) {
    return NULL;
  }
  t = t->asRval();
  if (t->isConst()) {
    return NULL;
  }
  
  if (t->isPointerType() ||
      t->isEnumType() ||
      t->isPointerToMemberType()) {
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

bool pointerOrEnum(Type *t)
{
  return t->isPointer() || t->isEnumType();
}

bool pointerOrEnumOrPTM(Type *t)
{
  return t->isPointer() || t->isEnumType() || t->isPointerToMemberType();
}

bool pointerToAny(Type *t)
{
  // the LUB will ensure this, actually, but it won't
  // hurt to check again
  return t->isPointer();
}

bool anyType(Type *)
{                  
  return true;
}


// EOF
