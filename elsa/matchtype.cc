// matchtype.cc
// code for matchtype.h


#include "matchtype.h"       // this module
#include "variable.h"        // Variable
#include "trace.h"           // trace


// FIX: I'll bet the matchDepth here isn't right; not sure what should
// go there
//   if (b->isReferenceToConst()) return match0(a, b->getAtType(), matchDepth);

int MatchTypes::recursionDepthLimit = 500; // what g++ 3.4.0 uses by default

// ---------------------- utilities ---------------------

// this is bizarre, but I can't think of a better way
class CVFlagsIter {
  int which;
  public:
  CVFlagsIter() : which(0) {}
  CVFlags data() {
    switch(which) {
    default: xfailure("urk!?");
    case 0: return CV_CONST;
    case 1: return CV_VOLATILE;
    }
  }
  bool isDone() { return which == 1; }
  void adv() {
    xassert(which<1);
    ++which;
  }
};


// -------------------- XMatchDepth ---------------------
// thrown when the match0() function exceeds its recursion depth limit
class XMatchDepth : public xBase {
public:
  XMatchDepth();
  XMatchDepth(XMatchDepth const &obj);
  ~XMatchDepth();
};

void throw_XMatchDepth() NORETURN;


// ---------------------- MatchBindings ---------------------

void MatchBindings::put0(Variable *key, STemplateArgument *val) {
  xassert(key);
  xassert(val);
  if (map.get(key)) xfailure("attempted to re-bind var");
  ++entryCount;                 // note: you can't rebind a var
  map.add(key, val);
}

void MatchBindings::putObjVar(Variable *key, STemplateArgument *val) {
  xassert(key);
  xassert(!key->type->isTypeVariable());
  put0(key, val);
}

void MatchBindings::putTypeVar(TypeVariable *key, STemplateArgument *val) {
  xassert(key);
  put0(key->typedefVar, val);
}

STemplateArgument *MatchBindings::get0(Variable *key) {
  xassert(key);
  return map.get(key);
}

STemplateArgument *MatchBindings::getObjVar(Variable *key) {
  xassert(key);
  xassert(!key->type->isTypeVariable());
  return get0(key);
}

STemplateArgument *MatchBindings::getTypeVar(TypeVariable *key) {
  xassert(key);
  return get0(key->typedefVar);
}

bool MatchBindings::isEmpty() {
  return entryCount == 0;
}

void MatchBindings::gdb()
{
  bindingsGdb(map);
}

void bindingsGdb(PtrMap<Variable, STemplateArgument> &bindings)
{
  cout << "Bindings" << endl;
  for (PtrMap<Variable, STemplateArgument>::Iter bindIter(bindings);
       !bindIter.isDone();
       bindIter.adv()) {
    Variable *key = bindIter.key();
    STemplateArgument *value = bindIter.value();
    cout << "'" << key->name << "' ";
    printf("Variable* key: %p ", key);
//      printf("serialNumber %d ", key->serialNumber);
    cout << endl;
    value->debugPrint();
  }
  cout << "Bindings end" << endl;
}

// ---------- MatchTypes private ----------

CVFlags const MatchTypes::normalCvFlagMask = CV_CONST | CV_VOLATILE;

//  // helper function for when we find an int var
//  static bool unifyToIntVar(Type *a,
//                            Type *b,
//                            StringSObjDict<STemplateArgument> &bindings,
//                            int matchDepth)
//  {
//  }

bool MatchTypes::subtractFlags(CVFlags acv, CVFlags bcv, CVFlags &finalFlags)
{
  finalFlags = CV_NONE;
  // partition qualifiers again
  CVFlags acvNormal = acv & normalCvFlagMask;
  CVFlags bcvNormal = bcv & normalCvFlagMask;
  //      CVFlags bcvScott  = bcv & ~normalCvFlagMask;
  // Lets kill a mosquito with a hydraulic wedge
  for(CVFlagsIter cvIter; !cvIter.isDone(); cvIter.adv()) {
    CVFlags curFlag = cvIter.data();
    int aflagInt = (acvNormal & curFlag) ? 1 : 0;
    int bflagInt = (bcvNormal & curFlag) ? 1 : 0;
    int flagDifference = aflagInt - bflagInt;
    if (flagDifference < 0) {
      return false;           // can't subtract a flag that isn't there
    }
    if (flagDifference > 0) {
      finalFlags |= curFlag;
    }
    // otherwise, the flags match
  }
  return true;
}


// bind value 'a' to var 'b'; I find the spec less than totally clear on what
// happens to qualifiers, but some experiments with g++ suggest the
// following policy that I implement.
//   1 - Top level cv qualifiers are ignored on both sides
//   2 - Below top level, the are matched exactly; a) lack of matching
//   means a failure to match; b) those matched are subtracted and the
//   remaining attached to the typevar.
bool MatchTypes::bindValToVar(Type *a, Type *b, int matchDepth)
{
  xassert(b->isTypeVariable());
  STemplateArgument *targa = new STemplateArgument;

  // deal with CV qualifier strangness;
  // dsw: Const should be removed from the language because of things
  // like this!  I mean, look at it!
  CVFlags acv = a->getCVFlags();
  // partition qualifiers into normal ones and Scott's funky qualifier
  // extensions that aren't const or volatile
//    CVFlags acvNormal = acv &  normalCvFlagMask;
  CVFlags acvScott  = acv & ~normalCvFlagMask;
  if (matchDepth == 0) {
    // if we are at the top level, remove all CV qualifers before
    // binding; ignore the qualifiers on the type variable as they are
    // irrelevant
    a = tfac.setCVQualifiers(SL_UNKNOWN, CV_NONE, a, NULL /*syntax*/);
  } else {
    // if we are below the top level, subtract off the CV qualifiers
    // that match; if we get a negative qualifier set (arg lacks a
    // qualifer that the param has) we fail to match
    CVFlags finalFlags;
    bool aCvSuperBcv = subtractFlags(acv, b->getCVFlags(), finalFlags);
    if (!aCvSuperBcv) return false;

    // if there's been a change, must (shallow) clone a and apply new
    // CV qualifiers
    a = tfac.setCVQualifiers
      (SL_UNKNOWN,
       finalFlags | acvScott,  // restore Scott's funkyness
       a,
       NULL /*syntax*/
       );
  }

  targa->setType(a);
  bindings.putTypeVar(b->asTypeVariable(), targa);
  return true;
}


bool MatchTypes::match_rightTypeVar(Type *a, Type *b, int matchDepth)
{
  xassert(b->isTypeVariable());
  switch(mode) {
  default: xfailure("illegal MatchTypes mode"); break;

  case MM_BIND: {
    if (a->isTypeVariable()) xfailure("MatchTypes: got a type variable on the left");
    STemplateArgument *targb = bindings.getTypeVar(b->asTypeVariable());
    if (targb) {
      return targb->kind==STemplateArgument::STA_TYPE
        && match0(a, targb->value.t,
                  matchDepth    // FIX: this used to be not top, but I don't remember why
                  );
    } else {
      return bindValToVar(a, b, matchDepth);
    }
    break;
  } // end case MM_BIND

  case MM_WILD: {
    // match unless the cv qualifiers prevent us from not matching by
    // b having something a doesn't have
    CVFlags finalFlags;
    return subtractFlags(a->getCVFlags(),
                         b->getCVFlags(),
                         finalFlags);
    break;
  } // end case MM_WILD

  case MM_ISO: {
    STemplateArgument *targb = bindings.getTypeVar(b->asTypeVariable());
    if (targb) {
      if (targb->kind!=STemplateArgument::STA_TYPE) return false;
      if (!a->isTypeVariable()) return false;
      xassert(targb->value.t->isTypeVariable());
      // since this is the MM_ISO case, they must be semantically
      // identical
      return
        // must have the same qualifiers; FIX: I think even at the top level
        ((a->getCVFlags() & normalCvFlagMask) ==
         (targb->value.t->getCVFlags() & normalCvFlagMask))
        &&
        // must be the same typevar; NOTE: don't compare the types, as
        // they can change when cv qualifiers are added etc. but the
        // variables have to be the same
        ((a->asTypeVariable()->typedefVar ==
          targb->value.t->asTypeVariable()->typedefVar));
    } else {
      if (a->isTypeVariable()) {
        return bindValToVar(a, b, matchDepth);
      } else {
        return false;
      }
    }
    break;
  } // end case MM_ISO
  } // end switch
}


bool MatchTypes::match_cva(CVAtomicType *a, Type *b, int matchDepth)
{
  //   A non-ref, B ref to const: B's ref goes away
  if (b->isReferenceToConst()) return match0(a, b->getAtType(), matchDepth);
  //   A non-ref, B ref to non-const: failure to unify
  else if (b->isReference()) return false;

  // NOTE: do NOT reverse the order of the following two tests
  if (b->isTypeVariable()) return match_rightTypeVar(a, b, matchDepth);
  if (a->isTypeVariable()) {
    switch(mode) {
    default: xfailure("illegal MatchTypes mode"); break;

    case MM_BIND:
      xfailure("MatchTypes: got a type variable on the left");
      break;

    case MM_WILD:
      return false;
      break;

    case MM_ISO:
      return false;
      break;

    } // end switch
  }

  if (b->isCVAtomicType()) {
    // deal with top-level qualifiers; if we are not at MT_TOP, then
    // they had better match exactly
    if (matchDepth > 0) {
      CVFlags const mask = normalCvFlagMask; // ignore other kinds of funky Scott-flags
      if ( (a->getCVFlags() & mask) != (b->getCVFlags() & mask) ) return false;
    }

    // if they pass, then deal with the types themselves
    bool aIsCpdTemplate = a->isCompoundType()
      && a->asCompoundType()->typedefVar->isTemplate();
    bool bIsCpdTemplate = b->isCompoundType()
      && b->asCompoundType()->typedefVar->isTemplate();
    if (aIsCpdTemplate && bIsCpdTemplate) {
      TemplateInfo *aTI = a->asCompoundType()->typedefVar->templateInfo();
      TemplateInfo *bTI = b->asCompoundType()->typedefVar->templateInfo();
      // two type variable's classes match iff their template info's
      // match; you don't want to check for equality of the variables
      // because two instances of the same template will both be class
      // Foo but they will be different type variables; therefore,
      // don't write this code, even in conjunction with the below
      // code
      //   aTDvar == bTDvar /* bad and wrong */
      return match_TInfo(aTI, bTI,
                         // used to be not top but I don't remember why
                         0 /*matchDepth*/);
    } else if (!aIsCpdTemplate && !bIsCpdTemplate) {
      AtomicType *aAt = a->asCVAtomicType()->atomic;
      AtomicType *bAt = b->asCVAtomicType()->atomic;
      // I don't want to call BaseType::equals() at all since I'm
      // essentially duplicating my version of that; however I do need
      // to be able to ask if to AtomicType-s are equal.
      return aAt->equals(bAt);
    }
    // if there is a mismatch, they definitely don't match
  }
  return false;
}


bool MatchTypes::match_ptr(PointerType *a, Type *b, int matchDepth)
{
  //   A non-ref, B ref to const: B's ref goes away
  if (b->isReferenceToConst()) return match0(a, b->getAtType(), matchDepth);
  //   A non-ref, B ref to non-const: failure to unify
  else if (b->isReference()) return false;

  if (b->isTypeVariable()) return match_rightTypeVar(a, b, matchDepth);

  if (b->isPointer()) return match0(a->getAtType(), b->getAtType(), 2 /*matchDepth*/);
  return false;
}


bool MatchTypes::match_ref(ReferenceType *a, Type *b, int matchDepth)
{
  // NOTE: the line below occurs in all other five match_TYPE(TYPE *a,
  // Type *b, MFflags, matchDepth) methods EXCEPT IN THIS ONE.  THIS IS ON
  // PURPOSE.
//    if (b->isTypeVariable()) return match_rightTypeVar(a, b, matchDepth);

  // The policy on references and unification is as follows.
  //
  //   A non-ref, B ref to const: B's ref goes away
  //   A non-ref, B ref to non-const: failure to unify
  //     do those in each function
  //
  //   A ref, B non-ref: A's ref-ness silently goes away.
  //   A ref to non-const, B ref to const: both ref's and the const go away
  //     and unification continues below.
  //   A ref, B ref: the ref's match and unification continues below.
  //     do those here

  int matchDepth0 = 1;
  if (b->isReference()) {
    b = b->getAtType();
    if (b->isConst() && !a->isConst()) {
      // I think this is a special circumstance under which we remove
      // the const on b.  FIX: this seems quite ad-hoc to me.  It
      // seems to be what is required to get in/big/nsAtomTable.i to
      // go through; But how to get rid of the const in Scott's type
      // system!  This is the only way I can think of: start again at
      // the top.
      matchDepth0 = 0;
    }
  }
  return match0(a->getAtType(), b,
                // starting at line 22890 in
                // in/big/nsCLiveconnectFactory.i there is an example
                // of two function template declarations that differ
                // only in the fact that below the ref level one is a
                // const and one is not; experiments with g++ confirm
                // this; so most of the time we use matchDepth==1
                matchDepth0
                );
}


bool MatchTypes::match_func(FunctionType *a, Type *b, int matchDepth)
{
  //   A non-ref, B ref to const: B's ref goes away
  if (b->isReferenceToConst()) return match0(a, b->getAtType(), matchDepth);
  //   A non-ref, B ref to non-const: failure to unify
  else if (b->isReference()) return false;

  if (b->isTypeVariable()) return match_rightTypeVar(a, b, matchDepth);

  if (b->isPointer()) {
    // cppstd 14.8.2.1 para 2: "If P is not a reference type: --
    // ... If A is a function type, the pointer type produced by the
    // function-to-pointer standard conversion (4.3) is used in place
    // of A for type deduction"; rather than wrap the function type in
    // a pointer, I'll just unwrap the pointer-ness of 'b' and keep
    // going down.
    return match0(a, b->getAtType(), 2 /*matchDepth*/);
  }

  if (b->isFunctionType()) {
    FunctionType *ftb = b->asFunctionType();

    // check all the parameters
    if (a->params.count() != ftb->params.count()) {
      return false;
    }
    SObjListIterNC<Variable> iterA(a->params);
    SObjListIterNC<Variable> iterB(ftb->params);
    for(;
        !iterA.isDone();
        iterA.adv(), iterB.adv()) {
      Variable *varA = iterA.data();
      Variable *varB = iterB.data();
      xassert(!varA->hasFlag(DF_TYPEDEF)); // should not be possible
      xassert(!varB->hasFlag(DF_TYPEDEF)); // should not be possible
      if (!match0
          (varA->type, varB->type,
           // FIX: I don't know if this is right: are we at the top
           // level again when we recurse down into the parameters of
           // a function type that is itself an argument?
           0 /*matchDepth*/)) {
        return false; // conjunction
      }
    }
    xassert(iterB.isDone());

    // check the return type
    return match0
      (a->retType, ftb->retType,
       // FIX: I don't know if this is right: are we at the top level
       // again when we recurse down into the rerturn value (just as
       // with the parameters) of a function type that is itself an
       // argument?
       0 /*matchDepth*/);
  }
  return false;
}


bool MatchTypes::match_array(ArrayType *a, Type *b, int matchDepth)
{
  //   A non-ref, B ref to const: B's ref goes away
  if (b->isReferenceToConst()) return match0(a, b->getAtType(), matchDepth);
  //   A non-ref, B ref to non-const: failure to unify
  else if (b->isReference()) return false;

  if (b->isTypeVariable()) return match_rightTypeVar(a, b, matchDepth);

  if (b->isPointer() && matchDepth < 2) {
    // cppstd 14.8.2.1 para 2: "If P is not a reference type: -- if A
    // is an array type, the pointer type produced by the
    // array-to-pointer standard conversion (4.2) is used in place of
    // A for type deduction"; however, this only seems to apply at the
    // top level; see cppstd 14.8.2.4 para 13.
    return match0(a->eltType,
                  b->asPointerType()->atType,
                  2 /*matchDepth*/);
  }

  // FIX: if we are not at the top level then the array indicies
  // should be matched as well when we do Object STemplateArgument
  // matching
  if (b->isArrayType()) {
    return match0(a->eltType, b->asArrayType()->eltType, 2 /*matchDepth*/);
  }
  return false;
}


bool MatchTypes::match_ptm(PointerToMemberType *a, Type *b, int matchDepth)
{
  //   A non-ref, B ref to const: B's ref goes away
  if (b->isReferenceToConst()) return match0(a, b->getAtType(), matchDepth);
  //   A non-ref, B ref to non-const: failure to unify
  else if (b->isReference()) return false;

  if (b->isTypeVariable()) return match_rightTypeVar(a, b, matchDepth);
  if (b->isPointerToMemberType()) {
    return match_Atomic(a->inClassNAT, b->asPointerToMemberType()->inClassNAT, 0 /*matchDepth*/)
    && match0(a->atType, b->getAtType(), 2 /*matchDepth*/);
  }
  return false;
}


bool MatchTypes::match_Atomic(AtomicType *a, AtomicType *b, int matchDepth)
{
  // FIX: should there be some subtyping polymorphism here?
  //
  // I have to wrap the CompoundType in a CVAtomicType just so I can
  // do the unification;
  //
  // DO NOT MAKE THESE ON THE STACK as one might be a type variable
  // and the other then would get unified into permanent existence
  // on the heap
  CVAtomicType *aCv = tfac.makeCVAtomicType(SL_UNKNOWN, a, CV_NONE);
  CVAtomicType *bCv = tfac.makeCVAtomicType(SL_UNKNOWN, b, CV_NONE);
  return match0(aCv, bCv, matchDepth);
}


// FIX: AN EXACT COPY OF MatchTypes::match_Lists().  Have to figure
// out how to fix that.
//
// NOTE: the SYMMETRY in the list serf/ownerness.
bool MatchTypes::match_Lists2
  (ObjList<STemplateArgument> &listA,
   ObjList<STemplateArgument> &listB,
   int matchDepth)
{
  // FIX: why assert this?
//    xassert(!(mFlags & MT_TOP));

  ObjListIterNC<STemplateArgument> iterA(listA);
  ObjListIterNC<STemplateArgument> iterB(listB);

  while (!iterA.isDone() && !iterB.isDone()) {
    STemplateArgument *sA = iterA.data();
    STemplateArgument *sB = iterB.data();
    if (!match_STA(sA, sB, matchDepth)) {
      return false;
    }

    iterA.adv();
    iterB.adv();
  }

  return iterA.isDone() && iterB.isDone();
}


bool MatchTypes::match_TInfo(TemplateInfo *a, TemplateInfo *b, int matchDepth)
{
  // FIX: why assert this?
//    xassert(!(mFlags & MT_TOP));

  // are we from the same primary even?
  TemplateInfo *ati = a->getMyPrimaryIdem();
  xassert(ati);
  TemplateInfo *bti = b->getMyPrimaryIdem();
  xassert(bti);
  // FIX: why did I do it this way?  It seems that before if a and b
  // were equal and both primaries that they would fail to match
//    if (!ati || (ati != bti)) return false;
  if (ati != bti) return false;

  // do we match?
  //
  // If b is a primary then any a with the same priary (namely b) will
  // match it.  FIX: I wonder if we are going to omit some bindings
  // getting created in the the modes that create bindings by doing
  // this?
  //
  // FIX: I treat mutants just like anything else; I think this is
  // right
  if ((!b->isMutant()) && b->isPrimary()) return true;
  return match_Lists2(a->arguments, b->arguments, matchDepth);
}


// helper function for when we find an int
bool MatchTypes::unifyIntToVar(int i0, Variable *v1)
{
  STemplateArgument *v1old = bindings.getObjVar(v1);
  // is this variable already bound?
  if (v1old) {
    // check that the current value matches the bound value
    if (v1old->kind==STemplateArgument::STA_INT) {
      return v1old->value.i == i0;
    } else if (v1old->kind==STemplateArgument::STA_TYPE) {
      return false;             // types don't match objects
    } else {
      xfailure("illegal template unification binding");
    }
  }
  // otherwise, bind it
  STemplateArgument *a1new = new STemplateArgument;
  a1new->setInt(i0);
  bindings.putObjVar(v1, a1new);
  return true;
}


bool MatchTypes::match_STA(STemplateArgument *a, STemplateArgument const *b, int matchDepth)
{
  // FIX: why assert this?
//    xassert(!(mFlags & MT_TOP));

  switch (a->kind) {
  case STemplateArgument::STA_TYPE: // type argument
    if (STemplateArgument::STA_TYPE != b->kind) return false;
    return match_Type(a->value.t, b->value.t, 0 /*matchDepth*/);
    break;

  case STemplateArgument::STA_INT: // int or enum argument
    if (STemplateArgument::STA_INT == b->kind) {
      return a->value.i == b->value.i;
    } else if (STemplateArgument::STA_REFERENCE == b->kind) {
      return unifyIntToVar(a->value.i, b->value.v);
    } else {
      return false;
    }
    break;

  case STemplateArgument::STA_REFERENCE: // reference to global object
    if (STemplateArgument::STA_INT == b->kind) {
      // you can't unify a reference with an int
      return false;
    }
    if (STemplateArgument::STA_REFERENCE == b->kind) {
      if (mode == MM_WILD ||
          // FIX: THIS IS WRONG.  We should save the binding and
          // lookup and check if they match just as we would do for
          // type variables.
          mode == MM_ISO) {
        return true;
      }
    }
    // FIX: do MM_BIND mode also
    xfailure("reference arguments unimplemented");
    break;

  case STemplateArgument::STA_POINTER: // pointer to global object
    // FIX:
    xfailure("pointer arguments unimplemented");

  case STemplateArgument::STA_MEMBER: // pointer to class member
    // FIX:
    xfailure("pointer to member arguments unimplemented");
    break;

  case STemplateArgument::STA_TEMPLATE: // template argument (not implemented)
    // FIX:
    xfailure("STemplateArgument::STA_TEMPLATE non implemented");
    break;

  default:
    xfailure("illegal STemplateArgument::kind");
    break;
  }
}


bool MatchTypes::match0(Type *a, Type *b, int matchDepth)
{
  // prevent infinite loops; see note at the top of matchtype.h
  ++recursionDepth;
  if (recursionDepth > recursionDepthLimit) {
    // FIX: this should be a user error, but that would entail having
    // access to an Env object which is hard to pass down in one place
    // or putting try/catches everywhere; I'll leave it until we think
    // of the best solution.
    xfailure(stringc << "during type matching, recursion depth exceeded the limit " <<
             recursionDepthLimit);
//      env.error(stringc << "during type matching, recursion depth exceeded the limit " <<
//                recursionDepthLimit, EF_STRONG);
    throw_XMatchDepth(); 
  }

  // roll our own dynamic dispatch
  switch (a->getTag()) {
    default: xfailure("bad tag");
    case Type::T_ATOMIC:          return match_cva(a->asCVAtomicType(),        b, matchDepth);
    case Type::T_POINTER:         return match_ptr(a->asPointerType(),         b, matchDepth);
    case Type::T_REFERENCE:       return match_ref(a->asReferenceType(),       b, matchDepth);
    case Type::T_FUNCTION:        return match_func(a->asFunctionType(),       b, matchDepth);
    case Type::T_ARRAY:           return match_array(a->asArrayType(),         b, matchDepth);
    case Type::T_POINTERTOMEMBER: return match_ptm(a->asPointerToMemberType(), b, matchDepth);
  }

  --recursionDepth;
}


// ---------- MatchTypes public methods ----------

MatchTypes::MatchTypes(TypeFactory &tfac0, MatchMode mode0)
  : tfac(tfac0), mode(mode0), recursionDepth(0)
{
  xassert(mode!=MM_NONE);
  if (tracingSys("shortMTRecDepthLimit")) {
    recursionDepthLimit = 10;   // make test for this feature go alot faster
  }
}

MatchTypes::~MatchTypes()
{}


bool MatchTypes::match_Type(Type *a, Type *b, int matchDepth)
{
  return match0(a, b, matchDepth);
}


bool MatchTypes::match_Lists
  (SObjList<STemplateArgument> &listA,
   ObjList<STemplateArgument> &listB, // NOTE: Assymetry in the list serf/ownerness
   int matchDepth)
{
  // FIX: why assert this?
//    xassert(!(mFlags & MT_TOP));

  SObjListIterNC<STemplateArgument> iterA(listA);
  ObjListIterNC<STemplateArgument> iterB(listB);

  while (!iterA.isDone() && !iterB.isDone()) {
    STemplateArgument *sA = iterA.data();
    STemplateArgument *sB = iterB.data();
    if (!match_STA(sA, sB, matchDepth)) {
      return false;
    }

    iterA.adv();
    iterB.adv();
  }

  return iterA.isDone() && iterB.isDone();
}


// EOF
