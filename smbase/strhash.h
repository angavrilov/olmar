// strhash.h
// hash table mapping strings to arbitrary pointers, where
// the stored pointers can be used to derive the key, and
// cannot be NULL

#ifndef STRHASH_H
#define STRHASH_H

#include "typ.h"     // STATICDEF

class StringHash {
private:    // types
  // constants
  enum {
    initialTableSize = 33,
  };

public:     // types
  // given a stored data pointer, retrieve the associated key
  typedef char const* (*GetKeyFn)(void *data);

private:    // data
  // data -> key mapper
  GetKeyFn getKey;

  // array of pointers to data, indexed by the key hash value,
  // with collisions resolved by moving to adjacent entries;
  // some entries are NULL, meaning that hash value has no mapping
  void **hashTable;

  // number of slots in the hash table
  int tableSize;

  // number of mapped (non-NULL) entries
  int numEntries;

private:    // funcs
  // disallowed
  StringHash(StringHash&);
  void operator=(StringHash&);
  void operator==(StringHash&);

  // core hash function, which returns a 32-bit integer ready
  // to be mod'ed by the table size
  static unsigned coreHashFunction(char const *key);

  // hash fn for the current table size; always in [0,tableSize-1]
  unsigned hashFunction(char const *key) const;

  // given a collision at 'index', return the next index to try
  int nextIndex(int index) const { return (index+1) % tableSize; }
                                
  // resize the table, transferring all the entries to their
  // new positions
  void resizeTable(int newSize);
  
  // return the index of the entry corresponding to 'key' if it
  // is mapped, or a pointer to the entry that should be filled
  // with its mapping, if unmapped
  int getEntry(char const *key) const;

  // make a new table with the given size
  void makeTable(int size);
       
  // check a single entry for integrity
  void checkEntry(int entry) const;

public:     // funcs
  StringHash(GetKeyFn getKey);
  ~StringHash();

  // return # of mapped entries
  int getNumEntries() const { return numEntries; }

  // if this key has a mapping, return it; otherwise,
  // return NULL
  void *get(char const *key) const;

  // add a mapping from 'key' to 'value'; there must not already
  // be a mapping for this key
  void add(char const *key, void *value);

  // remove the mapping for 'key' -- it must exist
  void remove(char const *key);

  // check the data structure's invariants, and throw an exception
  // if there is a problem
  void selfCheck() const;
};

#endif // STRHASH_H
