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


  #if HASH_ALG == 1
  #warning Nelson hashfunction
  // this one is supposed to be better
  /* An excellent string hashing function.
     Adapted from glib's g_str_hash().
     Investigation by Karl Nelson <kenelson@ece.ucdavis.edu>.
     Do a web search for "g_str_hash X31_HASH" if you want to know more. */
  /* update: this is the same function as that described in Kernighan and Pike,
     "The Practice of Programming", section 2.9 */
  unsigned h = 0;
  for (; *key != '\0'; key += 1) {
    // old
//      h = ( h << 5 ) - h + *key;       // h*31 + *key
    // this one is better because it does the multiply last; otherwise
    // the last byte has no hope of modifying the high order bits
    h += *key;
    h = ( h << 5 ) - h;         // h *= 31
  }
  return h;


  #elif HASH_ALG == 2
  #warning Wilkerson/Goldsmith hashfunction
  // dsw: A slighly faster and likely more random implementation
  // invented in collaboration with Simon Goldsmith.
  //
  // Supposedly gcc will sometimes recognize this and generate a
  // single rotate instruction 'ROR'.  Thanks to Matt Harren for this.
  // http://groups.google.com/groups?q=rorl+x86&start=10&hl=en&lr=&ie=UTF-8&oe=UTF-8&
  // selm=359954C9.3B354F0%40cartsys.com&rnum=11
  // http://www.privacy.nb.ca/cryptography/archives/coderpunks/new/1998-10/0096.html
  #define ROTATE(n, b) (n >> b) | (n << (32 - b))

  // Note that UINT_MAX        = 4294967295U;
  // source of primes: http://www.utm.edu/research/primes/lists/small/small.html
  static unsigned const primeA = 1500450271U;
  static unsigned const primeB = 2860486313U;
  // source of primes: http://www.utm.edu/research/primes/lists/2small/0bit.html
  static unsigned const primeC = (1U<<31) -  99U;
  static unsigned const primeD = (1U<<30) -  35U;

  static int count = 0;
  ++count;

  unsigned h = primeA;

  // Stride a word at a time.  Note that this works best (or works at
  // all) if the string is 32-bit aligned.  This initial 'if' block is
  // to prevent an extra unneeded rotate.
  if (!key[0]) goto end0;
  if (!key[1]) goto end1;
  if (!key[2]) goto end2;
  if (!key[3]) goto end3;
  // No rotate here.
  h += *( (unsigned *) key );
  key += 4;
  while (1) {
    // invariant: when we get here, we are ready to rotate
    if (!key[0]) {h = ROTATE(h, 5); goto end0;}
    if (!key[1]) {h = ROTATE(h, 5); goto end1;}
    if (!key[2]) {h = ROTATE(h, 5); goto end2;}
    if (!key[3]) {h = ROTATE(h, 5); goto end3;}
    h = ROTATE(h, 5);
    h += *( (unsigned *) key ); // on my machine plus is faster than xor
    key += 4;
  }
  xfailure("shouldn't get here");

  // invariant: when we get here we are ready to add
end3:
  h += *key; h = ROTATE(h, 5); key += 1;
end2:
  h += *key; h = ROTATE(h, 5); key += 1;
end1:
  h += *key;                    // No rotate nor increment here.
  #ifndef NDEBUG
    key += 1;                   // this is only needed for the assertion below
  #endif
end0:
  xassertdb(*key=='\0');

  // I will say for now that the property of hash functions that we
  // want is that a change in any input bit has a 50% chance of
  // inverting any output bit.
  //
  // At this point, compare this hash function to the Nelson hash
  // function above.  In Nelson, everytime data is added, the
  // accumulator, 'h', is "stirred" with a multiplication.  Since data
  // is being added at the low byte and since multiplication
  // propagates dependencies towards the high bytes and since after
  // ever add there is at least one multiply, every byte has a chance
  // to affect the value of every bit above the first byte.
  //
  // However, in this hash function we have saved time by simply
  // "tiling" the data across the accumulator and at this point it
  // hasn't been "stirred" at all.  Since most hashvalues are used by
  // modding off some high bits, those bits have never had a chance to
  // affect the final value, so some stirring is needed.  How much
  // "stirring" do we need?
  //
  // Consider the 32-bit word in two halves, H and L.
  //
  // 1) With a single multiply, any bit of L "has a chance" to affect
  // any bit in H.  The reverse is not true.
  h *= primeB;

  // 2) We therefore swap H and L and multiply again.  Now, any bit in
  // H has had a chance to affect any bit in L.  We are not done
  // though, since the high order bits in H have not had a chance to
  // affect the low order bits of H (yes H).  Please note however,
  // that since L affected H and H affected L, the hight order bits of
  // L *have* had a chance to affect the low order bits of L.
  h = ROTATE(h, 16);
  h *= primeC;

  // 3) Therefore we swap H and L and multiply once again.  Now the
  // high order bits of H have had a chance to affect L (in 2) which
  // now can affect H again.  Any bit now has "had a chance" to affect
  // any other bit.
  h = ROTATE(h, 16);
  h *= primeD;

  return h;

  #undef ROTATE


  #else
    #error You must pick a hash function
  #endif // HASH_ALG multi-switch
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
