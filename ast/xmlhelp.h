// xmlhelp.h            see license.txt for copyright and terms of use
// included by generated ast code

// Generic serialization and de-serialization support.

#ifndef XMLHELP_H
#define XMLHELP_H

#include "str.h"         // string
#include "srcloc.h"      // SourceLoc

typedef unsigned int xmlUniqueId_t;

// manage identity canonicality; we now map addresses one to one to a
// sequence number; this means that the ids should be canonical now
// given isomorphic inputs
xmlUniqueId_t mapAddrToUniqueId(void const * const addr);

// manage identity of AST; FIX: I am not absolutely sure that we are
// not accidentally using this for user classes instead of just AST
// classes; to be absolutely sure, make a superclass of all of the AST
// classes and make the argument here take a pointer to that.
xmlUniqueId_t uniqueIdAST(void const * const obj);

// print a unique id with prefix, for example "FL12345678"; guaranteed
// to print (e.g.) "FL0" for NULL pointers; the "FL" part is the label
string xmlPrintPointer(char const *label, xmlUniqueId_t id);

// 2006-05-05 these used to take an rostring, but I changed them to const char
// *, because these functions are performance-critical and they were not
// strings until now, so don't allocate a string just to call these functions.

// I have manually mangled the name to include "_bool" or "_int" as
// otherwise what happens is that if a toXml() for some enum flag is
// missing then the C++ compiler will just use the toXml(bool)
// instead, which is a bug.
string toXml_bool(bool b);
void fromXml_bool(bool &b, const char *str);

string toXml_int(int i);
void fromXml_int(int &i, const char *str);

string toXml_long(long i);
void fromXml_long(long &i, const char *str);

string toXml_unsigned_int(unsigned int i);
void fromXml_unsigned_int(unsigned int &i, const char *str);

string toXml_unsigned_long(unsigned long i);
void fromXml_unsigned_long(unsigned long &i, const char *str);

string toXml_double(double x);
void fromXml_double(double &x, const char *str);

string toXml_SourceLoc(SourceLoc loc);
void fromXml_SourceLoc(SourceLoc &loc, const char *str);

// output SRC with encoding and quotes around it.
ostream &outputXmlAttrQuoted(ostream &o, const char *src);
static inline ostream &outputXmlAttrQuoted(ostream &o, string const &src)
{ return outputXmlAttrQuoted(o, src.c_str()); }

// output SRC with quotes, but no encoding.  Only use with strings that do not
// contain ["'<>&]
ostream &outputXmlAttrQuotedNoEscape(ostream &o, const char *src);
static inline ostream &outputXmlAttrQuotedNoEscape(ostream &o, string const &src)
{ return outputXmlAttrQuotedNoEscape(o, src.c_str()); }

// for quoting and unquoting xml attribute strings
string xmlAttrQuote(const char *src);
inline string xmlAttrQuote(rostring src) { return xmlAttrQuote(src.c_str()); }
// string xmlAttrEncode(char const *src);
// string xmlAttrEncode(char const *p, int len);

string xmlAttrDeQuote(const char *text);
// dsw: This function does not process all XML escapes.  I only
// process the ones that I use in the partner encoding function
// xmlAttrEncode().
string xmlAttrDecode(char const *src, const char *end, char delim);

#endif // XMLHELP_H
