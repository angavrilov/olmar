// flatten.cc
// code for flatten.h

// basically, this file provides some reasonable defaults
// assuming we are reading/writing binary files

#include "flatten.h"     // this module
#include "exc.h"         // formatAssert

Flatten::Flatten()
{}

Flatten::~Flatten()
{}


void Flatten::xferChar(char &c)
{
  xferSimple(&c, sizeof(c));
}

void Flatten::xferInt(int &i)
{
  xferSimple(&i, sizeof(i));
}

void Flatten::xferLong(long &l)
{
  xferSimple(&l, sizeof(l));
}

void Flatten::xferBool(bool &b)
{
  xferSimple(&b, sizeof(b));
}



void Flatten::xferCharString(char *&str)
{
  if (writing()) {
    int len = strlen(str);
    writeInt(len);

    // write the null terminator too, as a simple
    // sanity check when reading
    xferSimple(str, len+1);
  }
  else {
    int len = readInt();

    str = new char[len+1];
    xferSimple(str, len+1);
    formatAssert(str[len] == '\0');
  }
}


void Flatten::checkpoint(int code)
{
  if (writing()) {
    writeInt(code);
  }
  else {
    int c = readInt();
    formatAssert(c == code);
  }
}


void Flatten::writeInt(int i)
{
  xassert(writing());
  xferInt(i);
}

int Flatten::readInt()
{
  xassert(reading());
  int i;
  xferInt(i);
  return i;
}

