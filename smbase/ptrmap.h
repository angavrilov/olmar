// ptrmap.h
// map from KEY* to VALUE* for arbitrary types KEY and VALUE
// (neither are owned by the table)

// for const purposes, I regard the mapping itself as the only
// thing that cannot be modified in a "const" map; in particular,
// I allow a non-const VALUE* to be extracted

#ifndef PTRMAP_H
#define PTRMAP_H

#include "vptrmap.h"       // VoidPtrMap


template <class KEY, class VALUE>
class PtrMap {
private:     // data
  // underlying map implementation, around which this class
  // is a type-safe wrapper
  VoidPtrMap map;

public:      // funcs
  PtrMap()                         : map() {}
  ~PtrMap()                        {}

  // query # of mapped entries
  int getNumEntries() const        { return map.getNumEntries(); }
  bool isEmpty() const             { return getNumEntries() == 0; }
  bool isNotEmpty() const          { return !isEmpty(); }

  // if this key has a mapping, return it; otherwise, return NULL
  VALUE *get(KEY const *key) const { return (VALUE*)map.get((void const*)key); }

  // add a mapping from 'key' to 'value'; replaces existing
  // mapping, if any
  void add(KEY *key, VALUE *value) { map.add((void*)key, (void*)value); }

  // remove all mappings
  void empty()                     { map.empty(); }


public:      // iterators
  class Iter {
  private:     // data
    // underlying iterator state
    VoidPtrMap::Iter iter;

  public:      // fucs
    Iter(PtrMap<KEY,VALUE> const &map)   : iter(map.map) {}
    ~Iter()                              {}

    bool isDone() const            { return iter.isDone(); }
    void adv()                     { return iter.adv(); }

    // return information about the currently-referenced table entry
    KEY *key() const               { return (KEY*)iter.key(); }
    VALUE *value() const           { return (VALUE*)iter.value(); }
  };
  friend class Iter;
};


#endif // PTRMAP_H
