// xml_ast_reader.h         see license.txt for copyright and terms of use

// parse AST serialized as XML
// NOTE: the declarations of this class are generated

#ifndef XML_AST_READER_H
#define XML_AST_READER_H

#include "xml_reader.h"         // XmlReader
#include "cc.ast.gen.h"         // TranslationUnit

class XmlAstReader : public XmlReader {
  public:
  XmlAstReader() {}
  virtual ~XmlAstReader() {}

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

#include "xml_ast_reader_0decl.gen.h"
};

#endif // XML_AST_READER_H
