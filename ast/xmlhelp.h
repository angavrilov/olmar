// xmlhelp.h            see license.txt for copyright and terms of use
// included by generated ast code

// Generic serialization and de-serialization support.

#ifndef XMLHELP_H
#define XMLHELP_H

#include "str.h"         // string

// I have manually mangled the name to include "_bool" or "_int" as
// otherwise what happens is that if a toXml() for some enum flag is
// missing then the C++ compiler will just use the toXml(bool)
// instead, which is a bug.
string toXml_bool(bool b);
void fromXml_bool(bool &b, string str);

string toXml_int(int i);
void fromXml_int(int &i, string str);

string toXml_long(long i);
void fromXml_long(long &i, string str);

string toXml_unsigned_int(unsigned int i);
void fromXml_unsigned_int(unsigned int &i, string str);

string toXml_unsigned_long(unsigned long i);
void fromXml_unsigned_long(unsigned long &i, string str);

#endif // XMLHELP_H
