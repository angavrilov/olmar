// variable.cc            see license.txt for copyright and terms of use
// code for variable.h

#include "variable.h"      // this module
#include "cc_type.h"       // Type


// ---------------------- Variable --------------------
Variable::Variable(SourceLocation const &L, StringRef n, Type const *t, DeclFlags f)
  : loc(L),
    name(n),
    type(t),
    flags(f),
    overload(NULL),
    access(AK_PUBLIC),
    scope(NULL)
{
  xassert(type);        // (just a stab in the dark debugging effort)
}

Variable::~Variable()
{}

#include <assert.h>
string Variable::toString() const
{
  // don't care about printing the declflags right now

  const char *name0 = name;

  // dsw: FIX: I think this is a mistake.  Down in at least one of the
  // toCString-s that gets called, it is checking for null and
  // printing "/*anon*/".  This defeats that.  Scott says this is OK.
  name0 = name0 ? name0 : "";

  if (strcmp(name0, "constructor-special")==0) {
    name0 = scope->curCompound->name;
    assert(name0);
  }

  return type->toCString(name0);
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
