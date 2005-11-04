// xml_writer.cc            see license.txt for copyright and terms of use

#include "xml_writer.h"
#include "xmlhelp.h"            // xmlAttrDeQuote() etc.
#include "exc.h"                // xBase


bool sortNameMapDomainWhenSerializing = true;

XmlWriter::XmlWriter(ostream &out0, int &depth0, bool indent0)
  : out(out0)
  , depth(depth0)
  , indent(indent0)
{}

void XmlWriter::newline() {
  out << "\n";
  // FIX: turning off indentation makes the output go way faster, so
  // this loop is worth optimizing, probably by printing chunks of 10
  // if you can or something logarithmic like that.
  if (indent) {
    for (int i=0; i<depth; ++i) out << " ";
  }
}


// manage identity
char const *idPrefixAST(void const * const) {
  return "AST";
}

void const *addrAST(void const * const obj) {
  return reinterpret_cast<void const *>(obj);
}
