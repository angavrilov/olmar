// ast.cc
// user-written code for ast.ast

#include "ast.hand.h"     // declarations derived from ast.ast
#include "xassert.h"      // xassert
#include "strutil.h"      // stringToupper

string toString(AccessCtl acc)
{
  char const *arr[] = { "public", "private", "protected" };
  xassert((unsigned)acc < 3);
  return string(arr[acc]);
}

string ASTClass::kindName() const
{
  string ret = stringToupper(name);
  if (ret == name) {
    // this simplemindedly avoids collisions with itself, and I think
    // it even avoids collisions with other classes, since if they would
    // collide with this, they'd collide with themselves too, and hence
    // get an extra "KIND_" prepended..
    ret &= "KIND_";
  }
  return ret;
}
