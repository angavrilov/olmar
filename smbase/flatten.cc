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


void Flatten::xferString(string &str)
{
  if (writing()) {
    char *c = str.pchar();
    xferCharString(c);
  }                 
  else {
    char *c;
    xferCharString(c);    
    
    // not perfectly efficient ... but breaking the abstraction
    // is dangerous if 'string' happens to be 'stringBuilder' ..
    str = c;
    delete[] c;
  }
}


void Flatten::xferCharString(char *&str)
{
  if (writing()) {
    int len = strlen(str);
    xferInt(len);

    // write the null terminator too, as a simple
    // sanity check when reading
    xferSimple(str, len+1);
  }
  else {
    int len;
    xferInt(len);

    str = new char[len+1];
    xferSimple(str, len+1);
    formatAssert(str[len] == '\0');
  }
}


void Flatten::checkpoint(int code)
{
  if (writing()) {
    xferInt(code);
  }
  else {
    int c;
    xferInt(c);
    formatAssert(c == code);
  }
}



