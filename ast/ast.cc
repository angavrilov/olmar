// ast.cc
// user-written code for ast.ast

#include "ast.hand.h"     // declarations derived from ast.ast
#include "xassert.h"      // xassert

string toString(AccessCtl acc)
{
  char const *arr[] = { "public", "private", "protected" };
  xassert((unsigned)acc < 3);
  return string(arr[acc]);
}
