// str.h            see license.txt for copyright and terms of use
// a string class
// the representation uses just one char*, so that a smart compiler
//   can pass the entire object as a single word
// Scott McPeak, 1995-2000  This file is public domain.

#ifndef __STR_H
#define __STR_H

#include "typ.h"         // bool
#include <iostream.h>	 // istream, ostream
#include <stdarg.h>      // va_list

class Flatten;           // flatten.h

// certain unfortunate implementation decisions by some compilers
// necessitate avoiding the name 'string'
#define string mystring

class string {
protected:     // data
  // 10/12/00: switching to never letting s be NULL
  char *s;     	       	       	       // string contents; never NULL
  static char * const empty;           // a global ""; should never be modified

protected:     // funcs
  void dup(char const *source);        // copies, doesn't dealloc first
  void kill();                         // dealloc if str != 0

public:	       // funcs
  string(string const &src) { dup(src.s); }
  string(char const *src) { dup(src); }
  string(char const *src, int length);    // grab a substring
  string(int length) { s=empty; setlength(length); }
  string() { s=empty; }

  ~string() { kill(); }

  string(Flatten&);
  void xfer(Flatten &flat);

  // simple queries
  int length() const;  	       	// returns number of non-null chars in the string; length of "" is 0
  bool isempty() const { return s[0]==0; }
  bool contains(char c) const;

  // array-like access
  char& operator[] (int i) { return s[i]; }
  char operator[] (int i) const { return s[i]; }

  // substring
  string substring(int startIndex, int length) const;

  // conversions
  //operator char* () { return s; }      // ambiguities...
  operator char const* () const { return s; }
  char *pchar() { return s; }
  char const *pcharc() const { return s; }

  // assignment
  string& operator=(string const &src)
    { if (&src != this) { kill(); dup(src.s); } return *this; }
  string& operator=(char const *src)
    { if (src != s) { kill(); dup(src); } return *this; }

  // allocate 'newlen' + 1 bytes (for null); initial contents is ""
  string& setlength(int newlen);

  // comparison; return value has same meaning as strcmp's return value:
  //   <0   if   *this < src
  //   0    if   *this == src
  //   >0   if   *this > src
  int compareTo(string const &src) const;
  int compareTo(char const *src) const;
  bool equals(char const *src) const { return compareTo(src) == 0; }

  #define MAKEOP(op)							       	 \
    bool operator op (string const &src) const { return compareTo(src) op 0; }	 \
    /*bool operator op (const char *src) const { return compareTo(src) op 0; }*/ \
    /* killed stuff with char* because compilers are too flaky; use compareTo */
  MAKEOP(==)  MAKEOP(!=)
  MAKEOP(>=)  MAKEOP(>)
  MAKEOP(<=)  MAKEOP(<)
  #undef MAKEOP

  // concatenation (properly handles string growth)
  // uses '&' instead of '+' to avoid char* coercion problems
  string operator& (string const &tail) const;
  string& operator&= (string const &tail);

  // input/output
  friend istream& operator>> (istream &is, string &obj)
    { obj.readline(is); return is; }
  friend ostream& operator<< (ostream &os, string const &obj)
    { obj.write(os); return os; }

  // note: the read* functions are currently implemented in a fairly
  // inefficient manner (one char at a time)

  void readdelim(istream &is, char const *delim);
    // read from is until any character in delim is encountered; consumes that
    // character, but does not put it into the string; if delim is null or
    // empty, reads until EOF

  void readall(istream &is) { readdelim(is, NULL); }
    // read all remaining chars of is into this

  void readline(istream &is) { readdelim(is, "\n"); }
    // read a line from input stream; consumes the \n, but doesn't put it into
    // the string

  void write(ostream &os) const;
    // writes all stored characters (but not '\0')
    
  // debugging
  void selfCheck() const;
    // fail an assertion if there is a problem
};


// replace() and trimWhiteSpace() have been moved to strutil.h


// this class is specifically for appending lots of things
class stringBuilder : public string {
protected:
  enum { EXTRA_SPACE = 30 };    // extra space allocated in some situations
  char *end;          // current end of the string (points to the null)
  int size;           // amount of space (in bytes) allocated starting at 's'

protected:
  void init(int initSize);
  void dup(char const *src);

public:
  stringBuilder(int length=0);    // creates an empty string
  stringBuilder(char const *str);
  stringBuilder(char const *str, int length);
  stringBuilder(string const &str) { dup((char const*)str); }
  stringBuilder(stringBuilder const &obj) { dup((char const*)obj); }
  ~stringBuilder() {}

  stringBuilder& operator= (char const *src);
  stringBuilder& operator= (string const &s) { return operator= ((char const*)s); }
  stringBuilder& operator= (stringBuilder const &s) { return operator= ((char const*)s); }

  int length() const { return end-s; }
  bool isempty() const { return length()==0; }

  stringBuilder& setlength(int newlen);    // change length, forget current data

  // make sure we can store 'someLength' non-null chars; grow if necessary
  void ensure(int someLength) { if (someLength >= size) { grow(someLength); } }

  // grow the string's length (retaining data); make sure it can hold at least
  // 'newMinLength' non-null chars
  void grow(int newMinLength);

  // this can be useful if you modify the string contents directly..
  // it's not really the intent of this class, though
  void adjustend(char* newend);

  // concatenation, which is the purpose of this class
  stringBuilder& operator&= (char const *tail);

  // useful for appending substrings or strings with NUL in them
  void append(char const *tail, int length);

  // append a given number of spaces; meant for contexts where we're
  // building a multi-line string; returns '*this'
  stringBuilder& indent(int amt);

  // sort of a mixture of Java compositing and C++ i/o strstream
  // (need the coercion version (like int) because otherwise gcc
  // spews mountains of f-ing useless warnings)
  stringBuilder& operator << (char const *text) { return operator&=(text); }
  stringBuilder& operator << (char c);
  stringBuilder& operator << (unsigned char c) { return operator<<((char)c); }
  stringBuilder& operator << (long i);
  stringBuilder& operator << (unsigned long i);
  stringBuilder& operator << (int i) { return operator<<((long)i); }
  stringBuilder& operator << (unsigned i) { return operator<<((unsigned long)i); }
  stringBuilder& operator << (short i) { return operator<<((long)i); }
  stringBuilder& operator << (unsigned short i) { return operator<<((long)i); }
  stringBuilder& operator << (double d);
  stringBuilder& operator << (void *ptr);     // inserts address in hex
  #ifndef LACKS_BOOL
    stringBuilder& operator << (bool b) { return operator<<((long)b); }
  #endif // LACKS_BOOL

  // useful in places where long << expressions make it hard to
  // know when arguments will be evaluated, but order does matter
  typedef stringBuilder& (*Manipulator)(stringBuilder &sb);
  stringBuilder& operator<< (Manipulator manip);

  // stream readers
  friend istream& operator>> (istream &is, stringBuilder &sb)
    { sb.readline(is); return is; }
  void readall(istream &is) { readdelim(is, NULL); }
  void readline(istream &is) { readdelim(is, "\n"); }

  void readdelim(istream &is, char const *delim);

  // an experiment: hex formatting (something I've sometimes done by resorting
  // to sprintf in the past)
  class Hex {
  public:
    unsigned long value;

    Hex(unsigned long v) : value(v) {}
    Hex(Hex const &obj) : value(obj.value) {}
  };
  stringBuilder& operator<< (Hex const &h);
  #define SBHex stringBuilder::Hex
};


// the real strength of this entire module: construct strings in-place
// using the same syntax as C++ iostreams.  e.g.:
//   puts(stringb("x=" << x << ", y=" << y));
#define stringb(expr) (stringBuilder() << expr)

// experimenting with dropping the () in favor of <<
// (the "c" can be interpreted as "constructor", or maybe just
// the successor to "b" above)
#define stringc stringBuilder()


// experimenting with using toString as a general method for datatypes
string toString(int i);
string toString(unsigned i);
string toString(char c);
string toString(long i);
string toString(char const *str);
string toString(float f);


// printf-like construction of a string; often very convenient, since
// you can use any of the formatting characters (like %X) that your
// libc's sprintf knows about
string stringf(char const *format, ...);
string vstringf(char const *format, va_list args);


#endif // __STR_H

