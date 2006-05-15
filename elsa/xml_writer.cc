// xml_writer.cc            see license.txt for copyright and terms of use

#include "xml_writer.h"
// #include "xmlhelp.h"            // xmlAttrDeQuote() etc.
#include "exc.h"                // xBase


bool sortNameMapDomainWhenSerializing = true;

XmlWriter::XmlWriter(IdentityManager &idmgr0, ostream *out0, int &depth0, bool indent0)
  : idmgr(idmgr0)
  , out(out0)
  , depth(depth0)
  , indent(indent0)
{}

// write N spaces to OUT.
inline void writeSpaces(ostream &out, size_t n)
{
  static char const spaces[] =
    "                                                  "
    "                                                  "
    "                                                  "
    "                                                  ";

  static size_t const max_spaces = sizeof spaces - 1;

  // If we're printing more than this many spaces it's pretty useless anyway,
  // since it's only for human viewing pleasure!
  while (n > max_spaces) {
    out.write(spaces, max_spaces);
    n -= max_spaces;
  }
  out.write(spaces, n);
}

void XmlWriter::newline() {
  xassert(out != NULL);
  *out << '\n';
  // FIX: turning off indentation makes the output go way faster, so
  // this loop is worth optimizing, probably by printing chunks of 10
  // if you can or something logarithmic like that.
  if (indent) {
    writeSpaces(*out, depth);
  }
}

