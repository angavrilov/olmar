// variable.cc            see license.txt for copyright and terms of use
// code for variable.h

#include "variable.h"      // this module
#include "cc_type.h"       // Type


// ---------------------- Variable --------------------
Variable::Variable(SourceLoc L, StringRef n, Type *t, DeclFlags f)
  : loc(L),
    name(n),
    type(t),
    flags(f),
    value(NULL),
    funcDefn(NULL),
    overload(NULL),
    usingAlias(NULL),
    access(AK_PUBLIC),
    scope(NULL),
    scopeKind(SK_UNKNOWN)
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


bool Variable::isTemplateFunction() const
{
  return type &&
         type->isTemplateFunction() &&
         !hasFlag(DF_TYPEDEF);
}

bool Variable::isTemplateClass() const
{
  return hasFlag(DF_TYPEDEF) &&
         type->isTemplateClass();
}


string Variable::toString() const
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
  return type->toString(stringc << (name? name : "") << namePrintSuffix());
}


string Variable::toStringAsParameter() const
{
  stringBuilder sb;
  if (type->isTypeVariable()) {
    // type variable's name, then the parameter's name
    sb << type->asTypeVariable()->name << " " << name;
  }
  else {
    sb << type->toString(name);
  }

  if (value) {
    sb << renderExpressionAsString(" = ", value);
  }
  return sb;
}


// sm: I removed the cache; I'm much more concerned about wasted space
// than wasted time (because the latter is much easier to profile)
string Variable::fullyQualifiedName() const
{
  stringBuilder tmp;
  if (scope) tmp << scope->fullyQualifiedName();
  tmp << "::" << name;        // NOTE: not mangled
  return tmp;
}


string Variable::namePrintSuffix() const
{
  return "";
}


OverloadSet *Variable::getOverloadSet()
{
  if (!overload) {
    overload = new OverloadSet;
    overload->addMember(this);
  }
  return overload;
}


int Variable::overloadSetSize() const
{
  return overload? overload->count() : 1;
}


// I'm not sure what analyses' disposition towards usingAlias ought to
// be.  One possibility is to just say they should sprinke calls to
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


// --------------------- OverloadSet -------------------
OverloadSet::OverloadSet()
  : set()
{}

OverloadSet::~OverloadSet()
{}


void OverloadSet::addMember(Variable *v)
{
  set.prepend(v);
}


Variable *OverloadSet::findByType(FunctionType const *ft, CVFlags thisCV)
{
  SFOREACH_OBJLIST_NC(Variable, set, iter) {
    FunctionType *iterft = iter.data()->type->asFunctionType();

    // check the parameters other than 'this'
    if (!iterft->equalOmittingThisParam(ft)) continue;
    
    // if 'this' exists, it must match 'thisCV'
    if (iterft->getThisCV() != thisCV) continue;

    // ok, this is the right one
    return iter.data();
  }
  return NULL;    // not found
}
