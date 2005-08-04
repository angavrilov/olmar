// mtype.cc
// code for mtype.h

// 2005-08-03: This code is now exercised to 100% statement coverage,
// except for the code that deals with MF_POLYMORPHIC and
// MF_ISOMORPHIC and the places annotated with gcov-ignore or
// gcov-begin/end-ignore, by in/t0511.cc.
//
// My plan is to re-do the coverage analysis using the entire test
// suite once mtype is hooked up to the rest of the program as the
// module to use for type comparison and matching.

#include "mtype.h"       // this module
#include "trace.h"       // tracingSys


string toString(MatchFlags flags)
{
  static char const * const map[] = {
    "MF_IGNORE_RETURN",
    "MF_STAT_EQ_NONSTAT",
    "MF_IGNORE_IMPLICIT",
    "MF_IGNORE_PARAM_CV",
    "MF_IGNORE_TOP_CV",
    "MF_IGNORE_EXN_SPEC",
    "MF_SIMILAR",
    "MF_POLYMORPHIC",
    "MF_DEDUCTION",
    "MF_UNASSOC_TPARAMS",
    "MF_IGNORE_ELT_CV",
    "MF_MATCH",
    "MF_NO_NEW_BINDINGS",
    "MF_ISOMORPHIC",
  };
  STATIC_ASSERT(TABLESIZE(map) == MF_NUM_FLAGS);
  return bitmapString(flags, map, MF_NUM_FLAGS);
}


//-------------------------- Binding -------------------------
string toString_or_CV_NONE(CVFlags cv)
{
  if (cv == CV_NONE) {
    return "CV_NONE";
  }
  else {
    return toString(cv);
  }
}


string MType::Binding::asString() const
{
  if (sarg.isType()) {
    return stringc << getType()->toString() << " with " 
                   << toString_or_CV_NONE(cv);
  }
  else {
    return sarg.toString();
  }
}


// -------------------------- MType --------------------------
MType::MType(bool nonConst_)
  : bindings(),
    nonConst(nonConst_)
{}

MType::~MType()
{}


// gcov-begin-ignore: at the moment, only the non-const interface is used
bool MType::match(Type const *conc, Type const *pat, MatchFlags flags)
{
  // I can only uphold the promise of not modifying 'conc' and 'pat'
  // if asked to when I was created.
  xassert(!nonConst);

  return imatch(conc, pat, flags);
}
// gcov-end-ignore


bool MType::matchNC(Type *conc, Type *pat, MatchFlags flags)
{
  return imatch(conc, pat, flags);
}


bool MType::imatch(Type const *conc, Type const *pat, MatchFlags flags)
{
  // 14.8.2.1p2b3
  if ((flags & MF_DEDUCTION) &&
      !pat->isReferenceType()) {
    flags |= MF_IGNORE_TOP_CV;
  }

  bool result = matchType(conc, pat, flags);

  #ifndef NDEBUG
    static bool doTrace = tracingSys("mtype");
    if (doTrace) {
      ostream &os = trace("mtype");
      os << "conc=`" << conc->toString()
         << "' pat=`" << pat->toString()
         << "' flags={" << toString(flags)
         << "}; match=" << (result? "true" : "false")
         ;

      if (result) {
        // extract bindings
        os << "; bindings:";
        for (BindingMap::Iter iter(bindings); !iter.isDone(); iter.adv()) {
          os << " \"" << iter.key() << "\"=`" << iter.value()->asString() << "'";
        }
      }

      os << endl;
    }
  #endif // NDEBUG

  return result;
}


int MType::getNumBindings() const
{
  return bindings.getNumEntries();
}


STemplateArgument MType::getBoundValue(StringRef name, TypeFactory &tfac)
{
  // you can't do this with the matcher that promises to
  // only work with const pointers; this assertion provides
  // the justification for the const_casts below
  xassert(nonConst);

  Binding *b = bindings.get(name);
  if (!b) {
    return STemplateArgument();
  }

  if (b->sarg.isAtomicType()) {
    // the STA_ATOMIC kind is only for internal use by this module;
    // we translate it into STA_TYPE for external use (but the caller
    // has to provide a 'tfac' to allow the translation)
    Type *t = tfac.makeCVAtomicType(
      const_cast<AtomicType*>(b->sarg.getAtomicType()), CV_NONE);
    return STemplateArgument(t);
  }

  if (b->sarg.isType()) {
    // similarly, we have to combine the type in 'sarg' with 'cv'
    // before exposing it to the user
    Type *t = tfac.setQualifiers(SL_UNKNOWN, b->cv,
                                 const_cast<Type*>(b->getType()), NULL /*syntax*/);
    return STemplateArgument(t);
  }

  // other cases are already in the public format
  return b->sarg;
}


void MType::setBoundValue(StringRef name, STemplateArgument const &value)
{
  xassert(value.hasValue());
  
  Binding *b = new Binding;
  b->sarg = value;
  if (value.isType()) {
    b->cv = value.getType()->getCVFlags();
  }

  bindings.addReplace(name, b);
}


// ------------------ AtomicType and subclasses ---------------
STATICDEF bool MType::canUseAsVariable(Variable *var, MatchFlags flags)
{
  xassert(var);

  if (!( flags & MF_MATCH )) {
    // we're not using anything as a variable
    return false;
  }

  if ((flags & MF_UNASSOC_TPARAMS) &&
      var->getParameterizedEntity() != NULL) {
    // we cannot use 'var' as a variable because, even though it is a
    // template parameter, it is associated with a specific template
    // and MF_UNASSOC_TPARAMS means we can only use *unassociated*
    // variables as such
    return false;
  }

  // no problem
  return true;
}


bool MType::matchAtomicType(AtomicType const *conc, AtomicType const *pat, MatchFlags flags)
{
  if (conc == pat) {
    return true;
  }

  if (pat->isTypeVariable() &&
      canUseAsVariable(pat->asTypeVariableC()->typedefVar, flags)) {
    return matchAtomicTypeWithVariable(conc, pat->asTypeVariableC(), flags);
  }

  if (conc->getTag() != pat->getTag()) {
    // match an instantiation with a PseudoInstantiation
    if ((flags & MF_MATCH) &&
        pat->isPseudoInstantiation() &&
        conc->isCompoundType() &&
        conc->asCompoundTypeC()->typedefVar->templateInfo() &&
        conc->asCompoundTypeC()->typedefVar->templateInfo()->isCompleteSpecOrInstantiation()) {
      TemplateInfo *concTI = conc->asCompoundTypeC()->typedefVar->templateInfo();
      PseudoInstantiation const *patPI = pat->asPseudoInstantiationC();
      
      // these must be instantiations of the same primary
      if (concTI->getPrimary() != patPI->primary->templateInfo()->getPrimary()) {
        return false;
      }
      
      // compare arguments; use the args to the primary, not the args to
      // the partial spec (if any)
      return matchSTemplateArguments(concTI->getArgumentsToPrimary(),
                                     patPI->args, flags);
    }

    return false;
  }

  // for the template-related types, we have some structural equality
  switch (conc->getTag()) {
    default: xfailure("bad tag");

    case AtomicType::T_SIMPLE:
    case AtomicType::T_COMPOUND:
    case AtomicType::T_ENUM:
      // non-template-related type, physical equality only
      return false;

    #define CASE(tag,type) \
      case AtomicType::tag: return match##type(conc->as##type##C(), pat->as##type##C(), flags) /*user;*/
    CASE(T_TYPEVAR, TypeVariable);
    CASE(T_PSEUDOINSTANTIATION, PseudoInstantiation);
    CASE(T_DEPENDENTQTYPE, DependentQType);
    #undef CASE
  }
}


bool MType::matchTypeVariable(TypeVariable const *conc, TypeVariable const *pat, MatchFlags flags)
{
  // 2005-03-03: Let's try saying that TypeVariables are equal if they
  // are the same ordinal parameter of the same template.
  Variable *cparam = conc->typedefVar;
  Variable *pparam = pat->typedefVar;

  return cparam->sameTemplateParameter(pparam);
}


bool MType::matchPseudoInstantiation(PseudoInstantiation const *conc, 
                                     PseudoInstantiation const *pat, MatchFlags flags)
{
  if (conc->primary != pat->primary) {
    return false;
  }

  return matchSTemplateArguments(conc->args, pat->args, flags);
}


bool MType::matchSTemplateArguments(ObjList<STemplateArgument> const &conc,
                                    ObjList<STemplateArgument> const &pat,
                                    MatchFlags flags)
{
  ObjListIter<STemplateArgument> concIter(conc);
  ObjListIter<STemplateArgument> patIter(pat);

  while (!concIter.isDone() && !patIter.isDone()) {
    STemplateArgument const *csta = concIter.data();
    STemplateArgument const *psta = patIter.data();
    if (!matchSTemplateArgument(csta, psta, flags)) {
      return false;
    }

    concIter.adv();
    patIter.adv();
  }

  return concIter.isDone() && patIter.isDone();
}


bool MType::matchSTemplateArgument(STemplateArgument const *conc, 
                                   STemplateArgument const *pat, MatchFlags flags)
{
  if (pat->kind == STemplateArgument::STA_DEPEXPR &&
      pat->getDepExpr()->isE_variable() &&
      canUseAsVariable(pat->getDepExpr()->asE_variable()->var, flags)) {
    return matchNontypeWithVariable(conc, pat->getDepExpr()->asE_variable(), flags);
  }

  if (conc->kind != pat->kind) {
    return false;
  }

  switch (conc->kind) {
    default: 
      xfailure("bad or unimplemented STemplateArgument kind");

    case STemplateArgument::STA_TYPE: {
      // For convenience I have passed 'STemplateArgument' directly,
      // but this module's usage of that structure is supposed to be
      // consistent with it storing 'Type const *' instead of 'Type
      // *', so this extraction is elaborated to make it clear we are
      // pulling out const pointers.
      Type const *ct = conc->getType();
      Type const *pt = pat->getType();
      return matchType(ct, pt, flags);
    }

    case STemplateArgument::STA_INT:
      return conc->getInt() == pat->getInt();

    case STemplateArgument::STA_REFERENCE:
      return conc->getReference() == pat->getReference();

    case STemplateArgument::STA_POINTER:
      return conc->getPointer() == pat->getPointer();

    case STemplateArgument::STA_MEMBER:
      return conc->getMember() == pat->getMember();

    case STemplateArgument::STA_DEPEXPR:
      return matchExpression(conc->getDepExpr(), pat->getDepExpr(), flags);
  }
}


bool MType::matchNontypeWithVariable(STemplateArgument const *conc, 
                                     E_variable *pat, MatchFlags flags)
{
  // 'conc' should be a nontype argument
  if (!conc->isObject()) {
    return false;
  }

  if (flags & MF_ISOMORPHIC) {
    // check that we are only binding to a variable; ideally, this
    // would only be an STA_DEPEXPR of an E_variable, but infelicities
    // in other parts of the template-handling code can let this be
    // an STA_REFERENCE of a Variable that is a template parameter..
    // probably, that will be fixed at some point and the STA_REFERENCE
    // possibility can be eliminated
    if (conc->isDepExpr() &&
        conc->getDepExpr()->isE_variable() &&
        conc->getDepExpr()->asE_variable()->var->isTemplateParam()) {
      // ok
    }
    else if (conc->kind == STemplateArgument::STA_REFERENCE &&
             conc->getReference()->isTemplateParam()) {
      // ok, sort of
    }
    else {
      // not compatible with MF_ISOMORPHIC
      return false;
    }
  }

  StringRef vName = pat->name->getName();
  Binding *binding = bindings.get(vName);
  if (binding) {
    // check that 'conc' and 'binding->sarg' are equal
    return matchSTemplateArgument(conc, &(binding->sarg), flags & ~MF_MATCH);
  }
  else {
    if (flags & MF_NO_NEW_BINDINGS) {
      return false;
    }

    // bind 'pat->name' to 'conc'
    binding = new Binding;
    binding->sarg = *conc;
    bindings.add(vName, binding);
    return true;
  }
}


bool MType::matchDependentQType(DependentQType const *conc, 
                                DependentQType const *pat, MatchFlags flags)
{ 
  // compare first components
  if (!matchAtomicType(conc->first, pat->first, flags)) {
    return false;
  }

  // compare sequences of names as just that, names, since their
  // interpretation depends on what type 'first' ends up being;
  // I think this behavior is underspecified in the standard, but
  // it is what gcc and icc seem to do
  return matchPQName(conc->rest, pat->rest, flags);
}

bool MType::matchPQName(PQName const *conc, PQName const *pat, MatchFlags flags)
{
  if (conc->kind() != pat->kind()) {
    return false;
  }

  ASTSWITCH2C(PQName, conc, pat) {
    // recursive case
    ASTCASE2C(PQ_qualifier, cq, pq) {
      // compare as names (i.e., syntactically)
      if (cq->qualifier != pq->qualifier) {
        return false;
      }

      // the template arguments can be compared semantically because
      // the standard does specify that they are looked up in a
      // context that is effectively independent of what appears to
      // the left in the qualified name (cppstd 3.4.3.1p1b3)
      if (!matchSTemplateArguments(cq->sargs, pq->sargs, flags)) {
        return false;
      }

      // continue down the chain
      return matchPQName(cq->rest, pq->rest, flags);
    }

    // base cases
    ASTNEXT2C(PQ_name, cn, pn) {
      // compare as names
      return cn->name == pn->name;
    }
    ASTNEXT2C(PQ_operator, co, po) {
      return co->o == po->o;      // gcov-ignore: can only match on types, and operators are not types
    }
    ASTNEXT2C(PQ_template, ct, pt) {
      // like for PQ_qualifier, except there is no 'rest'
      return ct->name == pt->name &&
             matchSTemplateArguments(ct->sargs, pt->sargs, flags);
    }

    ASTDEFAULT2C {
      xfailure("bad kind");
    }
    ASTENDCASE2C
  }
}


// ----------------- Type and subclasses -----------------
bool MType::matchType(Type const *conc, Type const *pat, MatchFlags flags)
{
  if (pat->isTypeVariable() &&
      canUseAsVariable(pat->asTypeVariableC()->typedefVar, flags)) {
    return matchTypeWithVariable(conc, pat->asTypeVariableC(),
                                 pat->getCVFlags(), flags);
  }

  if (flags & MF_POLYMORPHIC) {
    if (pat->isSimpleType()) {
      SimpleTypeId objId = pat->asSimpleTypeC()->type;
      if (ST_PROMOTED_INTEGRAL <= objId && objId <= ST_ANY_TYPE) {
        return matchTypeWithPolymorphic(conc, objId, flags);
      }
    }
  }

  // further comparison requires that the types have equal tags
  Type::Tag tag = conc->getTag();
  if (pat->getTag() != tag) {
    return false;
  }

  switch (tag) {
    default: xfailure("bad type tag");

    #define CASE(tagName,typeName) \
      case Type::tagName: return match##typeName(conc->as##typeName##C(), pat->as##typeName##C(), flags) /*user;*/
    CASE(T_ATOMIC, CVAtomicType);
    CASE(T_POINTER, PointerType);
    CASE(T_REFERENCE, ReferenceType);
    CASE(T_FUNCTION, FunctionType);
    CASE(T_ARRAY, ArrayType);
    CASE(T_POINTERTOMEMBER, PointerToMemberType);
    #undef CASE
  }
}


bool MType::matchTypeWithVariable(Type const *conc, TypeVariable const *pat,
                                  CVFlags tvCV, MatchFlags flags)
{
  if ((flags & MF_ISOMORPHIC) &&
      !conc->isTypeVariable()) {
    return false;      // MF_ISOMORPHIC requires that we only bind to variables
  }

  StringRef tvName = pat->name;
  Binding *binding = bindings.get(tvName);
  if (binding) {
    // 'tvName' is already bound; compare its current
    // value (as modified by 'tvCV') against 'conc'
    return equalWithAppliedCV(conc, binding, tvCV, flags);
  }
  else {
    if (flags & MF_NO_NEW_BINDINGS) {
      // new bindings are disallowed, so unbound variables in 'pat'
      // cause failure
      return false;
    }

    // bind 'tvName' to 'conc'; 'conc' should have cv-flags
    // that are a superset of those in 'tvCV', and the
    // type to which 'tvName' is bound should have the cv-flags
    // in the difference between 'conc' and 'tvCV'
    return addTypeBindingWithoutCV(tvName, conc, tvCV, flags);
  }
}


// Typical scenario:
//   conc is 'int const'
//   binding is 'int'
//   cv is 'const'
// We want to compare the type denoted by 'conc' with the type denoted
// by 'binding' with the qualifiers in 'cv' added to it at toplevel.
bool MType::equalWithAppliedCV(Type const *conc, Binding *binding, CVFlags cv, MatchFlags flags)
{
  // turn off type variable binding/substitution
  flags &= ~MF_MATCH;

  if (binding->sarg.isType()) {
    Type const *t = binding->getType();

    // cv-flags for 't' are ignored, replaced by the cv-flags stored
    // in the 'binding' object
    cv |= binding->cv;

    return matchTypeWithSpecifiedCV(conc, t, cv, flags);
  }

  if (binding->sarg.isAtomicType()) {
    if (!conc->isCVAtomicType()) {
      return false;
    }

    // compare the atomics
    if (!matchAtomicType(conc->asCVAtomicTypeC()->atomic,
                         binding->sarg.getAtomicType(), flags)) {
      return false;
    }

    // When a name is bound directly to an atomic type, it is compatible
    // with any binding to a CVAtomicType for the same atomic; that is,
    // it is compatible with any cv-qualified variant.  So, if we
    // are paying attention to cv-flags at all, simply replace the
    // original (AtomicType) binding with 'conc' (a CVAtomicType) and
    // appropriate cv-flags.
    if (!( flags & MF_OK_DIFFERENT_CV )) {
      // The 'cv' flags supplied are subtracted from those in 'conc'.
      CVFlags concCV = conc->getCVFlags();
      if (concCV >= cv) {
        // example:
        //   conc = 'struct B volatile const'
        //   binding = 'struct B'
        //   cv = 'const'
        // Since 'const' was supplied (e.g., "T const") with the type
        // variable, we want to remove it from what we bind to, so here
        // we will bind to 'struct B volatile' ('concCV' = 'volatile').
        concCV = (concCV & ~cv);
      }
      else {
        // example:
        //   conc = 'struct B volatile'
        //   binding = 'struct B'
        //   cv = 'const'
        // This means we are effectively comparing 'struct B volatile' to
        // 'struct B volatile const' (the latter volatile arising because
        // being directly bound to an atomic means we can add qualifiers),
        // and these are not equal.
        return false;
      }

      binding->setType(conc);
      binding->cv = concCV;
      return true;
    }

  }

  // I *think* that the language rules that prevent same-named
  // template params from nesting will have the effect of preventing
  // this code from being reached, but if it turns out I am wrong, it
  // should be safe to simply remove the assertion and return false.
  xfailure("attempt to match a type with a variable bound to a non-type");

  return false;      // gcov-ignore
}

bool MType::matchTypeWithSpecifiedCV(Type const *conc, Type const *pat, CVFlags cv, MatchFlags flags)
{
  // compare underlying types, ignoring first level of cv
  return matchCVFlags(conc->getCVFlags(), cv, flags) &&
         matchType(conc, pat, flags | MF_IGNORE_TOP_CV);
}


bool MType::addTypeBindingWithoutCV(StringRef tvName, Type const *conc, 
                                    CVFlags tvcv, MatchFlags flags)
{
  CVFlags ccv = conc->getCVFlags();

  if ((flags & MF_DEDUCTION) && (flags & MF_IGNORE_TOP_CV)) {
    // trying to implement 14.8.2.1p2b3
    ccv = CV_NONE;
  }

  if (tvcv & ~ccv) {
    // the type variable was something like 'T const' but the concrete
    // type does not have all of the cv-flags (e.g., just 'int', no
    // 'const'); this means the match is a failure
    if (flags & MF_DEDUCTION) {
      // let it go anyway, as part of my poor approximationg of 14.8.2.1
    }
    else {
      return false;
    }
  }

  // 'tvName' will be bound to 'conc', except we will ignore the
  // latter's cv flags
  Binding *binding = new Binding;
  binding->setType(conc);

  // instead, compute the set of flags that are on 'conc' but not
  // 'tvcv'; this will be the cv-flags of the type to which 'tvName'
  // is bound
  binding->cv = (ccv & ~tvcv);

  // add the binding
  bindings.add(tvName, binding);
  return true;
}


// check if 'conc' matches the "polymorphic" type family 'polyId'
bool MType::matchTypeWithPolymorphic(Type const *conc, SimpleTypeId polyId,
                                     MatchFlags flags)
{
  // check those that can match any type constructor
  if (polyId == ST_ANY_TYPE) {
    return true;
  }

  if (polyId == ST_ANY_NON_VOID) {
    return !conc->isVoid();
  }

  if (polyId == ST_ANY_OBJ_TYPE) {
    return !conc->isFunctionType() &&
           !conc->isVoid();
  }

  // check those that only match atomics
  if (conc->isSimpleType()) {
    SimpleTypeId concId = conc->asSimpleTypeC()->type;
    SimpleTypeFlags concFlags = simpleTypeInfo(concId).flags;

    // see cppstd 13.6 para 2
    if (polyId == ST_PROMOTED_INTEGRAL) {
      return (concFlags & (STF_INTEGER | STF_PROM)) == (STF_INTEGER | STF_PROM);
    }

    if (polyId == ST_PROMOTED_ARITHMETIC) {
      return (concFlags & (STF_INTEGER | STF_PROM)) == (STF_INTEGER | STF_PROM) ||
             (concFlags & STF_FLOAT);      // need not be promoted!
    }

    if (polyId == ST_INTEGRAL) {
      return (concFlags & STF_INTEGER) != 0;
    }

    if (polyId == ST_ARITHMETIC) {
      return (concFlags & (STF_INTEGER | STF_FLOAT)) != 0;
    }

    if (polyId == ST_ARITHMETIC_NON_BOOL) {
      return concId != ST_BOOL &&
             (concFlags & (STF_INTEGER | STF_FLOAT)) != 0;
    }
  }

  // polymorphic type failed to match
  return false;
}


bool MType::matchAtomicTypeWithVariable(AtomicType const *conc,
                                        TypeVariable const *pat,
                                        MatchFlags flags)
{
  if ((flags & MF_ISOMORPHIC) &&
      !conc->isTypeVariable()) {
    return false;      // MF_ISOMORPHIC requires that we only bind to variables
  }

  StringRef tvName = pat->name;
  Binding *binding = bindings.get(tvName);
  if (binding) {
    // 'tvName' is already bound; it should be bound to the same
    // atomic as 'conc', possibly with some (extra, ignored) cv flags
    if (binding->sarg.isType()) {
      Type const *t = binding->getType();
      if (t->isCVAtomicType()) {
        return matchAtomicType(conc, t->asCVAtomicTypeC()->atomic, flags & ~MF_MATCH);
      }
      else {
        return false;
      }
    }
    else if (binding->sarg.isAtomicType()) {
      return matchAtomicType(conc, binding->sarg.getAtomicType(), flags & ~MF_MATCH);
    }
    else {              // gcov-ignore: cannot be triggered, the error is
      return false;     // gcov-ignore: diagnosed before mtype is entered 
    }
  }
  else {
    if (flags & MF_NO_NEW_BINDINGS) {
      // new bindings are disallowed, so unbound variables in 'pat'
      // cause failure
      return false;
    }

    // bind 'tvName' to 'conc'
    binding = new Binding;
    binding->sarg.setAtomicType(conc);
    bindings.add(tvName, binding);
    return true;
  }
}



bool MType::matchCVAtomicType(CVAtomicType const *conc, CVAtomicType const *pat,
                              MatchFlags flags)
{
  if ((flags & MF_MATCH) && 
      !(flags & MF_ISOMORPHIC) &&
      pat->asCVAtomicTypeC()->atomic->isDependentQType()) {
    // (in/t0487.cc, in/t0487b.cc) the pattern names a DQT; if the
    // first part of the DQT is resolvable in the current set of
    // bindings, then do so
    DependentQType const *patDQT = pat->asCVAtomicTypeC()->atomic->asDependentQTypeC();
    if (patDQT->first->isTypeVariable()) {
      // is the first part bound?
      Binding *binding = bindings.get(patDQT->first->asTypeVariableC()->name);
      if (binding) {
        // find the CompoundType to which it is bound
        CompoundType const *bindingCT = NULL;
        if (binding->sarg.isType() &&
            binding->getType()->isCompoundType()) {
          bindingCT = (binding->getType()->asCompoundTypeC());
        }
        else if (binding->sarg.isAtomicType() &&
                 binding->sarg.getAtomicType()->isCompoundType()) {
          bindingCT = binding->sarg.getAtomicType()->asCompoundTypeC();
        }

        // apply the remainder of the DQT name
        if (bindingCT) {
          Type *resolvedType = lookupPQInScope(bindingCT, patDQT->rest);
          if (resolvedType) {
            // take the resolved type, apply the cv-flags from 'pat', and
            // compare the result to 'conc'
            return matchTypeWithSpecifiedCV(conc, resolvedType,
                                            resolvedType->getCVFlags() | pat->cv,
                                            flags & ~MF_MATCH);
           }
        }

        // the type variable at the head of the DQT is bound, but either
        // not to a CompoundType, or to one for which the PQName doesn't
        // denote a valid type; matching as a whole fails
        return false;
      }
    }
  }

  // compare cv-flags
  return matchCVFlags(conc->cv, pat->cv, flags) &&
         matchAtomicType(conc->atomic, pat->atomic, flags);
}

  
bool MType::matchCVFlags(CVFlags conc, CVFlags pat, MatchFlags flags)
{
  if (flags & MF_OK_DIFFERENT_CV) {
    // anything goes
    return true;
  }
  else if (flags & MF_DEDUCTION) {
    // merely require that the pattern flags be a superset of
    // the concrete flags (in/t0315.cc, in/t0348.cc)
    //
    // TODO: this is wrong, as it does not correctly implement
    // 14.8.2.1; I am only implementing this because it emulates
    // what matchtype does right now
    if (pat >= conc) {
      return true;
    }
    else {
      return false;
    }
  }
  else {
    // require equality
    return conc == pat;
  }
}


// For now I am implementing this in MType rather than Scope, since
// I don't want to tamper with the existing lookup just for this
// somewhat special purpose.  However, I might merge them later.
// This routine either returns a type, or NULL to indicate the name
// doesn't denote a valid type.
Type *MType::lookupPQInScope(Scope const *scope, PQName const *name)
{
  ASTSWITCHC(PQName, name) {
    ASTCASEC(PQ_qualifier, q) {
      // map the name
      Variable *var = lookupNameInScope(scope, q->qualifier);

      // apply template args if present
      if (var && q->sargs.isNotEmpty()) {
        // Oops, there is a flaw in my design.  If a template-id is
        // used as a qualifier, then there is no reason that the
        // concrete type need have mentioned this template-id, which
        // means it might not have been instantiated.  See
        // in/t0511.cc, input "PQ_qualifier *with* template args".
        //
        // Fundamentally, if you say MF_MATCH then the process of
        // binding variables within DQTs can create the first mention
        // of a given template-id, necessatating its instantiation.
        // Therefore, an Env must be passed too, to account for this
        // possibility.
        //
        // For now, I just note that this flaw exists, since I do not
        // now need (nor do I forsee needing in the near future) to
        // handle PQ_qualifier with template args in a DQT.
        //
        // When I do get around to implementing this, my plan is to
        // add an Env* argument to the MType constructor, and require
        // that this be non-NULL when MF_MATCH is specified, and then
        // go ahead and do the instantiation here.  In fact, at that
        // point it probably makes sense to just use Env::lookupPQ,
        // passing LF_SUPPRESS_ERROR (and I think modifying it to
        // respect that flag), rather than use this lookup code.
        xunimp("PQ_qualifier with template args in a DQT");

        #if 0     // this code would work if all needed templates had been instantiated
        var = applyTemplateArgs(var, q->sargs);
        #endif // 0
      }

      // bail if either the lookup or the template argument
      // application failed
      if (!var) {
        return NULL;
      }

      // the result should be a CompoundType, because we have to use
      // it as the scope to search in next
      if (!var->isType() || !var->type->isCompoundType()) {
        return NULL;
      }

      // recursively search in it
      return lookupPQInScope(var->type->asCompoundType(), q->rest);
    }

    ASTNEXTC(PQ_name, n) {
      Variable *var = lookupNameInScope(scope, n->name);
      if (var && var->isType()) {
        return var->type;
      }
      else {
        return NULL;
      }
    }

    ASTNEXTC1(PQ_operator) {
      xfailure("a PQ_operator was parsed as a type!");
    }

    ASTNEXTC(PQ_template, t) {
      // similar the PQ_qualifier case
      Variable *var = lookupNameInScope(scope, t->name);
      if (var) {
        var = applyTemplateArgs(var, t->sargs);
      }
      if (!var) {
        return NULL;
      }

      // since we certainly called 'applyTemplateArgs', the result
      // must be a CompoundType, which is a valid Type
      xassert(var->isType());
      return var->type;
    }

    ASTDEFAULTC {
      xfailure("bad PQName kind");
    }

    ASTENDCASEC
  }
}


Variable *MType::lookupNameInScope(Scope const *scope0, StringRef name)
{
  // the lookup and what I do subsequently with the results do not
  // modify the scope ...
  Scope *scope = const_cast<Scope*>(scope0);

  // I had thought I could avoid calling into Scope::lookup, but I
  // think I need the base class searching mechanism.  I am fairly
  // certain I do *not* need the 'using' edge traversal mechanism,
  // though, hence LF_IGNORE_USING.
  return scope->lookup_one(name, NULL /*env*/,
                           LF_IGNORE_USING | LF_ONLY_TYPES);
}


Variable *MType::applyTemplateArgs(Variable *primary, 
                                   ObjList<STemplateArgument> const &sargs)
{
  if (!primary->isTemplateClass(false /*considerInherited*/)) {
    return NULL;
  }

  TemplateInfo *ti = primary->templateInfo();
  xassert(ti);

  // The key to making this work without an Env object is recognizing
  // that, for matching to ultimately succeed, any needed
  // instantiations must *already* have been created, because the
  // concrete type must have mentioned them.  Therefore, we can simply
  // search among the extant instantiations for one that matches
  // 'sargs'.
  //
  // Ack, this is not quite true; see the xunimp for PQ_qualifier, above.
  
  Variable *ret = searchForInstantiation(ti, sargs);
  if (ret) {
    return ret;
  }
  
  // Also look in the explicit (full) specializations and the
  // instantiations of partial specializations.
  SFOREACH_OBJLIST_NC(Variable, ti->specializations, iter) {
    TemplateInfo *spec = iter.data()->templateInfo();

    if (spec->isCompleteSpec()) {
      if (equalArgumentLists(spec->arguments, sargs)) {
        return iter.data();
      }
    }
    else {
      // partial spec; check its instantiations
      ret = searchForInstantiation(spec, sargs);
      if (ret) {
        return ret;
      }
    }
  }

  // didn't find it anywhere
  return NULL;
}


// This is similar to TemplateInfo::getSpecialization, but different
// because it does not require a TypeFactory.  Eventually, neither
// will the former, once it is adapted to use mtype instead of
// matchtype, so this exists only as an evolutionary stepping stone.
Variable *MType::searchForInstantiation(TemplateInfo *ti,
                                        ObjList<STemplateArgument> const &sargs)
{
  SFOREACH_OBJLIST_NC(Variable, ti->instantiations, iter) {
    TemplateInfo *instTI = iter.data()->templateInfo();

    // We look at 'arguments' or 'argumentsToPrimary', depending on
    // whether this is an instantiation of the primary or
    // instantiation of a partial spec.
    if (equalArgumentLists(instTI->isInstOfPartialSpec() ?
                             instTI->argumentsToPrimary :
                             instTI->arguments,
                           sargs)) {
      return iter.data();
    }
  }

  return NULL;    // not found
}


bool MType::matchPointerType(PointerType const *conc, PointerType const *pat, MatchFlags flags)
{
  // note how MF_IGNORE_TOP_CV allows *this* type's cv flags to differ,
  // but it's immediately suppressed once we go one level down; this
  // behavior is repeated in all 'match' methods

  return matchCVFlags(conc->cv, pat->cv, flags) &&
         matchType(conc->atType, pat->atType, flags & MF_PTR_PROP);
}


bool MType::matchReferenceType(ReferenceType const *conc, ReferenceType const *pat, MatchFlags flags)
{
  if (flags & MF_DEDUCTION) {
    // (in/t0114.cc, in/d0072.cc) allow pattern to be more cv-qualified;
    // this only approximates 14.8.2.1, but emulates current matchtype
    // behavior (actually, it emulates only the intended matchtype
    // behavior; see comment added 2005-08-03 to MatchTypes::match_ref)
    if (pat->atType->getCVFlags() >= conc->atType->getCVFlags()) {
      // disable further comparison of these cv-flags
      flags |= MF_IGNORE_TOP_CV;
    }
  }

  return matchType(conc->atType, pat->atType, flags & MF_PTR_PROP);
}


bool MType::matchFunctionType(FunctionType const *conc, FunctionType const *pat, MatchFlags flags)
{
  // I do not compare 'FunctionType::flags' explicitly since their
  // meaning is generally a summary of other information, or of the
  // name (which is irrelevant to the type)

  if (!(flags & MF_IGNORE_RETURN)) {
    // check return type
    if (!matchType(conc->retType, pat->retType, flags & MF_PROP)) {
      return false;
    }
  }

  if ((conc->flags | pat->flags) & FF_NO_PARAM_INFO) {
    // at least one of the types does not have parameter info,
    // so no further comparison is possible
    return true;            // gcov-ignore: cannot trigger in C++ mode
  }

  if (!(flags & MF_STAT_EQ_NONSTAT)) {
    // check that either both are nonstatic methods, or both are not
    if (conc->isMethod() != pat->isMethod()) {
      return false;
    }
  }

  // check that both are variable-arg, or both are not
  if (conc->acceptsVarargs() != pat->acceptsVarargs()) {
    return false;
  }

  // check the parameter lists
  if (!matchParameterLists(conc, pat, flags)) {
    return false;
  }

  if (!(flags & MF_IGNORE_EXN_SPEC)) {
    // check the exception specs
    if (!matchExceptionSpecs(conc, pat, flags)) {
      return false;
    }
  }
  
  return true;
}

bool MType::matchParameterLists(FunctionType const *conc, FunctionType const *pat,
                                MatchFlags flags)
{
  SObjListIter<Variable> concIter(conc->params);
  SObjListIter<Variable> patIter(pat->params);

  // skip the 'this' parameter(s) if desired, or if one has it
  // but the other does not (can arise if MF_STAT_EQ_NONSTAT has
  // been specified)
  {
    bool cm = conc->isMethod();
    bool pm = pat->isMethod();
    bool ignore = flags & MF_IGNORE_IMPLICIT;
    if (cm && (!pm || ignore)) {
      concIter.adv();
    }
    if (pm && (!cm || ignore)) {
      patIter.adv();
    }
  }

  // this takes care of matchFunctionType's obligation
  // to suppress non-propagated flags after consumption
  flags &= MF_PROP;

  // allow toplevel cv flags on parameters to differ
  flags |= MF_IGNORE_TOP_CV;

  for (; !concIter.isDone() && !patIter.isDone();
       concIter.adv(), patIter.adv()) {
    // parameter names do not have to match, but
    // the types do
    if (matchType(concIter.data()->type, patIter.data()->type, flags)) {
      // ok
    }
    else {
      return false;
    }
  }

  return concIter.isDone() == patIter.isDone();
}


// almost identical code to the above.. list comparison is
// always a pain..
bool MType::matchExceptionSpecs(FunctionType const *conc, FunctionType const *pat, MatchFlags flags)
{
  if (conc->exnSpec==NULL && pat->exnSpec==NULL)  return true;
  if (conc->exnSpec==NULL || pat->exnSpec==NULL)  return false;

  // hmm.. this is going to end up requiring that exception specs
  // be listed in the same order, whereas I think the semantics
  // imply they're more like a set.. oh well
  //
  // but if they are a set, how the heck do you do matching?
  // I think I see; the matching must have already led to effectively
  // concrete types on both sides.  But, since I only see 'pat' as
  // the pattern + substitutions, it would still be hard to figure
  // out the correct correspondence for the set semantics.
  
  // this will at least ensure I do not derive any bindings from
  // the attempt to compare exception specs
  flags |= MF_NO_NEW_BINDINGS;

  SObjListIter<Type> concIter(conc->exnSpec->types);
  SObjListIter<Type> patIter(pat->exnSpec->types);
  for (; !concIter.isDone() && !patIter.isDone();
       concIter.adv(), patIter.adv()) {
    if (matchType(concIter.data(), patIter.data(), flags)) {
      // ok
    }
    else {
      return false;
    }
  }

  return concIter.isDone() == patIter.isDone();
}


bool MType::matchArrayType(ArrayType const *conc, ArrayType const *pat, MatchFlags flags)
{
  // what flags to propagate?
  MatchFlags propFlags = (flags & MF_PROP);

  if (flags & MF_IGNORE_ELT_CV) {
    if (conc->eltType->isArrayType()) {
      // propagate the ignore-elt down through as many ArrayTypes
      // as there are
      propFlags |= MF_IGNORE_ELT_CV;
    }
    else {
      // the next guy is the element type, ignore *his* cv only
      propFlags |= MF_IGNORE_TOP_CV;
    }
  }

  if (!( matchType(conc->eltType, pat->eltType, propFlags) &&
         conc->hasSize() == pat->hasSize() )) {
    return false;
  }

  // TODO: At some point I will implement dependent-sized arrays
  // (t0435.cc), at which point the size comparison code here will
  // have to be generalized.

  if (conc->hasSize()) {
    return conc->size == pat->size;
  }
  else {
    return true;
  }
}


bool MType::matchPointerToMemberType(PointerToMemberType const *conc, 
                                     PointerToMemberType const *pat, MatchFlags flags)
{
  return matchAtomicType(conc->inClassNAT, pat->inClassNAT, flags) &&
         matchCVFlags(conc->cv, pat->cv, flags) &&
         matchType(conc->atType, pat->atType, flags & MF_PTR_PROP);
}


// ------------------------ Expression -------------------------
// This is not full-featured matching as with types, rather this
// is mostly just comparison for equality.
bool MType::matchExpression(Expression const *conc, Expression const *pat, MatchFlags flags)
{
  if (conc->isE_grouping()) {
    return matchExpression(conc->asE_groupingC()->expr, pat, flags);
  }
  if (pat->isE_grouping()) {
    return matchExpression(conc, pat->asE_groupingC()->expr, flags);
  }

  if (conc->kind() != pat->kind()) {
    return false;
  }

  // turn off variable matching for this part because expression
  // comparison should be fairly literal
  flags &= ~MF_MATCH;

  ASTSWITCH2C(Expression, conc, pat) {
    // only the expression constructors that yield integers and do not
    // have side effects are allowed within type constructors, so that
    // is all we deconstruct here
    //
    // TODO: should 65 and 'A' be regarded as equal here?  For now, I
    // do not regard them as equivalent...
    ASTCASE2C(E_boolLit, c, p) {
      return c->b == p->b;
    }
    ASTNEXT2C(E_intLit, c, p) {
      return c->i == p->i;
    }
    ASTNEXT2C(E_charLit, c, p) {
      return c->c == p->c;
    }
    ASTNEXT2C(E_variable, c, p) {
      if (c->var == p->var) {
        return true;
      }
      if (c->var->isTemplateParam() &&
          p->var->isTemplateParam()) {
        // like for matchTypeVariable
        return c->var->sameTemplateParameter(p->var);
      }
      return false;
    }
    ASTNEXT2C(E_sizeof, c, p) {
      // like above: is sizeof(int) the same as 4?
      return matchExpression(c->expr, p->expr, flags);
    }
    ASTNEXT2C(E_unary, c, p) {
      return c->op == p->op &&
             matchExpression(c->expr, p->expr, flags);
    }
    ASTNEXT2C(E_binary, c, p) {
      return c->op == p->op &&
             matchExpression(c->e1, p->e1, flags) &&
             matchExpression(c->e2, p->e2, flags);
    }
    ASTNEXT2C(E_cast, c, p) {
      return matchType(c->ctype->getType(), p->ctype->getType(), flags) &&
             matchExpression(c->expr, p->expr, flags);
    }
    ASTNEXT2C(E_cond, c, p) {
      return matchExpression(c->cond, p->cond, flags) &&
             matchExpression(c->th, p->th, flags) &&
             matchExpression(c->el, p->el, flags);
    }
    ASTNEXT2C(E_sizeofType, c, p) {
      return matchType(c->atype->getType(), p->atype->getType(), flags);
    }
    ASTNEXT2C(E_keywordCast, c, p) {
      return c->key == p->key &&
             matchType(c->ctype->getType(), p->ctype->getType(), flags) &&
             matchExpression(c->expr, p->expr, flags);
    }
    ASTDEFAULT2C {
      // For expressions that are *not* const-eval'able, we can't get
      // here because tcheck reports an error and bails before we get
      // a chance.  So, the only way to trigger this code is to extend
      // the constant-expression evaluator to handle a new form, and
      // then fail to update the comparison code here to compare such
      // forms with each other.
      xfailure("some kind of expression is const-eval'able but mtype "
               "does not know how to compare it");     // gcov-ignore
    }
    ASTENDCASE2C
  }
}



// EOF
