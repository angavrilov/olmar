// str.cpp
// code for str.h
// Scott McPeak, 1995-2000  This file is public domain.

#include "str.h"            // this module

#include <stdlib.h>         // atoi
#include <stdio.h>          // sprintf
#include <ctype.h>          // isspace
#include <string.h>         // strcmp
#include <iostream.h>       // ostream << char*
#include <assert.h>         // assert

#include "xassert.h"        // xassert


// ----------------------- string ---------------------
string::string(char const *src, int length)
{
  s=0;
  setlength(length);       // setlength already has the +1
  memcpy(s, src, length);
  //s[length] = 0;     	   // this was redundant, no?
}


void string::dup(char const *src)
{
  if (!src) {
    s = 0;
  }
  else {
    s = new char[ strlen(src) + 1 ];
    xassert(s);
    strcpy(s, src);
  }
}

void string::kill()
{
  if (s) {
    delete s;
  }
}


int string::length() const
{
  return s? strlen(s) : 0;
}

bool string::contains(char c) const
{
  xassert(s);
  return !!strchr(s, c);
}


string string::substring(int startIndex, int len) const
{
  xassert(startIndex >= 0 &&
          len >= 0 &&
          startIndex + len <= length());

  return string(s+startIndex, len);
}


string &string::setlength(int length)
{
  kill();
  s = new char[ length+1 ];
  xassert(s);
  s[length] = 0;
  s[0] = 0;
  return *this;
}


int string::compareTo(string const &src) const
{
  xassert(s && src.s);
  return strcmp(s, src.s);
}

int string::compareTo(char const *src) const
{
  xassert(s && src);
  return strcmp(s, src);
}


string string::operator&(string const &tail) const
{
  string dest(length() + tail.length());
  strcpy(dest.s, s);
  strcat(dest.s, tail.s);
  return dest;
}

string& string::operator&=(string const &tail)
{
  return *this = *this & tail;
}


void string::readdelim(istream &is, char const *delim)
{
  stringBuilder sb;
  sb.readdelim(is, delim);
  operator= (sb);
}


void string::write(ostream &os) const
{
  if (s != NULL) {
    os << s;     // standard char* writing routine
  }
  else {
    // I still debate the decision to allow s to be NULL,
    // but if I am going to, I should support it behaving
    // like "", I think.. (so I do nothing here)
  }
}


// --------------------- stringBuilder ------------------
stringBuilder::stringBuilder(int len)
{
  init(len);
}

void stringBuilder::init(int initSize)
{
  size = initSize + EXTRA_SPACE + 1;     // +1 to be like string::setlength
  s = new char[size];
  end = s;
  end[initSize] = 0;
}


void stringBuilder::dup(char const *str)
{
  int len = strlen(str);
  init(len);
  strcpy(s, str);
  end += len;
}


stringBuilder::stringBuilder(char const *str)
{
  dup(str);
}


stringBuilder::stringBuilder(char const *str, int len)
{
  init(len);
  memcpy(s, str, len);
  end += len;
}


stringBuilder& stringBuilder::operator=(char const *src)
{
  if (s != src) {
    kill();
    dup(src);
  }
  return *this;
}


stringBuilder& stringBuilder::setlength(int newlen)
{
  kill();
  init(newlen);
  return *this;
}


void stringBuilder::adjustend(char* newend) 
{
  xassert(s <= newend  &&  newend < s + size);

  end = newend;
}


stringBuilder& stringBuilder::operator&= (char const *tail)
{
  int len = strlen(tail);
  ensure(length() + len);

  strcpy(end, tail);
  end += len;
  return *this;
}


void stringBuilder::grow(int newMinLength)
{
  // I want at least EXTRA_SPACE extra
  int newMinSize = newMinLength + EXTRA_SPACE + 1;         // compute resulting allocated size

  // I want to grow at the rate of at least 50% each time
  int suggest = size * 3 / 2;

  // see which is bigger
  newMinSize = max(newMinSize, suggest);

  // remember old length..
  int len = length();

  // realloc s to be newMinSize bytes
  char *temp = new char[newMinSize];
  xassert(len+1 <= newMinSize);    // prevent overrun
  memcpy(temp, s, len+1);          // copy null too
  delete[] s;
  s = temp;

  // adjust other variables
  end = s + len;
  size = newMinSize;
}


stringBuilder& stringBuilder::operator<< (char c)
{
  ensure(length() + 1);
  *(end++) = c;
  *end = 0;
  return *this;
}


#define MAKE_LSHIFT(Argtype, fmt)                        \
  stringBuilder& stringBuilder::operator<< (Argtype arg) \
  {                                                      \
    char buf[60];      /* big enough for all types */    \
    int len = sprintf(buf, fmt, arg);                    \
    assert(len < 60);					 \
    return *this << buf;                                 \
  }

MAKE_LSHIFT(long, "%ld")
MAKE_LSHIFT(unsigned long, "%lu")
MAKE_LSHIFT(double, "%g")
MAKE_LSHIFT(void*, "%p")

#undef MAKE_LSHIFT


stringBuilder& stringBuilder::operator<< (
  stringBuilder::Hex const &h)
{
  char buf[32];        // should only need 19 for 64-bit word..
  int len = sprintf(buf, "0x%lX", h.value);
  assert(len < 20);
  return *this << buf;

  // the length check above isn't perfect because we only find out there is
  // a problem *after* trashing the environment.  it is for this reason I
  // use 'assert' instead of 'xassert' -- the former calls abort(), while the
  // latter throws an exception in anticipation of recoverability
}


// slow but reliable
void stringBuilder::readdelim(istream &is, char const *delim)
{
  char c;
  is.get(c);
  while (!is.eof() &&
         (!delim || !strchr(delim, c))) {
    *this << c;
    is.get(c);
  }
}


// ------------------ test code --------------------
#ifdef TEST_STR

#include <iostream.h>    // cout

void test(unsigned long val)
{
  //cout << stringb(val << " in hex: 0x" << stringBuilder::Hex(val)) << endl;

  cout << stringb(val << " in hex: " << SBHex(val)) << endl;
}

int main()
{
  // for the moment I just want to test the hex formatting
  test(64);
  test(0xFFFFFFFF);
  test(0);
  test((unsigned long)(-1));
  test(1);

  return 0;
}

#endif // TEST_STR

