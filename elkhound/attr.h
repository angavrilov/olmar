// attr.h
// represent attributes attached to GLR parse tree nodes

#ifndef __ATTR_H
#define __ATTR_H

#include "str.h"       // string
#include "objlist.h"   // ObjList
#include "util.h"      // OSTREAM_OPERATOR

// set of attributes attached to a GLR parse tree node
class Attributes {
private:    // types
  // an entry in the dictionary of attributes
  class Entry {
  public:
    string name;       // name of attribute
    int value;         // value of attribute (all are ints for now)

  public:
    Entry(char const *n, int v)
      : name(n), value(v) {}
    ~Entry();

    void print(ostream &os) const;       // format: name=val
    OSTREAM_OPERATOR(Entry)
    								     
    // for sorting and attr-set comparison
    static int compare(Entry const *left, Entry const *right, void*);
  };

private:    // data
  // dictionary or map
  ObjList<Entry> dict;

private:    // funcs
  Entry const *findEntryC(char const *name) const;
  Entry *findEntry(char const *name)
    { return const_cast<Entry*>(findEntryC(name)); }

public:     // funcs
  Attributes();
  ~Attributes();

  // selectors
  bool hasEntry(char const *name) const;           // true if the entry has a mapping
  int get(char const *name) const;                 // return value; must exist

  // mutators
  void set(char const *name, int value);           // creates if necessary
  void addEntry(char const *name, int value);      // add a new entry; must not already exist
  void changeEntry(char const *name, int value);   // must already exist
  void removeEntry(char const *name);              // remove the entry; must exist beforehand

  // whole attr-set at-a-time
  bool operator==(Attributes const &obj) const;
  bool operator!=(Attributes const &obj) const { return !operator==(obj); }

  // copy 'obj' to 'this', and remove all attributes from 'obj'
  void destructiveEquals(Attributes &obj);

  // debug printing
  void print(ostream &os) const;                   // format: { attr1=val1, attr2=val2, ... }
  OSTREAM_OPERATOR(Attributes)
};

#endif // __ATTR_H
