// main_astxmlparse.cc          see license.txt for copyright and terms of use

#include "main_astxmlparse.h"   // this module
#include "xmlhelp.h"            // toXml_int etc.
#include "xml.h"                // XmlReaderManager
#include "cc_type_xml.h"        // TypeXmlReader
#include "fstream.h"            // ifstream
#include "strutil.h"            // parseQuotedString
#include "astxml_lexer.h"       // AstXmlLexer
#include "file_xml.h"           // FileXmlReader
#include "cc.ast.gen.h"         // TranslationUnit, etc.


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
  virtual bool convertList2FakeList  (ASTList<char> *list, int listKind, void **target);
  virtual bool convertList2SObjList  (ASTList<char> *list, int listKind, void **target);
  virtual bool convertList2ObjList   (ASTList<char> *list, int listKind, void **target);
  virtual bool convertList2ArrayStack(ASTList<char> *list, int listKind, void **target);
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

bool ASTXmlReader::convertList2ObjList(ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool ASTXmlReader::convertList2ArrayStack(ASTList<char> *list, int listKind, void **target) {
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

  // prevent the SourceLocManager from looking at files in the file
  // system
  sourceLocManager->mayOpenFiles = false;

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
  while(true) {
    manager.parseOneTopLevelTag();
    if (lexer.haveSeenEof()) {
      manager.userError("unexpected EOF");
    }
    int lastKind = manager.getLastKind();
    if (lastKind == XTOK_File) {
      // complete the link graph so that the FileData object is
      // complete
      manager.satisfyLinks();

      SourceLocManager::FileData *fileData = (SourceLocManager::FileData*) manager.getLastNode();
      if (!fileData->complete()) {
        manager.userError("missing attributes to File tag");
      }
      sourceLocManager->loadFile(fileData);
      // FIX: recursively delete the file and its members here
    } else if (lastKind == XTOK_TranslationUnit) {
      break;                    // we are done
    } else {
      manager.userError("illegal top-level tag");
    }
  }

  // complete the link graph
  manager.satisfyLinks();

//    if (manager.getLastKind() != XTOK_TranslationUnit) {
//      manager.userError("top tag is not a TranslationUnit");
//    }
  TranslationUnit *tunit = (TranslationUnit*) manager.getLastNode();

  return tunit;
}
