// main_astxmlparse.cc          see license.txt for copyright and terms of use

#include "main_astxmlparse.h"   // this module
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
  // map a kind to its kind category
  KindCategory kind2kindCat(int kind);

  // operate on kinds of lists
  void *prepend2FakeList(void *list, int listKind, void *datum, int datumKind);
  void *reverseFakeList(void *list, int listKind);

  void append2ASTList(void *list, int listKind, void *datum, int datumKind);

  void prepend2ObjList(void *list, int listKind, void *datum, int datumKind);
  void reverseObjList(void *list, int listKind);

  void prepend2SObjList(void *list, int listKind, void *datum, int datumKind);
  void reverseSObjList(void *list, int listKind);

  // construct a node for a tag
  bool ctorNodeFromTag(int tag, void *&topTemp);
  // register an attribute into the current node
  void registerAttribute(void *target, int kind, int attr, char const *yytext0);

//    // INSERT per ast node
//    void registerAttr_TranslationUnit(TranslationUnit *obj, int attr, char const *strValue);
#include "astxml_parse1_0decl.gen.cc"
};

bool ReadXml_AST::ctorNodeFromTag(int tag, void *&topTemp) {
  switch(tag) {
  default: userError("unexpected token while looking for an open tag name");
  case 0: userError("unexpected file termination while looking for an open tag name");
  case XTOK_SLASH:
    return true;
    break;

    //      // INSERT per ast node
    //      case XTOK_TranslationUnit:
    //        topTemp = new TranslationUnit(0);
    //        break;
#include "astxml_parse1_2ctrc.gen.cc"
  }
  return false;
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

//  // INSERT per ast node
//  void ReadXml::registerAttr_TranslationUnit(TranslationUnit *obj, int attr, char const *strValue)
//  {
//    switch(attr) {
//    default:
//      userError("illegal attribute for a TranslationUnit");
//      break;
//    case XTOK_topForms:
//  //      obj->topForms = strdup(strValue);
//      break;
//    }
//  }

void ReadXml_AST::prepend2ObjList(void *list, int listKind, void *datum, int datumKind) {
  switch(listKind) {
  default: xfailure("attempt to prepend to a non-ObjList token kind");
//    case XTOK_FakeList_MemberInit:
//      if (!datumKind == XTOK_MemberInit) {
//        userError("can't put that onto a FakeList of MemberInit");
//      }
//      return ((FakeList<MemberInit>*)list)->prepend((MemberInit*)datum);
//      break;
  }
}

void ReadXml_AST::reverseObjList(void *list, int listKind) {
  switch(listKind) {
  default: xfailure("attempt to reverse a non-ObjList token kind");
//    case XTOK_FakeList_MemberInit:
//      return ((FakeList<MemberInit>*)list)->reverse();
//      break;
  }
}

void ReadXml_AST::prepend2SObjList(void *list, int listKind, void *datum, int datumKind) {
  switch(listKind) {
  default: xfailure("attempt to prepend to a non-SObjList token kind");
//    case XTOK_FakeList_MemberInit:
//      if (!datumKind == XTOK_MemberInit) {
//        userError("can't put that onto a FakeList of MemberInit");
//      }
//      return ((FakeList<MemberInit>*)list)->prepend((MemberInit*)datum);
//      break;
  }
}

void ReadXml_AST::reverseSObjList(void *list, int listKind) {
  switch(listKind) {
  default: xfailure("attempt to reverse a non-SObjList token kind");
//    case XTOK_FakeList_MemberInit:
//      return ((FakeList<MemberInit>*)list)->reverse();
//      break;
  }
}

#include "astxml_parse1_1defn.gen.cc"


// -------------------- astxmlparse -------------------

TranslationUnit *astxmlparse(StringTable &strTable, char const *inputFname)
{
  LinkSatisfier linkSatisifier;

  ifstream in(inputFname);
  AstXmlLexer lexer(inputFname);
  lexer.yyrestart(&in);

  // this is going to parse one top-level tag
  ReadXml_AST astReader(inputFname, lexer, strTable, linkSatisifier);
  bool sawEof = astReader.parse();
  xassert(!sawEof);
  TranslationUnit *tunit = (TranslationUnit*) astReader.getLastNode();

  // this is going to parse a sequence of top-level Type tags; stops
  // at EOF
  BasicTypeFactory tFac;
  ReadXml_Type typeReader(inputFname, lexer, strTable, linkSatisifier, tFac);
  while(1) {
    bool sawEof = typeReader.parse();
    if (sawEof) break;
    // should get entered into the linkSatisifier so no need to save
    // it here
  }

  // complete the link graph
  linkSatisifier.satisfyLinks();

  return tunit;
}