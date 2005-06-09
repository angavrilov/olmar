// main_astxmlparse.cc          see license.txt for copyright and terms of use

#include "main_astxmlparse.h"   // this module
#include "fstream.h"            // ifstream
#include "strsobjdict.h"        // StringSObjDict
#include "astxml_lexer.h"       // AstXmlLexer

#include "cc.ast.gen.h"         // TranslationUnit, etc.

class ReadXml {
  public:
  char const *inputFname;
  AstXmlLexer &lexer;

  // the node (and its kind) for the last closing tag we saw; useful
  // for extracting the top of the tree
  void *lastNode;
  int lastKind;

  // parsing stack
  SObjStack<void> nodeStack;
  ObjStack<int> kindStack;

  // datastructures for dealing with unsatisified links; FIX: we can
  // do the in-place recording of a lot of these unsatisified links
  // (not the ast links)
  struct unsatLink {
    void *ptr; string id;
    unsatLink(void *ptr0, string id0) : ptr(ptr0), id(id0) {};
  };
  // Since AST nodes are embedded, we have to put this on to a
  // different list than the ususal pointer unsatisfied links.  I have
  // to separate ASTList unsatisfied links out, so I might as well
  // just separate everything.
  ASTList<unsatLink> unsatLinks_ASTList;
  ASTList<unsatLink> unsatLinks_FakeList;
  ASTList<unsatLink> unsatLinks;

  // map object ids to the actual object
  StringSObjDict<void> id2obj;

  public:
  ReadXml(char const *inputFname0, AstXmlLexer &lexer0)
    : inputFname(inputFname0)
    , lexer(lexer0)
    , lastNode(NULL)
    , lastKind(0)
  {}

//    // INSERT per ast node
//    void registerAttr_TranslationUnit(TranslationUnit *obj, int attr, char const *strValue);
#include "astxml_parse1_0rdecl.gen.cc"

  void userError(char const *msg) NORETURN;
  void readAttributes();
  void go();
};

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
#include "astxml_parse1_1rdefn.gen.cc"

void ReadXml::userError(char const *msg) {
  THROW(xBase(stringc << inputFname << ":" << lexer.linenumber << ":" << msg));
}

void ReadXml::go() {
  while(1) {
    // state: looking for a tag start
    int start = lexer.yylex();
//      printf("start:%s\n", lexer.tokenKindDesc(start).c_str());
    switch(start) {
    default: userError("unexpected token while looking for '<' of an open tag");
    case 0: return;             // we are done
    case XTOK_LESSTHAN:
      break;                    // go to next state
    }

    // state: read a tag name
    int tag = lexer.yylex();
    void *topTemp;
    switch(tag) {
    default: userError("unexpected token while looking for an open tag name");
    case 0: userError("unexpected file termination while looking for an open tag name");
    case XTOK_SLASH:
      goto close_tag;
//      // INSERT per ast node
//      case XTOK_TranslationUnit:
//        topTemp = new TranslationUnit(0);
//        break;
#include "astxml_parse1_2ccall.gen.cc"
    }
    nodeStack.push(topTemp);
    kindStack.push(new int(tag));
    readAttributes();
    continue;

    // state: read a close tag name
  close_tag:
    int closeTag = lexer.yylex();
    if (!closeTag) {
      userError("unexpected file termination while looking for a close tag name");
    }
    if (nodeStack.isEmpty()) {
      userError("too many close tags");
    }
    if (*kindStack.top() != closeTag) {
      userError("close tag does not match open tag");
    }

    int closeGreaterThan = lexer.yylex();
    switch(closeGreaterThan) {
    default: userError("unexpected token while looking for '>' of a close tag");
    case 0: userError("unexpected file termination while looking for '>' of a close tag");
    case XTOK_GREATERTHAN:
      break;
    }

    lastNode = nodeStack.pop();
    // FIX: do I delete this int on the heap or does the stack do it?
    lastKind = *kindStack.pop();
  }
}

// state: read the attributes
void ReadXml::readAttributes() {
  while(1) {
    int attr = lexer.yylex();
    switch(attr) {
    default: break;             // go on; assume it is a legal attribute tag
    case 0: userError("unexpected file termination while looking for an attribute name");
    case XTOK_GREATERTHAN:
      return;
    }

    int eq = lexer.yylex();
    switch(eq) {
    default: userError("unexpected token while looking for an '='");
    case 0: userError("unexpected file termination while looking for an '='");
    case XTOK_EQUAL:
      break;                    // go on
    }

    int value = lexer.yylex();
    switch(value) {
    default: userError("unexpected token while looking for an attribute value");
    case 0: userError("unexpected file termination while looking for an attribute value");
    case XTOK_INT_LITERAL:
      // get it out of yytext below
      break;
    case XTOK_STRING_LITERAL:
      // get it out of yytext below
      break;                    // go on
    }

    // register the attribute
    xassert(nodeStack.isNotEmpty());
    // special case the .id attribute
    if (attr == XTOK_DOT_ID) {
      // FIX: I really hope the map makes a copy of this string
      if (strcmp(lexer.YYText(), "\"FL0\"") == 0
          // these should not be possible and the isMapped check below
          // would catch it if they occured:
          //  || strcmp(lexer.YYText(), "0AL") == 0
          //  || strcmp(lexer.YYText(), "0ND") == 0
          ) {
        // otherwise its null and there is nothing to record
        if (nodeStack.top()) {
          userError("FakeList with FL0 id should be empty");
        }
      } else {
        if (id2obj.isMapped(lexer.YYText())) {
          userError(stringc << "this id is taken " << lexer.YYText());
        }
        id2obj.add(lexer.YYText(), nodeStack.top());
      }
    } else {
      switch(*kindStack.top()) {
      default: xfailure("illegal kind");
//        // INSERT per ast node
//        case XTOK_TranslationUnit:
//          registerAttr_TranslationUnit((TranslationUnit*)nodeStack.top(), attr, lexer.YYText());
#include "astxml_parse1_3rcall.gen.cc"
      }
    }
  }
}

TranslationUnit *astxmlparse(StringTable &strTable, char const *inputFname)
{
  ifstream in(inputFname);
  AstXmlLexer lexer(inputFname);
  lexer.yyrestart(&in);
  ReadXml reader(inputFname, lexer);
  reader.go();
  if (reader.lastKind != XTOK_TranslationUnit) {
    reader.userError("top tag is not a TranslationUnit");
  }
  return (TranslationUnit*) reader.lastNode;
}
