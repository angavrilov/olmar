// vptrmap.h
// map from void* to void*
// based partly on hashtbl.h

// Design considerations:
//
// Keys are pointers to objects.  They are likely to have the same
// high bits (page) and low bits (alignment), and be distinguished
// primarily by the bits in the middle.  No key is NULL.
//
// Deletion is never done.  To delete some mappings you have to
// rebuild the table.
//
// No adversary is present; hash function is fixed in advance.


#ifndef VPTRMAP_H
#define VPTRMAP_H


class VoidPtrMap {
private:     // types
  // single entry in the hash table
  struct Entry {
    void *key;               // NULL only for unused entries
    void *value;             // NULL if key is NULL
  };

private:     // data
  // hash table itself; collision is resolved with double hashing,
  // which is why efficient deletion is impossible
  Entry *hashTable;

  // number of (allocated) slots in the hash table; this is always a
  // power of 2
  int tableSize;
  
  // tableSize always equals 1 << tableSizeBits
  int tableSizeBits;

  // number of mappings (i.e. key!=NULL); always numEntries < tableSize
  int numEntries;

  // number of outstanding iterators; used to check that we don't
  // modify the table while one is out (experimental)
  int iterators;

public:      // data
  // total # of lookups
  static int lookups;
  
  // total # of entries examined during lookups; perfect hashing
  // would yield lookups==probes
  static int probes;

private:     // funcs
  // 'bits' becomes tableSizeBits; alsoe set hashTable and tableSize
  void alloc(int bits);

  // multiplicative hash function
  inline unsigned hashFunc(unsigned multiplier, unsigned key) const;

  // return the first entry in key's probe sequence that has either
  // a NULL key or a key equal to 'key'
  Entry &VoidPtrMap::findEntry(void *key) const;

  // make the table twice as big, and move all the entries into
  // that new table
  void expand();

  // not allowed
  VoidPtrMap(VoidPtrMap&);
  void operator=(VoidPtrMap&);

public:      // funcs
  VoidPtrMap();              // empty map
  ~VoidPtrMap();

  // return # of mapped entries
  int getNumEntries() const { return numEntries; }

  // if this key has a mapping, return it; otherwise, return NULL
  void *get(void *key) const { return findEntry(key).value; }

  // add a mapping from 'key' to 'value'; replaces existing
  // mapping, if any
  void add(void *key, void *value);

  // remove all mappings
  void empty();
  
  
public:      // iterators
  // iterate over all stored values in a VoidPtrMap
  // NOTE: you can't change the table while an iter exists
  class Iter {
  private:      // data
    VoidPtrMap &map;       // table we're iterating over
    int index;             // current slot to return in adv(); -1 when done

  public:       // funcs
    Iter(VoidPtrMap &map);
    ~Iter();

    bool isDone() const { return index < 0; }
    void adv();            // must not be isDone()

    // return information about the currently-referenced table entry
    void *key() const      // key (never NULL)
      { return map.hashTable[index].key; }
    void *value() const    // associated value
      { return map.hashTable[index].value; }
  };
  friend class Iter;
};


#endif // VPTRMAP_H
