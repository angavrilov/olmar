// file_xml.h            see license.txt for copyright and terms of use

// Serialization of file information for purposes of capturing the
// state of the SourceLocManager

#ifndef FILE_XML_H
#define FILE_XML_H

#include "objlist.h"            // ObjList
#include "srcloc.h"             // SourceLocManager
#include "hashline.h"           // HashLineMap
#include "xml.h"                // xml macros


class FileToXml : public ToXml {
  public:
  FileToXml(ostream &out0, int &depth0, bool indent0);

  void toXml(ObjList<SourceLocManager::File> &files);
  void toXml(SourceLocManager::File *file);
  // this is an exception to the generic toXml() mechanism since
  // lineLengths are not self-contained
  void toXml_lineLengths(SourceLocManager::File *file);
  void toXml(HashLineMap *hashLines);
  void toXml(HashLineMap::HashLine *hashLine);
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
  virtual bool convertList2FakeList  (ASTList<char> *list, int listKind, void **target);
  virtual bool convertList2SObjList  (ASTList<char> *list, int listKind, void **target);
  virtual bool convertList2ObjList   (ASTList<char> *list, int listKind, void **target);
  virtual bool convertList2ArrayStack(ASTList<char> *list, int listKind, void **target);
  virtual bool convertNameMap2StringRefMap
    (StringRefMap<char> *map, int mapKind, void *target);
  virtual bool convertNameMap2StringSObjDict
    (StringRefMap<char> *map, int mapKind, void *target);
};

#endif // FILE_XML_H
