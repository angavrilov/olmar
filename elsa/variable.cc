// variable.cc            see license.txt for copyright and terms of use
// code for variable.h

#include "variable.h"      // this module
#include "cc_type.h"       // Type

#if CC_QUAL
SObjList<Variable> Variable::instances;
#endif

// ---------------------- Variable --------------------
Variable::Variable(SourceLocation const &L, StringRef n, Type const *t, DeclFlags f)
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
#if CC_QUAL
  instances.prepend(this);      // reverse order
#endif
}

Variable::~Variable()
{}


Variable *Variable::deepClone() const {
  Variable *tmp = new Variable(loc
                               ,name // don't see a reason to clone the name
                               ,type->deepClone()
                               ,flags // an enum, so nothing to clone
                               );
  // tmp->overload left as NULL as set by the ctor; not cloned
  tmp->access = access;         // an enum, so nothing to clone
  tmp->scope = scope;           // don't see a reason to clone the scope
  return tmp;
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
