// matchtype.cc
// code for matchtype.h

#include "matchtype.h"       // this module
#include "variable.h"        // Variable


MatchTypes::~MatchTypes()
{}


bool MatchTypes::atLeastAsSpecificAs_STemplateArgument_list
  (SObjList<STemplateArgument> &list1,
   ObjList<STemplateArgument> &list2, // NOTE: Assymetry in the list serf/ownerness
   Flags asaFlags)
{
  xassert(!(asaFlags & ASA_TOP));

  SObjListIterNC<STemplateArgument> iter1(list1);
  ObjListIterNC<STemplateArgument> iter2(list2);

  while (!iter1.isDone() && !iter2.isDone()) {
    STemplateArgument *sta1 = iter1.data();
    STemplateArgument *sta2 = iter2.data();
    if (!atLeastAsSpecificAs(sta1, sta2, asaFlags)) {
      return false;
    }

    iter1.adv();
    iter2.adv();
  }

  return iter1.isDone() && iter2.isDone();
}


// FIX: EXACT COPY OF THE CODE ABOVE.  Have to figure out how to fix
// that.  NOTE: the SYMMETRY in the list serf/ownerness.
bool MatchTypes::atLeastAsSpecificAs_STemplateArgument_list
  (ObjList<STemplateArgument> &list1,
   ObjList<STemplateArgument> &list2,
   Flags asaFlags)
{
  xassert(!(asaFlags & ASA_TOP));

  ObjListIterNC<STemplateArgument> iter1(list1);
  ObjListIterNC<STemplateArgument> iter2(list2);

  while (!iter1.isDone() && !iter2.isDone()) {
    STemplateArgument *sta1 = iter1.data();
    STemplateArgument *sta2 = iter2.data();
    if (!atLeastAsSpecificAs(sta1, sta2, asaFlags)) {
      return false;
    }

    iter1.adv();
    iter2.adv();
  }

  return iter1.isDone() && iter2.isDone();
}


bool MatchTypes::atLeastAsSpecificAs
  (TemplateInfo *concrete, TemplateInfo *tinfo,
   Flags asaFlags)
{
  xassert(!(asaFlags & ASA_TOP));

  // is tinfo from the same primary even?
  if (!concrete->myPrimary ||
      (concrete->myPrimary != tinfo->myPrimary)) return false;
  // are we at least as specific as tinfo?
  return atLeastAsSpecificAs_STemplateArgument_list
    (concrete->arguments, tinfo->arguments, asaFlags);
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


bool MatchTypes::atLeastAsSpecificAs
  (STemplateArgument *concrete,
   STemplateArgument const *obj,
   Flags asaFlags)
{
  xassert(!(asaFlags & ASA_TOP));

  switch (concrete->kind) {
  case STemplateArgument::STA_TYPE:                // type argument
    if (STemplateArgument::STA_TYPE != obj->kind) return false;
    return atLeastAsSpecificAs
      (concrete->value.t,
       obj->value.t,
       // FIX: I have no idea why this cast is necessary here but not
       // for other flag enums
       //
       // sm: b/c you need to add ENUM_BITWISE_OPS at decl..
       // which I have now done
       /*(Flags)*/ (asaFlags | ASA_TOP) );
    break;

  case STemplateArgument::STA_INT:                 // int or enum argument
    if (STemplateArgument::STA_INT == obj->kind) {
      return concrete->value.i == obj->value.i;
    } else if (STemplateArgument::STA_REFERENCE == obj->kind) {
      return unifyIntToVar(concrete->value.i, obj->value.v);
    } else {
      return false;
    }
    break;

  case STemplateArgument::STA_REFERENCE:           // reference to global object
    if (STemplateArgument::STA_INT == obj->kind) {
      // you can't unify a reference with an int
      return false;
    }
    // FIX:
    xfailure("reference arguments unimplemented");
    break;

  case STemplateArgument::STA_POINTER:             // pointer to global object
    // FIX:
    xfailure("pointer arguments unimplemented");

  case STemplateArgument::STA_MEMBER:              // pointer to class member
    // FIX:
    xfailure("pointer to member arguments unimplemented");
    break;

  case STemplateArgument::STA_TEMPLATE:            // template argument (not implemented)
    // FIX:
    xfailure("STemplateArgument::STA_TEMPLATE non implemented");
    break;

  default:
    xfailure("illegal STemplateArgument::kind");
    break;
  }
}


// ---------------------- atLeastAsSpecificAs ---------------------

void bindingsGdb(StringSObjDict<STemplateArgument> &bindings)
{
  cout << "Bindings" << endl;
  for (StringSObjDict<STemplateArgument>::Iter bindIter(bindings);
       !bindIter.isDone();
       bindIter.next()) {
    string const &name = bindIter.key();
    STemplateArgument *value = bindIter.value();
    cout << "'" << name << "' ";
    value->gdb();
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
bool MatchTypes::unifyToTypeVar(Type *t0,
                               Type *t1,
                               Flags asaFlags)
{
  TypeVariable *tv = t1->asTypeVariable();
  STemplateArgument *targ1 = bindings.queryif(tv->name);
  // is this type variable already bound?
  if (targ1) {
    // check that the current value matches the bound value
    // FIX: the EF_IGNORE_PARAM_CV is just a guess
    Type::EqFlags eqFlags0 = Type::EF_IGNORE_PARAM_CV;
    if (asaFlags & ASA_TOP) eqFlags0 |= Type::EF_IGNORE_TOP_CV;
    return targ1->kind==STemplateArgument::STA_TYPE
      && t0->equals(targ1->value.t, eqFlags0);
  }
  // otherwise, bind it
  STemplateArgument *targ0 = new STemplateArgument;

  // deal with CV qualifier strangness;
  // dsw: Const should be removed from the language because of things
  // like this!  I mean, look at it!
  CVFlags t0cv = t0->getCVFlags();
  // partition qualifiers into normal ones and Scott's funky qualifier
  // extensions that aren't const or volatile
  CVFlags t0cvNormal = t0cv &  (CV_CONST | CV_NONE);
  CVFlags t0cvScott  = t0cv & ~(CV_CONST | CV_NONE);
  if (asaFlags & ASA_TOP) {
    // if we are at the top level, remove all CV qualifers before
    // binding; ignore the qualifiers on the type variable as they are
    // irrelevant
    t0 = tfac.setCVQualifiers(SL_UNKNOWN, CV_NONE, t0, NULL /*syntax*/);
  } else {
    // if we are below the top level, subtract off the CV qualifiers
    // that match; if we get a negative qualifier set (arg lacks a
    // qualifer that the param has) we fail to match
    CVFlags finalFlags = CV_NONE;

    CVFlags t1cv = t1->getCVFlags();
    // partition qualifiers again
    CVFlags t1cvNormal = t1cv &  (CV_CONST | CV_NONE);
//      CVFlags t1cvScott  = t1cv & ~(CV_CONST | CV_NONE);
    // Lets kill a mosquito with a hydraulic wedge
    for(CVFlagsIter cvIter; !cvIter.isDone(); cvIter.adv()) {
      CVFlags curFlag = cvIter.data();
      int t0flagInt = (t0cvNormal & curFlag) ? 1 : 0;
      int t1flagInt = (t1cvNormal & curFlag) ? 1 : 0;
      int flagDifference = t0flagInt - t1flagInt;
      if (flagDifference < 0) {
        return false;           // can't subtract a flag that isn't there
      }
      if (flagDifference > 0) {
        finalFlags |= curFlag;
      }
      // otherwise, the flags match
    }

    // if there's been a change, must (shallow) clone t0 and apply new
    // CV qualifiers
    t0 = tfac.setCVQualifiers
      (SL_UNKNOWN,
       finalFlags | t0cvScott,  // restore Scott's funkyness
       t0,
       NULL /*syntax*/
       );
  }

  targ0->setType(t0);
  bindings.add(tv->name, targ0);
  return true;
}


//  // helper function for when we find an int var
//  static bool unifyToIntVar(Type *t0,
//                            Type *t1,
//                            StringSObjDict<STemplateArgument> &bindings,
//                            Flags asaFlags)
//  {
//  }


bool MatchTypes::match_cva(CVAtomicType *concrete, Type *t,
                          Flags asaFlags)
{
  if (t->isReference() && t->asPointerType()->atType->isConst()) {
    t = t->asPointerType()->atType;
  }
  // I'm tempted to do this, but when I test it, it ends up testing
  // false in the end anyway.  It is hard to construct a test for it,
  // so I don't have one checked in.
//    if (isTypeVariable() && !t->isTypeVariable()) {
//      return false;
//    }
  if (t->isTypeVariable()) {
    return unifyToTypeVar(concrete, t, asaFlags);
  }
  if (t->isCVAtomicType()) {
    bool isCpdTemplate = concrete->isCompoundType() && concrete->asCompoundType()->typedefVar->isTemplate();
    bool tIsCpdTemplate = t->isCompoundType() && t->asCompoundType()->typedefVar->isTemplate();
    if (isCpdTemplate && tIsCpdTemplate) {
      return
        atLeastAsSpecificAs(concrete->asCompoundType()->typedefVar->templateInfo(),
                            t->asCompoundType()->typedefVar->templateInfo(),
                            (asaFlags & ~ASA_TOP));
    } else if (!isCpdTemplate && !tIsCpdTemplate) {
      return concrete->equals(t);
    }
    // if there is a mismatch, they definitely don't match
  }
  return false;
}

bool MatchTypes::match_ptr(PointerType *concrete, Type *t,
                          Flags asaFlags)
{
  // The policy on references and unification is as follows.
  //   A non-ref, B ref to const: B's ref goes away
  //   A non-ref, B ref to non-const: failure to unify
  //   A ref, B non-ref: A's ref-ness silently goes away.
  //   A ref, B ref: the ref's match and unification continues below.

  // unify the reference-ness
  Type *thisType = concrete;
  if (concrete->isReference() && !t->isReference()) {
    thisType = concrete->asRval();
  }
  else if (!concrete->isReference() &&
           t->isReference() && t->asPointerType()->atType->isConst()) {
    t = t->asPointerType()->atType;
  }

  xassert(thisType->isReference() == t->isReference());

  if (t->isTypeVariable()) {
    return unifyToTypeVar(thisType, t, asaFlags);
  }
  if (t->isPointerType() && thisType->isPointerType()
      && thisType->asPointerType()->op==t->asPointerType()->op) {
    return atLeastAsSpecificAs
      (thisType->asPointerType()->atType, 
       t->asPointerType()->atType, (asaFlags & ~ASA_TOP) );
  }
  if (concrete == thisType) {
    return false;               // there is no progress to make
  }
  return
    atLeastAsSpecificAs(thisType, t,
                        // I don't know if this is right or not, but
                        // for now I consider that if you unreference
                        // that you are still considered to be a the
                        // top level
                        asaFlags);
}

bool MatchTypes::match_func(FunctionType *concrete, Type *t,
                           Flags asaFlags)
{
  if (t->isReference() && t->asPointerType()->atType->isConst()) {
    t = t->asPointerType()->atType;
  }

  if (t->isPointer()
      // see my rant below about isPointer() and isPointerType()
      && t->asPointerType()->op == PO_POINTER) {
    // cppstd 14.8.2.1 para 2: "If P is not a reference type: --
    // ... If A is a function type, the pointer type produced by the
    // function-to-pointer standard conversion (4.3) is used in place
    // of A for type deduction"; rather than wrap the function type in
    // a pointer, I'll just unwrap the pointer-ness of 't' and keep
    // going down.
    return atLeastAsSpecificAs(concrete, 
                               t->asPointerType()->atType,
                               (asaFlags & ~ASA_TOP));
  }
  if (t->isTypeVariable()) {
    return unifyToTypeVar(concrete, t, asaFlags);
  }
  if (t->isFunctionType()) {
    FunctionType *ft = t->asFunctionType();

    // check all the parameters
    if (concrete->params.count() != ft->params.count()) {
      return false;
    }
    SObjListIterNC<Variable> iter0(concrete->params);
    SObjListIterNC<Variable> iter1(ft->params);
    for(;
        !iter0.isDone();
        iter0.adv(), iter1.adv()) {
      Variable *var0 = iter0.data();
      Variable *var1 = iter1.data();
      xassert(!var0->hasFlag(DF_TYPEDEF)); // should not be possible
      xassert(!var1->hasFlag(DF_TYPEDEF)); // should not be possible
      if (!atLeastAsSpecificAs
          (var0->type, var1->type,
           // FIX: I don't know if this is right: are we at the top
           // level again when we recurse down into the parameters of
           // a function type that is itself an argument?
           (asaFlags | ASA_TOP) )) {
        return false; // conjunction
      }
    }
    xassert(iter1.isDone());

    // check the return type
    return atLeastAsSpecificAs
      (concrete->retType, ft->retType,
       // FIX: I don't know if this is right: are we at the top level
       // again when we recurse down into the rerturn value (just as
       // with the parameters) of a function type that is itself an
       // argument?
       (asaFlags | ASA_TOP) );
  }
  return false;
}

bool MatchTypes::match_array
  (ArrayType *concrete, Type *t,
   Flags asaFlags)
{
  if (t->isReference() && t->asPointerType()->atType->isConst()) {
    t = t->asPointerType()->atType;
  }
  if (t->isTypeVariable()) {
    return unifyToTypeVar(concrete, t, asaFlags);
  }
  if (t->isPointer()
      // this second test is probably redundant but I really find it
      // confusing that you can test if something is a pointer or
      // reference with isPointerType() and if it is a pointer and in
      // particular not a reference with isPointer(); therefore the
      // solution is to be way extra verbose and redundant
      && t->asPointerType()->op == PO_POINTER
      && (asaFlags & ASA_TOP)
      ) {
    // cppstd 14.8.2.1 para 2: "If P is not a reference type: -- if A
    // is an array type, the pointer type produced by the
    // array-to-pointer standard conversion (4.2) is used in place of
    // A for type deduction"; however, this only seems to apply at the
    // top level; see cppstd 14.8.2.4 para 13.
    return atLeastAsSpecificAs(concrete->eltType,
                               t->asPointerType()->atType,
                               (asaFlags & ~ASA_TOP) );
  }
  if (t->isArrayType()) {
    ArrayType *tArray = t->asArrayType();
    bool baseUnifies =
      atLeastAsSpecificAs(concrete->eltType,
                          tArray->eltType,
                          (asaFlags & ~ASA_TOP) );
    return baseUnifies;
  }
  return false;
}

bool MatchTypes::match_ptm(PointerToMemberType *concrete, Type *t,
                          Flags asaFlags)
{
  if (t->isReference() && t->asPointerType()->atType->isConst()) {
    t = t->asPointerType()->atType;
  }
  if (t->isTypeVariable()) {
    return unifyToTypeVar(concrete, t, asaFlags);
  }
  if (t->isPointerToMemberType()) {
    // FIX: should there be some subtyping polymorphism here?

    // I have to wrap the CompoundType in a CVAtomicType just so I can
    // do the unification
    CVAtomicType *inClassNATcvAtomic =
      tfac.makeCVAtomicType(SL_UNKNOWN, concrete->inClassNAT, CV_NONE);
    CVAtomicType *t_inClassNATcvAtomic =
      tfac.makeCVAtomicType(SL_UNKNOWN, t->asPointerToMemberType()->inClassNAT, CV_NONE);
    bool inClassUnifies;
    if (t_inClassNATcvAtomic->isTypeVariable()) {
      inClassUnifies =
        unifyToTypeVar(inClassNATcvAtomic, t_inClassNATcvAtomic, asaFlags);
    } else {
      inClassUnifies =
        atLeastAsSpecificAs(inClassNATcvAtomic,
                            t_inClassNATcvAtomic,
                            (asaFlags & ~ASA_TOP));
    }
    return inClassUnifies &&
      atLeastAsSpecificAs(concrete->atType,
                          t->asPointerToMemberType()->atType,
                          (asaFlags & ~ASA_TOP));
  }
  return false;
}


bool MatchTypes::atLeastAsSpecificAs(Type *concrete, Type *pattern,
                                     Flags asaFlags)
{
  switch (concrete->getTag()) {
    default: xfailure("bad tag");

    case Type::T_ATOMIC:
      return match_cva(concrete->asCVAtomicType(),
        pattern, asaFlags);

    case Type::T_POINTER:
      return match_ptr(concrete->asPointerType(),
        pattern, asaFlags);

    case Type::T_FUNCTION:
      return match_func(concrete->asFunctionType(),
        pattern, asaFlags);

    case Type::T_ARRAY:
      return match_array(concrete->asArrayType(),
        pattern, asaFlags);

    case Type::T_POINTERTOMEMBER:
      return match_ptm(concrete->asPointerToMemberType(),
        pattern, asaFlags);
  }
}


// EOF
