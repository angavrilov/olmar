// xml_file_reader.cc            see license.txt for copyright and terms of use

#include "xml_file_reader.h"    // this module
#include "xml_reader.h"         // XmlReader
#include "hashline.h"           // HashLineMap
#include "xmlhelp.h"            // xmlAttrDeQuote
#include "xml_enum.h"           // XTOK_*

void *XmlFileReader::ctorNodeFromTag(int tag) {
  switch(tag) {
  default: return NULL;
  case 0: userError("unexpected file termination while looking for an open tag name");
  case XTOK_File:
    return new SourceLocManager::FileData();
    break;
  case XTOK_LineLengths:
    // NOTE: This is not technically a list object has it does not
    // have list item children, only data; it is a regular node that
    // happens to be a list of data.
    return new ArrayStack<unsigned char>;
    break;
  case XTOK_HashLineMap:
    return new HashLineMap("");
    break;
  case XTOK_HashLine:
    return new HashLineMap::HashLine();
    break;
  case XTOK_List_HashLineMap_directives:
    return new ASTList<HashLineMap::HashLine>;
    break;
  }
}


bool XmlFileReader::registerStringToken(void *target, int kind, char const *yytext0) {
  switch(kind) {
  default: return false; break;

  case XTOK_File: {
    userError("cannot register data with a File tag");
    break;
  }

  case XTOK_LineLengths: {
    ArrayStack<unsigned char> *lineLengths = (ArrayStack<unsigned char>*)target;
    // FIX: this does not detect any errors if it is not a non-neg int
    lineLengths->push(atoi(yytext0));
    break;
  }

  }
  return true;
}


bool XmlFileReader::registerAttribute(void *target0, int kind, int attr, char const *strValue) {
  switch(kind) {
  default: return false; break;

  case XTOK_File: {
    SourceLocManager::FileData *obj = (SourceLocManager::FileData*)target0;
    switch(attr) {
    default: userError("illegal attribute for a File tag"); break;
    case XTOK_name: obj->name = manager->strTable(xmlAttrDeQuote(strValue)); break;
    case XTOK_numChars: fromXml_int(obj->numChars, xmlAttrDeQuote(strValue)); break;
    case XTOK_numLines: fromXml_int(obj->numLines, xmlAttrDeQuote(strValue)); break;
    case XTOK_lineLengths: ul(lineLengths, XTOK_LineLengths); break;
    case XTOK_hashLines: ul(hashLines, XTOK_HashLineMap); break;
    }
    break;
  }

  case XTOK_LineLengths: {
    // currently unused so I turned it off to avoid the compiler warning
//      ArrayStack<unsigned char> *obj = (ArrayStack<unsigned char>*)target0;
    switch(attr) {
    default: userError("illegal attribute for a LineLengths tag"); break;
    }
    break;
  }

  case XTOK_HashLineMap: {
    HashLineMap *obj = (HashLineMap*) target0;
    switch(attr) {
    default: userError("illegal attribute for a HashLineMap tag"); break;
    case XTOK_ppFname: obj->serializationOnly_set_ppFname(xmlAttrDeQuote(strValue)); break;
    case XTOK_directives:
      ulList(_List, directives, XTOK_List_HashLineMap_directives);
      break;
    // NOTE: there is no XTOK_filenames; the file names dictionary is
    // redundant and reconstructed from the File names fields
    }
    break;
  }

  case XTOK_HashLine: {
    HashLineMap::HashLine *obj = (HashLineMap::HashLine*) target0;
    switch(attr) {
    default: userError("illegal attribute for a HashLine tag"); break;
    case XTOK_ppLine: fromXml_int(obj->ppLine, xmlAttrDeQuote(strValue)); break;
    case XTOK_origLine: fromXml_int(obj->origLine, xmlAttrDeQuote(strValue)); break;
    case XTOK_origFname: obj->origFname = manager->strTable(xmlAttrDeQuote(strValue)); break;
    }
    break;
  }

  }
  return true;
}

bool XmlFileReader::kind2kindCat(int kind, KindCategory *kindCat) {
  switch(kind) {
  default: return false;        // we don't know this kind
  case XTOK_File:
  case XTOK_LineLengths:
  case XTOK_HashLineMap:
  case XTOK_HashLine:
    *kindCat = KC_Node;
    break;

  case XTOK_List_HashLineMap_directives:
    *kindCat = KC_ArrayStack;
    break;
  }
  return true;
}

bool XmlFileReader::recordKind(int kind, bool& answer) {
  switch(kind) {
  default: return false;        // we don't know this kind
  // **** do not record these
  case XTOK_File:
  case XTOK_LineLengths:
  case XTOK_HashLineMap:
  case XTOK_HashLine:
  case XTOK_List_HashLineMap_directives:
    answer = false;
    return true;
    break;
  }
}

bool XmlFileReader::callOpAssignToEmbeddedObj(void *obj, int kind, void *target) {
  xassert(obj);
  xassert(target);
  switch(kind) {
  default:
    // This handler conflates two situations; see the node in
    // XmlTypeReader::callOpAssignToEmbeddedObj().
    return false;
    break;
  }
}

bool XmlFileReader::upcastToWantedType(void *obj, int kind, void **target, int targetKind) {
  xassert(obj);
  xassert(target);
  // This handler conflates two situations; see the node in
  // XmlTypeReader::upcastToWantedType
  return false;
}

bool XmlFileReader::convertList2FakeList(ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool XmlFileReader::convertList2SObjList(ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool XmlFileReader::convertList2ObjList(ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool XmlFileReader::convertList2ArrayStack(ASTList<char> *list, int listKind, void **target) {
  xassert(list);
  switch(listKind) {
  default: return false;        // we did not find a matching tag

  case XTOK_List_HashLineMap_directives:
    // FIX: the HashLine objects are being copied by value here, so
    // the originals should be destructed
    convertArrayStack(ArrayStack, HashLineMap::HashLine);
    break;
  }

  return true;
}

bool XmlFileReader::convertNameMap2StringRefMap
  (StringRefMap<char> *map, int mapKind, void *target) {
  return false;
}

bool XmlFileReader::convertNameMap2StringSObjDict
  (StringRefMap<char> *map, int mapKind, void *target) {
  return false;
}
