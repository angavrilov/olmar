// bitarray.cc            see license.txt for copyright and terms of use
// code for bitarray.h

#include "bitarray.h"     // this module
#include "flatten.h"      // Flatten

#include <string.h>       // memset


BitArray::BitArray(int n)
  : numBits(n)
{
  bits = new unsigned char[allocdBytes()];
  clearAll();
}


BitArray::~BitArray()
{
  delete[] bits;
}


BitArray::BitArray(Flatten&)
  : bits(NULL)
{}

void BitArray::xfer(Flatten &flat)
{   
  flat.xferInt(numBits);

  if (flat.reading()) {
    bits = new unsigned char[allocdBytes()];
  }
  flat.xferSimple(bits, allocdBytes());
}


void BitArray::clearAll()
{
  memset(bits, 0, allocdBytes());
}


// ----------------------- BitArray::Iter ------------------------
void BitArray::Iter::adv()
{
  // THIS CODE HAS NOT BEEN TESTED YET

  curBit++;
  
  while (curBit < arr.numBits) {
    if (curBit & 7 == 0) {
      // beginning a new byte; is it entirely empty?
      while (arr.bits[curBit >> 3] == 0) {
        // yes, skip to next
        curBit += 8;
        
        if (curBit >= arr.numBits) {
          return;     // done iterating
        }
      }
    }
             
    // this could be made a little faster by using the trick to scan
    // for the first nonzero bit.. but since I am only going to scan
    // within a single byte, it shouldn't make that much difference
    if (arr.test(curBit)) {
      return;         // found element
    }

    curBit++;
  }
}

