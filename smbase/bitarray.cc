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

