// strtable.h
// implements a collection of immutable strings (assumed relatively
// short length) with unique representatives

#ifndef STRTABLE_H
#define STRTABLE_H

#include "strhash.h"     // StringHash


// the type of references to strings in a string table; the pointer
// can be used directly in equality comparisons, because several calls
// to 'add' return the same pointer; and it points to the represented
// string (null-terminated), so it can be printed directly, etc.
typedef char const *StringRef;


class StringTable {
private:    // types                                       
  // constants
  enum {
    rackSize = 16000,      // size of one rack
    longThreshold = 1000,  // minimum length of a "long" string
  };

  // some of the strings stored in the table
  struct Rack {
    Rack *next;            // (owner) next rack, if any; for deallocation
    int usedBytes;         // # of bytes of 'data' that are used
    char data[rackSize];   // data where strings are stored

  public:
    Rack(Rack *n) : next(n), usedBytes(0) {}
    int availBytes() const { return rackSize - usedBytes; }
    char *nextByte() { return data + usedBytes; }
  };

  // stores long strings
  struct LongString {
    LongString *next;      // (owner) next long string
    char *data;            // (owner) string data, any length (null terminated)

  public:
    LongString(LongString *n, char *d) : next(n), data(d) {}
  };

private:    // data
  // hash table mapping strings to pointers into one
  // of the string racks
  StringHash hash;

  // linked list of racks; only walked at dealloc time; we add new
  // strings to the first rack, and prepend a new one if necessary;
  // 'racks' is never null
  Rack *racks;

  // similar for long strings
  LongString *longStrings;

private:    // funcs
  // not allowed
  StringTable(StringTable&);
  void operator=(StringTable&);
  void operator==(StringTable&);

  // for mapping data to keys, in the hashtable
  static char const *identity(void *data);

public:     // funcs
  StringTable();
  ~StringTable();

  // add 'src' to the table, if it isn't already there; return a
  // unique representative, such that multiple calls to 'add' with
  // the same string contents will always yield the same value
  StringRef add(char const *src);

  // if 'src' is in the table, return its representative; if not,
  // return NULL
  StringRef get(char const *src) const;
};


#endif // STRTABLE_H
