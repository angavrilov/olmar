// cparse.cc            see license.txt for copyright and terms of use
// code for cparse.h

#include <iostream.h>    // cout

#include "cparse.h"      // this module

ParseEnv::ParseEnv(StringTable &table)
  : str(table)
{
  intType = table.add("int");
}

ParseEnv::~ParseEnv()
{}


static char const *identity(void *data)
{
  return (char const*)data;
}

void ParseEnv::enterScope()
{
  types.prepend(new StringHash(identity));
}

void ParseEnv::leaveScope()
{
  delete types.removeAt(0);
}

void ParseEnv::addType(StringRef type)
{
  StringHash *h = types.first();
  if (h->get(type)) {
    // this happens for C++ code which has both the implicit
    // and explicit typedefs (and/or, explicit 'class Foo' mentions
    // in places)
    //cout << "duplicate entry for " << type << " -- will ignore\n";
  }
  else {
    h->add(type, (void*)type);
  }
}

bool ParseEnv::isType(StringRef name)
{
  if (name == intType) {
    return true;
  }

  FOREACH_OBJLIST(StringHash, types, iter) {
    if (iter.data()->get(name)) {
      return true;
    }
  }
  return false;
}
