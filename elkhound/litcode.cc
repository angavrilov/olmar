// litcode.cc
// code for litcode.h

#include "litcode.h"      // this module


// ----------------- LiteralCode -------------------
LiteralCode::~LiteralCode()
{}


// ------------------ LitCodeDict ------------------
LitCodeDict::LitCodeDict()
{}

LitCodeDict::~LitCodeDict()
{}


#if 0
bool getNamesHelper(string const &key, LiteralCode const *, void *extra)
{
  SObjList<string> *names = (SObjList<string>*)extra;
  names->append(const_cast<string*>(&key));
  return false;   // keep going
}

void LitCodeDict::getNames(SObjList</*const*/ string> &names) const
{
  foreachC(getNamesHelper, &names);
}
#endif // 0
