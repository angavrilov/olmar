// cc_err.cc            see license.txt for copyright and terms of use
// code for cc_err.h

#include "cc_err.h"      // this module


// ----------------- ErrorMsg -----------------
ErrorMsg::~ErrorMsg()
{}


string ErrorMsg::toString() const
{
  return stringc << ::toString(loc)
                 << (isWarning()? ": warning: " : ": error: ")
                 << msg;
}
