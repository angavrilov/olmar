// attr.h
// represent attributes attached to GLR parse tree nodes

#error This module is obsolete.

#ifndef __ATTR_H
#define __ATTR_H

#include "str.h"       // string
#include "objlist.h"   // ObjList
#include "util.h"      // OSTREAM_OPERATOR

// type of value stored as attribute
typedef int AttrValue;

// how to name attributes
typedef string AttrName;


// set of attributes attached to a GLR parse tree node
class Attributes {
private:    // types
  // an entry in the dictionary of attributes
  class Entry {
  public:
    AttrName name;           // name of attribute
    AttrValue value;         // value of attribute (all are ints for now)

  public:
    Entry(AttrName n, AttrValue v)
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
  Entry const *findEntryC(AttrName name) const;
  Entry *findEntry(AttrName name)
    { return const_cast<Entry*>(findEntryC(name)); }

public:     // funcs
  Attributes();
  ~Attributes();

  // selectors
  bool hasEntry(AttrName name) const;                 // true if the entry has a mapping
  AttrValue get(AttrName name) const;                 // return value; must exist

  // mutators
  void set(AttrName name, AttrValue value);           // creates if necessary
  void addEntry(AttrName name, AttrValue value);      // add a new entry; must not already exist
  void changeEntry(AttrName name, AttrValue value);   // must already exist
  void removeEntry(AttrName name);                    // remove the entry; must exist beforehand

  // whole attr-set at-a-time
  bool operator==(Attributes const &obj) const;
  bool operator!=(Attributes const &obj) const { return !operator==(obj); }

  // copy 'obj' to 'this', and remove all attributes from 'obj'
  void destructiveEquals(Attributes &obj);

  // debug printing
  void print(ostream &os) const;                   // format: { attr1=val1, attr2=val2, ... }
  OSTREAM_OPERATOR(Attributes)

  void selfCheck() const;
};


#endif // __ATTR_H
