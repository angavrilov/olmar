// strhash.cc            see license.txt for copyright and terms of use
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


  #ifdef NELSON_HASH
  #warning NELSON_HASH
  // this one is supposed to be better
  /* An excellent string hashing function.
     Adapted from glib's g_str_hash().
     Investigation by Karl Nelson <kenelson@ece.ucdavis.edu>.
     Do a web search for "g_str_hash X31_HASH" if you want to know more. */
  /* update: this is the same function as that described in Kernighan and Pike,
     "The Practice of Programming", section 2.9 */
  unsigned h = 0;
  for (; *key != '\0'; key += 1) {
    h = ( h << 5 ) - h + *key;       // h*31 + *key
  }
  return h;


  #elif WILKERSON_GOLDSMITH_HASH
  #warning WILKERSON_GOLDSMITH_HASH
  // dsw: A slighly faster and likely more random implementation
  // invented in collaboration with Simon Goldsmith.  Note that this
  // one assumes that strings are allocated on word boundaries
  // otherwise you could segfault while going off the end.
  //
  // Thanks to Matt Harren for this.  Supposedly gcc will sometimes
  // recognize this and generate a single rotate instruction 'ROR'.
  // http://groups.google.com/groups?q=rorl+x86&start=10&hl=en&lr=&ie=UTF-8&oe=UTF-8&
  // selm=359954C9.3B354F0%40cartsys.com&rnum=11
  #define ROTATE(n, b) (n >> b) | (n << (32 - b))

  // source of primes: http://www.utm.edu/research/primes/lists/2small/0bit.html
  static unsigned const primeA = (1U<<30) - 173U; // prime
  static unsigned const primeB = (1U<<29) -  43U; // prime
  unsigned h = primeA;
  for (; key[0] && key[1] && key[2] && key[3]; key += 4) {
    h += *( (int*) key );
    h = ROTATE(h, 5);
  }

  if (*key == '\0') goto endHashing;
  h += *key; h = ROTATE(h, 5); key += 1;
  if (*key == '\0') goto endHashing;
  h += *key; h = ROTATE(h, 5); key += 1;
  if (*key == '\0') goto endHashing;
  h += *key; h = ROTATE(h, 5); key += 1;
//    xassert(*key == '\0');

endHashing:
  h *= primeB;
  return h;

  #undef ROTATE


  #else
    #error You must pick a hash function
  #endif // hash function multi-switch
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
