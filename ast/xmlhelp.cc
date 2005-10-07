#include "xmlhelp.h"            // this module
#include <stdlib.h>             // atof, atol
#include "strtokp.h"            // StrtokParse

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
  long i0 = strtol(str.c_str(), NULL, 10);
  i = i0;
}


string toXml_long(long i) {
  return stringc << i;
}

void fromXml_long(long &i, rostring str) {
  long i0 = strtol(str.c_str(), NULL, 10);
  i = i0;
}


string toXml_unsigned_int(unsigned int i) {
  return stringc << i;
}

void fromXml_unsigned_int(unsigned int &i, rostring str) {
  unsigned long i0 = strtoul(str.c_str(), NULL, 10);
  i = i0;
}


string toXml_unsigned_long(unsigned long i) {
  return stringc << i;
}

void fromXml_unsigned_long(unsigned long &i, rostring str) {
  unsigned long i0 = strtoul(str.c_str(), NULL, 10);
  i = i0;
}


string toXml_double(double x) {
  return stringc << x;
}

void fromXml_double(double &x, rostring str) {
  x = atof(str.c_str());
}


string toXml_SourceLoc(SourceLoc loc) {
  // NOTE: the nohashline here is very important; never change it
  return sourceLocManager->getString_nohashline(loc);
}

void fromXml_SourceLoc(SourceLoc &loc, rostring str) {
  // the file format is filename:line:column
  StrtokParse tok(str, ":");
  if ((int)tok != 3) {
    // FIX: this is a parsing error but I don't want to throw an
    // exception out of this library function
    loc = SL_UNKNOWN;
    return;
  }
  char const *file = tok[0];
  if (streq(file, "<noloc>")) {
    loc = SL_UNKNOWN;
    return;
  }
  if (streq(file, "<init>")) {
    loc = SL_INIT;
    return;
  }
  int line = atoi(tok[1]);
  int col  = atoi(tok[2]);
  loc = sourceLocManager->encodeLineCol(file, line, col);
}
