// hashtbl.cc
// code for hashtbl.h

#include "hashtbl.h"     // this module
#include "xassert.h"     // xassert

#include <string.h>      // memset


unsigned HashTable::hashFunction(void const *key) const
{
  return coreHashFn(key) % (unsigned)tableSize;
}


HashTable::HashTable(GetKeyFn gk, HashFn hf, EqualKeyFn ek)
  : getKey(gk),
    coreHashFn(hf),
    equalKeys(ek)
{
  makeTable(initialTableSize);
}

HashTable::~HashTable()
{
  delete[] hashTable;
}


void HashTable::makeTable(int size)
{
  hashTable = new void*[size];
  tableSize = size;
  memset(hashTable, 0, sizeof(void*) * tableSize);
  numEntries = 0;
}


int HashTable::getEntry(void const *key) const
{
  int index = hashFunction(key);
  int originalIndex = index;
  for(;;) {
    if (hashTable[index] == NULL) {
      // unmapped
      return index;
    }
    if (equalKeys(key, getKey(hashTable[index]))) {
      // mapped here
      return index;
    }

    // this entry is mapped, but not with this key, i.e.
    // we have a collision -- so just go to the next entry,
    // wrapping as necessary
    index = nextIndex(index);

    // detect infinite looping
    xassert(index != originalIndex);
  }
}


void *HashTable::get(void const *key) const
{
  return hashTable[getEntry(key)];
}


void HashTable::resizeTable(int newSize)
{
  // save old stuff
  void **oldTable = hashTable;
  int oldSize = tableSize;
  int oldEntries = numEntries;

  // make the new table
  makeTable(newSize);

  // move entries to the new table
  for (int i=0; i<oldSize; i++) {
    if (oldTable[i] != NULL) {
      add(getKey(oldTable[i]), oldTable[i]);
      oldEntries--;
    }
  }
  xassert(oldEntries == 0);

  // deallocate the old table
  delete[] oldTable;
}


void HashTable::add(void const *key, void *value)
{
  if (numEntries+1 > tableSize*2/3) {
    // we're over the usage threshold; increase table size
    resizeTable(tableSize * 2 + 1);
  }

  int index = getEntry(key);
  xassert(hashTable[index] == NULL);    // must not be a mapping yet

  hashTable[index] = value;
  numEntries++;
}


void HashTable::remove(void const *key)
{
  if (numEntries-1 < tableSize/5  &&
      tableSize > initialTableSize) {
    // we're below threshold; reduce table size
    resizeTable(tableSize / 2);
  }

  int index = getEntry(key);
  xassert(hashTable[index] != NULL);    // must be a mapping to remove

  // remove this entry
  hashTable[index] = NULL;
  numEntries--;

  // now, if we ever inserted something and it collided with this one,
  // leaving things like this would prevent us from finding that other
  // mapping because the search stops as soon as a NULL entry is
  // discovered; so we must examine all entries that could have
  // collided, and re-insert them
  int originalIndex = index;
  for(;;) {
    index = nextIndex(index);
    xassert(index != originalIndex);    // prevent infinite loops

    if (hashTable[index] == NULL) {
      // we've reached the end of the list of possible colliders
      break;
    }

    // remove this one
    void *data = hashTable[index];
    hashTable[index] = NULL;
    numEntries--;

    // add it back
    add(getKey(data), data);
  }
}


void HashTable::selfCheck() const
{
  int ct=0;
  for (int i=0; i<tableSize; i++) {
    if (hashTable[i] != NULL) {
      checkEntry(i);
      ct++;
    }
  }

  xassert(ct == numEntries);
}

void HashTable::checkEntry(int entry) const
{
  int index = getEntry(getKey(hashTable[entry]));
  int originalIndex = index;
  for(;;) {
    if (index == entry) {
      // the entry lives where it will be found, so that's good
      return;
    }
    if (hashTable[index] == NULL) {
      // the search for this entry would stop before finding it,
      // so that's bad!
      xfailure("checkEntry: entry in wrong slot");
    }

    // collision; keep looking
    index = nextIndex(index);
    xassert(index != originalIndex);
  }
}
