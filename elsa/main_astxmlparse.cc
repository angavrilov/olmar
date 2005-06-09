// main_astxmlparse.cc          see license.txt for copyright and terms of use

#include "main_astxmlparse.h"   // this module
#include "fstream.h"            // ifstream
#include "astxml_lexer.h"       // AstXmlLexer

#include "cc.ast.gen.h"         // TranslationUnit, etc.

class ReadXml {
  public:
  char const *inputFname;
  AstXmlLexer &lexer;
  void *lastNode;
  int lastKind;
  SObjStack<void> nodeStack;
  ObjStack<int> kindStack;

  public:
  ReadXml(char const *inputFname0, AstXmlLexer &lexer0)
    : inputFname(inputFname0)
    , lexer(lexer0)
    , lastNode(NULL)
    , lastKind(0)
  {}

  // these will be generated per AST node
  void registerAttr_TranslationUnit(TranslationUnit *obj, int attr, char const *strValue);

  void userError(char *msg) NORETURN;
  void readAttributes();
  void go();
};

void ReadXml::registerAttr_TranslationUnit(TranslationUnit *obj, int attr, char const *strValue)
{
  switch(attr) {
  default:
    userError("illegal attribute for a TranslationUnit");
    break;
  case XTOK_topForms:
//      obj->topForms = strdup(strValue);
    break;
  }
}

void ReadXml::userError(char *msg) {
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
    case XTOK_TranslationUnit:
      topTemp = new TranslationUnit(0);
      break;
    case XTOK_ASTList_TopForm:
      topTemp = new int;        // FIX:
      break;
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
    // FIX: do I delete this or does the stack do it?
    lastKind = *kindStack.pop();
  }
}

// state: read the attributes
void ReadXml::readAttributes() {
  while(1) {
    int attr = lexer.yylex();
    switch(attr) {
    case 0: userError("unexpected file termination while looking for an attribute name");
    case XTOK_GREATERTHAN:
      return;
    default:
      break;                    // go on; assume it is a legal attribute tag
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
      // FIX: file the object under its id
    } else {
      switch(*kindStack.top()) {
      case XTOK_TranslationUnit:
        registerAttr_TranslationUnit((TranslationUnit*)nodeStack.top(), attr, lexer.YYText());
      }
    }
  }
}

TranslationUnit *astxmlparse(StringTable &strTable, char const *inputFname)
{
  ifstream in(inputFname);
  AstXmlLexer lexer;
  lexer.yyrestart(&in);
  ReadXml reader(inputFname, lexer);
  reader.go();
  if (reader.lastKind != XTOK_TranslationUnit) {
    reader.userError("top tag is not a TranslationUnit");
  }
  return (TranslationUnit*) reader.lastNode;
}
