// locstr.cc
// code for locstr.h

#include "locstr.h"     // this module

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

