// str.h
// a string class
// the representation uses just one char*, so that a smart compiler
//   can pass the entire object as a single word
// Scott McPeak, 1995-1999  This file is public domain.

#ifndef __STR_H
#define __STR_H

#include "typ.h"       // bool

// forward-declare the stream types
class istream;
class ostream;

// certain unfortunate implementation decisions by some compilers
// necessitate avoiding the name 'string'
#define string mystring

class string {
protected:
  char *s;
  void dup(char const *source);        // copies, doesn't dealloc first
  void kill();                         // dealloc if str != 0

public:
  string(string const &src) { dup(src.s); }
  string(char const *src) { dup(src); }
  string(char const *src, int length);    // grab a substring
  string(int length) { s=0; setlength(length); }
  string() { s=0; }

  ~string() { kill(); }

  // simple queries
  int length() const;
  int isempty() const { return !s || s[0]==0; }
  int contains(char c) const;

  // array-like access
  char& operator[] (int i) { return s[i]; }
  char operator[] (int i) const { return s[i]; }

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

  string& setlength(int newlen);       // uninitialized, except for null

  // comparison; same semantics as strcmp
  int compareTo(string const &src) const;

  #define MAKEOP(op) \
  bool operator op (string const &src) const { return compareTo(src) op 0; }
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
};


string replace(char const *src, char const *oldstr, char const *newstr);
  // direct string replacement, replacing instances of oldstr with newstr
  // (newstr may be "")


// this class is specifically for appending lots of things
class stringBuilder : public string {
protected:
  enum { EXTRA_SPACE = 30 };    // extra space allocated in some situations
  char *end;          // current end of the string (points to the null)
  int size;           // amount of space allocated starting at 's'

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
  int isempty() const { return length()==0; }

  stringBuilder& setlength(int newlen);    // change length, forget current data

  void ensure(int someLength) { if (someLength >= size) { grow(someLength+1); } }
  void grow(int newMinLength);   // retain data, growing capacity to at least new size

  // concatenation, which is the purpose of this class
  stringBuilder& operator&= (char const *tail);

  // sorta a mixture of Java compositing and C++ i/o strstream
  stringBuilder& operator << (char const *text) { return operator&=(text); }
  stringBuilder& operator << (char c);
  stringBuilder& operator << (long i);
  stringBuilder& operator << (unsigned long i);
  stringBuilder& operator << (int i) { return operator<<((long)i); }
  stringBuilder& operator << (unsigned i) { return operator<<((unsigned long)i); }
  stringBuilder& operator << (double d);
  stringBuilder& operator << (void *ptr);     // inserts address in hex

  // stream readers
  friend istream& operator>> (istream &is, stringBuilder &sb)
    { sb.readline(is); return is; }
  void readall(istream &is) { readdelim(is, NULL); }
  void readline(istream &is) { readdelim(is, "\n"); }

  void readdelim(istream &is, char const *delim);
  
  // Dan added this, probably because he's modifying string contents directly,
  // which I hadn't really intended, but the interface does allow..
  void adjustend(char* newend) {end = newend;}

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


#endif // __STR_H

