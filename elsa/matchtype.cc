// matchtype.cc
// code for matchtype.h

// FIX: I match too liberally: A pointer to a const pointer will match
// a pointer to a non-const pointer, even though they shouldn't since
// the const-ness differs and not at the top.

#include "matchtype.h"       // this module
#include "variable.h"        // Variable


bool VoidPairSet::lookup(void *a, void *b, bool insertIfMissing) {
  Pair const p0(a, b);          // stack temporary
  unsigned int h = p0.hashvalue();
  ObjList<Pair> *pairList =
    reinterpret_cast<ObjList<Pair>*>
    (setMap.get(reinterpret_cast<void*>(h)));
  if (!pairList) {
    if (insertIfMissing) {
      pairList = new ObjList<Pair>;
      pairList->prepend(new Pair(p0));
      setMap.add(reinterpret_cast<void*>(h), pairList);
      ObjList<Pair> *pairList0 =
        reinterpret_cast<ObjList<Pair>*>
        (setMap.get(reinterpret_cast<void*>(h)));
      xassert(pairList0 == pairList);
    }
    return false;
  }
  // It is mighty rare you are going to have to iterate over this loop
  // more than once.
  if (pairList->count() > 1) {
    cout << "VoidPairSet: Wow!" << endl;
  }
  FOREACH_OBJLIST(Pair, *pairList, iter) {
    Pair const *p1 = iter.data();
    xassert(h == p1->hashvalue());
    if (p0.equals(p1)) return true;
  }
  // This is super rare too.
  cout << "VoidPairSet: Amazing!" << endl;
  if (insertIfMissing) {
    pairList->prepend(new Pair(p0));
  }
  return false;
}


// ---------------------- match ---------------------

MatchTypes::~MatchTypes()
{}


bool MatchTypes::match
  (SObjList<STemplateArgument> &listA,
   ObjList<STemplateArgument> &listB, // NOTE: Assymetry in the list serf/ownerness
   MFlags mFlags)
{
  xassert(!(mFlags & MT_TOP));

  SObjListIterNC<STemplateArgument> iterA(listA);
  ObjListIterNC<STemplateArgument> iterB(listB);

  while (!iterA.isDone() && !iterB.isDone()) {
    STemplateArgument *sA = iterA.data();
    STemplateArgument *sB = iterB.data();
    if (!match(sA, sB, mFlags)) {
      return false;
    }

    iterA.adv();
    iterB.adv();
  }

  return iterA.isDone() && iterB.isDone();
}


// FIX: EXACT COPY OF THE CODE ABOVE.  Have to figure out how to fix
// that.  NOTE: the SYMMETRY in the list serf/ownerness.
bool MatchTypes::match
  (ObjList<STemplateArgument> &listA,
   ObjList<STemplateArgument> &listB,
   MFlags mFlags)
{
  xassert(!(mFlags & MT_TOP));

  ObjListIterNC<STemplateArgument> iterA(listA);
  ObjListIterNC<STemplateArgument> iterB(listB);

  while (!iterA.isDone() && !iterB.isDone()) {
    STemplateArgument *sA = iterA.data();
    STemplateArgument *sB = iterB.data();
    if (!match(sA, sB, mFlags)) {
      return false;
    }

    iterA.adv();
    iterB.adv();
  }

  return iterA.isDone() && iterB.isDone();
}


bool MatchTypes::match(TemplateInfo *a, TemplateInfo *b, MFlags mFlags)
{
  xassert(!(mFlags & MT_TOP));
  // are we from the same primary even?
  TemplateInfo *ati = a->getMyPrimary();
  TemplateInfo *bti = b->getMyPrimary();
  if (!ati || (ati != bti)) return false;
  // do we match?
  return match(a->arguments, b->arguments, mFlags);
}


// helper function for when we find an int
bool MatchTypes::unifyIntToVar(int i0, Variable *v1)
{
  STemplateArgument *v1old = bindings.queryif(v1->name);
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
  bindings.add(v1->name, a1new);
  return true;
}


bool MatchTypes::match(STemplateArgument *a, STemplateArgument const *b, MFlags mFlags)
{
  xassert(!(mFlags & MT_TOP));

  switch (a->kind) {
  case STemplateArgument::STA_TYPE:                // type argument
    if (STemplateArgument::STA_TYPE != b->kind) return false;
    return match(a->value.t, b->value.t,
                 // FIX: I have no idea why this cast is necessary
                 // here but not for other flag enums
                 //
                 // sm: b/c you need to add ENUM_BITWISE_OPS at decl..
                 // which I have now done
                 /*(MFlags)*/ (mFlags | MT_TOP) );
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
    // FIX:
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


void bindingsGdb(StringSObjDict<STemplateArgument> &bindings)
{
  cout << "Bindings" << endl;
  for (StringSObjDict<STemplateArgument>::Iter bindIter(bindings);
       !bindIter.isDone();
       bindIter.next()) {
    string const &name = bindIter.key();
    STemplateArgument *value = bindIter.value();
    cout << "'" << name << "' ";
    value->debugPrint();
  }
  cout << "Bindings end" << endl;
}


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


// helper function for when we find a type var; I find the spec less
// than totally clear on what happens to qualifiers, but some
// experiments with g++ suggest the following policy that I implement.
//   1 - Top level cv qualifiers are ignored on both sides
//   2 - Below top level, the are matched exactly; a) lack of matching
//   means a failure to match; b) those matched are subtracted and the
//   remaining attached to the typevar.
bool MatchTypes::unifyToTypeVar(Type *a, Type *b, MFlags mFlags)
{
  TypeVariable *tvb = b->asTypeVariable();
  STemplateArgument *targb = bindings.queryif(tvb->name);
  // if 'b' is already bound, resolve to its binding and recurse
  if (targb) {
    // Please note that it is an important part of the design that
    // pairs are cached and checked for in the top-level match
    // function just so that we can do this here and not worry about
    // infinite loops.
    return targb->kind==STemplateArgument::STA_TYPE
      && match(a, targb->value.t, mFlags & ~MT_TOP);
  }

  // otherwise, bind it
  STemplateArgument *targa = new STemplateArgument;

  // deal with CV qualifier strangness;
  // dsw: Const should be removed from the language because of things
  // like this!  I mean, look at it!
  CVFlags acv = a->getCVFlags();
  // partition qualifiers into normal ones and Scott's funky qualifier
  // extensions that aren't const or volatile
  CVFlags acvNormal = acv &  (CV_CONST | CV_NONE);
  CVFlags acvScott  = acv & ~(CV_CONST | CV_NONE);
  if (mFlags & MT_TOP) {
    // if we are at the top level, remove all CV qualifers before
    // binding; ignore the qualifiers on the type variable as they are
    // irrelevant
    a = tfac.setCVQualifiers(SL_UNKNOWN, CV_NONE, a, NULL /*syntax*/);
  } else {
    // if we are below the top level, subtract off the CV qualifiers
    // that match; if we get a negative qualifier set (arg lacks a
    // qualifer that the param has) we fail to match
    CVFlags finalFlags = CV_NONE;

    CVFlags bcv = b->getCVFlags();
    // partition qualifiers again
    CVFlags bcvNormal = bcv &  (CV_CONST | CV_NONE);
//      CVFlags bcvScott  = bcv & ~(CV_CONST | CV_NONE);
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
  bindings.add(tvb->name, targa);
  return true;
}


//  // helper function for when we find an int var
//  static bool unifyToIntVar(Type *a,
//                            Type *b,
//                            StringSObjDict<STemplateArgument> &bindings,
//                            MFlags mFlags)
//  {
//  }


bool MatchTypes::match_cva(CVAtomicType *a, Type *b, MFlags mFlags)
{
  //   A non-ref, B ref to const: B's ref goes away
  if (b->isReferenceToConst()) return match(a, b->getAtType(), mFlags);
  //   A non-ref, B ref to non-const: failure to unify
  else if (b->isReference()) return false;

  if (b->isTypeVariable()) return unifyToTypeVar(a, b, mFlags);
  if (a->isTypeVariable()) {
    TypeVariable *tva = a->asTypeVariable();
    STemplateArgument *targa = bindings.queryif(tva->name);
    // if 'a' is already bound, resolve to its binding and recurse
    if (targa) {
      // Please note that it is an important part of the design that
      // pairs are cached and checked for in the top-level match
      // function just so that we can do this here and not worry about
      // infinite loops.
      return targa->kind==STemplateArgument::STA_TYPE
        && match(targa->value.t, b, mFlags & ~MT_TOP);
    }
    return false;               // if unbound we fail
  }

  if (b->isCVAtomicType()) {
    bool isCpdTemplate = a->isCompoundType()
      && a->asCompoundType()->typedefVar->isTemplate();
    bool tIsCpdTemplate = b->isCompoundType()
      && b->asCompoundType()->typedefVar->isTemplate();
    if (isCpdTemplate && tIsCpdTemplate) {
      return
        match(a->asCompoundType()->typedefVar->templateInfo(),
              b->asCompoundType()->typedefVar->templateInfo(),
              mFlags & ~MT_TOP);
    } else if (!isCpdTemplate && !tIsCpdTemplate) {
      // I don't want to call BaseType::equals() at all since I'm
      // essentially duplicating my version of that; however I do need
      // to be able to ask if to AtomicType-s are equal.
      bool atomicMatches = a->asCVAtomicType()->atomic->equals(b->asCVAtomicType()->atomic);
      if (mFlags & MT_TOP) {
        return atomicMatches;
      } else {
        CVFlags const mask = CV_VOLATILE & CV_CONST; // ignore other kinds of flags
        return ((a->asCVAtomicType()->cv & mask) == (b->asCVAtomicType()->cv & mask))
          && atomicMatches;
      }
    }
    // if there is a mismatch, they definitely don't match
  }
  return false;
}


bool MatchTypes::match_ref(ReferenceType *a, Type *b, MFlags mFlags)
{
  // The policy on references and unification is as follows.
  //   A non-ref, B ref to const: B's ref goes away
  //   A non-ref, B ref to non-const: failure to unify
  //     do those in each function
  //   A ref, B non-ref: A's ref-ness silently goes away.
  //   A ref, B ref: the ref's match and unification continues below.
  //     do those here
  if (b->isReference()) b = b->getAtType();
  return match(a->getAtType(), b,
               // I don't know if this is right or not, but for now I
               // consider that if you unreference that you are still
               // considered to be a the top level
               mFlags);
}


bool MatchTypes::match_ptr(PointerType *a, Type *b, MFlags mFlags)
{
  //   A non-ref, B ref to const: B's ref goes away
  if (b->isReferenceToConst()) return match(a, b->getAtType(), mFlags);
  //   A non-ref, B ref to non-const: failure to unify
  else if (b->isReference()) return false;

  if (b->isTypeVariable()) return unifyToTypeVar(a, b, mFlags);

  if (b->isPointer()) return match(a->getAtType(), b->getAtType(), mFlags & ~MT_TOP);
  return false;
}


bool MatchTypes::match_func(FunctionType *a, Type *b, MFlags mFlags)
{
  //   A non-ref, B ref to const: B's ref goes away
  if (b->isReferenceToConst()) return match(a, b->getAtType(), mFlags);
  //   A non-ref, B ref to non-const: failure to unify
  else if (b->isReference()) return false;

  if (b->isTypeVariable()) return unifyToTypeVar(a, b, mFlags);

  if (b->isPointer()) {
    // cppstd 14.8.2.1 para 2: "If P is not a reference type: --
    // ... If A is a function type, the pointer type produced by the
    // function-to-pointer standard conversion (4.3) is used in place
    // of A for type deduction"; rather than wrap the function type in
    // a pointer, I'll just unwrap the pointer-ness of 'b' and keep
    // going down.
    return match(a, b->getAtType(), mFlags & ~MT_TOP);
  }

  if (b->isFunctionType()) {
    FunctionType *ft = b->asFunctionType();

    // check all the parameters
    if (a->params.count() != ft->params.count()) {
      return false;
    }
    SObjListIterNC<Variable> iter0(a->params);
    SObjListIterNC<Variable> iterA(ft->params);
    for(;
        !iter0.isDone();
        iter0.adv(), iterA.adv()) {
      Variable *var0 = iter0.data();
      Variable *var1 = iterA.data();
      xassert(!var0->hasFlag(DF_TYPEDEF)); // should not be possible
      xassert(!var1->hasFlag(DF_TYPEDEF)); // should not be possible
      if (!match
          (var0->type, var1->type,
           // FIX: I don't know if this is right: are we at the top
           // level again when we recurse down into the parameters of
           // a function type that is itself an argument?
           mFlags | MT_TOP )) {
        return false; // conjunction
      }
    }
    xassert(iterA.isDone());

    // check the return type
    return match
      (a->retType, ft->retType,
       // FIX: I don't know if this is right: are we at the top level
       // again when we recurse down into the rerturn value (just as
       // with the parameters) of a function type that is itself an
       // argument?
       mFlags | MT_TOP);
  }
  return false;
}


bool MatchTypes::match_array(ArrayType *a, Type *b, MFlags mFlags)
{
  //   A non-ref, B ref to const: B's ref goes away
  if (b->isReferenceToConst()) return match(a, b->getAtType(), mFlags);
  //   A non-ref, B ref to non-const: failure to unify
  else if (b->isReference()) return false;

  if (b->isTypeVariable()) return unifyToTypeVar(a, b, mFlags);

  if (b->isPointer() && (mFlags & MT_TOP)) {
    // cppstd 14.8.2.1 para 2: "If P is not a reference type: -- if A
    // is an array type, the pointer type produced by the
    // array-to-pointer standard conversion (4.2) is used in place of
    // A for type deduction"; however, this only seems to apply at the
    // top level; see cppstd 14.8.2.4 para 13.
    return match(a->eltType,
                 b->asPointerType()->atType,
                 mFlags & ~MT_TOP);
  }

  // FIX: if we are not at the top level then the array indicies
  // should be matched as well when we do Object STemplateArgument
  // matching
  if (b->isArrayType()) {
    return match(a->eltType, b->asArrayType()->eltType, mFlags & ~MT_TOP);
  }
  return false;
}


bool MatchTypes::match_ptm(PointerToMemberType *a, Type *b, MFlags mFlags)
{
  //   A non-ref, B ref to const: B's ref goes away
  if (b->isReferenceToConst()) return match(a, b->getAtType(), mFlags);
  //   A non-ref, B ref to non-const: failure to unify
  else if (b->isReference()) return false;

  if (b->isTypeVariable()) return unifyToTypeVar(a, b, mFlags);

  if (b->isPointerToMemberType()) {
    // FIX: should there be some subtyping polymorphism here?

    // I have to wrap the CompoundType in a CVAtomicType just so I can
    // do the unification;
    //
    // DO NOT MAKE THESE ON THE STACK as one might be a type variable
    // and the other then would get unified into permanent existence
    // on the heap
    CVAtomicType *a_inClassNATcvAtomic =
      tfac.makeCVAtomicType(SL_UNKNOWN, a->inClassNAT, CV_NONE);
    CVAtomicType *b_inClassNATcvAtomic =
      tfac.makeCVAtomicType(SL_UNKNOWN, b->asPointerToMemberType()->inClassNAT, CV_NONE);
    return
      match(a_inClassNATcvAtomic, b_inClassNATcvAtomic, mFlags & ~MT_TOP)
      && match(a->atType, b->getAtType(), mFlags & ~MT_TOP);
  }
  return false;
}


bool MatchTypes::match(Type *a, Type *b, MFlags mFlags)
{
  // prevent infinite loops;
  //
  // FIX: we use the pointer identity of the type here; it makes me
  // worry about two things:
  //
  // First, if someone ever switched to multiple inheritance, then
  // casting the static type up and down could actually change the
  // pointer and make this miss something; I think this is unlikely to
  // be a problem as I add and check right here in the same spot using
  // the same pointers;
  //
  // Second, the semantics of this will change if hash-consing is ever
  // implemented, and would be different in client code (Oink) where
  // hash-consing is turned off.
  if (typePairSet.lookup(a, b, true /*insertIfMissing*/)) return true;
  // roll our own dynamic dispatch
  switch (a->getTag()) {
    default: xfailure("bad tag");
    case Type::T_ATOMIC:          return match_cva(a->asCVAtomicType(),        b, mFlags);
    case Type::T_POINTER:         return match_ptr(a->asPointerType(),         b, mFlags);
    case Type::T_REFERENCE:       return match_ref(a->asReferenceType(),       b, mFlags);
    case Type::T_FUNCTION:        return match_func(a->asFunctionType(),       b, mFlags);
    case Type::T_ARRAY:           return match_array(a->asArrayType(),         b, mFlags);
    case Type::T_POINTERTOMEMBER: return match_ptm(a->asPointerToMemberType(), b, mFlags);
  }
}


// EOF
