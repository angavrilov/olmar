#include "xmlhelp.h"            // this module
#include <stdlib.h>             // atof, atol

// dsw: NOTE: I don't know where to put this so I'll put it here.  I
// de-serialize a string of digits representing an int of some kind
// using atoll(), which returns a long long, which I have named here
// intParseTarget_t.  The reason is that an unsigned int actually can
// go higher than a signed long, so de-serializing with atol() is not
// sufficient when the result is to be stored in an unsigned it: if
// the number is greater than LONG_MAX then it becomes LONG_MAX.
// However, all this will fail if we compile on a platform where long
// long is just as wide as a long.
typedef long long intParseTarget_t;
#define parseInt(X) atoll(X)

string toXml_bool(bool b) {
  if (b) return "true";
  else return "false";
}

void fromXml_bool(bool &b, rostring str) {
  b = streq(str, "true");
}


string toXml_int(int i) {
  return stringc << i;
}

void fromXml_int(int &i, rostring str) {
  intParseTarget_t i0 = parseInt(str.c_str());
  i = i0;
}


string toXml_long(long i) {
  return stringc << i;
}

void fromXml_long(long &i, rostring str) {
  intParseTarget_t i0 = parseInt(str.c_str());
  i = i0;
}


string toXml_unsigned_int(unsigned int i) {
  return stringc << i;
}

void fromXml_unsigned_int(unsigned int &i, rostring str) {
  intParseTarget_t i0 = parseInt(str.c_str());
  i = i0;
}


string toXml_unsigned_long(unsigned long i) {
  return stringc << i;
}

void fromXml_unsigned_long(unsigned long &i, rostring str) {
  intParseTarget_t i0 = parseInt(str.c_str());
  i = i0;
}


string toXml_double(double x) {
  return stringc << x;
}

void fromXml_double(double &x, rostring str) {
  x = atof(str.c_str());
}


string toXml_SourceLoc(SourceLoc loc) {
  return sourceLocManager->getString(loc);
}

void fromXml_SourceLoc(SourceLoc &loc, rostring str) {
  xfailure("not implemented");
}
