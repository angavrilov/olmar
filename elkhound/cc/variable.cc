// variable.cc            see license.txt for copyright and terms of use
// code for variable.h

#include "variable.h"      // this module
#include "cc_type.h"       // Type

Variable::Variable(SourceLocation const &L, StringRef n, Type const *t, DeclFlags f)
  : loc(L),
    name(n),
    type(t),
    flags(f)
{
  xassert(type);        // (just a stab in the dark debugging effort)
}

Variable::~Variable()
{}


string Variable::toString() const
{
  // don't care about printing the declflags right now
  return type->toCString(name? name : "");
}
