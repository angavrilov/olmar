// ohashtbl.h
// hash table that owns the values; uses void* keys
// see hashtbl.h for more detail on the semantics of the member fns

#ifndef OHASHTBL_H
#define OHASHTBL_H

#include "hashtbl.h"       // HashTable

template <class T> class OwnerHashTableIter;

template <class T>
class OwnerHashTable {
public:     // types
  friend class OwnerHashTableIter<T>;

  // see hashtbl.h
  typedef void const* (*GetKeyFn)(T *data);
  typedef unsigned (*HashFn)(void const *key);
  typedef bool (*EqualKeyFn)(void const *key1, void const *key2);

private:    // data
  // inner table that does the hash mapping
  HashTable table;

public:     // funcs
  OwnerHashTable(GetKeyFn gk, HashFn hf, EqualKeyFn ek)
    : table((HashTable::GetKeyFn)gk, hf, ek) {}
  ~OwnerHashTable() { empty(); }

  int getNumEntries() const               { return table.getNumEntries(); }
  T *get(void const *key) const           { return (T*)table.get(key); }
  void add(void const *key, T *value)     { table.add(key, value); }
  T *remove(void const *key)              { return (T*)table.remove(key); }
  void empty();
  void selfCheck() const                  { table.selfCheck(); }
};

template <class T>
void OwnerHashTable<T>::empty()
{
  HashTableIter iter(table);
  for (; !iter.isDone(); iter.adv()) {
    delete (T*)iter.data();
  }
  table.empty();
}


template <class T>
class OwnerHashTableIter {
private:      // data
  HashTableIter iter;      // internal iterator

public:       // funcs
  OwnerHashTableIter(OwnerHashTable<T> &table)
    : iter(table.table) {}

  bool isDone() const      { return iter.isDone(); }
  void adv()               { iter.adv(); }
  T *data()                { return (T*)iter.data(); }
};

#endif // OHASHTBL_H
