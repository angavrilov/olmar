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
  LocString();
  LocString(LocString const &obj);
  LocString(SourceLocation const &loc, StringRef str);
  LocString(FileLocation const &floc, SourceFile *f, StringRef str);

  LocString(Flatten&);
  void xfer(Flatten &flat);

  // deallocates its argument; intended for convenient use in bison grammar files
  EXPLICIT LocString(LocString *obj);

  LocString& operator= (LocString const &obj)
    { SourceLocation::operator=(obj); str = obj.str; return *this; }

  // string with location info
  string locString() const { return SourceLocation::toString(); }

  // (read-only) string-like behavior
  ostream& operator<< (ostream &os) const
    { return os << str; }
  StringRef strref() const { return str; }
  operator StringRef () const { return str; }
  char operator [] (int index) const { return str[index]; }
  bool equals(char const *other) const;

  // experimenting with allowing 'str' to be null, which is convenient
  // when the string table isn't available
  bool isNull() const { return str == NULL; }
  bool isNonNull() const { return !isNull(); }
};

// yields simply the string, no location info
string toString(LocString const &s);

#endif // LOCSTR_H
