// litcode.h
// literal embedded code object

#ifndef __LITCODE_H
#define __LITCODE_H

#error This is obsolete; use LocString instead.

#include "fileloc.h"      // SourceLocation, string
#include "strobjdict.h"   // StringObjDict
#include "sobjlist.h"     // SObjList

// a single literal-code object
class LiteralCode {
public:
  SourceLocation loc;     // where the code came from
  string code;            // the code snippet itself

public:
  LiteralCode(SourceLocation const &L, char const *C)
    : loc(L), code(C) {}
  ~LiteralCode();
};


// dictionary of literal-code objects
class LitCodeDict : public StringObjDict<LiteralCode> {
public:
  LitCodeDict();
  ~LitCodeDict();

  // retrieve the list of names
  //void getNames(SObjList</*const*/ string> &names) const;
};


#endif // __LITCODE_H
