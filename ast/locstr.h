// locstr.h
// location & string table reference

#ifndef LOCSTR_H
#define LOCSTR_H
                                          
#include <iostream.h>    // ostream
#include <string.h>      // strlen

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
  friend ostream& operator<< (ostream &os, LocString const &loc)
    { return os << loc.str; }
  StringRef strref() const { return str; }
  operator StringRef () const { return str; }
  char operator [] (int index) const { return str[index]; }
  bool equals(char const *other) const;
  int length() const { return strlen(str); }

  // experimenting with allowing 'str' to be null, which is convenient
  // when the string table isn't available
  bool isNull() const { return str == NULL; }
  bool isNonNull() const { return !isNull(); }
};

// yields simply the string, no location info
string toString(LocString const &s);


// useful for constructing literal strings in source code; column
// information isn't available so I just put in "1"
#define LITERAL_LOCSTRING(str)                                   \
  LocString(SourceLocation(FileLocation(__LINE__, 1),            \
                           sourceFileList.open(__FILE__)),       \
            str)



#endif // LOCSTR_H
