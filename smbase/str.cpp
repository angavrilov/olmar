// str.cpp
// code for str.h
// Scott McPeak, 1995-1999  This file is public domain.

#include "str.h"            // this module

#include <stdlib.h>         // atoi
#include <stdio.h>          // sprintf
#include <ctype.h>          // isspace
#include <string.h>         // strcmp
#include <iostream.h>       // ostream << char*

#include "xassert.h"        // xassert


// ----------------------- string ---------------------
string::string(char const *src, int length)
{
  s=0;
  setlength(length+1);
  memcpy(s, src, length);
  s[length] = 0;
}


void string::dup(char const *src)
{
  if (!src) {
    s = 0;
  }
  else {
    s = new char[ strlen(src) + 1];
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

int string::contains(char c) const
{
  xassert(s);
  return !!strchr(s, c);
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
  os << s;     // standard char* writing routine
}


// replace all instances of oldstr in src with newstr, return result
string replace(char const *src, char const *oldstr, char const *newstr)
{
  stringBuilder ret("");

  while (*src) {
    char const *next = strstr(src, oldstr);
    if (!next) {
      ret &= string(src);
      break;
    }

    // make a substring out of the characters between src and next
    string upto(src, next-src);

    // add it to running string
    ret &= upto;

    // add the replace-with string
    ret &= string(newstr);

    // move src to beyond replaced substring
    src += (next-src) + strlen(oldstr);
  }

  return ret;
}


// --------------------- stringBuilder ------------------
stringBuilder::stringBuilder(int len)
{
  init(len);
}

void stringBuilder::init(int initSize)
{
  size = initSize + EXTRA_SPACE;
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


stringBuilder& stringBuilder::operator&= (char const *tail)
{
  int len = strlen(tail);
  ensure(length() + len);

  strcpy(end, tail);
  end += len;
  return *this;
}


void stringBuilder::grow(int newsize)
{
  // I want at least EXTRA_SPACE extra
  newsize += EXTRA_SPACE;

  // I want to grow at the rate of at least 50% each time
  int suggest = size * 3 / 2;

  // see which is bigger
  newsize = max(newsize, suggest);

  // write this down
  int len = length();

  // realloc s to be newsize bytes
  char *temp = new char[newsize];
  memcpy(temp, s, len+1);   // copy null too
  delete[] s;
  s = temp;

  // adjust other variables
  end = s + len;
  size = newsize;
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
    sprintf(buf, fmt, arg);                              \
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
  char buf[20];    // should only need 17 for 64-bit word..
  sprintf(buf, "0x%lX", h.value);
  return *this << buf;
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

