// strhash.cc
// code for strhash.h

#include "strhash.h"     // this module
#include "xassert.h"     // xassert

#include <string.h>      // strcmp


StringHash::StringHash(GetKeyFn gk)
  : HashTable((HashTable::GetKeyFn)gk,
              (HashTable::HashFn)coreHash,
              (HashTable::EqualKeyFn)keyCompare)
{}

StringHash::~StringHash()
{}


STATICDEF unsigned StringHash::coreHash(char const *key)
{
  #if 0
  // I pulled this out of my ass.. it's supposed to mimic
  // a linear congruential random number generator
  unsigned val = 0x42e9d115;    // arbitrary
  while (*key != 0) {
    val *= (unsigned)(*key);
    val += 1;
    key++;
  }
  return val;
  #endif // 0

  // this one is supposed to be better
  /* An excellent string hashing function.
     Adapted from glib's g_str_hash().
     Investigation by Karl Nelson <kenelson@ece.ucdavis.edu>.
     Do a web search for "g_str_hash X31_HASH" if you want to know more. */
  unsigned h = 0;
  for (; *key != '\0'; key += 1) {
    h = ( h << 5 ) - h + *key;       // h*31 + *key
  }
  return h;
}


STATICDEF bool StringHash::keyCompare(char const *key1, char const *key2)
{
  return 0==strcmp(key1, key2);
}


// ---------------------- test code --------------------
#ifdef TEST_STRHASH

#include <iostream.h>    // cout
#include <stdlib.h>      // rand
#include "trace.h"       // traceProgress
#include "crc.h"         // crc32

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
  traceAddSys("progress");

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
      hash.selfCheck();
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
        hash.selfCheck();
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
        hash.selfCheck();
      }
    }}
    hash.selfCheck();
    xassert(hash.getNumEntries() == 0);

    // test performance of the hash function
    traceProgress() << "start of hashes\n";
    loopj(1000) {
      loopi(300) {
        StringHash::coreHash(table[i]);
        //crc32((unsigned char*)table[i], strlen(table[i]));
        //crc32((unsigned char*)table[i], 10);
      }
    }
    traceProgress() << "end of hashes\n";

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
