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
    access(AK_PUBLIC),
    scope(NULL)
{
  xassert(type);        // (just a stab in the dark debugging effort)
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
  return type->isTemplateFunction() &&
         !hasFlag(DF_TYPEDEF);
}

bool Variable::isTemplateClass() const
{
  return type->isTemplateClass() &&
         hasFlag(DF_TYPEDEF);
}


string Variable::toString() const
{
  // The purpose of this method is to print the name and type
  // of this Variable object, in a debugging context.  It is
  // not necessarily intended to print them in a way consistent
  // with the C syntax that might give rise to the Variable.
  // If more specialized printing is desired, do that specialized
  // printing from outside (by directly accessing 'name', 'type',
  // 'flags', etc.).
  return type->toCString(name? name : "");
}


string Variable::toStringAsParameter() const
{
  stringBuilder sb;
  if (type->isTypeVariable()) {
    // type variable's name, then the parameter's name
    sb << type->asTypeVariable()->name << " " << name;
  }
  else {
    sb << type->toCString(name);
  }

  if (value) {
    sb << renderExpressionAsString(" = ", value);
  }
  return sb;
}


OverloadSet *Variable::getOverloadSet()
{
  if (!overload) {
    overload = new OverloadSet;
    overload->addMember(this);
  }
  return overload;
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


Variable *OverloadSet::findByType(FunctionType const *ft)
{
  SFOREACH_OBJLIST_NC(Variable, set, iter) {
    FunctionType *iterft = iter.data()->type->asFunctionType();

    if (iterft->equalOmittingThisParam(ft)) {
      // ok, this is the right one
      return iter.data();
    }
  }
  return NULL;    // not found
}
