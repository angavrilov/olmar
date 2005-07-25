// mtype.cc
// code for mtype.h

#include "mtype.h"       // this module
#include "trace.h"       // tracingSys


string toString(MatchFlags flags)
{
  if (flags == MF_EXACT) {
    return "MF_EXACT";
  }

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


bool MType::match(Type const *conc, Type const *pat, MatchFlags flags)
{
  // I can only uphold the promise of not modifying 'conc' and 'pat'
  // if asked to when I was created.
  xassert(!nonConst);

  return imatch(conc, pat, flags);
}


bool MType::matchNC(Type *conc, Type *pat, MatchFlags flags)
{
  return imatch(conc, pat, flags);
}


// This internal layer exists just to have a place for the debugging code.
bool MType::imatch(Type const *conc, Type const *pat, MatchFlags flags)
{
  bool result = matchType(conc, pat, flags);

  #ifndef NDEBUG
    static bool doTrace = tracingSys("mtype");
    if (doTrace) {
      ostream &os = trace("mtype");
      os << "conc=`" << conc->toString()
         << "' pat=`" << pat->toString()
         << "' flags=" << toString(flags)
         << "; match=" << (result? "true" : "false")
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


// ------------------ AtomicType and subclasses ---------------
bool MType::matchAtomicType(AtomicType const *conc, AtomicType const *pat, MatchFlags flags)
{
  if (conc == pat) {
    return true;
  }

  if (conc->getTag() != pat->getTag()) {
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

  ObjListIter<STemplateArgument> concIter(conc->args);
  ObjListIter<STemplateArgument> patIter(pat->args);

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
  if ((flags & MF_MATCH) &&
      pat->kind == STemplateArgument::STA_DEPEXPR &&
      pat->getDepExpr()->isE_variable()) {
    return matchNontypeWithVariable(conc, pat->getDepExpr()->asE_variable(), flags);
  }

  if (conc->kind != pat->kind) {
    return false;
  }

  switch (conc->kind) {
    default: xfailure("bad or unimplemented STemplateArgument kind");

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
  // for now, only physical equality
  //
  // this is a bug; structural equality (along the lines of
  // PseudoInstantiation::innerEquals) should be allowed; but I'm
  // having trouble writing a testcase because of the more fundamental
  // problem that I do not implement matching against parameter types
  // that are DQTs (in/t0487.cc)
  return conc == pat;
}


// ----------------- Type and subclasses -----------------
bool MType::matchType(Type const *conc, Type const *pat, MatchFlags flags)
{
  if ((flags & MF_MATCH) &&
      pat->isTypeVariable()) {
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
    return addTypeBindingWithoutCV(tvName, conc, tvCV);
  }
}


bool MType::equalWithAppliedCV(Type const *conc, Binding *binding, CVFlags cv, MatchFlags flags)
{                    
  // turn off type variable binding/substitution
  flags &= ~MF_MATCH;

  if (binding->sarg.isType()) {
    Type const *t = binding->getType();

    if (!( flags & MF_OK_DIFFERENT_CV )) {
      // cv-flags for the bound type are ignored, replaced by the
      // cv-flags stored in the 'binding' object
      cv |= binding->cv;

      // compare cv-flags
      if (cv != conc->getCVFlags()) {
        return false;
      }
    }

    // compare underlying types, ignoring first level of cv
    return matchType(conc, t, flags | MF_IGNORE_TOP_CV);
  }
  
  if (binding->sarg.isAtomicType()) {
    if (!conc->isCVAtomicType()) {
      return false;
    }

    if (!( flags & MF_OK_DIFFERENT_CV )) {
      // compare cv-flags
      if (cv != conc->getCVFlags()) {
        return false;
      }
    }

    // compare the atomics
    return matchAtomicType(conc->asCVAtomicTypeC()->atomic, 
                           binding->sarg.getAtomicType(), flags);
  }
                                                                          
  // I *think* that the language rules preventing same-named template
  // params from nesting will prevent this code from being reached,
  // but if it turns out I am wrong, it should be safe to simply remove
  // the assertion and return false.
  xfailure("attempt to match a type with a variable bound to a non-type");
  return false;
}


bool MType::addTypeBindingWithoutCV(StringRef tvName, Type const *conc, CVFlags tvcv)
{
  CVFlags ccv = conc->getCVFlags();
  if (tvcv & ~ccv) {
    // the type variable was something like 'T const' but the concrete
    // type does not have all of the cv-flags (e.g., just 'int', no
    // 'const'); this means the match is a failure
    return false;
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



bool MType::matchCVAtomicType(CVAtomicType const *conc, CVAtomicType const *pat, 
                              MatchFlags flags)
{
  return matchAtomicType(conc->atomic, pat->atomic, flags) &&
         ((flags & MF_OK_DIFFERENT_CV) || (conc->cv == pat->cv));
}


bool MType::matchPointerType(PointerType const *conc, PointerType const *pat, MatchFlags flags)
{
  // note how MF_IGNORE_TOP_CV allows *this* type's cv flags to differ,
  // but it's immediately suppressed once we go one level down; this
  // behavior is repeated in all 'match' methods

  return ((flags & MF_OK_DIFFERENT_CV) || (conc->cv == pat->cv)) &&
         matchType(conc->atType, pat->atType, flags & MF_PTR_PROP);
}


bool MType::matchReferenceType(ReferenceType const *conc, ReferenceType const *pat, MatchFlags flags)
{
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
    return true;
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
         ((flags & MF_OK_DIFFERENT_CV) || (conc->cv == pat->cv)) &&
         matchType(conc->atType, pat->atType, flags & MF_PTR_PROP);
}


// ------------------------ Expression -------------------------
// This is not full-featured matching as with types, rather this
// is mostly just comparison for equality.
bool MType::matchExpression(Expression const *conc, Expression const *pat, MatchFlags flags)
{
  if (conc->kind() != pat->kind()) {
    return false;
  }

  if (conc->isE_grouping()) {
    return matchExpression(conc->asE_groupingC()->expr, pat, flags);
  }
  if (pat->isE_grouping()) {
    return matchExpression(conc, pat->asE_groupingC()->expr, flags);
  }

  // turn off variable matching for this part because expression
  // comparison should be fairly literal
  flags &= ~MF_MATCH;

  ASTSWITCH2C(Expression, conc, pat) {
    // only the expression constructors that yield integers and do not
    // have side effects are allowed within type constructors, so that
    // is all we deconstruct here
    //
    // TODO: should 65 and 'A' be regarded as equal here?
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
      return false;
    }
    ASTENDCASE2C
  }
}



// EOF
