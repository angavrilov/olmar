// template.cc
// template stuff implementation; code for template.h, and for
// the template section of cc_env.h at the end of Env

#include "template.h"      // this module
#include "cc_env.h"        // also kind of this module
#include "trace.h"         // tracingSys
#include "strtable.h"      // StringTable
#include "cc_lang.h"       // CCLang
#include "strutil.h"       // pluraln
#include "overload.h"      // selectBestCandidate_templCompoundType
#include "matchtype.h"     // MatchType, STemplateArgumentCMap
#include "typelistiter.h"  // TypeListIter
#include "cc_ast_aux.h"    // ASTTemplVisitor


// I'm doing this fairly frequently, and it is pretty safe (really should
// be returning a const ref...), so I'll consolidate a bit
template <class T>
inline SObjList<T>& objToSObjList(ObjList<T> &list)
{
  return reinterpret_cast<SObjList<T>&>(list);
}

// this one should be completely safe
template <class T>
inline SObjList<T> const & objToSObjListC(ObjList<T> const &list)
{
  return reinterpret_cast<SObjList<T> const &>(list);
}


void copyTemplateArgs(ObjList<STemplateArgument> &dest,
                      SObjList<STemplateArgument> const &src)
{
  xassert(dest.isEmpty());    // otherwise prepend/reverse won't work
  SFOREACH_OBJLIST(STemplateArgument, src, iter) {
    dest.prepend(new STemplateArgument(*(iter.data())));
  }
  dest.reverse();
}


// ------------------ TypeVariable ----------------
TypeVariable::~TypeVariable()
{}

string TypeVariable::toCString() const
{
  // use the "typename" syntax instead of "class", to distinguish
  // this from an ordinary class, and because it's syntax which
  // more properly suggests the ability to take on *any* type,
  // not just those of classes
  //
  // but, the 'typename' syntax can only be used in some specialized
  // circumstances.. so I'll suppress it in the general case and add
  // it explicitly when printing the few constructs that allow it
  //
  // 8/09/04: sm: truncated down to just the name, since the extra
  // clutter was annoying and not that helpful
  return stringc //<< "/""*typevar"
//                   << "typedefVar->serialNumber:"
//                   << (typedefVar ? typedefVar->serialNumber : -1)
                 //<< "*/"
                 << name;
}

int TypeVariable::reprSize() const
{
  //xfailure("you can't ask a type variable for its size");

  // this happens when we're typechecking a template class, without
  // instantiating it, and we want to verify that some size expression
  // is constant.. so make up a number
  return 4;
}


// -------------------- PseudoInstantiation ------------------
PseudoInstantiation::PseudoInstantiation(CompoundType *p)
  : NamedAtomicType(p->name),
    primary(p),
    args()        // empty initially
{}

PseudoInstantiation::~PseudoInstantiation()
{}

string PseudoInstantiation::toCString() const
{
  return stringc << primary->name << sargsToString(args);
}

int PseudoInstantiation::reprSize() const
{
  // it shouldn't matter what we say here, since the query will only
  // be made in the context of checking (but not instantiating) a
  // template definition body
  return 4;
}


// ------------------ TemplateParams ---------------
TemplateParams::TemplateParams(TemplateParams const &obj)
  : params(obj.params)
{}

TemplateParams::~TemplateParams()
{}


string TemplateParams::paramsToCString() const
{
  return ::paramsToCString(params);
}

string paramsToCString(SObjList<Variable> const &params)
{
  stringBuilder sb;
  sb << "template <";
  int ct=0;
  SFOREACH_OBJLIST(Variable, params, iter) {
    Variable const *p = iter.data();
    if (ct++ > 0) {
      sb << ", ";
    }

    if (p->type->isTypeVariable()) {
      sb << "class " << p->name;
      if (p->type->asTypeVariable()->name != p->name) {
        // this should never happen, but if it does then I just want
        // it to be visible, not (directly) cause a crash
        sb << " /""* but type name is " << p->type->asTypeVariable()->name << "! */";
      }
    }
    else {
      // non-type parameter
      sb << p->toCStringAsParameter();
    }
  }
  sb << ">";
  return sb;
}

    
string TemplateParams::paramsLikeArgsToString() const
{
  stringBuilder sb;
  sb << "<";
  int ct=0;
  SFOREACH_OBJLIST(Variable, params, iter) {
    if (ct++) { sb << ", "; }
    sb << iter.data()->name;
  }
  sb << ">";
  return sb;
}


// defined in cc_type.cc
bool parameterListCtorSatisfies(TypePred &pred,
                                SObjList<Variable> const &params);

bool TemplateParams::anyParamCtorSatisfies(TypePred &pred) const
{
  return parameterListCtorSatisfies(pred, params);
}


// --------------- InheritedTemplateParams ---------------
InheritedTemplateParams::InheritedTemplateParams(InheritedTemplateParams const &obj)
  : TemplateParams(obj),
    enclosing(obj.enclosing)
{}

InheritedTemplateParams::~InheritedTemplateParams()
{}


// ------------------ TemplateInfo -------------
TemplateInfo::TemplateInfo(SourceLoc il, Variable *v)
  : TemplateParams(),
    var(NULL),
    declSyntax(NULL),
    instantiationOf(NULL),
    instantiations(),
    specializationOf(NULL),
    specializations(),
    arguments(),
    instLoc(il),
    partialInstantiationOf(NULL),
    partialInstantiations(),
    argumentsToPrimary(),
    defnScope(NULL),
    definitionTemplateInfo(NULL),
    instantiateBody(false)
{
  if (v) {
    // this sets 'this->var' too
    v->setTemplateInfo(this);
  }
}


// this is called by Env::makeUsingAliasFor ..
TemplateInfo::TemplateInfo(TemplateInfo const &obj)
  : TemplateParams(obj),
    var(NULL),                // caller must call Variable::setTemplateInfo
    declSyntax(NULL),
    instantiationOf(NULL),
    instantiations(obj.instantiations),      // suspicious... oh well
    specializationOf(NULL),
    specializations(obj.specializations),    // also suspicious
    arguments(),                             // copied below
    instLoc(obj.instLoc),
    partialInstantiationOf(NULL),
    partialInstantiations(),
    argumentsToPrimary(),                    // copied below
    defnScope(NULL),
    definitionTemplateInfo(NULL),
    instantiateBody(false)
{
  // inheritedParams
  FOREACH_OBJLIST(InheritedTemplateParams, obj.inheritedParams, iter2) {
    inheritedParams.prepend(new InheritedTemplateParams(*(iter2.data())));
  }
  inheritedParams.reverse();

  // arguments
  copyArguments(obj.arguments);

  // argumentsToPrimary
  copyTemplateArgs(argumentsToPrimary, objToSObjListC(obj.argumentsToPrimary));
}


TemplateInfo::~TemplateInfo()
{
  if (definitionTemplateInfo) {
    delete definitionTemplateInfo;
  }
}


TemplateThingKind TemplateInfo::getKind() const
{
  if (!instantiationOf && !specializationOf) {
    if (!isPartialInstantiation()) {
      xassert(arguments.isEmpty());
    }
    return TTK_PRIMARY;
  }

  // specialization or instantiation
  xassert(arguments.isNotEmpty());

  if (specializationOf) {
    return TTK_SPECIALIZATION;
  }
  else {     
    xassert(instantiationOf);
    return TTK_INSTANTIATION;
  }
}


bool TemplateInfo::isPartialSpec() const
{
  return isSpecialization() &&
         hasParameters();
}

bool TemplateInfo::isCompleteSpec() const
{
  return isSpecialization() &&
         !hasParameters();
}


bool TemplateInfo::isCompleteSpecOrInstantiation() const
{
  return isNotPrimary() &&
         !hasParameters();
}


bool TemplateInfo::isInstOfPartialSpec() const
{
  return isInstantiation() &&
         instantiationOf->templateInfo()->isPartialSpec();
}


#if 0
StringRef TemplateInfo::getBaseName() const
{
  if (var && var->type->isCompoundType()) {
    return var->type->asCompoundType()->name;
  }
  else {
    return NULL;     // function template (or 'var' not set yet)
  }
}


void TemplateInfo::setMyPrimary(TemplateInfo *prim)
{
  xassert(prim);                // argument must be non-NULL
  xassert(prim->isPrimary());   // myPrimary can only be a primary
  xassert(!myPrimary);          // can only do this once
  xassert(isNotPrimary());      // can't set myPrimary of a primary
  myPrimary = prim;
}
#endif // 0


// this is idempotent
TemplateInfo const *TemplateInfo::getPrimaryC() const
{
  if (instantiationOf) {
    return instantiationOf->templateInfo()->getPrimaryC();
  }
  else if (specializationOf) {
    return specializationOf->templateInfo();     // always only one level
  }
  else {
    return this;
  }
}


void TemplateInfo::addToList(Variable *elt, SObjList<Variable> &children,
                             Variable * const &parentPtr)
{
  // the key to this routine is casting away the constness of
  // 'parentPtr'; it's the one routine entitled to do so
  Variable * &parent = const_cast<Variable*&>(parentPtr);

  // add to list, ensuring (if in debug mode) it isn't already there
  xassertdb(!children.contains(elt));
  children.append(elt);             // could use 'prepend' for performance..

  // backpointer, ensuring we don't overwrite one
  xassert(!parent);
  xassert(this->var);
  parent = this->var;
}

void TemplateInfo::addInstantiation(Variable *inst)
{
  addToList(inst, instantiations,
            inst->templateInfo()->instantiationOf);
}

void TemplateInfo::addSpecialization(Variable *inst)
{
  addToList(inst, specializations,
            inst->templateInfo()->specializationOf);
}

void TemplateInfo::addPartialInstantiation(Variable *pinst)
{
  addToList(pinst, partialInstantiations,
            pinst->templateInfo()->partialInstantiationOf);
}


ObjList<STemplateArgument> &TemplateInfo::getArgumentsToPrimary()
{
  if (isInstOfPartialSpec()) {
    return argumentsToPrimary;
  }
  else {
    return arguments;
  }
}


#if 0
Variable *TemplateInfo::addInstantiation(TypeFactory &tfac, Variable *inst0,
                                         bool suppressDup)
{
  xassert(inst0);
  xassert(inst0->templateInfo());
  inst0->templateInfo()->setMyPrimary(this);
  // check that we don't match something that is already on the list
  // FIX: I suppose we should turn this off since it makes it quadratic

  // FIX: could do this if we had an Env
//    xassert(!getInstThatMatchesArgs(env, inst->templateInfo()->arguments));

  bool inst0IsMutant = inst0->templateInfo()->isMutant();
  SFOREACH_OBJLIST_NC(Variable, getInstantiations(), iter) {
    Variable *inst1 = iter.data();
    MatchTypes match(tfac, MatchTypes::MM_ISO);
    bool unifies = match.match_Lists
      (inst1->templateInfo()->arguments,
       inst0->templateInfo()->arguments,
       2 /*matchDepth*/);
    if (unifies) {
      if (inst0IsMutant) {
        xassert(inst1->templateInfo()->isMutant());
        // Note: Never remove duplicate mutants; isomorphic mutants
        // are not interchangable.
      } else {
        xassert(!inst1->templateInfo()->isMutant());
        // other, non-mutant instantiations should never be duplicated
        //        xassert(!unifies);
        xassert(suppressDup);
        return inst1;
      }
    }
  }

  instantiations.append(inst0);
  return inst0;
}


SObjList<Variable> &TemplateInfo::getInstantiations()
{
  return instantiations;
}


Variable *TemplateInfo::getInstantiationOfVar(TypeFactory &tfac, Variable *var)
{
  xassert(var->templateInfo());
  ObjList<STemplateArgument> &varTArgs = var->templateInfo()->arguments;
  Variable *matchingVar = NULL;
  SFOREACH_OBJLIST_NC(Variable, getInstantiations(), iter) {
    Variable *candidate = iter.data();
    MatchTypes match(tfac, MatchTypes::MM_ISO);
    // FIX: I use MT_NONE only because the matching is supposed to be
    // exact.  If you wanted the standard effect of const/volatile not
    // making a difference at the top of a parameter type, you would
    // have to match each parameter separately
    if (!match.match_Type(var->type, candidate->type, 2 /*matchDepth*/)) continue;
    if (!match.match_Lists(varTArgs, candidate->templateInfo()->arguments, 2 /*matchDepth*/)) {
      continue;
    }
    xassert(!matchingVar);      // shouldn't be in there twice
    matchingVar = iter.data();
  }
  return matchingVar;
}
#endif // 0


bool equalArgumentLists(TypeFactory &tfac,
                        SObjList<STemplateArgument> const &list1,
                        SObjList<STemplateArgument> const &list2)
{
  SObjListIter<STemplateArgument> iter1(list1);
  SObjListIter<STemplateArgument> iter2(list2);

  while (!iter1.isDone() && !iter2.isDone()) {
    STemplateArgument const *sta1 = iter1.data();
    STemplateArgument const *sta2 = iter2.data();
    if (!sta1->isomorphic(tfac, sta2)) {
      return false;
    }

    iter1.adv();
    iter2.adv();
  }

  return iter1.isDone() && iter2.isDone();
}

bool TemplateInfo::equalArguments
  (TypeFactory &tfac, SObjList<STemplateArgument> const &list) const
{
  return equalArgumentLists(tfac, objToSObjListC(arguments), list);
}


bool TemplateInfo::argumentsContainTypeVariables() const
{
  FOREACH_OBJLIST(STemplateArgument, arguments, iter) {
    STemplateArgument const *sta = iter.data();
    if (sta->kind == STemplateArgument::STA_TYPE) {
      if (sta->value.t->containsTypeVariables()) return true;
    }
    // FIX: do other kinds
  }
  return false;
}


bool TemplateInfo::argumentsContainVariables() const
{
  FOREACH_OBJLIST(STemplateArgument, arguments, iter) {
    if (iter.data()->containsVariables()) return true;
  }
  return false;
}


bool TemplateInfo::hasParameters() const
{
  if (isPartialInstantiation()) {
    return true;
  }

  // check params attached directly to this object
  if (params.isNotEmpty()) {
    return true;
  }
  
  // check for inherited parameters
  FOREACH_OBJLIST(InheritedTemplateParams, inheritedParams, iter) {
    if (iter.data()->params.isNotEmpty()) {
      return true;
    }
  }
               
  // no parameters at any level
  return false;                
}


bool TemplateInfo::hasSpecificParameter(Variable const *v) const
{
  // 'params'?
  if (params.contains(v)) { return true; }
  
  // inherited?
  FOREACH_OBJLIST(InheritedTemplateParams, inheritedParams, iter) {
    if (iter.data()->params.contains(v)) { 
      return true; 
    }
  }

  return false;     // 'v' does not appear in any parameter list
}


void TemplateInfo::copyArguments(ObjList<STemplateArgument> const &sargs)
{
  copyTemplateArgs(arguments, objToSObjListC(sargs));
}

void TemplateInfo::copyArguments(SObjList<STemplateArgument> const &sargs)
{
  copyTemplateArgs(arguments, sargs);
}


string TemplateInfo::templateName() const
{
  if (isPrimary()) {
    return stringc << var->fullyQualifiedName()
                   << paramsLikeArgsToString();
  }

  if (isSpecialization()) {
    return stringc << var->fullyQualifiedName()
                   << sargsToString(arguments);
  }

  // instantiation; but of the primary or of a specialization?
  TemplateInfo *parentTI = instantiationOf->templateInfo();
  if (parentTI->isPrimary()) {
    return stringc << var->fullyQualifiedName()
                   << sargsToString(arguments);
  }
  else {
    // spec params + inst args, e.g. "A<T*><int>" to mean that
    // this is an instantiation of spec "A<T*>" with args "<int>",
    // i.e. original arguments "<int*>"
    return stringc << var->fullyQualifiedName()
                   << sargsToString(parentTI->arguments)
                   << sargsToString(arguments);
  }
}


void TemplateInfo::gdb()
{
  debugPrint(0);
}


void TemplateInfo::debugPrint(int depth, bool printPartialInsts)
{
  ind(cout, depth*2) << "TemplateInfo for "
                     << (var? var->name : "(null var)") << " {" << endl;

  depth++;

  if (isPartialInstantiation()) {
    // the template we are a partial instantiation of has all the
    // parameter info, so print it; but then *it* better not turn
    // around and print its partial instantiation list, otherwise we
    // get an infinite loop!  (discovered the hard way...)
    ind(cout, depth*2) << "partialInstantiatedFrom:\n";
    partialInstantiationOf->templateInfo()->
      debugPrint(depth+1, false /*printPartialInsts*/);
  }

  // inherited params
  FOREACH_OBJLIST(InheritedTemplateParams, inheritedParams, iter) {
    ind(cout, depth*2) << "inherited from " << iter.data()->enclosing->name
                       << ": " << iter.data()->paramsToCString() << endl;
  }

  // my params
  ind(cout, depth*2) << "params: " << paramsToCString() << endl;

  ind(cout, depth*2) << "arguments:" << endl;
  FOREACH_OBJLIST_NC(STemplateArgument, arguments, iter) {
    iter.data()->debugPrint(depth+1);
  }

  ind(cout, depth*2) << "instantiations:" << endl;
  depth++;
  SFOREACH_OBJLIST_NC(Variable, instantiations, iter) {
    Variable *var = iter.data();
    ind(cout, depth*2) << var->type->toString() << endl;
    var->templateInfo()->debugPrint(depth+1);
  }
  depth--;

  if (printPartialInsts) {
    ind(cout, depth*2) << "partial instantiations:" << endl;
    depth++;
    SFOREACH_OBJLIST_NC(Variable, partialInstantiations, iter) {
      Variable *var = iter.data();
      ind(cout, depth*2) << var->toString() << endl;
      var->templateInfo()->debugPrint(depth+1);
    }
    depth--;
  }

  depth--;

  ind(cout, depth*2) << "}" << endl;
}


// ------------------- STemplateArgument ---------------
STemplateArgument::STemplateArgument(STemplateArgument const &obj)
  : kind(obj.kind)
{
  if (kind == STA_TYPE) {
    value.t = obj.value.t;
  }
  else if (kind == STA_INT) {
    value.i = obj.value.i;
  }
  else {
    value.v = obj.value.v;
  }
}


STemplateArgument *STemplateArgument::shallowClone() const
{
  return new STemplateArgument(*this);
}


bool STemplateArgument::isObject() const
{
  switch (kind) {
  default:
    xfailure("illegal STemplateArgument kind"); break;

  case STA_TYPE:                // type argument
    return false; break;

  case STA_INT:                 // int or enum argument
  case STA_REFERENCE:           // reference to global object
  case STA_POINTER:             // pointer to global object
  case STA_MEMBER:              // pointer to class member
    return true; break;

  case STA_TEMPLATE:            // template argument (not implemented)
    return false; break;
  }
}
    

bool STemplateArgument::isDependent() const
{
  if (isType()) {
    // we don't (or shouldn't...) stack constructors on top of
    // ST_DEPENDENT, so just check at the top level
    //
    // 8/10/04: I had simply been calling Type::isDependent, but that
    // answers yes for type variables.  In the context in which I'm
    // calling this, I am only interested in '<dependent>'.  I realize
    // this is a bit of a non-orthogonality, but the fix isn't clear
    // at the moment.
    return getType()->isSimple(ST_DEPENDENT);
  }
  else {
    return false;      // what about non-type args?  template args?
  }
}


bool STemplateArgument::equals(STemplateArgument const *obj) const
{
  if (kind != obj->kind) {
    return false;
  }

  // At one point I tried making type arguments unequal if they had
  // different typedefs, but that does not work, because I need A<int>
  // to be the *same* type as A<my_int>, for example to do base class
  // constructor calls.

  switch (kind) {
    case STA_TYPE:     return value.t->equals(obj->value.t);
    case STA_INT:      return value.i == obj->value.i;
    default:           return value.v == obj->value.v;
  }
}


bool STemplateArgument::containsVariables() const
{
  if (kind == STemplateArgument::STA_TYPE) {
    if (value.t->containsVariables()) {
      return true;
    }
  } else if (kind == STemplateArgument::STA_REFERENCE) {
    // FIX: I am not at all sure that my interpretation of what
    // STemplateArgument::kind == STA_REFERENCE means; I think it
    // means it is a non-type non-template (object) variable in an
    // argument list
    return true;
  }
  // FIX: do other kinds
  return false;
}


bool STemplateArgument::isomorphic(TypeFactory &tfac, STemplateArgument const *obj) const
{
  if (kind != obj->kind) {
    return false;
  }

  switch (kind) {
    case STA_TYPE: {
      MatchTypes match(tfac, MatchTypes::MM_ISO);
      return match.match_Type(value.t, obj->value.t);
    }

    // TODO: these are wrong, because we don't have a proper way
    // to represent non-type template parameters in argument lists
    case STA_INT:      return value.i == obj->value.i;
    default:           return value.v == obj->value.v;
  }
}


string STemplateArgument::toString() const
{
  switch (kind) {
    default: xfailure("bad kind");
    case STA_NONE:      return string("STA_NONE");
    case STA_TYPE:      return value.t->toString();   // assume 'type' if no comment
    case STA_INT:       return stringc << "/*int*/ " << value.i;
    case STA_REFERENCE: return stringc << "/*ref*/ " << value.v->name;
    case STA_POINTER:   return stringc << "/*ptr*/ &" << value.v->name;
    case STA_MEMBER:    return stringc
      << "/*member*/ &" << value.v->scope->curCompound->name 
      << "::" << value.v->name;
    case STA_TEMPLATE:  return string("template (?)");
  }
}


void STemplateArgument::gdb()
{
  debugPrint(0);
}


void STemplateArgument::debugPrint(int depth)
{
  for (int i=0; i<depth; ++i) cout << "  ";
  cout << "STemplateArgument: " << toString() << endl;
}


SObjList<STemplateArgument> *cloneSArgs(SObjList<STemplateArgument> &sargs)
{
  SObjList<STemplateArgument> *ret = new SObjList<STemplateArgument>();
  SFOREACH_OBJLIST_NC(STemplateArgument, sargs, iter) {
    ret->append(iter.data());
  }
  return ret;
}


string sargsToString(SObjList<STemplateArgument> const &list)
{
  stringBuilder sb;
  sb << "<";

  int ct=0;
  SFOREACH_OBJLIST(STemplateArgument, list, iter) {
    if (ct++ > 0) {
      sb << ", ";
    }
    sb << iter.data()->toString();
  }

  sb << ">";
  return sb;
}


bool containsTypeVariables(SObjList<STemplateArgument> const &args)
{
  SFOREACH_OBJLIST(STemplateArgument, args, iter) {
    if (iter.data()->containsVariables()) {
      return true;
    }
  }
  return false;
}


bool hasDependentArgs(SObjList<STemplateArgument> const &args)
{
  SFOREACH_OBJLIST(STemplateArgument, args, iter) {
    if (iter.data()->isDependent()) {
      return true;
    }
  }
  return false;
}


// ---------------------- TemplCandidates ------------------------
STATICDEF
TemplCandidates::STemplateArgsCmp TemplCandidates::compareSTemplateArgs
  (TypeFactory &tfac, STemplateArgument const *larg, STemplateArgument const *rarg)
{
  xassert(larg->kind == rarg->kind);

  switch(larg->kind) {
  default:
    xfailure("illegal TemplateArgument kind");
    break;

  case STemplateArgument::STA_NONE: // not yet resolved into a valid template argument
    xfailure("STA_NONE TemplateArgument kind");
    break;

  case STemplateArgument::STA_TYPE: // type argument
    {
    // check if left is at least as specific as right
    bool leftAtLeastAsSpec;
    {
      MatchTypes match(tfac, MatchTypes::MM_WILD);
      if (match.match_Type(larg->value.t, rarg->value.t)) {
        leftAtLeastAsSpec = true;
      } else {
        leftAtLeastAsSpec = false;
      }
    }
    // check if right is at least as specific as left
    bool rightAtLeastAsSpec;
    {
      MatchTypes match(tfac, MatchTypes::MM_WILD);
      if (match.match_Type(rarg->value.t, larg->value.t)) {
        rightAtLeastAsSpec = true;
      } else {
        rightAtLeastAsSpec = false;
      }
    }

    // change of basis matrix
    if (leftAtLeastAsSpec) {
      if (rightAtLeastAsSpec) {
        return STAC_EQUAL;
      } else {
        return STAC_LEFT_MORE_SPEC;
      }
    } else {
      if (rightAtLeastAsSpec) {
        return STAC_RIGHT_MORE_SPEC;
      } else {
        return STAC_INCOMPARABLE;
      }
    }

    }
    break;

  case STemplateArgument::STA_INT: // int or enum argument
    if (larg->value.i == rarg->value.i) {
      return STAC_EQUAL;
    }
    return STAC_INCOMPARABLE;
    break;

  case STemplateArgument::STA_REFERENCE: // reference to global object
  case STemplateArgument::STA_POINTER: // pointer to global object
  case STemplateArgument::STA_MEMBER: // pointer to class member
    if (larg->value.v == rarg->value.v) {
      return STAC_EQUAL;
    }
    return STAC_INCOMPARABLE;
    break;

  case STemplateArgument::STA_TEMPLATE: // template argument (not implemented)
    xfailure("STA_TEMPLATE TemplateArgument kind; not implemented");
    break;
  }
}


STATICDEF int TemplCandidates::compareCandidatesStatic
  (TypeFactory &tfac, TemplateInfo const *lti, TemplateInfo const *rti)
{
  // I do not even put the primary into the set so it should never
  // show up.
  xassert(lti->isNotPrimary());
  xassert(rti->isNotPrimary());

  // they should have the same primary
  xassert(lti->getPrimaryC() == rti->getPrimaryC());

  // they should always have the same number of arguments; the number
  // of parameters is irrelevant
  xassert(lti->arguments.count() == rti->arguments.count());

  STemplateArgsCmp leaning = STAC_EQUAL;// which direction are we leaning?
  // for each argument pairwise
  ObjListIter<STemplateArgument> lIter(lti->arguments);
  ObjListIter<STemplateArgument> rIter(rti->arguments);
  for(;
      !lIter.isDone();
      lIter.adv(), rIter.adv()) {
    STemplateArgument const *larg = lIter.data();
    STemplateArgument const *rarg = rIter.data();
    STemplateArgsCmp cmp = compareSTemplateArgs(tfac, larg, rarg);
    switch(cmp) {
    default: xfailure("illegal STemplateArgsCmp"); break;
    case STAC_LEFT_MORE_SPEC:
      if (leaning == STAC_EQUAL) {
        leaning = STAC_LEFT_MORE_SPEC;
      } else if (leaning == STAC_RIGHT_MORE_SPEC) {
        leaning = STAC_INCOMPARABLE;
      }
      // left stays left and incomparable stays incomparable
      break;
    case STAC_RIGHT_MORE_SPEC:
      if (leaning == STAC_EQUAL) {
        leaning = STAC_RIGHT_MORE_SPEC;
      } else if (leaning == STAC_LEFT_MORE_SPEC) {
        leaning = STAC_INCOMPARABLE;
      }
      // right stays right and incomparable stays incomparable
      break;
    case STAC_EQUAL:
      // all stay same
      break;
    case STAC_INCOMPARABLE:
      leaning = STAC_INCOMPARABLE; // incomparable is an absorbing state
    }
  }
  xassert(rIter.isDone());      // we checked they had the same number of arguments earlier

  switch(leaning) {
  default: xfailure("illegal STemplateArgsCmp"); break;
  case STAC_LEFT_MORE_SPEC: return -1; break;
  case STAC_RIGHT_MORE_SPEC: return 1; break;
  case STAC_EQUAL:
    // FIX: perhaps this should be a user error?
    xfailure("Two template argument tuples are identical");
    break;
  case STAC_INCOMPARABLE: return 0; break;
  }
}


int TemplCandidates::compareCandidates(Variable const *left, Variable const *right)
{
  TemplateInfo *lti = const_cast<Variable*>(left)->templateInfo();
  xassert(lti);
  TemplateInfo *rti = const_cast<Variable*>(right)->templateInfo();
  xassert(rti);

  return compareCandidatesStatic(tfac, lti, rti);
}


// ----------------------- Env ----------------------------
// These are not all of Env's methods, just the ones declared in the
// section for templates.

Env::TemplTcheckMode Env::getTemplTcheckMode() const
{
  // one step towards removing this altogether...
  return tcheckMode;
  
  // ack!  the STA_REFERENCE hack depends on the mode...
  //return TTM_1NORMAL;
}


// FIX: this lookup doesn't do very well for overloaded function
// templates where one primary is more specific than the other; the
// more specific one matches both itself and the less specific one,
// and this gets called ambiguous
Variable *Env::lookupPQVariable_primary_resolve(
  PQName const *name, LookupFlags flags, FunctionType *signature,
  MatchTypes::MatchMode matchMode)
{
  // only makes sense in one context; will push this spec around later
  xassert(flags & LF_TEMPL_PRIMARY);

  // first, call the version that does *not* do overload selection
  Variable *var = lookupPQVariable(name, flags);

  if (!var) return NULL;
  OverloadSet *oloadSet = var->getOverloadSet();
  if (oloadSet->count() > 1) {
    xassert(var->type->isFunctionType()); // only makes sense for function types
    // FIX: Somehow I think this isn't right
    var = findTemplPrimaryForSignature(oloadSet, signature, matchMode);
    if (!var) {
      // FIX: sometimes I just want to know if there is one; not sure
      // this is always an error.
//        error("function template specialization does not match "
//              "any primary in the overload set");
      return NULL;
    }
  }
  return var;
}


Variable *Env::findTemplPrimaryForSignature
  (OverloadSet *oloadSet,
   FunctionType *signature,
   MatchTypes::MatchMode matchMode)
{
  if (!signature) {
    xfailure("This is one more place where you need to add a signature "
             "to the call to Env::lookupPQVariable() to deal with function "
             "template overload resolution");
    return NULL;
  }

  Variable *candidatePrim = NULL;
  SFOREACH_OBJLIST_NC(Variable, oloadSet->set, iter) {
    Variable *var0 = iter.data();
    // skip non-template members of the overload set
    if (!var0->isTemplate()) continue;

    TemplateInfo *tinfo = var0->templateInfo();
    xassert(tinfo);
    xassert(tinfo->isPrimary()); // can only have primaries at the top level

    // check that the function type could be a special case of the
    // template primary
    //
    // FIX: I don't know if this is really as precise a lookup as is
    // possible.
    MatchTypes match(tfac, matchMode);
    if (match.match_Type
        (signature,
         var0->type
         // FIX: I don't know if this should be top or not or if it
         // matters.
         )) {
      if (candidatePrim) {
        error("ambiguous attempt to lookup "
              "overloaded function template primary from specialization",
              EF_STRONG);
        return candidatePrim;            // error recovery
      } else {
        candidatePrim = var0;
      }
    }
  }
  return candidatePrim;
}


void Env::initArgumentsFromASTTemplArgs
  (TemplateInfo *tinfo,
   ASTList<TemplateArgument> const &templateArgs)
{
  xassert(tinfo);
  xassert(tinfo->arguments.count() == 0); // don't use this if there are already arguments
  FOREACH_ASTLIST(TemplateArgument, templateArgs, iter) {
    TemplateArgument const *targ = iter.data();
    xassert(targ->sarg.hasValue());
    tinfo->arguments.append(new STemplateArgument(targ->sarg));
  }
}


bool Env::checkIsoToASTTemplArgs
  (ObjList<STemplateArgument> &templateArgs0,
   ASTList<TemplateArgument> const &templateArgs1)
{
  MatchTypes match(tfac, MatchTypes::MM_ISO);
  ObjListIterNC<STemplateArgument> iter0(templateArgs0);
  FOREACH_ASTLIST(TemplateArgument, templateArgs1, iter1) {
    if (iter0.isDone()) return false;
    TemplateArgument const *targ = iter1.data();
    xassert(targ->sarg.hasValue());
    STemplateArgument sta(targ->sarg);
    if (!match.match_STA(iter0.data(), &sta, 2 /*matchDepth*/)) return false;
    iter0.adv();
  }
  return iter0.isDone();
}


#if 0    // borked
Variable *Env::getInstThatMatchesArgs
  (TemplateInfo *tinfo, SObjList<STemplateArgument> &arguments, Type *type0)
{
  Variable *prevInst = NULL;
  SFOREACH_OBJLIST_NC(Variable, tinfo->getInstantiations(), iter) {
    Variable *instantiation = iter.data();
    if (type0 && !instantiation->getType()->equals(type0)) continue;
    MatchTypes match(tfac, MatchTypes::MM_ISO);
    bool unifies = match.match_Lists
      (instantiation->templateInfo()->arguments, arguments, 2 /*matchDepth*/);
    if (unifies) {
      if (instantiation->templateInfo()->isMutant() &&
          prevInst->templateInfo()->isMutant()) {
        // if one mutant matches, more may, so just use the first one
        continue;
      }
      // any other combination of double matching is an error; that
      // is, I don't think you can get a non-mutant instance in more
      // than once
      xassert(!prevInst);
      prevInst = instantiation;
    }
  }
  return prevInst;
}
#endif // 0


bool Env::loadBindingsWithExplTemplArgs(Variable *var, ASTList<TemplateArgument> const &args,
                                        MatchTypes &match)
{
  xassert(var->templateInfo());
  xassert(var->templateInfo()->isPrimary());

  SObjListIterNC<Variable> paramIter(var->templateInfo()->params);
  ASTListIter<TemplateArgument> argIter(args);
  while (!paramIter.isDone() && !argIter.isDone()) {
    Variable *param = paramIter.data();

    // no bindings yet and param names unique
    //
    // FIX: maybe I shouldn't assume that param names are
    // unique; then this would be a user error
    if (param->type->isTypeVariable()) {
      xassert(!match.bindings.getTypeVar(param->type->asTypeVariable()));
    } else {
      // for, say, int template parameters
      xassert(!match.bindings.getObjVar(param));
    }

    TemplateArgument const *arg = argIter.data();

    // FIX: when it is possible to make a TA_template, add
    // check for it here.
    //              xassert("Template template parameters are not implemented");

    if (param->hasFlag(DF_TYPEDEF) && arg->isTA_type()) {
      STemplateArgument *bound = new STemplateArgument;
      bound->setType(arg->asTA_typeC()->type->getType());
      match.bindings.putTypeVar(param->type->asTypeVariable(), bound);
    }
    else if (!param->hasFlag(DF_TYPEDEF) && arg->isTA_nontype()) {
      STemplateArgument *bound = new STemplateArgument;
      Expression *expr = arg->asTA_nontypeC()->expr;
      if (expr->getType()->isIntegerType()) {
        int staticTimeValue;
        bool constEvalable = expr->constEval(*this, staticTimeValue);
        if (!constEvalable) {
          // error already added to the environment
          return false;
        }
        bound->setInt(staticTimeValue);
      } else if (expr->getType()->isReference()) {
        // Subject: Re: why does STemplateArgument hold Variables?
        // From: Scott McPeak <smcpeak@eecs.berkeley.edu>
        // To: "Daniel S. Wilkerson" <dsw@eecs.berkeley.edu>
        // The Variable is there because STA_REFERENCE (etc.)
        // refer to (linker-visible) symbols.  You shouldn't
        // be making an STemplateArgument out of an expression
        // directly; if the template parameter is
        // STA_REFERENCE (etc.) then dig down to find the
        // Variable the programmer wants to use.
        //
        // haven't tried to exhaustively enumerate what kinds
        // of expressions this could be; handle other types as
        // they come up
        xassert(expr->isE_variable());
        bound->setReference(expr->asE_variable()->var);
      } else {
        unimp("unhandled kind of template argument");
        return false;
      }
      match.bindings.putObjVar(param, bound);
    }
    else {
      // mismatch between argument kind and parameter kind
      char const *paramKind = param->hasFlag(DF_TYPEDEF)? "type" : "non-type";
      char const *argKind = arg->isTA_type()? "type" : "non-type";
      // FIX: make a provision for template template parameters here

      // NOTE: condition this upon a boolean flag reportErrors if it
      // starts to fail while filtering functions for overload
      // resolution; see Env::inferTemplArgsFromFuncArgs() for an
      // example
      error(stringc
            << "`" << param->name << "' is a " << paramKind
            << " parameter, but `" << arg->argString() << "' is a "
            << argKind << " argument",
            EF_STRONG);
      return false;
    }
    paramIter.adv();
    argIter.adv();
  }
  return true;
}


bool Env::inferTemplArgsFromFuncArgs
  (Variable *var,
   TypeListIter &argListIter,
   MatchTypes &match,
   bool reportErrors)
{
  xassert(var->templateInfo());
  xassert(var->templateInfo()->isPrimary());
  xassert(match.hasEFlag(Type::EF_DEDUCTION));

  TRACE("template", "deducing template arguments from function arguments");

  // TODO: make this work for static member template functions: the
  // caller has passed in information about the receiver object (so
  // this function can work for nonstatic members) but now we need to
  // skip the receiver if the function in question is static
  //
  // The hard part about this is we can't easily tell whether the
  // caller actually passed a receiver object...

  // FIX: make this work for vararg functions
  int i = 1;            // for error messages
  SFOREACH_OBJLIST_NC(Variable, var->type->asFunctionType()->params, paramIter) {
    Variable *param = paramIter.data();
    xassert(param);
    // we could run out of args and it would be ok as long as we
    // have default arguments for the rest of the parameters
    if (!argListIter.isDone()) {
      Type *curArgType = argListIter.data();
      bool argUnifies = match.match_Type(curArgType, param->type);
      if (!argUnifies) {
        if (reportErrors) {
          error(stringc << "during function template argument deduction: "
                << "argument " << i << " `" << curArgType->toString() << "'"
                << " is incompatable with parameter, `"
                << param->type->toString() << "'");
        }
        return false;             // FIX: return a variable of type error?
      }
    } else {
      // we don't use the default arugments for template arugment
      // inference, but there should be one anyway.
      if (!param->value) {
        if (reportErrors) {
          error(stringc << "during function template argument deduction: too few arguments to " <<
                var->name);
        }
        return false;
      }
    }
    ++i;
    // advance the argIterCur if there is one
    if (!argListIter.isDone()) argListIter.adv();
  }
  if (!argListIter.isDone()) {
    if (reportErrors) {
      error(stringc << "during function template argument deduction: too many arguments to " <<
            var->name);
    }
    return false;
  }
  return true;
}


bool Env::getFuncTemplArgs
  (MatchTypes &match,
   ObjList<STemplateArgument> &sargs,
   PQName const *final,
   Variable *var,
   TypeListIter &argListIter,
   bool reportErrors)
{
  TemplateInfo *varTI = var->templateInfo();
  xassert(varTI->isPrimary());

  // 'final' might be NULL in the case of doing overload resolution
  // for templatized ctors (that is, the ctor is templatized, but not
  // necessarily the class)
  if (final && final->isPQ_template()) {
    if (!loadBindingsWithExplTemplArgs(var, final->asPQ_templateC()->args, match)) {
      return false;
    }
  }

  if (!inferTemplArgsFromFuncArgs(var, argListIter, match, reportErrors)) {
    return false;
  }

  #if 0     // wrong place for this
  // a partial instantiation provides some of the template arguments
  // on its own, before matching provides the rest
  ObjListIter<STemplateArgument> piArgIter(varTI->arguments);
  if (varTI->isPartialInstantiation()) {
    // and the parameters are found in the original
    varTI = varTI->partialInstantiationOf->templateInfo();
  }
  #endif // 0

  // put the bindings in a list in the right order
  bool haveAllArgs = true;

  // inherited params first
  FOREACH_OBJLIST(InheritedTemplateParams, varTI->inheritedParams, iter) {
    getFuncTemplArgs_oneParamList(match, sargs, reportErrors, haveAllArgs,
                                  /*piArgIter,*/ iter.data()->params);
  }

  // then main params
  getFuncTemplArgs_oneParamList(match, sargs, reportErrors, haveAllArgs,
                                /*piArgIter,*/ varTI->params);

  // certainly the partial instantiation should not have provided
  // more arguments than there are parameters!  it should not even
  // provide as many, but that's slightly harder to check
  //xassert(piArgIter.isDone());

  return haveAllArgs;
}

void Env::getFuncTemplArgs_oneParamList
  (MatchTypes &match,
   ObjList<STemplateArgument> &sargs,
   bool reportErrors,
   bool &haveAllArgs,
   //ObjListIter<STemplateArgument> &piArgIter,
   SObjList<Variable> const &paramList)
{
  SFOREACH_OBJLIST(Variable, paramList, templPIter) {
    Variable const *param = templPIter.data();

    #if 0     // wrong place
    if (!piArgIter.isDone()) {
      // the partial instantiation provides this argument
      sargs.append(piArgIter.data()->shallowClone());
      TRACE("template", "partial inst provided arg " << piArgIter.data()->toString() <<
                        " for param " << param->name);
      piArgIter.adv();
      continue;
    }
    #endif // 0

    STemplateArgument const *sta = NULL;
    if (param->type->isTypeVariable()) {
      sta = match.bindings.getTypeVar(param->type->asTypeVariable());
    }
    else {
      sta = match.bindings.getObjVar(param);
    }

    if (!sta) {
      if (reportErrors) {
        error(stringc << "No argument for parameter `" << templPIter.data()->name << "'");
      }
      haveAllArgs = false;
    }
    else {
      // the 'sta' we have is owned by 'match' and will go away when
      // it does; make a copy that 'sargs' can own
      sargs.append(new STemplateArgument(*sta));
    }
  }
}


// lookup with template argument inference based on argument expressions
Variable *Env::lookupPQVariable_function_with_args(
  PQName const *name, LookupFlags flags,
  FakeList<ArgExpression> *funcArgs)
{
  // first, lookup the name but return just the template primary
  Scope *scope;      // scope where found
  Variable *var = lookupPQVariable(name, flags | LF_TEMPL_PRIMARY, scope);

  // apply arguments
  return lookupPQVariable_applyArgs(scope, var, name, funcArgs);
}

// TODO: remove 'foundScope'
Variable *Env::lookupPQVariable_applyArgs(
  Scope *foundScope, Variable *primary, PQName const *name,
  FakeList<ArgExpression> *funcArgs)
{
  if (!primary || !primary->isTemplate()) {
    // nothing unusual to do
    return primary;
  }

  if (!primary->isTemplateFunction()) {
    // most of the time this can't happen, b/c the func/ctor ambiguity
    // resolution has already happened and inner2 is only called when
    // the 'primary' looks ok; but if there are unusual ambiguities, e.g.
    // in/t0171.cc, then we get here even after 'primary' has yielded an
    // error.. so just bail, knowing that an error has already been
    // reported (hence the ambiguity will be resolved as something else)
    return NULL;
  }

  // if we are inside a function definition, just return the primary
  TemplTcheckMode ttm = getTemplTcheckMode();
  // FIX: this can happen when in the parameter list of a function
  // template definition a class template is instantiated (as a
  // Mutant)
  // UPDATE: other changes should prevent this
  xassert(ttm != TTM_2TEMPL_FUNC_DECL);
  if (ttm == TTM_2TEMPL_FUNC_DECL || ttm == TTM_3TEMPL_DEF) {
    xassert(primary->templateInfo()->isPrimary());
    return primary;
  }

  PQName const *final = name->getUnqualifiedNameC();

  // duck overloading
  OverloadSet *oloadSet = primary->getOverloadSet();
  if (oloadSet->count() > 1) {
    xassert(primary->type->isFunctionType()); // only makes sense for function types
    // FIX: the correctness of this depends on someone doing
    // overload resolution later, which I don't think is being
    // done.
    return primary;
    // FIX: this isn't right; do overload resolution later;
    // otherwise you get a null signature being passed down
    // here during E_variable::itcheck_x()
    //              primary = oloadSet->findTemplPrimaryForSignature(signature);
    // // FIX: make this a user error, or delete it
    // xassert(primary);
  }
  xassert(primary->templateInfo()->isPrimary());

  // get the semantic template arguments
  ObjList<STemplateArgument> sargs;
  {
    TypeListIter_FakeList argListIter(funcArgs);
    MatchTypes match(tfac, MatchTypes::MM_BIND, Type::EF_DEDUCTION);
    if (!getFuncTemplArgs(match, sargs, final, primary, 
                          argListIter, true /*reportErrors*/)) {
      return NULL;
    }
  }

  // apply the template arguments to yield a new type based on
  // the template; note that even if we had some explicit
  // template arguments, we have put them into the bindings now,
  // so we omit them here
  return instantiateFunctionTemplate(loc(), primary, sargs);
}


#if 0
static bool doesUnificationRequireBindings
  (TypeFactory &tfac,
   SObjList<STemplateArgument> &sargs,
   ObjList<STemplateArgument> &arguments)
{
  // re-unify and check that no bindings get added
  MatchTypes match(tfac, MatchTypes::MM_BIND);
  bool unifies = match.match_Lists(sargs, arguments, 2 /*matchDepth*/);
  xassert(unifies);             // should of course still unify
  // bindings should be trivial for a complete specialization
  // or instantiation
  return !match.bindings.isEmpty();
}    
#endif // 0


// insert bindings into SK_TEMPLATE_ARG scopes, from template
// parameters to concrete template arguments
void Env::insertTemplateArgBindings
  (Variable *baseV, SObjList<STemplateArgument> &sargs)
{
  xassert(baseV);
  TemplateInfo *baseVTI = baseV->templateInfo();

  // begin iterating over arguments
  SObjListIterNC<STemplateArgument> argIter(sargs);

  // if 'baseV' is a partial instantiation, then it provides
  // a block of arguments at the beginning, and then we use 'sargs'
  SObjList<STemplateArgument> expandedSargs;
  if (baseVTI->isPartialInstantiation()) {
    // copy partial inst args first
    FOREACH_OBJLIST_NC(STemplateArgument, baseVTI->arguments, iter) {
      expandedSargs.append(iter.data());
    }

    // then 'sargs'
    SFOREACH_OBJLIST_NC(STemplateArgument, sargs, iter2) {
      expandedSargs.append(iter2.data());
    }
    
    // now, reset the iterator to walk the expanded list instead
    argIter.reset(expandedSargs);
    
    // finally, set 'baseVTI' to point at the original template,
    // since it has the parameter list for the definition
    baseVTI = baseVTI->partialInstantiationOf->templateInfo();
  }

  // does the definition parameter list differ from the declaration
  // parameter list?
  if (baseVTI->definitionTemplateInfo) {
    // use the params from the definition instead
    baseVTI = baseVTI->definitionTemplateInfo;
  }

  // first, apply them to the inherited parameters
  FOREACH_OBJLIST(InheritedTemplateParams, baseVTI->inheritedParams, iter) {
    // mark the current template argument scope as associated with
    // these inherited params' template
    Scope *mine = scope();
    xassert(mine->isTemplateArgScope());
    mine->setParameterizedPrimary(iter.data()->enclosing->typedefVar);

    if (!insertTemplateArgBindings_oneParamList(baseV, argIter, iter.data()->params)) {
      // error already reported
      return;
    }
        
    // each inherited set of args goes into its own template argument
    // scope, so make another one now to hold the next set
    //
    // TODO: probably scope creating and binding insertion should be
    // closer together
    enterScope(SK_TEMPLATE_ARGS, "template argument bindings after inherited");
  }

  // then, apply to "my" parameters
  insertTemplateArgBindings_oneParamList(baseV, argIter, baseVTI->params);

  if (!argIter.isDone()) {
    error(stringc
          << "too many template arguments to `" << baseV->name << "'", EF_STRONG);
  }
}

// returns false if an error is detected
bool Env::insertTemplateArgBindings_oneParamList
  (Variable *baseV, SObjListIterNC<STemplateArgument> &argIter,
   SObjList<Variable> const &params)
{
  SObjListIter<Variable> paramIter(params);
  while (!paramIter.isDone()) {
    Variable const *param = paramIter.data();

    // if we have exhaused the explicit arguments, use a NULL 'sarg'
    // to indicate that we need to use the default arguments from
    // 'param' (if available)
    //
    // 8/10/04: Default arguments are now handled elsewhere
    // TODO: fully collapse this code to reflect that
    xassert(!argIter.isDone());       // should not get here with too few args
    STemplateArgument *sarg = /*argIter.isDone()? NULL :*/ argIter.data();

    if (sarg && sarg->isTemplate()) {
      xassert("Template template parameters are not implemented");
    }


    if (param->hasFlag(DF_TYPEDEF) &&
        (!sarg || sarg->isType())) {
      if (!sarg && !param->defaultParamType) {
        error(stringc
          << "too few template arguments to `" << baseV->name << "'");
        return false;
      }

      // bind the type parameter to the type argument
      Type *t = sarg? sarg->getType() : param->defaultParamType;
      Variable *binding = makeVariable(param->loc, param->name,
                                       t, DF_TYPEDEF | DF_BOUND_TEMPL_VAR);
      addVariable(binding);
    }
    else if (!param->hasFlag(DF_TYPEDEF) &&
             (!sarg || sarg->isObject())) {
      if (!sarg && !param->value) {
        error(stringc
          << "too few template arguments to `" << baseV->name << "'");
        return false;
      }

      // TODO: verify that the argument in fact matches the parameter type

      // bind the nontype parameter directly to the nontype expression;
      // this will suffice for checking the template body, because I
      // can const-eval the expression whenever it participates in
      // type determination; the type must be made 'const' so that
      // E_variable::constEval will believe it can evaluate it
      Type *bindType = tfac.applyCVToType(param->loc, CV_CONST,
                                          param->type, NULL /*syntax*/);
      Variable *binding = makeVariable(param->loc, param->name,
                                       bindType, DF_BOUND_TEMPL_VAR);

      // set 'bindings->value', in some cases creating AST
      if (!sarg) {
        binding->value = param->value;

        // sm: the statement above seems reasonable, but I'm not at
        // all convinced it's really right... has it been tcheck'd?
        // has it been normalized?  are these things necessary?  so
        // I'll wait for a testcase to remove this assertion... before
        // this assertion *is* removed, someone should read over the
        // applicable parts of cppstd
        xfailure("unimplemented: default non-type argument");
      }
      else if (sarg->kind == STemplateArgument::STA_INT) {
        E_intLit *value0 = buildIntegerLiteralExp(sarg->getInt());
        // FIX: I'm sure we can do better than SL_UNKNOWN here
        value0->type = tfac.getSimpleType(SL_UNKNOWN, ST_INT, CV_CONST);
        binding->value = value0;
      }
      else if (sarg->kind == STemplateArgument::STA_REFERENCE) {
        E_variable *value0 = new E_variable(NULL /*PQName*/);
        value0->var = sarg->getReference();
        binding->value = value0;
      }
      else {
        unimp(stringc << "STemplateArgument objects that are of kind other than "
              "STA_INT are not implemented: " << sarg->toString());
        return false;
      }
      xassert(binding->value);
      addVariable(binding);
    }
    else {
      // mismatch between argument kind and parameter kind
      char const *paramKind = param->hasFlag(DF_TYPEDEF)? "type" : "non-type";
      // FIX: make a provision for template template parameters here
      char const *argKind = sarg->isType()? "type" : "non-type";
      error(stringc
            << "`" << param->name << "' is a " << paramKind
            << " parameter, but `" << sarg->toString() << "' is a "
            << argKind << " argument", EF_STRONG);
    }

    paramIter.adv();
    if (!argIter.isDone()) {
      argIter.adv();
    }
  }

  // having added the bindings, turn off name acceptance
  scope()->canAcceptNames = false;

  xassert(paramIter.isDone());
  return true;
}

void Env::insertTemplateArgBindings
  (Variable *baseV, ObjList<STemplateArgument> &sargs)
{
  insertTemplateArgBindings(baseV, objToSObjList(sargs));
}


// The situation here is we have a partial specialization, for
// example
//
//   template <class T, class U>
//   class A<int, T*, U**> { ... }
//
// and we'd like to instantiate it with some concrete arguments.  What
// we have is arguments that apply to the *primary*, for example
//
//   <int, float*, char***>
//
// and we want to derive the proper arguments to the partial spec,
// namely
//
//   <float, char*>
//
// so we can pass these on to the instantiation routines.  
//
// It's a bit odd to be doing this matching again, since to even
// discover that the partial spec applied we would have already done
// it once.  For now I'll let that be...
void Env::mapPrimaryArgsToSpecArgs(
  Variable *baseV,         // partial specialization
  MatchTypes &match,       // carries 'bindings', which will own new arguments
  SObjList<STemplateArgument> &partialSpecArgs,      // dest. list
  SObjList<STemplateArgument> const &primaryArgs)    // source list
{
  // though the caller created this, it is really this function that
  // is responsible for deciding what arguments are used; so make sure
  // the caller did the right thing
  xassert(match.getMode() == MatchTypes::MM_BIND);

  // TODO: add 'const' to matchtypes
  SObjList<STemplateArgument> &hackPrimaryArgs =
    const_cast<SObjList<STemplateArgument>&>(primaryArgs);

  // execute the match to derive the bindings; we should not have
  // gotten here if they do not unify (Q: why the matchDepth==2?)
  TemplateInfo *baseVTI = baseV->templateInfo();
  bool matches = match.match_Lists(hackPrimaryArgs, baseVTI->arguments, 2 /*matchDepth*/);
  xassert(matches);

  // Now the arguments are bound in 'bindings', for example
  //
  //   T |-> float
  //   U |-> char
  //
  // We just need to run over the partial spec's parameters and
  // build an argument corresponding to each parameter.

  // first get args corresp. to inherited params
  FOREACH_OBJLIST(InheritedTemplateParams, baseVTI->inheritedParams, iter) {
    mapPrimaryArgsToSpecArgs_oneParamList(iter.data()->params, match, partialSpecArgs);
  }

  // then the main params
  mapPrimaryArgsToSpecArgs_oneParamList(baseVTI->params, match, partialSpecArgs);
}

void Env::mapPrimaryArgsToSpecArgs_oneParamList(
  SObjList<Variable> const &params,     // one arg per parameter
  MatchTypes &match,                    // carries bindingsto use
  SObjList<STemplateArgument> &partialSpecArgs)      // dest. list
{
  SFOREACH_OBJLIST(Variable, params, iter) {
    Variable const *param = iter.data();

    STemplateArgument const *arg = NULL;
    if (param->type->isTypeVariable()) {
      arg = match.bindings.getTypeVar(param->type->asTypeVariable());
    }
    else {
      arg = match.bindings.getObjVar(param);
    }
    if (!arg) {
      error(stringc
            << "during partial specialization parameter `" << param->name
            << "' not bound in inferred bindings", EF_STRONG);
      return;
    }

    // Cast away constness... the reason it's const to begin with is
    // that 'bindings' doesn't want me to change it, and as a reminder
    // that 'bindings' owns it so it will go away when 'bindings'
    // does.  Since passing 'arg' to 'insertTemplateArgBindings' will
    // not change it, and since I understand the lifetime
    // relationships, this should be safe.
    partialSpecArgs.append(const_cast<STemplateArgument*>(arg));
  }
}


#if 0
void Env::insertBindings(Variable *baseV, SObjList<STemplateArgument> &sargs)
{
  if (baseV->templateInfo()->isPartialSpec()) {
    // unify again to compute the bindings again since we forgot
    // them already
    MatchTypes match(tfac, MatchTypes::MM_BIND);
    SObjList<STemplateArgument> partialSpecArgs;   // pointers into 'match.bindings'

    // map the arguments to what the primary can use
    mapPrimaryArgsToSpecArgs(baseV, match, partialSpecArgs, sargs);

    // use them
    insertTemplateArgBindings(baseV, partialSpecArgs);
  }
  else {
    xassert(baseV->templateInfo()->isPrimary());
    insertTemplateArgBindings(baseV, sargs);
  }

  // having inserted these bindings, turn off name acceptance to simulate
  // the behavior of TemplateDeclaration::tcheck
  Scope *s = scope();
  xassert(s->scopeKind == SK_TEMPLATE_ARGS);
  s->canAcceptNames = false;
}

void Env::insertBindings(Variable *baseV, ObjList<STemplateArgument> &sargs)
{
  // little hack...
  insertBindings(baseV, objToSObjList(sargs));
}
#endif // 0


// go over the list of arguments, and make a list of semantic
// arguments
void Env::templArgsASTtoSTA
  (ASTList<TemplateArgument> const &arguments,
   SObjList<STemplateArgument> &sargs)
{
  FOREACH_ASTLIST(TemplateArgument, arguments, iter) {
    // the caller wants me not to modify the list, and I am not going
    // to, but I need to put non-const pointers into the 'sargs' list
    // since that's what the interface to 'equalArguments' expects..
    // this is a problem with const-polymorphism again
    TemplateArgument *ta = const_cast<TemplateArgument*>(iter.data());
    if (!ta->sarg.hasValue()) {
      // sm: 7/24/04: This used to be a user error, but I think it should
      // be an assertion because it is referring to internal implementation
      // details, not anything the user knows about.
      xfailure(stringc << "TemplateArgument has no value " << ta->argString());
    }
    sargs.prepend(&(ta->sarg));     // O(1)
  }
  sargs.reverse();                  // O(n)
}


#if 0
Variable *Env::instantiateTemplate_astArgs
  (SourceLoc loc, Scope *foundScope,
   Variable *baseV, Variable *instV,
   ASTList<TemplateArgument> const &astArgs)
{
  SObjList<STemplateArgument> sargs;
  templArgsASTtoSTA(astArgs, sargs);
  return instantiateTemplate(loc, foundScope, baseV, instV, NULL /*bestV*/, sargs);
}    
#endif // 0


// TODO: can this be removed?  what goes wrong if we use MM_BIND
// always?
MatchTypes::MatchMode Env::mapTcheckModeToTypeMatchMode(TemplTcheckMode tcheckMode)
{
  // map the typechecking mode to the type matching mode
  MatchTypes::MatchMode matchMode = MatchTypes::MM_NONE;
  switch(tcheckMode) {
  default: xfailure("bad mode"); break;
  case TTM_1NORMAL:
    matchMode = MatchTypes::MM_BIND;
    break;
  case TTM_2TEMPL_FUNC_DECL:
    matchMode = MatchTypes::MM_ISO;
    break;
  case TTM_3TEMPL_DEF:
    // 8/09/04: we used to not instantiate anything in mode 3 (template
    // definition), but in the new design I see no harm in allowing it;
    // I think we should use bind mode
    matchMode = MatchTypes::MM_BIND;
    break;
  }
  return matchMode;
}


// find most specific specialization that matches the given arguments
Variable *Env::findMostSpecific
  (Variable *baseV, SObjList<STemplateArgument> const &sargs)
{
  // baseV should be a template primary
  TemplateInfo *baseVTI = baseV->templateInfo();
  xassert(baseVTI->isPrimary());

  // ?
  MatchTypes::MatchMode matchMode =
    mapTcheckModeToTypeMatchMode(getTemplTcheckMode());

  // iterate through all of the specializations and build up a set of
  // candidates
  TemplCandidates templCandidates(tfac);
  SFOREACH_OBJLIST_NC(Variable, baseVTI->specializations, iter) {
    Variable *spec = iter.data();
    TemplateInfo *specTI = spec->templateInfo();
    xassert(specTI);        // should have templateness

    // TODO: add 'const' to matchtype
    SObjList<STemplateArgument> &hackSargs =
      const_cast<SObjList<STemplateArgument>&>(sargs);

    // see if this candidate matches
    MatchTypes match(tfac, matchMode);
    if (match.match_Lists(hackSargs, specTI->arguments, 2 /*matchDepth (?)*/)) {
      templCandidates.add(spec);
    }
  }

  // there are no candidates so we just use the primary
  if (templCandidates.candidates.isEmpty()) {
    return baseV;
  }

  // there are candidates to try; select the best
  Variable *bestV = selectBestCandidate_templCompoundType(templCandidates);

  // if there is not best candidate, then the call is ambiguous and
  // we should deal with that error
  if (!bestV) {
    // TODO: expand this error message
    error(stringc << "ambiguous attempt to instantiate template", EF_STRONG);
    return baseV;      // recovery: use the primary
  }

  // otherwise, use the best one
  return bestV;
}


// remove scopes from the environment until the innermost
// scope on the scope stack is the same one that the template
// definition appeared in; template definitions can see names
// visible from their defining scope only [cppstd 14.6 para 1]
//
// update: (e.g. t0188.cc) pop scopes until we reach one that
// *contains* (or equals) the defining scope
//
// 4/20/04: Even more (e.g. t0204.cc), we need to push scopes
// to dig back down to the defining scope.  So the picture now
// looks like this:
//
//       global                   /---- this is "foundScope"
//         |                     /
//         namespace1           /   }
//         | |                 /    }- 2. push these: "pushedScopes"
//         | namespace11  <---/     }
//         |   |
//         |   template definition
//         |
//         namespace2               }
//           |                      }- 1. pop these: "poppedScopes"
//           namespace21            }
//             |
//             point of instantiation
//
// actually, it's *still* not right, because
//   - this allows the instantiation to see names declared in
//     'namespace11' that are below the template definition, and
//   - it's entirely wrong for dependent names, a concept I
//     largely ignore at this time
// however, I await more examples before continuing to refine
// my approximation to the extremely complex lookup rules
//
// makeEatScope: first step towards removing SK_EAT_TEMPL_SCOPE
// altogether
Scope *Env::prepArgScopeForTemlCloneTcheck
  (ObjList<Scope> &poppedScopes, SObjList<Scope> &pushedScopes, Scope *foundScope,
   bool makeEatScope)
{
  xassert(foundScope);
  // pop scope scopes
  while (!scopes.first()->enclosesOrEq(foundScope)) {
    poppedScopes.prepend(scopes.removeFirst());
    if (scopes.isEmpty()) {
      xfailure("emptied scope stack searching for defining scope");
    }
  }

  // make a list of the scopes to push; these form a path from our
  // current scope to the 'foundScope'
  Scope *s = foundScope;
  while (s != scopes.first()) {
    pushedScopes.prepend(s);
    s = s->parentScope;
    if (!s) {
      if (scopes.first()->isGlobalScope()) {
        // ok, hit the global scope in the traversal
        break;
      }
      else {
        xfailure("missed the current scope while searching up "
                 "from the defining scope");
      }
    }
  }

  // now push them in list order, which means that 'foundScope'
  // will be the last one to be pushed, and hence the innermost
  // (I waited until now b/c this is the opposite order from the
  // loop above that fills in 'pushedScopes')
  SFOREACH_OBJLIST_NC(Scope, pushedScopes, iter) {
    // Scope 'iter.data()' is now on both lists, but neither owns
    // it; 'scopes' does not own Scopes that are named, as explained
    // in the comments near its declaration (cc_env.h)
    scopes.prepend(iter.data());
  }

  if (makeEatScope) {
    // the variable created by tchecking the instantiation would
    // normally be inserted into the current scope, but instantiations
    // are found in their own list, not via environment lookup; so
    // this scope will just catch the instantiation so we can discard it
    enterScope(SK_EAT_TEMPL_INST, "dummy scope to eat the template instantiation");
  }

  // make a new scope for the template arguments
  Scope *argScope = enterScope(SK_TEMPLATE_ARGS, "template argument bindings");

  return argScope;
}


void Env::unPrepArgScopeForTemlCloneTcheck
  (Scope *argScope, ObjList<Scope> &poppedScopes, SObjList<Scope> &pushedScopes,
   bool makeEatScope)
{
  // bit of a hack: we might have put an unknown number of inherited
  // parameter argument scopes on after 'argScope'...
  while (scope() != argScope) {
    Scope *s = scope();
    xassert(s->isTemplateArgScope());
    exitScope(s);
  }

  // remove the argument scope
  exitScope(argScope);

  if (makeEatScope) {
    // pull off the dummy, and check it's what we expect
    Scope *dummy = scopes.first();
    xassert(dummy->scopeKind == SK_EAT_TEMPL_INST);
    exitScope(dummy);
  }

  // restore the original scope structure
  pushedScopes.reverse();
  while (pushedScopes.isNotEmpty()) {
    // make sure the ones we're removing are the ones we added
    xassert(scopes.first() == pushedScopes.first());
    scopes.removeFirst();
    pushedScopes.removeFirst();
  }

  // re-add the inner scopes removed above
  while (poppedScopes.isNotEmpty()) {
    scopes.prepend(poppedScopes.removeFirst());
  }
}


// Get or create an instantiation Variable for a class template.
// Note that this does *not* instantiate the class body; instead,
// instantiateClassBody() has that responsibility.
//
// Return NULL if there is a problem with the arguments.
Variable *Env::instantiateClassTemplate
  (SourceLoc loc,                             // location of instantiation request
   Variable *primary,                         // template primary to instantiate
   SObjList<STemplateArgument> const &origPrimaryArgs)  // arguments to apply to 'primary'
{
  // I really don't know what's the best place to do this, but I
  // need it here so this is a start...
  primary = primary->skipAlias();

  TemplateInfo *primaryTI = primary->templateInfo();
  xassert(primaryTI->isPrimary());

  // Augment the supplied arguments with defaults from the primary
  // (note that defaults on a specialization are not used, and are
  // consequently illegal [14.5.4 para 10]).
  //
  // This also checks whether the arguments match the parameters,
  // and returns false if they do not.
  ObjList<STemplateArgument> owningPrimaryArgs;
  if (!supplyDefaultTemplateArguments(primaryTI, owningPrimaryArgs,
                                      origPrimaryArgs)) {
    return NULL;
  }

  // The code below wants to use SObjLists, and since they are happy
  // accepting const versions, this is safe.  An alternative fix would
  // be to push owningness down into those interfaces, but I'm getting
  // tired of doing that ...
  SObjList<STemplateArgument> const &primaryArgs =     // non-owning
    objToSObjListC(owningPrimaryArgs);

  // find the specialization that should be used (might turn
  // out to be the primary; that's fine)
  Variable *spec = findMostSpecific(primary, primaryArgs);
  TemplateInfo *specTI = spec->templateInfo();
  if (specTI->isCompleteSpec()) {
    return spec;      // complete spec; good to go
  }

  // if this is a partial specialization, we need the arguments
  // to be relative to the partial spec before we can look for
  // the instantiation
  MatchTypes match(tfac, MatchTypes::MM_BIND);
  SObjList<STemplateArgument> partialSpecArgs;
  if (spec != primary) {
    xassertdb(specTI->isPartialSpec());
    mapPrimaryArgsToSpecArgs(spec, match, partialSpecArgs, primaryArgs);
  }

  // look for an existing instantiation that has the right arguments
  Variable *inst = spec==primary?
    findInstantiation(specTI, primaryArgs) :
    findInstantiation(specTI, partialSpecArgs);
  if (inst) {
    return inst;      // found it; that's all we need
  }

  // since we didn't find an existing instantiation, we have to make
  // one from scratch
  if (spec==primary) {
    TRACE("template", "instantiating class decl: " <<
                      primary->fullyQualifiedName() << sargsToString(primaryArgs));
  }
  else {
    TRACE("template", "instantiating partial spec decl: " <<
                      primary->fullyQualifiedName() <<
                      sargsToString(specTI->arguments) <<
                      sargsToString(partialSpecArgs));
  }

  // create the CompoundType
  CompoundType const *specCT = spec->type->asCompoundType();
  CompoundType *instCT = tfac.makeCompoundType(specCT->keyword, specCT->name);
  instCT->forward = true;
  instCT->instName = str(stringc << specCT->name << sargsToString(primaryArgs));
  instCT->parentScope = specCT->parentScope;

  // wrap the compound in a regular type
  Type *instType = makeType(loc, instCT);

  // create the representative Variable
  inst = makeInstantiationVariable(spec, instType);

  // this also functions as the implicit typedef for the class,
  // though it is not entered into any scope
  instCT->typedefVar = inst;

  // also make the self-name, which *does* go into the scope
  // (testcase: t0167.cc)
  if (lang.compoundSelfName) {
    Variable *self = makeVariable(loc, instCT->name, instType,
                                  DF_TYPEDEF | DF_SELFNAME);
    instCT->addUniqueVariable(self);
    addedNewVariable(instCT, self);
  }

  // make a TemplateInfo for this instantiation
  TemplateInfo *instTI = new TemplateInfo(loc, inst);

  // fill in its arguments
  instTI->copyArguments(spec==primary? primaryArgs : partialSpecArgs);
  
  // if it is an instance of a partial spec, keep the primaryArgs too ...
  if (spec!=primary) {
    copyTemplateArgs(instTI->argumentsToPrimary, primaryArgs);
  }

  // attach it as an instance
  specTI->addInstantiation(inst);

  // this is an instantiation
  xassert(instTI->isInstantiation());

  return inst;
}

Variable *Env::instantiateClassTemplate
  (SourceLoc loc,
   Variable *primary,
   ObjList<STemplateArgument> const &sargs)
{
  return instantiateClassTemplate(loc, primary, objToSObjListC(sargs));
}


void Env::instantiateClassBody(Variable *inst)
{
  TemplateInfo *instTI = inst->templateInfo();
  CompoundType *instCT = inst->type->asCompoundType();

  Variable *spec = instTI->instantiationOf;
  TemplateInfo *specTI = spec->templateInfo();
  CompoundType *specCT = spec->type->asCompoundType();

  TRACE("template", "instantiating " <<
                    (specTI->isPrimary()? "class" : "partial spec") <<
                    " body: " << instTI->templateName());

  if (specCT->forward) {
    error(stringc << "attempt to instantiate `" << instTI->templateName()
                  << "', but no definition has been provided for `"
                  << specTI->templateName() << "'");
    return;
  }

  // isolate context
  InstantiationContextIsolator isolator(*this, loc());

  // defnScope: the scope where the class definition appeared
  Scope *defnScope;

  // do we have a function definition already?
  if (instCT->syntax) {
    // inline definition
    defnScope = instTI->defnScope;
  }
  else {
    // out-of-line definition; must clone the spec's definition
    instCT->syntax = specCT->syntax->clone();
    defnScope = specTI->defnScope;
  }
  xassert(instCT->syntax);
  xassert(defnScope);

  // set up the scopes in a way similar to how it was when the
  // template definition was first seen
  ObjList<Scope> poppedScopes;
  SObjList<Scope> pushedScopes;
  Scope *argScope = prepArgScopeForTemlCloneTcheck
    (poppedScopes, pushedScopes, defnScope,
     // I want to remove SK_EAT_TEMPL_SCOPE altogether, since the
     // 'instV' argument to Function::tcheck obviates it, but it's
     // currently being used for class instantiation, so for now
     // I just set makeEatScope=false to disable it locally.
     false /*makeEatScope*/);

  // bind the template arguments in scopes so that when we tcheck the
  // body, lookup will find them
  insertTemplateArgBindings(spec, instTI->arguments);

  // check the class body, forcing it to use 'instCT'
  instCT->syntax->name->tcheck(*this);
  instCT->syntax->ctype = instCT;
  instCT->syntax->tcheckIntoCompound(*this, DF_NONE, instCT,
                                     false /*inTemplate*/,
                                     false /*tcheckMethodBodies*/);

  // Now, we've just tchecked the clone in an environment that
  // makes all the type variables map to concrete types, so we
  // now have a nice, ordinary, non-template class with a bunch
  // of members.  But there is information stored in the
  // original AST that needs to be transferred over to the
  // clone, namely information about out-of-line definitions.
  // We need both the Function pointers and the list of template
  // params used at the definition site (since we have arguments
  // but don't know what names to bind them to).  So, walk over
  // both member lists, transferring information as necessary.
  transferTemplateMemberInfo(loc(), specCT->syntax, instCT->syntax,
                             instTI->arguments);

  // the instantiation is now complete
  instCT->forward = false;

  // restore the scopes
  if (argScope) {
    unPrepArgScopeForTemlCloneTcheck
      (argScope, poppedScopes, pushedScopes, false /*makeEatScope*/);
  }
  xassert(poppedScopes.isEmpty() && pushedScopes.isEmpty());
}


// return false on error
bool Env::supplyDefaultTemplateArguments
  (TemplateInfo *primaryTI,
   ObjList<STemplateArgument> &dest,          // arguments to use for instantiation
   SObjList<STemplateArgument> const &src)    // arguments supplied by user
{
  // since default arguments can refer to earlier parameters,
  // maintain a map of the arguments known so far
  STemplateArgumentCMap map;

  // simultanously iterate over arguments and parameters, building
  // 'dest' as we go
  SObjListIter<Variable> paramIter(primaryTI->params);
  SObjListIter<STemplateArgument> argIter(src);
  while (!paramIter.isDone()) {
    Variable const *param = paramIter.data();

    STemplateArgument *arg = NULL;     // (owner)

    // take argument from explicit list?
    if (!argIter.isDone()) {
      arg = argIter.data()->shallowClone();
      argIter.adv();

      // TODO: check that this argument matches the template parameter
    }

    // default?
    else {
      arg = makeDefaultTemplateArgument(paramIter.data(), map);
      if (arg) {
        TRACE("template", "supplied default argument `" <<
                          arg->toString() << "' for param `" <<
                          param->name << "'");
      }
    }

    if (!arg) {
      error(stringc << "no argument supplied for template parameter `"
                    << param->name << "'");
      return false;
    }

    // save this argument
    dest.append(arg);
    map.add(param->name, arg);

    paramIter.adv();
  }

  if (!argIter.isDone()) {
    error(stringc << "too many arguments supplied to template `"
                  << primaryTI->templateName() << "'");
    return false;
  }

  return true;
}


STemplateArgument *Env::makeDefaultTemplateArgument
  (Variable const *param, STemplateArgumentCMap &map)
{
  // type parameter?
  if (param->hasFlag(DF_TYPEDEF) &&
      param->defaultParamType) {
    // use 'param->defaultParamType', but push it through the map
    // so it can refer to previous arguments 
    Type *t = applyArgumentMapToType(map, param->defaultParamType);
    return new STemplateArgument(t);
  }
  
  // non-type parameter?
  else if (!param->hasFlag(DF_TYPEDEF) &&
           param->value) {
    // This was unimplemented in the old code, so I'm going to
    // keep that behavior.  Back then I said, in reference to
    // simply using 'param->value' directly:
    //
    // sm: the statement above seems reasonable, but I'm not at
    // all convinced it's really right... has it been tcheck'd?
    // has it been normalized?  are these things necessary?  so
    // I'll wait for a testcase to remove this assertion... before
    // this assertion *is* removed, someone should read over the
    // applicable parts of cppstd
    xfailure("unimplemented: default non-type argument");
  }
  
  return NULL;
}


#if 0
static bool doSTemplArgsContainVars(SObjList<STemplateArgument> &sargs)
{
  SFOREACH_OBJLIST_NC(STemplateArgument, sargs, iter) {
    if (iter.data()->containsVariables()) return true;
  }
  return false;
}
#endif // 0


#if 0
// sm: tcheck the declarator portion of 'func', ensuring that it uses
// 'var' as its declarator Variable.  This used to be done by passing
// 'priorTemplInst' into Function::tcheck and Declarator::tcheck, but
// I'm trying to avoid contaminating that code with so much knowledge
// about templates.  So, I'd rather do this hacky little dance from
// the outside than muck up the non-template core.
static void tcheckFunctionInstanceDecl_setVar
  (Env &env, Function *func, Variable *var)
{
  if (!var) {
    // not trying to set anything; just let it tcheck normally
    func->tcheck(env, false /*checkBody*/);
    return;
  }

  // this only works because 'checkBody' is false; if it were true,
  // then we'd be contaminating the lookups that happen while checking
  // the body ...

  Scope *s = env.acceptingScope();
  xassert(s->scopeKind == SK_EAT_TEMPL_INST);

  s->addSoleVariableToEatScope(var);

  // temporarily adjust the var's scope to circumvent a check
  // down in Env::createDeclaration ...
  Restorer<Scope*> restore(var->scope, s);

  func->tcheck(env, false /*checkBody*/);
  xassert(func->nameAndParams->var == var);
  
  s->removeSoleVariableFromEatScope();
}
#endif // 0


#if 0     // disabled
// see comments in cc_env.h
//
// sm: TODO: I think this function should be split into two, one for
// classes and one for functions.  To the extent they share mechanism
// it would be better to encapsulate the mechanism than to share it
// by threading two distinct flow paths through the same code.
//
//
// dsw: There are three dimensions to the space of paths through this
// code.
//
// 1 - class or function template
//
// 2 - baseForward: true exactly when we are typechecking a forward
// declaration and not a definition.
//
// 3 - instV: non-NULL when
//   a) a template primary was forward declared
//   b) it has now been defined
//   c) it has forwarded instantiations (& specializations?)  which
//   when they were instantiated, did not find a specialization and so
//   they are now being instantiated
// Note the test 'if (!instV)' below.  In this case, we are always
// going to instantiate the primary and not the specialization,
// because if the specialization comes after the instantiation point,
// it is not relevant anyway.
//
//
// dsw: We have three typechecking modes, enum Env::TemplTcheckMode:
//
//   TTM_1NORMAL:
//     This mode is for when you are in normal code.
//     Template instantiations are done fully.
//
//   TTM_2TEMPL_FUNC_DECL:
//     This mode is for when you're in the parameter list (D_func)
//     of a function template.
//     Template declarations are instantiated but not definitions.
//     This is necessary so type matching may be done for template
//     argument inference when template functions are called.
//
//   TTM_3TEMPL_DEF:
//     This mode is for when you're in the body of a function template.
//     Template instantiation requests just result in the primary being
//     returned; nothing is instantiated.
//
// Further, inside template definitions, default arguments are not
// typechecked and function template calls do not attempt to infer
// their template arguments from their function arguments.
//
// Two stacks in Env allow us to distinguish these modes record when
// the type-checker is typechecking a node below a TemplateDeclaration
// and a D_func/Initializer respectively.  See the implementation of
// Env::getTemplTcheckMode() for the details.
Variable *Env::instantiateTemplate
  (SourceLoc loc,
   Scope *foundScope,
   Variable *baseV,
   Variable *instV,
   Variable *bestV,
   SObjList<STemplateArgument> &primaryArgs,
   Variable *funcFwdInstV)
{
  if (baseV->type->isFunctionType()) {
    xfailure("don't call this for function templates");
  }

  // NOTE: There is an ordering bug here, e.g. it fails t0209.cc.  The
  // problem here is that we choose an instantiation (or create one)
  // based only on the arguments explicitly present.  Since default
  // arguments are delayed until later, we don't realize that C<> and
  // C<int> are the same class if it has a default argument of 'int'.
  //
  // The fix I think is to insert argument bindings first, *then*
  // search for or create an instantiation.  In some cases this means
  // we bind arguments and immediately throw them away (i.e. when an
  // instantiation already exists), but I don't see any good
  // alternatives.

  xassert(baseV->templateInfo()->isPrimary());
  // if we are in a template definition typechecking mode just return
  // the primary
  TemplTcheckMode tcheckMode = getTemplTcheckMode();
  if (tcheckMode == TTM_3TEMPL_DEF) {
    return baseV;
  }

  // isolate context
  InstantiationContextIsolator isolator(*this, loc);

  // 1 **** what template are we going to instanitate?  We now find
  // the instantiation/specialization/primary most specific to the
  // template arguments we have been given.  If it is a complete
  // specialization, we use it as-is.  If it is a partial
  // specialization, we reassign baseV.  Otherwise we fall back to the
  // primary.  At the end of this section if we have not returned then
  // baseV is the template to instantiate.

  Variable *oldBaseV = baseV;   // save baseV for other uses later
  // has this class already been instantiated?
  if (!instV) {
    // Search through the instantiations of this primary and do an
    // argument/specialization pattern match.
    if (!bestV) {
      // FIX: why did I do this?  It doesn't work.
//        if (tcheckMode == TTM_2TEMPL_FUNC_DECL) {
//          // in mode 2, just instantiate the primary
//          bestV = baseV;
//        }
      bestV = findMostSpecific(baseV, primaryArgs);
      // if no bestV then the lookup was ambiguous, error has been reported
      if (!bestV) return NULL;
    }
    if (bestV->templateInfo()->isCompleteSpecOrInstantiation()) {
      return bestV;
    }
    baseV = bestV;              // baseV is saved in oldBaseV above
  }
  // there should be something non-trivial to instantiate
  xassert(baseV->templateInfo()->argumentsContainVariables()
          // or the special case of a primary, which doesn't have
          // arguments but acts as if it did
          || baseV->templateInfo()->isPrimary()
          );

  // render the template arguments into a string that we can use
  // as the name of the instantiated class; my plan is *not* that
  // this string serve as the unique ID, but rather that it be
  // a debugging aid only
  StringRef instName = str(stringc << baseV->name << sargsToString(primaryArgs));

  // sm: what does this block do?  compute baseForward?  what is that?
  bool baseForward = false;
  bool sTemplArgsContainVars = doSTemplArgsContainVars(primaryArgs);
  if (baseV->type->isCompoundType()) {
    if (tcheckMode == TTM_2TEMPL_FUNC_DECL) {
      // This ad-hoc condition is to prevent a class template
      // instantiation in mode 2 that happens to not have any
      // quantified template variables from being treated as
      // forwarded.  Otherwise, what will happen is that if the
      // same template arguments are seen in mode 3, the class will
      // not be re-instantiated and a normal mode 3 class will have
      // no body.  To see this happen turn of the next line and run
      // in/d0060.cc and watch it fail.  Now comment out the 'void
      // g(E<A> &a)' in d0060.cc and run again and watch it pass.
      if (sTemplArgsContainVars) {
        // don't instantiate the template in a function template
        // definition parameter list
        baseForward = true;
      }
    } else {
      baseForward = baseV->type->asCompoundType()->forward;
    }
  } else {
    xassert(baseV->type->isFunctionType());
    // we should never get here in TTM_2TEMPL_FUNC_DECL mode
    xassert(tcheckMode != TTM_2TEMPL_FUNC_DECL);
    Declaration *declton = baseV->templateInfo()->declSyntax;
    if (declton && !baseV->funcDefn) {
      Declarator *decltor = declton->decllist->first();
      xassert(decltor);
      // FIX: is this still necessary?
      baseForward = (decltor->context == DC_TD_PROTO);
      // lets find out
      xassert(baseForward);
    } else {
      baseForward = false;
    }
  }

  // A non-NULL instV is marked as no longer forwarded before being
  // passed in.
  if (instV) {
    xassert(!baseForward);
  }
  TRACE("template", (baseForward ? "(forward) " : "") << "instantiating "
                 << (baseV->type->isCompoundType()? "class: " : "function: ")
                 << baseV->fullyQualifiedName() << sargsToString(primaryArgs));

  // if 'baseV' is a partial specialization, then transform the arguments
  // from being arguments to the primary into arguments to the partial spec
  MatchTypes match(tfac, MatchTypes::MM_BIND);   // will own new STemplateArguments (if any)
  SObjList<STemplateArgument> partialSpecArgs;   // pointers into 'match.bindings'
  SObjList<STemplateArgument> *sargs;            // args to use below
  if (baseV->templateInfo()->isPartialSpec()) {
    mapPrimaryArgsToSpecArgs(baseV, match, partialSpecArgs, primaryArgs);
    sargs = &partialSpecArgs;

    // refine the above report
    TRACE("template", (baseForward ? "(forward) " : "") << "instantiating "
                   << "class partial specialization "
                   << baseV->type->toString() << " with arguments "
                   << sargsToString(partialSpecArgs));
  }
  else {
    // not a partial spec, use the arguments we already have
    sargs = &primaryArgs;
  }

  // 2 **** Prepare the argument scope for template instantiation

  // pick a different 'foundScope' if we have a Function definition
  // to use (TODO: how many uses of 'foundScope' can I now remove?)
  if (baseV &&
      baseV->funcDefn) {
    foundScope = baseV->funcDefn->defnScope;
  }

  ObjList<Scope> poppedScopes;
  SObjList<Scope> pushedScopes;
  Scope *argScope = NULL;
  // don't mess with it if not going to tcheck anything; we need it in
  // either case for function types
  if (!(baseForward && baseV->type->isCompoundType())) {
    argScope = prepArgScopeForTemlCloneTcheck(poppedScopes, pushedScopes, foundScope);
    insertTemplateArgBindings(baseV, *sargs);
    
    // let's try setting the association for argument scopes here,
    // to reduce guesswork later
    //
    // actually, 'insertBindings' sets the parameterized primaries for
    // all of the inherited param scopes, so we just need to set the
    // last one
    //
    // use 'oldBaseV' since that really is the primary, whereas
    // 'baseV' might be a partial specialization
    scope()->setParameterizedPrimary(oldBaseV);
  }

  // 3 **** make a copy of the template definition body AST (unless
  // this is a forward declaration)

  TS_classSpec *copyCpd = NULL;
  TS_classSpec *cpdBaseSyntax = NULL; // need this below
  Function *copyFun = NULL;
  if (baseV->type->isCompoundType()) {
    cpdBaseSyntax = baseV->type->asCompoundType()->syntax;
    if (baseForward) {
      copyCpd = NULL;
    } else {
      xassert(cpdBaseSyntax);
      copyCpd = cpdBaseSyntax->clone();
    }
  } else {
    xassert(baseV->type->isFunctionType());
    Function *baseSyntax = baseV->funcDefn;
    if (baseForward) {
      copyFun = NULL;
    } else {
      xassert(baseSyntax);
      copyFun = baseSyntax->clone();
    }
  }

  // FIX: merge these
  if (copyCpd && tracingSys("cloneAST")) {
    cout << "--------- clone of " << instName << " ------------\n";
    copyCpd->debugPrint(cout, 0);
  }
  if (copyFun && tracingSys("cloneAST")) {
    cout << "--------- clone of " << instName << " ------------\n";
    copyFun->debugPrint(cout, 0);
  }

  // 4 **** make the Variable that will represent the instantiated
  // class during typechecking of the definition; this allows template
  // classes that refer to themselves and recursive function templates
  // to work.  The variable is assigned to instV; if instV already
  // exists, this work does not need to be done: the template was
  // previously forwarded and that forwarded definition serves the
  // purpose.

  if (!instV) {
    // copy over the template arguments so we can recognize this
    // instantiation later
    TemplateInfo *instTInfo = new TemplateInfo(loc);
    SFOREACH_OBJLIST(STemplateArgument, *sargs, iter) {
      instTInfo->arguments.append(new STemplateArgument(*iter.data()));
    }

    // record the location which provoked this instantiation; this
    // information will be useful if it turns out we can only to a
    // forward-declaration here, since later when we do the delayed
    // instantiation, 'loc' will be only be available because we
    // stashed it here
    //instTInfo->instLoc = loc;
    // actually, this is now part of the constructor argument above, 
    // but I'll leave the comment

    // FIX: Scott, its almost as if you just want to clone the type
    // here.
    //
    // sm: No, I want to type-check the instantiated cloned AST.  The
    // resulting type will be quite different, since it will refer to
    // concrete types instead of TypeVariables.
    if (baseV->type->isCompoundType()) {
      // 1/21/03: I had been using 'instName' as the class name, but
      // that causes problems when trying to recognize constructors
      CompoundType *baseVCpdType = baseV->type->asCompoundType();
      CompoundType *instVCpdType = tfac.makeCompoundType(baseVCpdType->keyword,
                                                         baseVCpdType->name);
      instVCpdType->instName = instName; // stash it here instead
      instVCpdType->forward = baseForward;
      instVCpdType->parentScope = baseVCpdType->parentScope;

      // wrap the compound in a regular type
      SourceLoc copyLoc = copyCpd ? copyCpd->loc : SL_UNKNOWN;
      Type *type = makeType(copyLoc, instVCpdType);

      // make a fake implicit typedef; this class and its typedef
      // won't actually appear in the environment directly, but making
      // the implicit typedef helps ensure uniformity elsewhere; also
      // must be done before type checking since it's the typedefVar
      // field which is returned once the instantiation is found via
      // 'instantiations'
      instV = makeVariable(copyLoc, baseV->name, type,
                           DF_TYPEDEF | DF_IMPLICIT);
      instVCpdType->typedefVar = instV;

      if (lang.compoundSelfName) {
        // also make the self-name, which *does* go into the scope
        // (testcase: t0167.cc)
        Variable *var2 = makeVariable(copyLoc, baseV->name, type,
                                      DF_TYPEDEF | DF_SELFNAME);
        instVCpdType->addUniqueVariable(var2);
        addedNewVariable(instVCpdType, var2);
      }

      baseV->templateInfo()->addInstantiation(instV);
    } else {
      xassert(baseV->type->isFunctionType());
      // sm: It seems to me the sequence should be something like this:
      //   1. bind template parameters to concrete types
      //        dsw: this has been done already above
      //   2. tcheck the declarator portion, thus yielding a FunctionType
      //      that refers to concrete types (not TypeVariables), and
      //      also yielding a Variable that can be used to name it
      //        dsw: this is done here
      if (copyFun) {
        xassert(!baseForward);
        // NOTE: 1) the whole point is that we don't check the body,
        // and 2) it is very important that we do not add the variable
        // to the namespace, otherwise the primary is masked if the
        // template body refers to itself
        tcheckFunctionInstanceDecl_setVar(*this, copyFun, funcFwdInstV);
        instV = copyFun->nameAndParams->var;
        //   3. add said Variable to the list of instantiations, so if the
        //      function recursively calls itself we'll be ready
        //        dsw: this is done below
        //   4. tcheck the function body
        //        dsw: this is done further below.
      } else {
        xassert(baseForward);
        // We do have to clone the forward declaration before
        // typechecking.
        Declaration *fwdDecl = baseV->templateInfo()->declSyntax;
        xassert(fwdDecl);
        // use the same context again but make sure it is well defined
        xassert(fwdDecl->decllist->count() == 1);
        DeclaratorContext ctxt = fwdDecl->decllist->first()->context;
        xassert(ctxt != DC_UNKNOWN);
        Declaration *copyDecl = fwdDecl->clone();
        xassert(argScope);
        xassert(!funcFwdInstV);
        copyDecl->tcheck(*this, ctxt);
        xassert(copyDecl->decllist->count() == 1);
        Declarator *copyDecltor = copyDecl->decllist->first();
        instV = copyDecltor->var;
      }
    }

    xassert(instV);
    xassert(foundScope);
    foundScope->registerVariable(instV);

    // tell the base template about this instantiation; this has to be
    // done before invoking the type checker, to handle cases where the
    // template refers to itself recursively (which is very common)
    //
    // dsw: this is the one place where I use oldBase instead of base;
    // looking at the other code above, I think it is the only one
    // where I should, but I'm not sure.
    xassert(oldBaseV->templateInfo() && oldBaseV->templateInfo()->isPrimary());
    // dsw: this had to be moved down here as you can't get the
    // typedefVar until it exists
    instV->setTemplateInfo(instTInfo);
    if (funcFwdInstV) {
      // addInstantiation would have done this
      instV->templateInfo()->setMyPrimary(oldBaseV->templateInfo());
    } else {
      // NOTE: this can mutate instV if instV is a duplicated mutant
      // in the instantiation list; FIX: perhaps find another way to
      // prevent creating duplicate mutants in the first place; this
      // is quite wasteful if we have cloned an entire class of AST
      // only to throw it away again
      Variable *newInstV = oldBaseV->templateInfo()->
        addInstantiation(tfac, instV, true /*suppressDup*/);
      if (newInstV != instV) {
        // don't do stage 5 below; just use the newInstV and be done
        // with it
        copyCpd = NULL;
        copyFun = NULL;
        instV = newInstV;
      }
      xassert(instV->templateInfo()->getMyPrimaryIdem() == oldBaseV->templateInfo());
    }
    xassert(instTInfo->isNotPrimary());
    xassert(instTInfo->getMyPrimaryIdem() == oldBaseV->templateInfo());
  }

  // 5 **** typecheck the cloned AST

  if (copyCpd || copyFun) {
    xassert(!baseForward);
    xassert(argScope);

    if (instV->type->isCompoundType()) {
      // FIX: unlike with function templates, we can't avoid
      // typechecking the compound type clone when not in 'normal'
      // mode because otherwise implicit members such as ctors don't
      // get elaborated into existence
//          if (getTemplTcheckMode() == TTM_1NORMAL) { . . .
      xassert(copyCpd);
      // invoke the TS_classSpec typechecker, giving to it the
      // CompoundType we've built to contain its declarations; for
      // now I don't support nested template instantiation
      copyCpd->ctype = instV->type->asCompoundType();

      // check 'copyCpd->name', to assist in pretty printing
      copyCpd->name->tcheck(*this);

      // preserve the baseV and sargs so that when the member
      // function bodies are typechecked later we have them
      copyCpd->tcheckIntoCompound
        (*this,
         DF_NONE,
         copyCpd->ctype,
         false /*inTemplate*/,
         // that's right, really check the member function bodies
         // exactly when we are instantiating something that is not
         // a complete specialization; if it *is* a complete
         // specialization, then the tcheck will be done later, say,
         // when the function is used
         sTemplArgsContainVars /*reallyTcheckFunctionBodies*/,
         NULL /*containingClass*/);
      // this is turned off because it doesn't work: somewhere the
      // mutants are actually needed; Instead we just avoid them
      // above.
      //      deMutantify(baseV);

      // Now, we've just tchecked the clone in an environment that
      // makes all the type variables map to concrete types, so we
      // now have a nice, ordinary, non-template class with a bunch
      // of members.  But there is information stored in the
      // original AST that needs to be transferred over to the
      // clone, namely information about out-of-line definitions.
      // We need both the Function pointers and the list of template
      // params used at the definition site (since we have arguments
      // but don't know what names to bind them to).  So, walk over
      // both member lists, transferring information as necessary.
      transferTemplateMemberInfo(loc, cpdBaseSyntax, copyCpd, *sargs);

      #if 0     // old
      // find the funcDefn's for all of the function declarations
      // and then instantiate them; FIX: do the superclass members
      // also?
      FOREACH_ASTLIST_NC(Member, cpdBaseSyntax->members->list, memIter) {
        Member *mem = memIter.data();
        if (!mem->isMR_decl()) continue;
        Declaration *decltn = mem->asMR_decl()->d;
        Declarator *decltor = decltn->decllist->first();
        // this seems to happen with anonymous enums
        if (!decltor) continue;
        if (!decltor->var->type->isFunctionType()) continue;
        xassert(decltn->decllist->count() == 1);
        Function *funcDefn = decltor->var->funcDefn;
        // skip functions that haven't been defined yet; hopefully
        // they'll be defined later and if they are, their
        // definitions will be patched in then
        if (!funcDefn) continue;
        // I need to instantiate this funcDefn.  This means: 1)
        // clone it; [NOTE: this is not being done: 2) all the
        // arguments are already in the scope, but if the definition
        // named them differently, it won't work, so throw out the
        // current template scope and make another]; then 3) just
        // typecheck it.
        Function *copyFuncDefn = funcDefn->clone();
        // find the instantiation of the cloned declaration that
        // goes with it; FIX: this seems brittle to me; I'd rather
        // have something like a pointer from the cloned declarator
        // to the one it was cloned from.
        Variable *funcDefnInstV = instV->type->asCompoundType()->lookupVariable
          (decltor->var->name, *this,
           LF_INNER_ONLY |
           // this shouldn't be necessary, but if it turns out to be
           // a function template, we don't want it instantiated
           LF_TEMPL_PRIMARY);
        xassert(funcDefnInstV);
        xassert(funcDefnInstV->name == decltor->var->name);
        if (funcDefnInstV->templateInfo()) {
          // FIX: I don't know what to do with function template
          // members of class templates just now, but what we do
          // below is probably wrong, so skip them
          continue;
        }
        xassert(!funcDefnInstV->funcDefn);

        // FIX: I wonder very much if the right thing is happening
        // to the scopes at this point.  I think all the current
        // scopes up to the global scope need to be removed, and
        // then the template scope re-created, and then typecheck
        // this.  The extra scopes are probably harmless, but
        // shouldn't be there.

        // urk, there's nothing to do here.  The declaration has
        // already been tchecked, and we don't want to tcheck the
        // definition yet, so we can just point the var at the
        // cloned definition; I'll run the tcheck with
        // checkBody=false anyway just for uniformity.
        //
        // sm: once again trying it with this disabled... it should
        // not be necessary, since we should automatically find
        // 'funcFwdInstV' when tchecking the defn for real
        //tcheckFunctionInstanceDecl_setVar(*this, copyFuncDefn, funcFwdInstV);

        // paste in the definition for later use
        xassert(!funcDefnInstV->funcDefn);
        funcDefnInstV->funcDefn = copyFuncDefn;
      }
      #endif // 0
    } else {
      xassert(instV->type->isFunctionType());
      // if we are in a template definition, don't typecheck the
      // cloned function body
      if (tcheckMode == TTM_1NORMAL) {
        xassert(copyFun);
        copyFun->funcType = instV->type->asFunctionType();

        // sm: why assert this?  t0231.cc is an example of good
        // code that causes it to fail
        //xassert(scope()->isGlobalTemplateScope());

        // NOTE: again we do not add the variable to the namespace
        //
        // sm: all that is necessary is to check the body, not the
        // declarator again
        xassert(instV == copyFun->nameAndParams->var);
        copyFun->tcheckBody(*this);
        xassert(instV->funcDefn == copyFun);
      }
    }

    if (tracingSys("cloneTypedAST")) {
      cout << "--------- typed clone of " << instName << " ------------\n";
      if (copyCpd) copyCpd->debugPrint(cout, 0);
      if (copyFun) copyFun->debugPrint(cout, 0);
    }
  }
  // this else case can now happen if we find that there was a
  // duplicated instV and therefore jump out during stage 4
//    } else {
//      xassert(baseForward);
//    }

  // 6 **** Undo the scope, reversing step 2
  if (argScope) {
    unPrepArgScopeForTemlCloneTcheck(argScope, poppedScopes, pushedScopes);
    argScope = NULL;
  }
  // make sure we haven't forgotten these
  xassert(poppedScopes.isEmpty() && pushedScopes.isEmpty());

  return instV;
}


// variant that accepts an ObjList of arguments
Variable *Env::instantiateTemplate
  (SourceLoc loc,
   Scope *foundScope,
   Variable *baseV,
   Variable *instV,
   Variable *bestV,
   ObjList<STemplateArgument> &sargs,
   Variable *funcFwdInstV)
{
  // hack it with a cast
  return instantiateTemplate(loc, foundScope, baseV, instV, bestV,
                             objToSObjList(sargs),
                             funcFwdInstV);
}
#endif // 0


// transfer template info from members of 'source' to corresp.
// members of 'dest'; 'dest' is a clone of 'source'
void Env::transferTemplateMemberInfo
  (SourceLoc instLoc, TS_classSpec *source,
   TS_classSpec *dest, ObjList<STemplateArgument> const &sargs)
{
  // simultanous iteration
  ASTListIterNC<Member> srcIter(source->members->list);
  ASTListIterNC<Member> destIter(dest->members->list);

  for (; !srcIter.isDone() && !destIter.isDone(); 
         srcIter.adv(), destIter.adv()) {
    if (srcIter.data()->isMR_decl()) {
      FakeList<Declarator> *srcDeclarators = srcIter.data()->asMR_decl()->d->decllist;
      FakeList<Declarator> *destDeclarators = destIter.data()->asMR_decl()->d->decllist;

      // simultanously iterate over the declarators
      for (; srcDeclarators->isNotEmpty() && destDeclarators->isNotEmpty();
             srcDeclarators = srcDeclarators->butFirst(),
             destDeclarators = destDeclarators->butFirst()) {
        Variable *srcVar = srcDeclarators->first()->var;
        Variable *destVar = destDeclarators->first()->var;

        if (srcVar->type->isFunctionType()) {
          // srcVar -> destVar
          transferTemplateMemberInfo_one(instLoc, srcVar, destVar, sargs);

          #if 0    // funcDefn handled differently now
          // transfer knowledge about the function definition, if any
          if (srcVar->funcDefn) {
            // clone the definition
            destVar->funcDefn = srcVar->funcDefn->clone();

            // copy over one result from the tcheck of the original
            destVar->funcDefn->defnScope = srcVar->funcDefn->defnScope;

            // but don't instantiate it until we have to
            destVar->setFlag(DF_DELAYED_INST);
          }
          #endif // 0
        }

        // TODO: what about nested classes?
        else if (srcIter.data()->asMR_decl()->d->spec->isTS_classSpec()) {
          unimp("nested class of a template class");
        }
      }
      xassert(srcDeclarators->isEmpty() && destDeclarators->isEmpty());
    }

    else if (srcIter.data()->isMR_func()) {
      Variable *srcVar = srcIter.data()->asMR_func()->f->nameAndParams->var;
      Variable *destVar = destIter.data()->asMR_func()->f->nameAndParams->var;

      transferTemplateMemberInfo_one(instLoc, srcVar, destVar, sargs);
      
      // the instance 'destVar' needs to have a 'defnScope'; it didn't
      // get set earlier b/c at the time the declaration was tchecked,
      // the Function didn't know it was an instantiation (but why is
      // that?)
      TemplateInfo *destTI = destVar->templateInfo();
      xassert(!destTI->defnScope);
      destTI->defnScope = destVar->scope;
      xassert(destTI->defnScope);
    }

    else if (srcIter.data()->isMR_template()) {
      TemplateDeclaration *srcTDecl = srcIter.data()->asMR_template()->d;
      TemplateDeclaration *destTDecl = destIter.data()->asMR_template()->d;

      // I've decided that member templates should just be treated as
      // primaries in their own right, right no relation to the
      // "original" definition, hence no action is needed!
      //
      // ah, but there is still the need to xfer the funcDefn, and to
      // find the instantiations later, for out-of-line defns, plus they
      // need to remember the template arguments.  so, I'm introducing
      // the notion of "partial instantiation"

      if (srcTDecl->isTD_proto()) {
        Variable *srcVar = srcTDecl->asTD_proto()->d->decllist->first()->var;
        Variable *destVar = destTDecl->asTD_proto()->d->decllist->first()->var;

        transferTemplateMemberInfo_membert(instLoc, srcVar, destVar, sargs);
      }

      else if (srcTDecl->isTD_func()) {
        Variable *srcVar = srcTDecl->asTD_func()->f->nameAndParams->var;
        Variable *destVar = destTDecl->asTD_func()->f->nameAndParams->var;

        transferTemplateMemberInfo_membert(instLoc, srcVar, destVar, sargs);
      }

      else if (srcTDecl->isTD_class()) {
        unimp("template class with member template class");
      }

      else if (srcTDecl->isTD_tmember()) {
        // not sure if this would even parse... if it does I don't
        // know what it might mean
        error("more than one template <...> declaration inside a class body?");
      }

      else {
        xfailure("unknown TemplateDeclaration kind");
      }
    }

    else {
      // other kinds of member decls: don't need to do anything
    }
  }

  // one is clone of the other, so same length lists
  xassert(srcIter.isDone() && destIter.isDone());
}


// transfer template information from primary 'srcVar' to
// instantiation 'destVar'
void Env::transferTemplateMemberInfo_one
  (SourceLoc instLoc, Variable *srcVar, Variable *destVar,
   ObjList<STemplateArgument> const &sargs)
{
  TRACE("templateXfer", "associated primary " << srcVar->toQualifiedString()
                     << " with inst " << destVar->toQualifiedString());

  // make the TemplateInfo for this member instantiation
  TemplateInfo *destTI = new TemplateInfo(instLoc);

  // copy arguments into 'destTI'
  destTI->copyArguments(sargs);

  // attach 'destTI' to 'destVar'
  destVar->setTemplateInfo(destTI);

  // 'destVar' is an instantiation of 'srcVar' with 'sargs'
  TemplateInfo *srcTI = srcVar->templateInfo();
  xassert(srcTI);
  srcTI->addInstantiation(destVar);
}


// this is for member templates ("membert")
void Env::transferTemplateMemberInfo_membert
  (SourceLoc instLoc, Variable *srcVar, Variable *destVar,
   ObjList<STemplateArgument> const &sargs)
{
  // what follows is a modified version of 'transferTemplateMemberInfo_one'

  // 'destVar' is a partial instantiation of 'srcVar' with 'args'
  TemplateInfo *srcTI = srcVar->templateInfo();
  xassert(srcTI);
  TemplateInfo *destTI = destVar->templInfo;
  xassert(destTI);

  destTI->copyArguments(sargs);

  srcTI->addPartialInstantiation(destVar);

  if (!destVar->funcDefn) {      // do this only for out-of-line definitions
    // finally, make them share a function definition (if any); it
    // will be cloned before a real instantiation is made
    destVar->funcDefn = srcVar->funcDefn;
    destTI->defnScope = srcTI->defnScope;
  }

  // do this last so I have full info to print for 'destVar'
  TRACE("templateXfer", "associated primary " << srcVar->toQualifiedString()
                     << " with partial inst " << destVar->toQualifiedString());
}


// given a name that was found without qualifiers or template arguments,
// see if we're currently inside the scope of a template definition
// with that name
CompoundType *Env::findEnclosingTemplateCalled(StringRef name)
{
  FOREACH_OBJLIST(Scope, scopes, iter) {
    Scope const *s = iter.data();

    if (s->curCompound &&
        s->curCompound->templateInfo() &&
        s->curCompound->name == name) {
      return s->curCompound;
    }
  }
  return NULL;     // not found
}


#if 0
void Env::provideDefForFuncTemplDecl
  (Variable *forward, TemplateInfo *primaryTI, Function *f)
{
  xassert(forward);
  xassert(primaryTI);
  xassert(forward->templateInfo()->getPrimary() == primaryTI);
  xassert(primaryTI->isPrimary());
  Variable *fVar = f->nameAndParams->var;
  // update things in the declaration; I copied this from
  // Env::createDeclaration()
  TRACE("odr", "def'n of " << forward->name
        << " at " << toString(f->getLoc())
        << " overrides decl at " << toString(forward->loc));
  forward->loc = f->getLoc();
  forward->setFlag(DF_DEFINITION);
  forward->clearFlag(DF_EXTERN);
  forward->clearFlag(DF_FORWARD); // dsw: I added this
  // make this idempotent
  if (forward->funcDefn) {
    xassert(forward->funcDefn == f);
  } else {
    forward->funcDefn = f;
    if (tracingSys("template")) {
      cout << "definition of function template " << fVar->toString()
           << " attached to previous forwarded declaration" << endl;
      primaryTI->debugPrint();
    }
  }
  // make this idempotent
  if (fVar->templateInfo()->getPrimary()) {
    xassert(fVar->templateInfo()->getPrimary() == primaryTI);
  } else {
    fVar->templateInfo()->setMyPrimary(primaryTI);
  }
}
#endif // 0


void Env::ensureFuncBodyTChecked(Variable *instV)
{
  if (!instV) {
    return;      // error recovery
  }
  if (!instV->type->isFunctionType()) {
    // I'm not sure what circumstances can cause this, but it used
    // to be that all call sites to this function were guarded by
    // this 'isFunctionType' check, so I pushed it inside
    return;
  }

  TemplateInfo *instTI = instV->templateInfo();
  if (!instTI) {
    // not a template instantiation
    return;
  }
  if (!instTI->isCompleteSpecOrInstantiation()) {
    // not an instantiation; this might be because we're in
    // the middle of tchecking a template definition, so we
    // just used a function template primary sort of like
    // a PseudoInstantiation; skip checking it
    return;
  }
  if (instTI->instantiateBody) {
    // we've already seen this request, so either the function has
    // already been instantiated, or else we haven't seen the
    // definition yet so there's nothing we can do
    return;
  }

  // acknowledge the request
  instTI->instantiateBody = true;

  // what template am I an instance of?
  Variable *baseV = instTI->instantiationOf;
  if (!baseV) {
    // This happens for explicit complete specializations.  It's
    // not clear whether such things should have templateInfos
    // at all, but there seems little harm, so I'll just bail in
    // that case
    return;
  }

  // have we seen a definition of it?
  if (!baseV->funcDefn) {
    // nope, nothing we can do yet
    TRACE("template", "want to instantiate func body: " << 
                      instV->toQualifiedString() << 
                      ", but cannot because have not seen defn");
    return;
  }

  // ok, at this point we're committed to going ahead with
  // instantiating the function body
  instantiateFunctionBody(instV);
}

void Env::instantiateFunctionBody(Variable *instV)
{
  TRACE("template", "instantiating func body: " << instV->toQualifiedString());
  
  // reconstruct a few variables from above
  TemplateInfo *instTI = instV->templateInfo();
  Variable *baseV = instTI->instantiationOf;

  // someone should have requested this
  xassert(instTI->instantiateBody);

  // isolate context
  InstantiationContextIsolator isolator(*this, loc());

  // defnScope: the scope where the function definition appeared.
  Scope *defnScope;

  // do we have a function definition already?
  if (instV->funcDefn) {
    // inline definition
    defnScope = instTI->defnScope;

    // temporary, until we remove it: check with Function::defnScope
    xassert(defnScope == instV->funcDefn->defnScope);
  }
  else {
    // out-of-line definition; must clone the primary's definition
    instV->funcDefn = baseV->funcDefn->clone();
    defnScope = baseV->templateInfo()->defnScope;
    xassert(defnScope == baseV->funcDefn->defnScope);
  }

  // set up the scopes in a way similar to how it was when the
  // template definition was first seen
  ObjList<Scope> poppedScopes;
  SObjList<Scope> pushedScopes;
  Scope *argScope = prepArgScopeForTemlCloneTcheck
    (poppedScopes, pushedScopes, defnScope,
     // I want to remove SK_EAT_TEMPL_SCOPE altogether, since the
     // 'instV' argument to Function::tcheck obviates it, but it's
     // currently being used for class instantiation, so for now
     // I just set makeEatScope=false to disable it locally.
     false /*makeEatScope*/);

  // bind the template arguments in scopes so that when we tcheck the
  // body, lookup will find them
  insertTemplateArgBindings(baseV, instTI->arguments);

  // push the declaration scopes for inline definitions, since
  // we don't get those from the declarator (that is in fact a
  // mistake of the current implementation; eventually, we should
  // 'pushDeclarationScopes' regardless of DF_INLINE_DEFN)
  if (instV->funcDefn->dflags & DF_INLINE_DEFN) {
    pushDeclarationScopes(instV, defnScope);
  }

  // check the body, forcing it to use 'instV'
  instV->funcDefn->tcheck(*this, true /*checkBody*/, instV);

  if (instV->funcDefn->dflags & DF_INLINE_DEFN) {
    popDeclarationScopes(instV, defnScope);
  }

  if (argScope) {
    unPrepArgScopeForTemlCloneTcheck
      (argScope, poppedScopes, pushedScopes, false /*makeEatScope*/);
  }
  xassert(poppedScopes.isEmpty() && pushedScopes.isEmpty());
}


void Env::instantiateForwardFunctions(Variable *primary)
{
  SFOREACH_OBJLIST_NC(Variable, primary->templateInfo()->instantiations, iter) {
    Variable *inst = iter.data();
    
    if (inst->templateInfo()->instantiateBody) {
      instantiateFunctionBody(inst);
    }
  }

  #if 0   // old
  TemplateInfo *primaryTI = primary->templateInfo();
  xassert(primaryTI);
  xassert(primaryTI->isPrimary());

  // temporarily supress TTM_3TEMPL_DEF and return to TTM_1NORMAL for
  // purposes of instantiating the forward function templates
  Restorer<TemplTcheckMode> restoreMode(tcheckMode, TTM_1NORMAL);

  // Find all the places where this declaration was instantiated,
  // where this function template specialization was
  // called/instantiated after it was declared but before it was
  // defined.  In each, now instantiate with the same template
  // arguments and fill in the funcDefn; UPDATE: see below, but now we
  // check that the definition was already provided rather than
  // providing one.
  SFOREACH_OBJLIST_NC(Variable, primaryTI->getInstantiations(), iter) {
    Variable *instV = iter.data();
    TemplateInfo *instTI = instV->templateInfo();
    xassert(instTI);
    xassert(instTI->isNotPrimary());
    if (instTI->instantiatedFrom != forward) continue;
    // should not have a funcDefn as it is instantiated from a forward
    // declaration that does not yet have a definition and we checked
    // that above already
    xassert(!instV->funcDefn);

    // instantiate this definition
    SourceLoc instLoc = instTI->instLoc;

    // sm: case 1: some calling context I don't quite understand; leave
    // the behavior as-is
    if (forward != primary) {
      Variable *instWithDefn = instantiateTemplate
        (instLoc,
         // FIX: I don't know why it is possible for the primary here to
         // not have a scope, but it is.  Since 1) we are in the scope
         // of the definition that we want to instantiate, and 2) from
         // experiments with gdb, I have to put the definition of the
         // template in the same scope as the declaration, I conclude
         // that I can use the same scope as we are in now
  //         primary->scope,    // FIX: ??
         // UPDATE: if the primary is in the global scope, then I
         // suppose its scope might be NULL; I don't remember the rule
         // for that.  So, maybe it's this:
  //         primary->scope ? primary->scope : env.globalScope()
         scope(),
         primary,
         NULL /*instV; only used by instantiateForwardClasses; this
                seems to be a fundamental difference*/,
         forward /*bestV*/,
         instV->templateInfo()->arguments,
         // don't actually add the instantiation to the primary's
         // instantiation list; we will do that below
         instV
         );
      // previously the instantiation of the forward declaration
      // 'forward' produced an instantiated declaration; we now provide
      // a definition for it; UPDATE: we now check that when the
      // definition of the function was typechecked that a definition
      // was provided for it, since we now re-use the declaration's var
      // in the defintion declartion (the first pass when
      // checkBody=false).
      //
      // FIX: I think this works for functions the definition of which
      // is going to come after the instantiation request of the
      // containing class template since I think both of these will be
      // NULL; we will find out
      xassert(instV->funcDefn == instWithDefn->funcDefn);
    }

    // sm: case 2: a context I do understand, namely the instantiation
    // of function bodies that had to be postponed because the template
    // definition hadn't been provided
    else {
      if (instTI->instantiateBody) {     // body was requested at some point
        instantiateFunctionBody(instV);
      }
    }
  }
  #endif // 0
}


void Env::instantiateForwardClasses(Variable *baseV)
{
  // temporarily supress TTM_3TEMPL_DEF and return to TTM_1NORMAL for
  // purposes of instantiating the forward classes
  Restorer<TemplTcheckMode> restoreMode(tcheckMode, TTM_1NORMAL);

  SFOREACH_OBJLIST_NC(Variable, baseV->templateInfo()->instantiations, iter) {
    instantiateClassBody(iter.data());

    #if 0
    Variable *instV = iter.data();
    xassert(instV->templateInfo());
    CompoundType *inst = instV->type->asCompoundType();
    // this assumption is made below
    xassert(inst->templateInfo() == instV->templateInfo());

    if (inst->forward) {
      TRACE("template", "instantiating previously forward " << inst->name);
      inst->forward = false;

      // this location was stored back when the template was
      // forward-instantiated
      SourceLoc instLoc = inst->templateInfo()->instLoc;

      instantiateTemplate(instLoc,
                          scope,
                          baseV,
                          instV /*use this one*/,
                          NULL /*bestV*/,
                          inst->templateInfo()->arguments
                          );
    }
    else {
      // this happens in e.g. t0079.cc, when the template becomes
      // an instantiation of itself because the template body
      // refers to itself with template arguments supplied

      // update: maybe not true anymore?
    }
    #endif // 0
  }
}


// --------------------- from cc_tcheck.cc ----------------------
// go over all of the function bodies and make sure they have been
// typechecked
class EnsureFuncBodiesTcheckedVisitor : public ASTTemplVisitor {
  Env &env;
public:
  EnsureFuncBodiesTcheckedVisitor(Env &env0) : env(env0) {}
  bool visitFunction(Function *f);
  bool visitDeclarator(Declarator *d);
};

bool EnsureFuncBodiesTcheckedVisitor::visitFunction(Function *f) {
  xassert(f->nameAndParams->var);
  env.ensureFuncBodyTChecked(f->nameAndParams->var);
  return true;
}

bool EnsureFuncBodiesTcheckedVisitor::visitDeclarator(Declarator *d) {
//    printf("EnsureFuncBodiesTcheckedVisitor::visitDeclarator d:%p\n", d);
  // a declarator can lack a var if it is just the declarator for the
  // ASTTypeId of a type, such as in "operator int()".
  if (!d->var) {
    // I just want to check something that will make sure that this
    // didn't happen because the enclosing function body never got
    // typechecked
    xassert(d->context == DC_UNKNOWN);
//      xassert(d->decl);
//      xassert(d->decl->isD_name());
//      xassert(!d->decl->asD_name()->name);
  } else {
    env.ensureFuncBodyTChecked(d->var);
  }
  return true;
}


void instantiateRemainingMethods(Env &env, TranslationUnit *tunit)
{
  // Given the current architecture, it is impossible to ensure that
  // all called functions have had their body tchecked.  This is
  // because if implicit calls to existing functions.  That is, one
  // user-written (not implicitly defined) function f() that is
  // tchecked implicitly calls another function g() that is
  // user-written, but the call is elaborated into existence.  This
  // means that we don't see the call to g() until the elaboration
  // stage, but by then typechecking is over.  However, since the
  // definition of g() is user-written, not implicitly defined, it
  // does indeed need to be tchecked.  Therefore, at the end of a
  // translation unit, I simply typecheck all of the function bodies
  // that have not yet been typechecked.  If this doesn't work then we
  // really need to change something non-trivial.
  //
  // It seems that we are ok for now because it is only function
  // members of templatized classes that we are delaying the
  // typechecking of.  Also, I expect that the definitions will all
  // have been seen.  Therefore, I can just typecheck all of the
  // function bodies at the end of typechecking the translation unit.
  //
  // sm: Only do this if tchecking doesn't have any errors yet,
  // b/c the assertions in the traversal will be potentially invalid
  // if errors were found
  //
  // 8/03/04: Um, why are we doing this at all?!  It defeats the
  // delayed instantiation mechanism altogether!  I'm disabling it.
  #if 0    // no!
  if (env.numErrors() == 0) {
    EnsureFuncBodiesTcheckedVisitor visitor(env);
    tunit->traverse(visitor);
  }
  #endif // 0
}


// we (may) have just encountered some syntax which declares
// some template parameters, but found that the declaration
// matches a prior declaration with (possibly) some other template
// parameters; verify that they match (or complain), and then
// discard the ones stored in the environment (if any)
//
// return false if there is some problem, true if it's all ok
// (however, this value is ignored at the moment)
bool verifyCompatibleTemplates(Env &env, CompoundType *prior)
{
  Scope *scope = env.scope();
  if (!scope->hasTemplateParams() && !prior->isTemplate()) {
    // neither talks about templates, forget the whole thing
    return true;
  }

  if (!scope->hasTemplateParams() && prior->isTemplate()) {
    env.error(stringc
      << "prior declaration of " << prior->keywordAndName()
      << " at " << prior->typedefVar->loc
      << " was templatized with parameters "
      << prior->templateInfo()->paramsToCString()
      << " but the this one is not templatized",
      EF_DISAMBIGUATES);
    return false;
  }

  if (scope->hasTemplateParams() && !prior->isTemplate()) {
    env.error(stringc
      << "prior declaration of " << prior->keywordAndName()
      << " at " << prior->typedefVar->loc
      << " was not templatized, but this one is, with parameters "
      << paramsToCString(scope->templateParams),
      EF_DISAMBIGUATES);
    return false;
  }

  // now we know both declarations have template parameters;
  // check them for naming equivalent types
  //
  // furthermore, fix the names in 'prior' in case they differ
  // with those of 'scope->curTemplateParams'
  //
  // even more, merge their default arguments
  bool ret = env.mergeParameterLists(
    prior->typedefVar,
    prior->templateInfo()->params,     // dest
    scope->templateParams);            // src

  return ret;
}


// context: I have previously seen a (forward) template
// declaration, such as
//   template <class S> class C;             // dest
//                   ^
// and I want to modify it to use the same names as another
// declaration later on, e.g.
//   template <class T> class C { ... };     // src
//                   ^
// since in fact I am about to discard the parameters that
// come from 'src' and simply keep using the ones from
// 'dest' for future operations, including processing the
// template definition associated with 'src'
bool Env::mergeParameterLists(Variable *prior,
                              SObjList<Variable> &destParams,
                              SObjList<Variable> const &srcParams)
{
  TRACE("templateParams", "mergeParameterLists: prior=" << prior->name
    << ", dest=" << paramsToCString(destParams)
    << ", src=" << paramsToCString(srcParams));

  // keep track of whether I've made any naming changes
  // (alpha conversions)
  bool anyNameChanges = false;

  SObjListMutator<Variable> destIter(destParams);
  SObjListIter<Variable> srcIter(srcParams);
  for (; !destIter.isDone() && !srcIter.isDone();
       destIter.adv(), srcIter.adv()) {
    Variable *dest = destIter.data();
    Variable const *src = srcIter.data();

    // are the types equivalent?
    if (!isomorphicTypes(dest->type, src->type)) {
      error(stringc
        << "prior declaration of " << prior->toString()
        << " at " << prior->loc
        << " was templatized with parameter `"
        << dest->name << "' of type `" << dest->type->toString()
        << "' but this one has parameter `"
        << src->name << "' of type `" << src->type->toString()
        << "', and these are not equivalent",
        EF_DISAMBIGUATES);
      return false;
    }

    // what's up with their default arguments?
    if (dest->value && src->value) {
      // this message could be expanded...
      error("cannot specify default value of template parameter more than once");
      return false;
    }

    // there is a subtle problem if the prior declaration has a
    // default value which refers to an earlier template parameter,
    // but the name of that parameter has been changed
    if (anyNameChanges &&              // earlier param changed
        dest->value) {                 // prior has a value
      // leave it as a to-do for now; a proper implementation should
      // remember the name substitutions done so far, and apply them
      // inside the expression for 'dest->value'
      xfailure("unimplemented: alpha conversion inside default values"
               " (workaround: use consistent names in template parameter lists)");
    }

    // merge their default values
    if (src->value && !dest->value) {
      dest->value = src->value;
    }

    // do they use the same name?
    if (dest->name != src->name) {
      // make the names the same
      TRACE("templateParams", "changing parameter " << dest->name
        << " to " << src->name);
      anyNameChanges = true;

      // Make a new Variable to hold the modified parameter.  I'm not
      // sure this is the right thing to do, b/c I'm concerned about
      // the fact that the original decl will be pointing at the old
      // Variable, but if I modify it in place I've got the problem
      // that the template params for a class are shared by all its
      // members, so I'd be changing all the members' params too.
      // Perhaps it's ok to make a few copies of template parameter
      // Variables, as they are somehow less concrete than the other
      // named entities...
      Variable *v = makeVariable(dest->loc, src->name, src->type, dest->flags);
      
      // copy a few other fields, including default value
      v->value = dest->value;
      v->defaultParamType = dest->defaultParamType;
      v->scope = dest->scope;
      v->scopeKind = dest->scopeKind;

      // replace the old with the new
      destIter.dataRef() = v;
    }
  }

  if (srcIter.isDone() && destIter.isDone()) {
    return true;   // ok
  }
  else {
    error(stringc
      << "prior declaration of " << prior->toString()
      << " at " << prior->loc
      << " was templatized with "
      << pluraln(destParams.count(), "parameter")
      << ", but this one has "
      << pluraln(srcParams.count(), "parameter"),
      EF_DISAMBIGUATES);
    return false;
  }
}


bool Env::mergeTemplateInfos(Variable *prior, TemplateInfo *dest,
                             TemplateInfo const *src)
{
  bool ok = mergeParameterLists(prior, dest->params, src->params);

  // sync up the inherited parameters too
  ObjListIterNC<InheritedTemplateParams> destIter(dest->inheritedParams);
  ObjListIter<InheritedTemplateParams> srcIter(src->inheritedParams);

  for (; !destIter.isDone() && !srcIter.isDone();
         destIter.adv(), srcIter.adv()) {
    if (!mergeParameterLists(prior, destIter.data()->params, srcIter.data()->params)) {
      ok = false;
    }
  }

  if (!destIter.isDone() || !srcIter.isDone()) {
    // TODO: expand this error message
    error("differing number of template parameter lists");
    ok = false;
  }
  
  return ok;
}


bool Env::isomorphicTypes(Type *a, Type *b)
{
  MatchTypes match(tfac, MatchTypes::MM_ISO);
  return match.match_Type(a, b);
}


#if 0     // not using ...
// compute a map from the names used in the parameter lists of 'source'
// to those used in the lists of 'target'
void Env::computeRenamingMap(StringRefMap &map, TemplateInfo *source, TemplateInfo *target)
{
  xfailure("do not use this");

  // main params
  computeRenamingMap(map, source->params, target->params);

  // inherited params
  ObjListIterNC<InheritedTemplateParams> srcIter(source->inheritedParams);
  ObjListIterNC<InheritedTemplateParams> tgtIter(target->inheritedParams);
  for (; !srcIter.isDone() && !tgtIter.isDone();
         srcIter.adv(), tgtIter.adv()) {
    // check that the params are inherited from same thing (this check
    // might be too strict...)
    xassert(srcIter.data()->enclosing == tgtIter.data()->enclosing);

    computeRenamingMap(map, srcIter.data()->params, tgtIter.data()->params);
  }
  xassert(srcIter.isDone() && tgtIter.isDone());    // should be same length
}

void Env::computeRenamingMap(StringRefMap &map, SObjList<Variable> &source, SObjList<Variable> &target)
{
  SObjListIter<Variable> srcIter(source);
  SObjListIter<Variable> tgtIter(target);
  for (; !srcIter.isDone() && !tgtIter.isDone();
         srcIter.adv(), tgtIter.adv()) {
    StringRef s = srcIter.data()->name;
    StringRef t = tgtIter.data()->name;
    if (s != t) {
      // map 's' to 't'
      StringRef existing = map.get(s);
      if (!existing) {
        map.add(s, t);
      }
      else {
        // if already mapped, should be mapped to same thing
        // (TODO: this should probably be a user error; make a test
        // case to demonstrate)
        xassert(existing == t);
      }
    }
  }
  xassert(srcIter.isDone() && tgtIter.isDone());    // should be same length
}
#endif // 0


Type *Env::applyArgumentMapToType(STemplateArgumentCMap &map, Type *origSrc)
{
  // my intent is to not modify 'origSrc', so I will use 'src', except
  // when I decide to return what I already have, in which case I will
  // use 'origSrc'
  Type const *src = origSrc;

  switch (src->getTag()) {
    default: xfailure("bad tag");

    case Type::T_ATOMIC: {
      CVAtomicType const *scat = src->asCVAtomicTypeC();
      Type *ret = applyArgumentMapToAtomicType(map, scat->atomic, scat->cv);
      if (!ret) {
        return origSrc;      // use original
      }
      else {
        return ret;
      }
    }

    case Type::T_POINTER: {
      PointerType const *spt = src->asPointerTypeC();
      return tfac.makePointerType(SL_UNKNOWN, spt->cv,
        applyArgumentMapToType(map, spt->atType));
    }

    case Type::T_REFERENCE: {
      ReferenceType const *srt = src->asReferenceTypeC();
      return tfac.makeReferenceType(SL_UNKNOWN,
        applyArgumentMapToType(map, srt->atType));
    }

    case Type::T_FUNCTION: {
      FunctionType const *sft = src->asFunctionTypeC();
      FunctionType *rft = tfac.makeFunctionType(SL_UNKNOWN,
        applyArgumentMapToType(map, sft->retType));

      // copy parameters
      SFOREACH_OBJLIST(Variable, sft->params, iter) {
        Variable const *sp = iter.data();
        Variable *rp = makeVariable(sp->loc, sp->name,
          applyArgumentMapToType(map, sp->type), sp->flags);
        rft->addParam(rp);
      }
      rft->doneParams();

      rft->flags = sft->flags;

      if (rft->exnSpec) {
        // TODO: it's not all that hard, I just want to skip it for now
        xfailure("unimplemented: exception spec on template function where "
                 "defn has differing parameters than declaration");
      }

      return rft;
    }

    case Type::T_ARRAY: {
      ArrayType const *sat = src->asArrayTypeC();
      return tfac.makeArrayType(SL_UNKNOWN,
        applyArgumentMapToType(map, sat->eltType), sat->size);
    }

    case Type::T_POINTERTOMEMBER: {
      PointerToMemberType const *spmt = src->asPointerToMemberTypeC();
      
      // slightly tricky mapping the 'inClassNAT' since we need to make
      // sure the mapped version is still a NamedAtomicType
      Type *retInClassNAT =
        applyArgumentMapToAtomicType(map, spmt->inClassNAT, CV_NONE);
      if (!retInClassNAT) {
        // use original 'spmt->inClassNAT'
        return tfac.makePointerToMemberType(SL_UNKNOWN,
          spmt->inClassNAT,
          spmt->cv,
          applyArgumentMapToType(map, spmt->atType));
      }
      else if (!retInClassNAT->isNamedAtomicType()) {
        return error(stringc << "during template instantiation: type `" <<
                                retInClassNAT->toString() <<
                                "' is not suitable as the class in a pointer-to-member");
      }
      else {
        return tfac.makePointerToMemberType(SL_UNKNOWN,
          retInClassNAT->asNamedAtomicType(),
          spmt->cv,
          applyArgumentMapToType(map, spmt->atType));
      }
    }
  }
}

Type *Env::applyArgumentMapToAtomicType
  (STemplateArgumentCMap &map, AtomicType *origSrc, CVFlags srcCV)
{
  AtomicType const *src = origSrc;

  if (src->isTypeVariable()) {
    TypeVariable const *stv = src->asTypeVariableC();

    STemplateArgument const *replacement = map.get(stv->name);
    if (!replacement) {
      xfailure(stringc << "applyArgumentMapToAtomicType: the type name `"
                       << stv->name << "' is not bound");
    }
    else if (!replacement->isType()) {
      xfailure(stringc << "applyArgumentMapToAtomicType: the type name `"
                       << stv->name << "' is bound to a non-type argument");
    }

    // take what we got and apply the cv-flags that were associated
    // with the type variable, e.g. "T const" -> "int const"
    Type *ret = tfac.applyCVToType(SL_UNKNOWN, srcCV,
                                   replacement->getType(), NULL /*syntax*/);
    if (!ret) {
      return error(stringc << "during template instantiation: type `" <<
                              replacement->getType() << "' cannot be cv-qualified");
    }
    else {
      return ret;     // good to go
    }
  }

  else if (src->isPseudoInstantiation()) {
    PseudoInstantiation const *spi = src->asPseudoInstantiationC();

    // build a concrete argument list, so we can make a real instantiation
    ObjList<STemplateArgument> args;
    FOREACH_OBJLIST(STemplateArgument, spi->args, iter) {
      STemplateArgument const *sta = iter.data();
      if (sta->isType()) {
        STemplateArgument *rta = new STemplateArgument;
        rta->setType(applyArgumentMapToType(map, sta->getType()));
        args.append(rta);
      }
      else {
        // TODO: need mapping for non-type params too, but that
        // requires first fixing the fact that we don't properly
        // represent them
        args.append(sta->shallowClone());
      }
    }

    // instantiate the class with our newly-created arguments
    Variable *instClass = 
      instantiateClassTemplate(loc(), spi->primary->typedefVar, args);
    if (!instClass) {
      instClass = errorVar;    // error already reported; this is recovery
    }

    // apply the cv-flags and return it
    return tfac.applyCVToType(SL_UNKNOWN, srcCV,
                              instClass->type, NULL /*syntax*/);
  }

  else {
    // all others do not refer to type variables; returning NULL
    // here means to use the original unchanged
    return NULL;
  }
}


#if 0       // not using
void Env::applyVariableMapToTemplateScopes(StringRefMap &map)
{
  xfailure("do not use this");

  FOREACH_OBJLIST_NC(Scope, scopes, iter) {
    Scope *s = iter.data();

    if (s->isTemplateParamScope()) {
      s->applyVariableMap(map);
    }
  }
}

void Scope::applyVariableMap(StringRefMap &map)
{
  for (StringSObjDict<Variable>::Iter iter(variables);
       !iter.isDone(); iter.next()) {
    // holy crap!  I'm storing strings in my scopes!  oh well....
    // TODO: change to using PtrMap
    Variable *v = iter.value();
    StringRef replacement = map.get(v->name);
    if (replacement) {
      // leave the name intact, but change which TypeVariable it
      // maps to
      xassert(v->type->isTypeVariable());
      TRACE("template", "in " << desc() <<
                        ", changing binding for type var " << v->name <<
                        " from " << v->type->asTypeVariable()->name <<
                        " to " << replacement);

      xfailure("oops, need a tfac ...");
      //v->type = new TypeVariable(replacement);
    }
  }
}
#endif // 0


// Get or create an instantiation Variable for a function template.
// Note that this does *not* instantiate the function body; instead,
// instantiateFunctionBody() has that responsibility.
Variable *Env::instantiateFunctionTemplate
  (SourceLoc loc,                              // location of instantiation request
   Variable *primary,                          // template primary to instantiate
   SObjList<STemplateArgument> const &sargs)   // arguments to apply to 'primary'
{
  TemplateInfo *primaryTI = primary->templateInfo();
  xassert(primaryTI->isPrimary());

  // look for a (complete) specialization that matches
  Variable *spec = findCompleteSpecialization(primaryTI, sargs);
  if (spec) {
    return spec;      // use it
  }

  // look for an existing instantiation that has the right arguments
  Variable *inst = findInstantiation(primaryTI, sargs);
  if (inst) {
    return inst;      // found it; that's all we need
  }

  // since we didn't find an existing instantiation, we have to make
  // one from scratch
  TRACE("template", "instantiating func decl: " <<
                    primary->fullyQualifiedName() << sargsToString(sargs));

  // I don't need this, right?
  // isolate context
  //InstantiationContextIsolator isolator(*this, loc);

  // bind the parameters in an STemplateArgumentMap
  STemplateArgumentCMap map;
  bindParametersInMap(map, primaryTI, sargs);

  // compute the type of the instantiation by applying 'map' to
  // the templatized type
  Type *instType = applyArgumentMapToType(map, primary->type);

  // create the representative Variable
  inst = makeInstantiationVariable(primary, instType);

  // TODO: fold the following three activities into
  // 'makeInstantiationVariable'

  // annotate it with information about its templateness
  TemplateInfo *instTI = new TemplateInfo(loc, inst);
  instTI->copyArguments(sargs);

  // insert into the instantiation list of the primary
  primaryTI->addInstantiation(inst);

  // this is an instantiation
  xassert(instTI->isInstantiation());

  return inst;
}

Variable *Env::instantiateFunctionTemplate
  (SourceLoc loc,
   Variable *primary,
   ObjList<STemplateArgument> const &sargs)
{
  return instantiateFunctionTemplate(loc, primary,
    objToSObjListC(sargs));
}


Variable *Env::findCompleteSpecialization(TemplateInfo *tinfo,
                                          SObjList<STemplateArgument> const &sargs)
{
  SFOREACH_OBJLIST_NC(Variable, tinfo->specializations, iter) {
    TemplateInfo *instTI = iter.data()->templateInfo();
    if (instTI->equalArguments(tfac /*why needed?*/, sargs)) {
      return iter.data();      // found it
    }
  }
  return NULL;                 // not found
}


Variable *Env::findInstantiation(TemplateInfo *tinfo,
                                 SObjList<STemplateArgument> const &sargs)
{
  if (tinfo->isCompleteSpec()) {
    xassertdb(tinfo->equalArguments(tfac /*?*/, sargs));
    return tinfo->var;
  }

  SFOREACH_OBJLIST_NC(Variable, tinfo->instantiations, iter) {
    TemplateInfo *instTI = iter.data()->templateInfo();
    if (instTI->equalArguments(tfac /*why needed?*/, sargs)) {
      return iter.data();      // found it
    }
  }
  return NULL;                 // not found
}


// make a Variable with type 'type' that will be an instantiation
// of 'templ'
Variable *Env::makeInstantiationVariable(Variable *templ, Type *instType)
{
  Variable *inst = makeVariable(templ->loc, templ->name, instType, templ->flags);
  inst->access = templ->access;
  inst->scope = templ->scope;
  inst->scopeKind = templ->scopeKind;
  return inst;
}


void Env::bindParametersInMap(STemplateArgumentCMap &map, TemplateInfo *tinfo,
                              SObjList<STemplateArgument> const &sargs)
{
  SObjListIter<STemplateArgument> argIter(sargs);

  // inherited parameters
  FOREACH_OBJLIST(InheritedTemplateParams, tinfo->inheritedParams, iter) {
    bindParametersInMap(map, iter.data()->params, argIter);
  }

  // main parameters
  bindParametersInMap(map, tinfo->params, argIter);
  
  if (!argIter.isDone()) {
    error(stringc << "too many template arguments supplied for "
                  << tinfo->var->name);
  }
}

void Env::bindParametersInMap(STemplateArgumentCMap &map,
                              SObjList<Variable> const &params,
                              SObjListIter<STemplateArgument> &argIter)
{
  SFOREACH_OBJLIST(Variable, params, iter) {
    Variable const *param = iter.data();

    if (map.get(param->name)) {
      error(stringc << "template parameter `" << param->name <<
                       "' occurs more than once");
    }
    else if (argIter.isDone()) {
      error(stringc << "no template argument supplied for parameter `" <<
                       param->name << "'");
    }
    else {
      map.add(param->name, argIter.data());
    }

    if (!argIter.isDone()) {
      argIter.adv();
    }
  }
}


// given a CompoundType that is a template (primary or partial spec),
// yield a PseudoInstantiation of that template with its own params
Type *Env::pseudoSelfInstantiation(CompoundType *ct, CVFlags cv)
{
  TemplateInfo *tinfo = ct->typedefVar->templateInfo();
  xassert(tinfo);     // otherwise why are we here?

  PseudoInstantiation *pi = new PseudoInstantiation(
    tinfo->getPrimary()->var->type->asCompoundType());   // awkward...

  if (tinfo->isPrimary()) {
    // 14.6.1 para 1

    // I'm guessing we just use the main params, and not any
    // inherited params?
    SFOREACH_OBJLIST_NC(Variable, tinfo->params, iter) {
      Variable *param = iter.data();

      // build a template argument that just refers to the template
      // parameter
      STemplateArgument *sta = new STemplateArgument;
      if (param->type->isTypeVariable()) {
        sta->setType(param->type);
      }
      else {
        // TODO: this is wrong, like it is basically everywhere else
        // we use STA_REFERENCE ...
        sta->setReference(param);
      }

      pi->args.append(sta);
    }
  }

  else /*partial spec*/ {
    // 14.6.1 para 2
    xassert(tinfo->isPartialSpec());

    // use the specialization arguments
    copyTemplateArgs(pi->args, objToSObjListC(tinfo->arguments));
  }

  return makeCVAtomicType(ct->typedefVar->loc, pi, cv);
}


Variable *Env::makeExplicitFunctionSpecialization
  (SourceLoc loc, DeclFlags dflags, PQName *name, FunctionType *ft)
{
  // find the overload set
  Variable *ovlVar = lookupPQVariable(name, LF_TEMPL_PRIMARY);
  if (!ovlVar) {
    error(stringc << "cannot find primary `" << name->toString() 
                  << "' to specialize");
    return NULL;
  }

  // find last component of 'name' so we can see if template arguments
  // are explicitly supplied
  PQ_template *pqtemplate = NULL;
  SObjList<STemplateArgument> nameArgs;
  if (name->getUnqualifiedName()->isPQ_template()) {
    pqtemplate = name->getUnqualifiedName()->asPQ_template();
    templArgsASTtoSTA(pqtemplate->args, nameArgs);
  }

  // get set of overloaded names (might be singleton)
  SObjList<Variable> set;
  ovlVar->getOverloadList(set);

  // look for a template member of the overload set that can
  // specialize to the type 'ft' and whose resulting parameter
  // bindings are 'sargs' (if supplied)
  Variable *best = NULL;     
  Variable *ret = NULL;
  SFOREACH_OBJLIST_NC(Variable, set, iter) {
    Variable *primary = iter.data();
    if (!primary->isTemplate()) continue;

    // can this element specialize to 'ft'?
    MatchTypes match(tfac, MatchTypes::MM_BIND);
    if (match.match_Type(ft, primary->type)) {
      // yes; construct the argument list that specializes 'primary'
      TemplateInfo *primaryTI = primary->templateInfo();
      SObjList<STemplateArgument> specArgs;
      
      if (primaryTI->inheritedParams.isNotEmpty()) {              
        // two difficulties:
        //   - both the primary and specialization types might refer
        //     to inherited type varibles, but MM_BIND doesn't want
        //     type variables occurring on both sides
        //   - I want to compute right now the argument list that
        //     specializes primary, but then later I will want the
        //     full argument list for making the TemplateInfo
        xfailure("unimplemented: specializing a member template");
      }
                            
      // just use the main (not inherited) parameters...
      SFOREACH_OBJLIST_NC(Variable, primaryTI->params, paramIter) {
        Variable *param = paramIter.data();

        // get the binding
        STemplateArgument const *binding = match.bindings.getVar(param->name);
        
        // remember it (I promise not to modify it...)
        specArgs.append(const_cast<STemplateArgument*>(binding));
      }
      
      // does the inferred argument list match 'nameArgs'?
      if (pqtemplate && 
          !equalArgumentLists(tfac, nameArgs, specArgs)) {
        // no match, go on to the next primary
        continue;
      }

      // ok, found a suitable candidate
      if (best) {
        error(stringc << "ambiguous specialization, could specialize `"
                      << best->type->toString() << "' or `"
                      << primary->type->toString()
                      << "'; use explicit template arguments to disambiguate",
                      EF_STRONG);
        // error recovery: use 'best' anyway
        break;
      }
      best = primary;

      // build a Variable to represent the specialization
      ret = makeSpecializationVariable(loc, dflags, primary, ft, specArgs);
      TRACE("template", "complete function specialization of " <<
                        primary->type->toCString(primary->fullyQualifiedName()) <<
                        ": " << ret->name << sargsToString(specArgs));
    } // initial candidate match check
  } // candidate loop

  if (!ret) {
    error("specialization does not match any function template", EF_STRONG);
    return NULL;
  }

  return ret;
}


Variable *Env::makeSpecializationVariable
  (SourceLoc loc, DeclFlags dflags, Variable *templ, Type *type,
   SObjList<STemplateArgument> const &args)
{
  // make the Variable
  Variable *spec = makeVariable(loc, templ->name, type, dflags);
  spec->access = templ->access;
  spec->scope = templ->scope;
  spec->scopeKind = templ->scopeKind;

  // make & attach the TemplateInfo
  TemplateInfo *ti = new TemplateInfo(loc, spec);
  ti->copyArguments(args);

  // attach to the template
  templ->templateInfo()->addSpecialization(spec);

  // this is a specialization
  xassert(ti->isSpecialization());

  return spec;
}


// this is for 14.7.1 para 4 only
void Env::ensureClassBodyInstantiated(CompoundType *ct)
{
  if (!ct->isComplete() && ct->isInstantiation()) {
    instantiateClassBody(ct->typedefVar);
  }
}


// ------------------- InstantiationContextIsolator -----------------------
InstantiationContextIsolator::InstantiationContextIsolator(Env &e, SourceLoc loc)
  : env(e),
    origNestingLevel(e.disambiguationNestingLevel)
{
  env.instantiationLocStack.push(loc);
  env.disambiguationNestingLevel = 0;
  origErrors.takeMessages(env.errors);
}

InstantiationContextIsolator::~InstantiationContextIsolator()
{
  env.instantiationLocStack.pop();
  env.disambiguationNestingLevel = origNestingLevel;

  // where do the newly-added errors, i.e. those from instantiation,
  // which are sitting in 'env.errors', go?
  if (env.hiddenErrors) {
    // shuttle them around to the hidden message list
    env.hiddenErrors->takeMessages(env.errors);
  }
  else {
    // put them at the end of the original errors, as if we'd never
    // squirreled away any messages
    origErrors.takeMessages(env.errors);
  }
  xassert(env.errors.isEmpty());

  // now put originals back into env.errors
  env.errors.takeMessages(origErrors);
}


// EOF
