// variable.cc            see license.txt for copyright and terms of use
// code for variable.h

#include "variable.h"      // this module
#include "template.h"      // Type, TemplateInfo, etc.
#include "trace.h"         // tracingSys


// ---------------------- SomeTypeVarNotInTemplParams_Pred --------------------

// existential search for a type variable that is not in the template
// parameters
class SomeTypeVarNotInTemplParams_Pred : public TypePred {
  TemplateInfo *ti;
  public:
  SomeTypeVarNotInTemplParams_Pred(TemplateInfo *ti0) : ti(ti0) {}
  virtual bool operator() (Type const *t);
  virtual ~SomeTypeVarNotInTemplParams_Pred() {}
};

bool SomeTypeVarNotInTemplParams_Pred::operator() (Type const *t0)
{
  // other tests on 't' seem to want a non-const version
  Type *t = const_cast<Type*>(t0);

  if (!t->isCVAtomicType()) return false;
  CVAtomicType *cv = t->asCVAtomicType();

  if (cv->isCompoundType()) {
    CompoundType *cpd = cv->asCompoundType();
    // recurse on all of the arugments of the template instantiation
    // if any
    if (cpd->templateInfo()) {
      FOREACH_OBJLIST_NC(STemplateArgument, cpd->templateInfo()->arguments, iter) {
        STemplateArgument *sta = iter.data();
        if (sta->isType()) {
          if (sta->getType()->anyCtorSatisfies(*this)) return true;
        }
      }
    }
    return false;
  }

  if (cv->isTypeVariable()) {
    // check that this tvar occurs in the parameters list of the
    // template info
    Variable *tvar = cv->asTypeVariable()->typedefVar;
    if (ti->hasSpecificParameter(tvar)) {
      return false;
    }
    return true;
  }

  return false;                 // some other type of compound type
};


// ---------------------- Variable --------------------
Variable::Variable(SourceLoc L, StringRef n, Type *t, DeclFlags f)
  : loc(L),
    name(n),
    type(t),
    flags(f),
    value(NULL),
    defaultParamType(NULL),
    funcDefn(NULL),
    overload(NULL),
    usingAlias(NULL),
    access(AK_PUBLIC),
    scope(NULL),
    scopeKind(SK_UNKNOWN),
    templInfo(NULL)
{                 
  if (!isNamespace()) {
    xassert(type);
  }
}

Variable::~Variable()
{}


void Variable::setFlagsTo(DeclFlags f)
{
  // this method is the one that gets to modify 'flags'
  const_cast<DeclFlags&>(flags) = f;
}


bool Variable::isUninstTemplateMember() const
{
  if (isTemplate() &&
      !templateInfo()->isCompleteSpecOrInstantiation()) {
    return true;
  }
  return scope && scope->isWithinUninstTemplate();
}


bool Variable::isTemplate(bool considerInherited) const
{
  return templateInfo() &&
         templateInfo()->hasParametersEx(considerInherited);
}

bool Variable::isTemplateFunction(bool considerInherited) const
{
  return type &&
         type->isFunctionType() &&
         isTemplate(considerInherited) &&
         !hasFlag(DF_TYPEDEF);
}

bool Variable::isTemplateClass(bool considerInherited) const
{
  return hasFlag(DF_TYPEDEF) &&
         type->isCompoundType() &&
         isTemplate(considerInherited);
}


bool Variable::isInstantiation() const
{
  return templInfo && templInfo->isInstantiation();
}


TemplateInfo *Variable::templateInfo() const
{
  return templInfo;
}

void Variable::setTemplateInfo(TemplateInfo *templInfo0)
{
  templInfo = templInfo0;
  xassert(!(templInfo && notQuantifiedOut()));
  
  // complete the association
  if (templInfo) {
    // I am the method allowed to change TemplateInfo::var
    const_cast<Variable*&>(templInfo->var) = this;
  }
  else {
    // this happens when we're not in a template at all, but the
    // parser just takes the pending template info (which is NULL)
    // and passes it in here anyway
  }
}


bool Variable::notQuantifiedOut()
{
  TemplateInfo *ti = templateInfo();
  if (!ti) return false;
  SomeTypeVarNotInTemplParams_Pred pred(ti);
  return type->anyCtorSatisfies(pred);
}


void Variable::gdb() const
{
  cout << toString() << endl;
}

string Variable::toString() const
{
  if (Type::printAsML) {
    return toMLString();
  }
  else {
    return toCString();
  }
}


string Variable::toCString() const
{
  // as an experiment, I'm saying public(field) in the .ast file
  // in a place where the Variable* might be NULL, so I will
  // tolerate a NULL 'this'
  if (this == NULL) {
    return "NULL";
  }

  // The purpose of this method is to print the name and type
  // of this Variable object, in a debugging context.  It is
  // not necessarily intended to print them in a way consistent
  // with the C syntax that might give rise to the Variable.
  // If more specialized printing is desired, do that specialized
  // printing from outside (by directly accessing 'name', 'type',
  // 'flags', etc.).
  return type->toCString(stringc << (name? name : "/*anon*/") << namePrintSuffix());
}


string Variable::toQualifiedString() const
{                             
  string qname;
  if (name) {
    qname = fullyQualifiedName();
  }
  else {
    qname = "/*anon*/";
  }
  return type->toCString(stringc << qname << namePrintSuffix());
}


string Variable::toCStringAsParameter() const
{
  stringBuilder sb;
  if (type->isTypeVariable()) {
    // type variable's name, then the parameter's name (if any)
    sb << type->asTypeVariable()->name;
    if (name) {
      sb << " " << name;
    }
  }
  else {
    sb << type->toCString(name);
  }

  if (value) {
    sb << renderExpressionAsString(" = ", value);
  }
  return sb;
}


string Variable::toMLString() const
{
  stringBuilder sb;
  #if USE_SERIAL_NUMBERS
    sb << printSerialNo("v", serialNumber, "-");
  #endif
  char const *name0 = "<no_name>";
  if (name) {
    name0 = name;
  }
  sb << "'" << name0 << "'->" << type->toMLString();
  return sb;
}


// sm: I removed the cache; I'm much more concerned about wasted space
// than wasted time (because the latter is much easier to profile)
string Variable::fullyQualifiedName() const
{
  stringBuilder tmp;
  if (scope && !scope->isGlobalScope()) {
    tmp << scope->fullyQualifiedCName();
  }
  tmp << "::" << name;        // NOTE: not mangled
  return tmp;
}


string Variable::namePrintSuffix() const
{
  return "";
}


OverloadSet *Variable::getOrCreateOverloadSet()
{
  xassert(type);
  xassert(type->isFunctionType());
  if (!overload) {
    overload = new OverloadSet;
    overload->addMember(this);
  }
  return overload;
}


void Variable::getOverloadList(SObjList<Variable> &set)
{
  if (overload) {
    set = overload->set;     // copy the elements
  }
  else {
    set.prepend(this);       // just put me into it
  }
}


int Variable::overloadSetSize() const
{
  return overload? overload->count() : 1;
}


bool Variable::isMemberOfTemplate() const
{ 
  if (!scope) { return false; }
  if (!scope->curCompound) { return false; }

  if (scope->curCompound->isTemplate()) {
    return true;
  }

  // member of non-template class; ask if that class is itself
  // a member of a template
  return scope->curCompound->typedefVar->isMemberOfTemplate();
}


// I'm not sure what analyses' disposition towards usingAlias ought to
// be.  One possibility is to just say they should sprinkle calls to
// skipAlias all over the place, but that's obviously not very nice.
// However, I can't just make the lookup functions call skipAlias,
// since the access control for the *alias* is what's relevant for
// implementing access control restrictions.  Perhaps there should be
// a pass that replaces all Variables in the AST with their skipAlias
// versions?  I don't know yet.  Aliasing is often convenient for the
// programmer but a pain for the analysis.

Variable const *Variable::skipAliasC() const
{
  // tolerate NULL 'this' so I don't have to conditionalize usage
  if (this && usingAlias) {
    return usingAlias->skipAliasC();
  }
  else {
    return this;
  }
}


// this isn't right if either is a set of overloaded functions...
bool sameEntity(Variable const *v1, Variable const *v2)
{
  v1 = v1->skipAliasC();
  v2 = v2->skipAliasC();
  
  if (v1 == v2) {
    return true;
  }
  
  if (v1 && v2 &&                   // both non-NULL
      v1->name == v2->name &&       // same simple name
      v1->hasFlag(DF_EXTERN_C) &&   // both are extern "C"
      v2->hasFlag(DF_EXTERN_C)) {
    // They are the same entity.. unfortunately, outside this
    // rather oblique test, there's no good way for the analysis
    // to know this in advance.  Ideally the tchecker should be
    // maintaining some symbol table of extern "C" names so that
    // it could use the *same* Variable object for multiple
    // occurrences in different namespaces, but I'm too lazy to
    // implement that now.
    return true;
  }
  
  return false;
}


bool Variable::namesTemplateFunction() const
{
  // we are using this to compare template arguments to the
  // preceding name, so we are only interested in the
  // template-ness of the name itself, not any parent scopes
  bool const considerInherited = false;

  if (isOverloaded()) {
    // check amongst all overloaded names; 14.2 is not terribly
    // clear about that, but 14.8.1 para 2 example 2 seems to
    // imply this behavior
    SFOREACH_OBJLIST(Variable, overload->set, iter) {
      if (iter.data()->isTemplate(considerInherited)) {
        return true;
      }
    }
  }

  else if (isTemplate(considerInherited)) {
    return true;
  }

  return false;
}


// --------------------- OverloadSet -------------------
OverloadSet::OverloadSet()
  : set()
{}

OverloadSet::~OverloadSet()
{}


void OverloadSet::addMember(Variable *v)
{
  // dsw: wow, this can happen when you import two names into a
  // namespace.  So the idea is we allow ambiguity and then only
  // report an error at lookup, which is The C++ Way.
//    xassert(!findByType(v->type->asFunctionType()));
  xassert(v->type->isFunctionType());
  set.prepend(v);
}


// The problem with these is they don't work for templatized types
// because they call 'equals', not MatchType.
//
// But I've re-enabled them for Oink ....
#if 1     // obsolete; see Env::findInOverloadSet

Variable *OverloadSet::findByType(FunctionType const *ft, CVFlags receiverCV)
{
  SFOREACH_OBJLIST_NC(Variable, set, iter) {
    FunctionType *iterft = iter.data()->type->asFunctionType();

    // check the parameters other than '__receiver'
    if (!iterft->equalOmittingReceiver(ft)) continue;

    // if 'this' exists, it must match 'receiverCV'
    if (iterft->getReceiverCV() != receiverCV) continue;

    // ok, this is the right one
    return iter.data();
  }
  return NULL;    // not found
}


Variable *OverloadSet::findByType(FunctionType const *ft) {
  return findByType(ft, ft->getReceiverCV());
}
#endif // 0

// EOF
