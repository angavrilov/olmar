// objmap.h            see license.txt for copyright and terms of use
// variant of ptrmap.h where the values are owned

// for const purposes, as the values are owned, in a const table
// the values cannot be modified

#ifndef OBJMAP_H
#define OBJMAP_H

#include "ptrmap.h"           // PtrMap

template <class KEY, class VALUE>
class ObjMap {
private:    // data
  PtrMap<KEY,VALUE> map;

public:     // funcs
  ObjMap() {}
  ~ObjMap() { empty(); }

  // query # of mapped entries
  int getNumEntries() const               { return map.getNumEntries(); }
  bool isEmpty() const                    { return map.isEmpty(); }
  bool isNotEmpty() const                 { return map.isNotEmpty(); }

  // if this key has a mapping, return it; otherwise, return NULL
  VALUE const *getC(KEY *key) const       { return map.get(key); }
  VALUE *get(KEY *key)                    { return map.get(key); }

  // add a mapping from 'key' to 'value'; must not be mapped already
  void add(KEY *key, VALUE *value)        { xassert(!map.get(key)); map.add(key, value); }
  
  // add with replacement
  void addReplace(KEY *key, VALUE *value)
  {
    VALUE *oldValue = map.get(key);
    if (oldValue) {
      delete oldValue;
    }
    map.add(key, value);
  }

  // remove all mappings, and deallocate all VALUEs
  void empty();


public:      // iterators
  class IterC {
  private:     // data
    // underlying iterator state
    PtrMap<KEY,VALUE>::Iter iter;

  public:      // fucs
    IterC(ObjMap<KEY,VALUE> const &map)   : iter(map.map) {}
    ~IterC()                              {}

    bool isDone() const                   { return iter.isDone(); }
    void adv()                            { return iter.adv(); }

    // return information about the currently-referenced table entry
    KEY *key() const                      { return iter.key(); }
    VALUE const *value() const            { return iter.value(); }
  };
  friend class IterC;

  class Iter {
  private:     // data
    // underlying iterator state
    PtrMap<KEY,VALUE>::Iter iter;

  public:      // fucs
    Iter(ObjMap<KEY,VALUE> &map)          : iter(map.map) {}
    ~Iter()                               {}

    bool isDone() const                   { return iter.isDone(); }
    void adv()                            { return iter.adv(); }

    // return information about the currently-referenced table entry
    KEY *key() const                      { return iter.key(); }
    VALUE *value() const                  { return iter.value(); }
  };
  friend class Iter;
};


template <class KEY, class VALUE>
void ObjMap<KEY,VALUE>::empty()
{ 
  // delete the values; enclose in {} so the iterator goes
  // away before the table is modified
  {
    PtrMap<KEY,VALUE>::Iter iter(map);
    for (; !iter.isDone(); iter.adv()) {
      delete iter.value();
    }
  }

  map.empty();
}


#endif // OBJMAP_H
