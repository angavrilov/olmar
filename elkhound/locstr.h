// locstr.h
// location & string table reference

#ifndef LOCSTR_H
#define LOCSTR_H
                                          
#include <iostream.h>    // ostream
#include "strtable.h"    // StringRef
#include "fileloc.h"     // SourceLocation

class LocString : public SourceLocation {
public:    // data
  StringRef str;

public:    // funcs
  LocString(LocString const &obj);
  LocString(SourceLocation const &loc, StringRef str);
  LocString(FileLocation const &floc, SourceFile *f, StringRef str);

  // deallocates its argument; intended for convenient use in bison grammar files
  LocString(LocString *obj);

  LocString& operator= (LocString const &obj)
    { SourceLocation::operator=(obj); str = obj.str; return *this; }

  // (read-only) string-like behavior
  ostream& operator<< (ostream &os) const
    { return os << str; }
};

#endif // LOCSTR_H
