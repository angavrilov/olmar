// strhash.cc
// code for strhash.h

#include "strhash.h"     // this module
#include "xassert.h"     // xassert

#include <string.h>      // strcmp, memset


STATICDEF unsigned StringHash::coreHashFunction(char const *key)
{
  // I pulled this out of my ass.. it's supposed to mimic
  // a linear congruential generator
  unsigned val = 0x42e9d115;    // arbitrary
  while (*key != 0) {
    val *= (unsigned)(*key);
    val += 1;
    key++;
  }
  return val;
}


unsigned StringHash::hashFunction(char const *key) const
{
  return coreHashFunction(key) % (unsigned)tableSize;
}


StringHash::StringHash(GetKeyFn gk)
  : getKey(gk)
{
  makeTable(initialTableSize);
}

StringHash::~StringHash()
{
  delete[] hashTable;
}


void StringHash::makeTable(int size)
{
  hashTable = new void*[size];
  tableSize = size;
  memset(hashTable, 0, sizeof(void*) * tableSize);
  numEntries = 0;
}


int StringHash::getEntry(char const *key) const
{
  int index = hashFunction(key);
  int originalIndex = index;
  for(;;) {
    if (hashTable[index] == NULL) {
      // unmapped
      return index;
    }
    if (0==strcmp(key, getKey(hashTable[index]))) {
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


void *StringHash::get(char const *key) const
{
  return hashTable[getEntry(key)];
}


void StringHash::resizeTable(int newSize)
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


void StringHash::add(char const *key, void *value)
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


void StringHash::remove(char const *key)
{
  if (numEntries-1 < tableSize/5) {
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
  int originalIndex;
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


void StringHash::selfCheck() const
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

void StringHash::checkEntry(int entry) const
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


// ---------------------- test code --------------------
#ifdef TEST_STRHASH

#include <iostream.h>    // cout
#include <stdlib.h>      // rand

char const *id(void *p)
{
  return (char const*)p;
}

char *randomString()
{
  char *ret = new char[11];
  loopi(10) {
    ret[i] = (rand()%26)+'a';
  }
  ret[10]=0;
  return ret;
}

int main()
{
  {
    // fill a table with random strings
    char **table = new char*[300];
    {loopi(300) {
      table[i] = randomString();
    }}

    // insert them all into a hash table
    StringHash hash(id);
    {loopi(300) {
      hash.add(table[i], table[i]);
    }}
    hash.selfCheck();
    xassert(hash.getNumEntries() == 300);

    // verify that they are all mapped properly
    {loopi(300) {
      xassert(hash.get(table[i]) == table[i]);
    }}
    hash.selfCheck();

    // remove every other one
    {loopi(300) {
      if (i%2 == 0) {
        hash.remove(table[i]);
      }
    }}
    hash.selfCheck();
    xassert(hash.getNumEntries() == 150);

    // verify it
    {loopi(300) {
      if (i%2 == 0) {
        xassert(hash.get(table[i]) == NULL);
      }
      else {
        xassert(hash.get(table[i]) == table[i]);
      }
    }}
    hash.selfCheck();

    // remove the rest
    {loopi(300) {
      if (i%2 == 1) {
        hash.remove(table[i]);
      }
    }}
    hash.selfCheck();
    xassert(hash.getNumEntries() == 0);

    // dealloc the test strings
    {loopi(300) {
      delete[] table[i];
    }}
    delete[] table;
  }

  cout << "strhash tests passed\n";
  return 0;
}

#endif // TEST_STRHASH
