// main_astxmlparse.cc          see license.txt for copyright and terms of use

#include "main_astxmlparse.h"   // this module
#include "xmlhelp.h"            // toXml_int etc.
#include "xml.h"                // XmlReaderManager
#include "cc_type_xml.h"        // TypeXmlReader
#include "fstream.h"            // ifstream
#include "strutil.h"            // parseQuotedString
#include "astxml_lexer.h"       // AstXmlLexer
#include "cc.ast.gen.h"         // TranslationUnit, etc.


// -------------------- FileXmlReader -------------------

// pair a GrowArray with its size
class LineLengths {
  public:
  int tableSize;
  GrowArray<int> table;
  bool appendable;

  LineLengths(int tableSize0 = 0)
    : tableSize(tableSize0)
    , table(tableSize)
    , appendable(tableSize == 0)
  {}

  void append(int data) {
    xassert(appendable);
    table.ensureIndexDoubler(tableSize);
    table[tableSize] = data;
    ++tableSize;
  }
};

// Holds basic data about files for use in initializing the
// SourceLocManager.
class FileData {
  public:
  char const *name;
  LineLengths *lineLengths;

  FileData()
    : name(NULL)
    , lineLengths(NULL)
  {}
};

// parse the File data serialized as XML
class FileXmlReader : public XmlReader {
  public:
  FileXmlReader() {}
  virtual ~FileXmlReader() {}

  virtual void *ctorNodeFromTag(int tag);
  virtual bool registerAttribute(void *target, int kind, int attr, char const *yytext0);
  virtual bool registerStringToken(void *target, int kind, char const *yytext0);
  virtual bool kind2kindCat(int kind, KindCategory *kindCat);
  // **** Generic Convert
  virtual bool recordKind(int kind, bool& answer);
  virtual bool callOpAssignToEmbeddedObj(void *obj, int kind, void *target);
  virtual bool upcastToWantedType(void *obj, int kind, void **target, int targetKind);
  virtual bool convertList2FakeList(ASTList<char> *list, int listKind, void **target);
  virtual bool convertList2SObjList(ASTList<char> *list, int listKind, void **target);
  virtual bool convertList2ObjList (ASTList<char> *list, int listKind, void **target);
  virtual bool convertNameMap2StringRefMap
    (StringRefMap<char> *map, int mapKind, void *target);
  virtual bool convertNameMap2StringSObjDict
    (StringRefMap<char> *map, int mapKind, void *target);
};

void *FileXmlReader::ctorNodeFromTag(int tag) {
  switch(tag) {
  default: return NULL;
  case 0: userError("unexpected file termination while looking for an open tag name");
  case XTOK_File:
    return new FileData();
    break;
  case XTOK_LineLengths:
    return new LineLengths();
    break;
  }
}


bool FileXmlReader::registerStringToken(void *target, int kind, char const *yytext0) {
  switch(kind) {
  default: return false; break;

  case XTOK_File: {
    userError("cannot register data with a File tag");
    break;
  }

  case XTOK_LineLengths: {
    LineLengths *lineLengths = (LineLengths*)target;
    // FIX: this does not detect any errors if it is not a non-neg int
    lineLengths->append(atoi(yytext0));
    break;
  }

  }
  return true;
}


bool FileXmlReader::registerAttribute(void *target0, int kind, int attr, char const *yytext0) {
  switch(kind) {
  default: return false; break;

  case XTOK_File: {
    FileData *target = (FileData*)target0;
    if (attr == XTOK_name) {
      target->name = strdup(yytext0);
    } else {
      userError("illegal attribute for a File tag");
    }
    break;
  }

  case XTOK_LineLengths: {
    userError("illegal attribute for a LineLengths tag");
    break;
  }

  }
  return true;
}

bool FileXmlReader::kind2kindCat(int kind, KindCategory *kindCat) {
  switch(kind) {
  default: return false;        // we don't know this kind
  case XTOK_File:
    *kindCat = KC_Node;
    break;
  case XTOK_LineLengths:
    *kindCat = KC_Node;
    break;
  }
  return true;
}

bool FileXmlReader::recordKind(int kind, bool& answer) {
  switch(kind) {
  default: return false;        // we don't know this kind
  // **** do not record these
  case XTOK_File:
  case XTOK_LineLengths:
    answer = false;
    return true;
    break;
  }
}

bool FileXmlReader::callOpAssignToEmbeddedObj(void *obj, int kind, void *target) {
  xassert(obj);
  xassert(target);
  switch(kind) {
  default:
    // This handler conflates two situations; see the node in
    // TypeXmlReader::callOpAssignToEmbeddedObj().
    return false;
    break;
  }
}

bool FileXmlReader::upcastToWantedType(void *obj, int kind, void **target, int targetKind) {
  xassert(obj);
  xassert(target);
  // This handler conflates two situations; see the node in
  // TypeXmlReader::upcastToWantedType
  return false;
}

bool FileXmlReader::convertList2FakeList(ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool FileXmlReader::convertList2SObjList(ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool FileXmlReader::convertList2ObjList(ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool FileXmlReader::convertNameMap2StringRefMap
  (StringRefMap<char> *map, int mapKind, void *target) {
  return false;
}

bool FileXmlReader::convertNameMap2StringSObjDict
  (StringRefMap<char> *map, int mapKind, void *target) {
  return false;
}

// -------------------- ASTXmlReader -------------------

// parse AST serialized as XML; the implementation of this class is
// generated
class ASTXmlReader : public XmlReader {
  public:
  ASTXmlReader() {}
  virtual ~ASTXmlReader() {}

  private:
  // Parse a tag: construct a node for a tag
  virtual void *ctorNodeFromTag(int tag);

  // Parse an attribute: register an attribute into the current node
  virtual bool registerAttribute(void *target, int kind, int attr, char const *yytext0);
  virtual bool registerStringToken(void *target, int kind, char const *yytext0);

  // implement an eq-relation on tag kinds by mapping a tag kind to a
  // category
  virtual bool kind2kindCat(int kind, KindCategory *kindCat);

  // **** Generic Convert

  virtual bool recordKind(int kind, bool& answer);

  // convert nodes
  virtual bool callOpAssignToEmbeddedObj(void *obj, int kind, void *target);
  virtual bool upcastToWantedType(void *obj, int kind, void **target, int targetKind);
  // all lists are stored as ASTLists; convert to the real list
  virtual bool convertList2FakeList(ASTList<char> *list, int listKind, void **target);
  virtual bool convertList2SObjList(ASTList<char> *list, int listKind, void **target);
  virtual bool convertList2ObjList (ASTList<char> *list, int listKind, void **target);
  // all name maps are stored as StringRefMaps; convert to the real name maps
  virtual bool convertNameMap2StringRefMap
    (StringRefMap<char> *map, int mapKind, void *target);
  virtual bool convertNameMap2StringSObjDict
    (StringRefMap<char> *map, int mapKind, void *target);

#include "astxml_parse1_0decl.gen.cc"
};

bool ASTXmlReader::convertList2SObjList(ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool ASTXmlReader::convertList2ObjList (ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool ASTXmlReader::convertNameMap2StringRefMap(StringRefMap<char> *map, int mapKind, void *target) {
  return false;
}

bool ASTXmlReader::convertNameMap2StringSObjDict(StringRefMap<char> *map, int mapKind, void *target)
{
  return false;
}

void *ASTXmlReader::ctorNodeFromTag(int tag) {
  switch(tag) {
  default: return NULL;
  case 0: userError("unexpected file termination while looking for an open tag name");
#include "astxml_parse1_2ctrc.gen.cc"
  }
}

bool ASTXmlReader::registerStringToken(void *target, int kind, char const *yytext0) {
  return false;
}

bool ASTXmlReader::registerAttribute(void *target, int kind, int attr, char const *yytext0) {
  switch(kind) {
  default: return false; break;
#include "astxml_parse1_3regc.gen.cc"
  }

  return true;
}

#include "astxml_parse1_1defn.gen.cc"


// -------------------- astxmlparse -------------------

TranslationUnit *astxmlparse(StringTable &strTable, char const *inputFname)
{
  // make reader manager
  ifstream in(inputFname);
  AstXmlLexer lexer(inputFname);
  lexer.restart(&in);
  XmlReaderManager manager(inputFname, lexer, strTable);

  // make file reader
  FileXmlReader fileReader;
  manager.registerReader(&fileReader);

  // make ast reader
  ASTXmlReader astReader;
  manager.registerReader(&astReader);

  // make type reader
//    BasicTypeFactory tFac;
  TypeXmlReader typeReader;
  manager.registerReader(&typeReader);

  // read until a TranslationUnit tag
  // FIX: not sure what happens if the last tag is not a TranslationUnit
  do {
    manager.parseOneTopLevelTag();
    if (lexer.haveSeenEof()) {
      manager.userError("unexpected EOF");
    }
  } while (manager.getLastKind() != XTOK_TranslationUnit);
//    if (manager.getLastKind() != XTOK_TranslationUnit) {
//      manager.userError("top tag is not a TranslationUnit");
//    }
  TranslationUnit *tunit = (TranslationUnit*) manager.getLastNode();

  // complete the link graph
  manager.satisfyLinks();

  return tunit;
}
