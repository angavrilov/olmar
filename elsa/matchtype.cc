// matchtype.cc
// code for matchtype.h


#include "matchtype.h"       // this module
#include "variable.h"        // Variable
#include "trace.h"           // tracingSys
#include "template.h"        // STemplateArgument, etc.


// FIX: I'll bet the matchDepth here isn't right; not sure what should
// go there
//   if (b->isReferenceToConst()) return match0(a, b->getAtType(), matchDepth);

int MatchTypes::recursionDepthLimit = 500; // what g++ 3.4.0 uses by default

// sm: TODO: Actually, the g++ limit reported by running in/d0062.cc
// is not a match algorithm depth, but rather the template
// instantiation depth.  I believe that matching (and unification) can
// be implemented such that a match depth limit is not required and
// termination is guaranteed.


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

// sm: why weren't these implemented?
void throw_XMatchDepth()
{
  XMatchDepth x;
  THROW(x);
}

XMatchDepth::XMatchDepth()
  : xBase("match depth exceeded")
{}

XMatchDepth::XMatchDepth(XMatchDepth const &obj)
  : xBase(obj)
{}

XMatchDepth::~XMatchDepth()
{}


// ---------------------- MatchBindings ---------------------
MatchBindings::MatchBindings() 
  : entryCount(0) 
{}

MatchBindings::~MatchBindings()
{
  // delete all the STemplateArguments from the map
  STemplateArgumentMap::Iter iter(map);
  for (; !iter.isDone(); iter.adv()) {
    delete iter.value();
  }
}


void MatchBindings::put0(Variable *key, STemplateArgument *val) {
  xassert(key);
  xassert(val);
  if (map.get(key->name)) xfailure("attempted to re-bind var");
  ++entryCount;                 // note: you can't rebind a var
  map.add(key->name, val);
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

STemplateArgument const *MatchBindings::get0(Variable const *key) {
  xassert(key);
  return getVar(key->name);
}

STemplateArgument const *MatchBindings::getVar(StringRef name) {
  xassert(name);
  return map.get(name);
}

STemplateArgument const *MatchBindings::getObjVar(Variable const *key) {
  xassert(key);
  xassert(!key->type->isTypeVariable());
  return get0(key);
}

STemplateArgument const *MatchBindings::getTypeVar(TypeVariable *key) {
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

void bindingsGdb(STemplateArgumentMap &bindings)
{
  cout << "Bindings" << endl;
  for (STemplateArgumentMap::Iter bindIter(bindings);
       !bindIter.isDone();
       bindIter.adv()) {
    StringRef key = bindIter.key();
    STemplateArgument *value = bindIter.value();
    cout << "'" << key << "' ";
//      cout << "Variable* key: " << stringf("%p ", key);
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
  TRACE("matchtype", "bound " << b->asTypeVariable()->name <<
                     " to " << a->toString());

  return true;
}


bool MatchTypes::match_rightTypeVar(Type *a, Type *b, int matchDepth)
{
  xassert(b->isTypeVariable());
  switch(mode) {
  default: xfailure("illegal MatchTypes mode"); break;

  case MM_BIND_UNIQUE:
  case MM_BIND: {
    if (mode == MM_BIND && a->isTypeVariable()) {
      xfailure("MatchTypes: got a type variable on the left");
    }
    STemplateArgument const *targb = bindings.getTypeVar(b->asTypeVariable());
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
    STemplateArgument const *targb = bindings.getTypeVar(b->asTypeVariable());
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


bool MatchTypes::match_variables(Type *a, TypeVariable *b, int matchDepth)
{
  // sm: Clearly, there is a relationship between this function and
  // bindValToVar and match_rightTypeVar; these three should share more.
  // But I don't understand the code well enough at the moment to
  // properly collapse them.

  xassert(a->isTypeVariable());
  TypeVariable *aTV = a->asTypeVariable();

  switch (mode) {
    default: xfailure("bad mode");
    case MM_BIND: xfailure("type var on left in MM_BIND");

    case MM_WILD:
      return true;

    case MM_BIND_UNIQUE:
    case MM_ISO: {
      // copied+modified from match_rightTypeVar
      STemplateArgument const *targb = bindings.getTypeVar(b);
      if (targb) {
        if (targb->kind!=STemplateArgument::STA_TYPE) return false;
        xassert(targb->value.t->isTypeVariable());

        // since this is the MM_ISO case, they must be semantically
        // identical
        return
          // must be the same typevar; NOTE: don't compare the types, as
          // they can change when cv qualifiers are added etc. but the
          // variables have to be the same
          ((aTV->typedefVar ==
            targb->value.t->asTypeVariable()->typedefVar));
      }
      else {
        // copied+modified from bindValToVar
        STemplateArgument *targa = new STemplateArgument;
        targa->setType(a);
        bindings.putTypeVar(b, targa);
        TRACE("matchtype", "bound " << b->asTypeVariable()->name << 
                           " to " << a->toString());
        return true;
      }
    }
  }
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

    case MM_BIND_UNIQUE:
    case MM_ISO:
      return false;
      break;

    } // end switch
  }

  if (b->isCVAtomicType()) {
    AtomicType *aAT = a->atomic;
    AtomicType *bAT = b->asCVAtomicType()->atomic;

    // deal with top-level qualifiers; if we are not at MT_TOP, then
    // they had better match exactly
    if (matchDepth > 0) {
      CVFlags const mask = normalCvFlagMask; // ignore other kinds of funky Scott-flags
      if ( (a->getCVFlags() & mask) != (b->getCVFlags() & mask) ) return false;
    }

    // sm: below here all supplied matchDepths were 0, so I've pulled
    // that decision out to here; a nearby comment said:
    //   used to be not top but I don't remember why
    matchDepth = 0;

    // sm: 8/10/04: changed 'isTemplate()' calls to
    // 'isInstantiation()'; we should never be matching against raw
    // templates, only instantiations and PseudoInstantiations
    // (t0220.cc was failing)
    bool aIsCpdTemplate = a->isCompoundType()
      && a->asCompoundType()->typedefVar->isInstantiation();
    bool bIsCpdTemplate = b->isCompoundType()
      && b->asCompoundType()->typedefVar->isInstantiation();

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
      return match_TInfo(aTI, bTI, matchDepth);
    }

    else if (!aIsCpdTemplate && !bIsCpdTemplate) {
      // sm: Previously, this code called into AtomicType::equals, but
      // since that didn't do quite the right thing, AtomicType::equals
      // was hacked to death.  Instead, I'm pulling out the
      // functionality (one line!) that was being used, and putting the
      // needed adjustments here.

      if (aAT == bAT) {       // this is the line that was being used from AtomicType::equals
        return true;
      }

      // adjustment 1: type variable direct matching
      if (aAT->isTypeVariable() && bAT->isTypeVariable()) {
        return match_variables(a, b->asTypeVariable(), matchDepth);
      }

      // adjustment 2: pseudo-instantiation argument matching
      if (aAT->isPseudoInstantiation() && bAT->isPseudoInstantiation()) {
        return match_PI(aAT->asPseudoInstantiation(), bAT->asPseudoInstantiation(),
                        matchDepth);
      }

      // not equal
      return false;
    }

    else if (aIsCpdTemplate && !bIsCpdTemplate) {
      // sm: extend matching to allow 'a' to be a concrete instantiation
      // and 'b' to be a PseudoInstantiation
      if (b->isPseudoInstantiation()) {
        TemplateInfo *aTI = a->asCompoundType()->typedefVar->templateInfo();
        PseudoInstantiation *bPI = b->asCVAtomicType()->atomic->asPseudoInstantiation();
        return match_TInfo_with_PI(aTI, bPI, matchDepth);
      }
    }
  }

  // if there is a mismatch, they definitely don't match
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
  //     sm: not anymore, see below
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
      //
      // sm: 8/07/04: Indeed it is ad-hoc, and wrong, as demonstrated
      // by t0114.cc.  ref-to-const-Foo is quite different from
      // ref-to-Foo.  I will find a better solution for whatever is
      // happening in nsAtomTable.i.
      //
      // sm: 8/10/04: Ok, d0072.cc seems to be the testcase for when
      // this behavior is desired, and it corresponds to function
      // template argument deduction.  The actual rules are spelled
      // out in 14.8.2.1, but for now I'll just put this back.
      if (eflags & Type::EF_DEDUCTION) {
        matchDepth0 = 0;
      }
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

    // set up iterators to examine the parameters
    SObjListIterNC<Variable> iterA(a->params);
    SObjListIterNC<Variable> iterB(ftb->params);

    // skip receiver parameters?
    if (eflags & Type::EF_IGNORE_IMPLICIT) {
      if (a->isMethod()) {
        iterA.adv();
      }
      if (ftb->isMethod()) {
        iterB.adv();
      }
    }

    // now check the lists
    for(;
        !iterA.isDone() && !iterB.isDone();
        iterA.adv(), iterB.adv()) {
      Variable *varA = iterA.data();
      Variable *varB = iterB.data();
      if (!match0
          (varA->type, varB->type,
           // FIX: I don't know if this is right: are we at the top
           // level again when we recurse down into the parameters of
           // a function type that is itself an argument?
           0 /*matchDepth*/)) {
        return false; // conjunction
      }
    }

    if (!iterA.isDone() || !iterB.isDone()) {
      return false;   // differing number of params
    }

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


bool MatchTypes::match_Lists
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
  TemplateInfo *ati = a->getPrimary();
  xassert(ati);
  TemplateInfo *bti = b->getPrimary();
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
  if (b->isPrimary()) return true;
  return match_Lists(a->arguments, b->arguments, matchDepth);
}


bool MatchTypes::match_TInfo_with_PI(TemplateInfo *a, PseudoInstantiation *b,
                                     int matchDepth)
{
  // preamble similar to match_TInfo, to check that these are
  // (pseudo) instantiations of the same primary
  TemplateInfo *ati = a->getPrimary();
  xassert(ati);
  TemplateInfo *bti = b->primary->templateInfo()->getPrimary();
  xassert(bti);
  if (ati != bti) return false;

  // compare arguments; use the args to the primary, not the args to
  // the partial spec (if any)
  return match_Lists(a->getArgumentsToPrimary(), b->args, matchDepth);
}


bool MatchTypes::match_PI(PseudoInstantiation *a, PseudoInstantiation *b,
                          int matchDepth)
{ 
  // same primary?
  if (a->primary != b->primary) {
    return false;
  }

  // compare arguments
  return match_Lists(a->args, b->args, matchDepth);
}


// helper function for when we find an int
bool MatchTypes::unifyIntToVar(int i0, Variable *v1)
{
  STemplateArgument const *v1old = bindings.getObjVar(v1);
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

MatchTypes::MatchTypes(TypeFactory &tfac0, MatchMode mode0, Type::EqFlags eflags0)
  : tfac(tfac0), mode(mode0), eflags(eflags0), recursionDepth(0)
{
  xassert(mode!=MM_NONE);
  if (tracingSys("shortMTRecDepthLimit")) {
    recursionDepthLimit = 10;   // make test for this feature go a lot faster
  }
}

MatchTypes::~MatchTypes()
{}


bool MatchTypes::match_Type(Type *a, Type *b, int matchDepth)
{
  bool ret = match0(a, b, matchDepth);
                      
  if (matchDepth == 0) {
    TRACE("matchtype", "match_Type[" << toString(mode) << "]("
                    << a->toString() << ", " << b->toString()
                    << ") yields " << ret);
  }
  
  return ret;
}


bool MatchTypes::match_Lists
  (SObjList<STemplateArgument> &listA,
   ObjList<STemplateArgument> &listB,
   int matchDepth)
{
  // using the cast is a bit of a hack; the "right" solution is to
  // use a template with a template parameter, but that's a bit over
  // the top (and may jeopardize portability)
  return match_Lists(reinterpret_cast<ObjList<STemplateArgument>&>(listA),
                     listB, 
                     matchDepth);
}

bool MatchTypes::match_Lists
  (ObjList<STemplateArgument> &listA,
   SObjList<STemplateArgument> &listB,
   int matchDepth)
{
  // using the cast is a bit of a hack; the "right" solution is to
  // use a template with a template parameter, but that's a bit over
  // the top (and may jeopardize portability)
  return match_Lists(listA,
                     reinterpret_cast<ObjList<STemplateArgument>&>(listB),
                     matchDepth);
}


char const *toString(MatchTypes::MatchMode m)
{
  static char const * const map[] = {
    "MM_NONE",
    "MM_BIND",
    "MM_BIND_UNIQUE",
    "MM_WILD",
    "MM_ISO"
  };
  ASSERT_TABLESIZE(map, MatchTypes::NUM_MATCH_MODES);

  xassert(0 <= m && m <= MatchTypes::NUM_MATCH_MODES);
  return map[m];
}


// EOF
