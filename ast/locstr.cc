// locstr.cc
// code for locstr.h

#include "locstr.h"     // this module

LocString::LocString()
  : SourceLocation(),
    str(NULL)           // problem with "" is we don't have the string table here..
{}

LocString::LocString(LocString const &obj)
  : SourceLocation(obj),
    str(obj.str)
{}

LocString::LocString(SourceLocation const &loc, StringRef s)
  : SourceLocation(loc),
    str(s)
{}

LocString::LocString(FileLocation const &floc, SourceFile *f, StringRef s)
  : SourceLocation(floc, f),
    str(s)
{}

LocString::LocString(LocString *obj)
  : SourceLocation(*obj),
    str(obj->str)
{
  delete obj;
}


LocString::LocString(Flatten&)
  : str(NULL)
{}

void LocString::xfer(Flatten &flat)
{
  xassert(flattenStrTable);
  flattenStrTable->xfer(flat, str);
}


bool LocString::equals(char const *other) const
{
  if (!str) {
    return !other;                            // equal if both null
  }
  else {
    return other && 0==strcmp(str, other);    // or same contents
  }
}

string toString(LocString const &s)
{
  return string(s.str);
}
