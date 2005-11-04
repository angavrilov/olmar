// xml_ast_reader.cc          see license.txt for copyright and terms of use

// NOTE: the implementation of this class is generated

#include "xml_ast_reader.h"     // this module
#include "xmlhelp.h"            // toXml_int etc.
#include "xml_enum.h"           // XTOK_*

bool XmlAstReader::convertList2SObjList(ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool XmlAstReader::convertList2ObjList(ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool XmlAstReader::convertList2ArrayStack(ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool XmlAstReader::convertNameMap2StringRefMap(StringRefMap<char> *map, int mapKind, void *target) {
  return false;
}

bool XmlAstReader::convertNameMap2StringSObjDict(StringRefMap<char> *map, int mapKind, void *target)
{
  return false;
}

void *XmlAstReader::ctorNodeFromTag(int tag) {
  switch(tag) {
  default: return NULL;
  case 0: userError("unexpected file termination while looking for an open tag name");
#include "xml_ast_reader_2ctrc.gen.cc"
  }
}

bool XmlAstReader::registerStringToken(void *target, int kind, char const *yytext0) {
  return false;
}

bool XmlAstReader::registerAttribute(void *target, int kind, int attr, char const *yytext0) {
  switch(kind) {
  default: return false; break;
#include "xml_ast_reader_3regc.gen.cc"
  }

  return true;
}

#include "xml_ast_reader_1defn.gen.cc"
