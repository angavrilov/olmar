// asockind.cc
// code for asockind.h

#include "asockind.h"    // this module
#include "xassert.h"     // xassert

string toString(AssocKind k)
{
  static char const * const arr[NUM_ASSOC_KINDS] = {
    "AK_LEFT", "AK_RIGHT", "AK_NONASSOC"
  };
  xassert((unsigned)k < NUM_ASSOC_KINDS);
  return string(arr[k]);
}
