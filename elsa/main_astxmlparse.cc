// main_astxmlparse.cc          see license.txt for copyright and terms of use

#include "main_astxmlparse.h"   // this module
#include "xmlhelp.h"            // toXml_int etc.
#include "xml.h"                // ReadXml
#include "cc_type_xml.h"        // ReadXml_Type
#include "fstream.h"            // ifstream
#include "strutil.h"            // parseQuotedString
#include "astxml_lexer.h"       // AstXmlLexer
#include "cc.ast.gen.h"         // TranslationUnit, etc.


// -------------------- ReadXml_AST -------------------

// parse AST serialized as XML; the implementation of this class is
// generated
class ReadXml_AST : public ReadXml {
  public:
  ReadXml_AST(char const *inputFname0,
              AstXmlLexer &lexer0,
              StringTable &strTable0,
              LinkSatisfier &linkSat0)
    : ReadXml(inputFname0, lexer0, strTable0, linkSat0)
  {}

  private:
  void append2List(void *list, int listKind, void *datum);
  void insertIntoNameMap(void *map, int mapKind, StringRef name, void *datum);

  bool kind2kindCat0(int kind, KindCategory *ret);

  bool convertList2FakeList(ASTList<char> *list, int listKind, void **target);
  bool convertList2SObjList(ASTList<char> *list, int listKind, void **target);
  bool convertList2ObjList (ASTList<char> *list, int listKind, void **target);

  bool convertNameMap2StringRefMap
    (StringRefMap<char> *map, int mapKind, void *target);
  bool convertNameMap2StringSObjDict
    (StringRefMap<char> *map, int mapKind, void *target);

  void *ctorNodeFromTag(int tag);
  void registerAttribute(void *target, int kind, int attr, char const *yytext0);

//    // INSERT per ast node
//    void registerAttr_TranslationUnit(TranslationUnit *obj, int attr, char const *strValue);
#include "astxml_parse1_0decl.gen.cc"
};

void ReadXml_AST::insertIntoNameMap
  (void *map, int mapKind, StringRef name, void *datum) {
  xfailure("should not be called during AST parsing as there are no Maps in the AST");
}

bool ReadXml_AST::convertList2SObjList(ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool ReadXml_AST::convertList2ObjList (ASTList<char> *list, int listKind, void **target) {
  return false;
}

bool ReadXml_AST::convertNameMap2StringRefMap(StringRefMap<char> *map, int mapKind, void *target) {
  return false;
}

bool ReadXml_AST::convertNameMap2StringSObjDict(StringRefMap<char> *map, int mapKind, void *target)
{
  return false;
}

void *ReadXml_AST::ctorNodeFromTag(int tag) {
  switch(tag) {
  default: userError("unexpected token while looking for an open tag name");
  case 0: userError("unexpected file termination while looking for an open tag name");
    //      // INSERT per ast node
    //      case XTOK_TranslationUnit:
    //        topTemp = new TranslationUnit(0);
    //        break;
#include "astxml_parse1_2ctrc.gen.cc"
  }
}

void ReadXml_AST::registerAttribute(void *target, int kind, int attr, char const *yytext0) {
  switch(kind) {
  default: xfailure("illegal kind");

//        // INSERT per ast node
//        case XTOK_TranslationUnit:
//          registerAttr_TranslationUnit((TranslationUnit*)nodeStack.top(), attr, lexer.YYText());
#include "astxml_parse1_3regc.gen.cc"
  }
}

#include "astxml_parse1_1defn.gen.cc"


// -------------------- astxmlparse -------------------

TranslationUnit *astxmlparse(StringTable &strTable, char const *inputFname)
{
  LinkSatisfier linkSatisifier;

  ifstream in(inputFname);
  AstXmlLexer lexer(inputFname);
  lexer.restart(&in);

  // this is going to parse one top-level tag
  ReadXml_AST astReader(inputFname, lexer, strTable, linkSatisifier);
  linkSatisifier.registerReader(&astReader);
  astReader.reset();            // actually done in ctor
  astReader.parseOneTopLevelTag();
  if (lexer.haveSeenEof()) {
    astReader.userError("unexpected EOF");
  }
  if (astReader.getLastKind() != XTOK_TranslationUnit) {
    astReader.userError("top tag is not a TranslationUnit");
  }
  TranslationUnit *tunit = (TranslationUnit*) astReader.getLastNode();

  // this is going to parse a sequence of top-level Type tags; stops
  // at EOF
  BasicTypeFactory tFac;
  ReadXml_Type typeReader(inputFname, lexer, strTable, linkSatisifier, tFac);
  linkSatisifier.registerReader(&typeReader);
  // FIX: a do-while loop is always a bug; we need to be able to see
  // that the EOF is coming to turn this into a while loop
  do {
    typeReader.reset();
    typeReader.parseOneTopLevelTag();
  } while (!lexer.haveSeenEof());

  // complete the link graph
  linkSatisifier.satisfyLinks();

  return tunit;
}
