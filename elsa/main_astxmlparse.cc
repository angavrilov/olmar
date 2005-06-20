// main_astxmlparse.cc          see license.txt for copyright and terms of use

#include "main_astxmlparse.h"   // this module
#include "fstream.h"            // ifstream
#include "strsobjdict.h"        // StringSObjDict
#include "strutil.h"            // parseQuotedString
#include "astxml_lexer.h"       // AstXmlLexer

#include "cc.ast.gen.h"         // TranslationUnit, etc.

// there are 3 categories of kinds of Tags
enum KindCategory {
  KC_Node,                      // a normal node
  KC_ASTList,                   // an ast list
  KC_FakeList,                  // a fake list
};

// datastructures for dealing with unsatisified links; FIX: we can
// do the in-place recording of a lot of these unsatisified links
// (not the ast links)
struct UnsatLink {
  void **ptr;
  string id;
  UnsatLink(void **ptr0, string id0)
    : ptr(ptr0), id(id0)
  {};
};

// manages recording unsatisfied links and satisfying them later
class LinkSatisfier {
  public:
  // Since AST nodes are embedded, we have to put this on to a
  // different list than the ususal pointer unsatisfied links.  I have
  // to separate ASTList unsatisfied links out, so I might as well
  // just separate everything.
  ASTList<UnsatLink> unsatLinks_ASTList;
  ASTList<UnsatLink> unsatLinks_FakeList;
  ASTList<UnsatLink> unsatLinks;

  // map object ids to the actual object
  StringSObjDict<void> id2obj;

  public:
  LinkSatisfier() {}

  void satisfyLinks();
};

class ReadXml {
  private:
  // **** input state
  char const *inputFname;       // just for error messages
  AstXmlLexer &lexer;           // a lexer on a stream already opened from the file
  StringTable &strTable;        // for canonicalizing the StringRef's in the input file

  LinkSatisfier &linkSat;

  // **** internal state
  // the node (and its kind) for the last closing tag we saw; useful
  // for extracting the top of the tree
  void *lastNode;
  int lastKind;

  // the last FakeList id seen; just one of those odd things you need
  // in a parser
  char const *lastFakeListId;

  // parsing stack
  SObjStack<void> nodeStack;
  ObjStack<int> kindStack;

  public:
  ReadXml(char const *inputFname0,
          AstXmlLexer &lexer0,
          StringTable &strTable0,
          LinkSatisfier &linkSat0)
    : inputFname(inputFname0)
    , lexer(lexer0)
    , strTable(strTable0)
    , linkSat(linkSat0)
    // done in reset()
//      , lastNode(NULL)
//      , lastKind(0)
//      , lastFakeListId(NULL)
  {
    reset();
  }

  private:
//    // INSERT per ast node
//    void registerAttr_TranslationUnit(TranslationUnit *obj, int attr, char const *strValue);
#include "astxml_parse1_0decl.gen.cc"
  
  // the implementation of these is generated
  KindCategory kind2kindCat(int kind);// map a kind to its kind category
  void *prepend2FakeList(void *list, int listKind, void *datum, int datumKind); // generic prepend
  void *reverseFakeList(void *list, int listKind); // generic reverse
  void append2ASTList(void *list, int listKind, void *datum, int datumKind); // generic append

  void userError(char const *msg) NORETURN;
  void readAttributes();

  public:
  void reset();
  bool parse();
  void *getLastNode() {return lastNode;}
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
#include "astxml_parse1_1defn.gen.cc"

void ReadXml::userError(char const *msg) {
  THROW(xBase(stringc << inputFname << ":" << lexer.linenumber << ":" << msg));
}

void ReadXml::reset() {
  lastNode = NULL;
  lastKind = 0;
  lastFakeListId = NULL;
  xassert(nodeStack.isEmpty());
  xassert(kindStack.isEmpty());
}

bool ReadXml::parse() {
  reset();
  while(1) {
    // state: looking for a tag start
    int start = lexer.yylex();
//      printf("start:%s\n", lexer.tokenKindDesc(start).c_str());
    switch(start) {
    default: userError("unexpected token while looking for '<' of an open tag");
    case 0: return true;        // we are done
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
#include "astxml_parse1_2ctrc.gen.cc"
    }
    nodeStack.push(topTemp);
    kindStack.push(new int(tag));

    // This funnyness is due to the fact that when we see a FakeList
    // tag, we make an empty FakeList, so there is nothing to put into
    // in the id2obj map, so we don't put anything there.  Instead we
    // store the id in lastFakeListId and then when we get the first
    // child, we finally know the FakeList address so we file that
    // under the id.
    if (lastFakeListId) {
      if (linkSat.id2obj.isMapped(lastFakeListId)) {
        userError(stringc << "this id is taken " << lastFakeListId);
      }
//        cout << "(FakeList) id2obj.add(" << lastFakeListId
//             << ", " << static_cast<void const*>(nodeStack.top())
//             << ")\n";
      linkSat.id2obj.add(lastFakeListId, nodeStack.top());
      delete lastFakeListId;
      lastFakeListId = NULL;
    }

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

    // if the last kind was a fakelist, reverse it
    if (kind2kindCat(lastKind) == KC_FakeList) {
      reverseFakeList(lastNode, lastKind);
      // NOTE: it may seem rather bizarre, but we just throw away the
      // reversed list.  The first node in the list was the one that
      // we saw first and it was filed in the hashtable under the id
      // of the first element.  Therefore, whatever points to the
      // first element in the list will do so when the unsatisfied
      // links are filled in from the hashtable.
    }

    // if the node up the stack is a list, put this element onto that
    // list
    if (nodeStack.isEmpty()) {
      // If the stack is empty, return
      xassert(kindStack.isEmpty());
      return false;
    } else {
      int topKind = *kindStack.top();
      int topKindCat = kind2kindCat(topKind);
      if (topKindCat == KC_FakeList) {
        // there is no way to reset the top so I just pop and push again
//          cout << "prepend2FakeList nodeStack.top(): " << static_cast<void const*>(nodeStack.top());
        void *tmpNewTop = prepend2FakeList(nodeStack.pop(), topKind, lastNode, lastKind);
//          cout << ", tmpNewTop: " << static_cast<void const*>(tmpNewTop) << endl;
        nodeStack.push(tmpNewTop);
      } else if (topKindCat == KC_ASTList) {
        append2ASTList(nodeStack.top(), topKind, lastNode, lastKind);
      }
    }
  }
  if (lastKind != XTOK_TranslationUnit) {
    userError("top tag is not a TranslationUnit");
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
      string id0 = parseQuotedString(lexer.YYText());
      if (strcmp(id0.c_str(), "FL0") == 0
          // these should not be possible and the isMapped check below
          // would catch it if they occured:
          //  || strcmp(id0.c_str(), "0AL") == 0
          //  || strcmp(id0.c_str(), "0ND") == 0
          ) {
        // otherwise its null and there is nothing to record; UPDATE:
        // I no longer understand this code here
        if (nodeStack.top()) {
          userError("FakeList with FL0 id should be empty");
        }
      } else if (strncmp(id0.c_str(), "FL", 2) == 0) {
        xassert(!lastFakeListId);
        lastFakeListId = strdup(id0.c_str());
      } else {
        if (linkSat.id2obj.isMapped(id0)) {
          userError(stringc << "this id is taken " << id0);
        }
//          cout << "id2obj.add(" << id0
//               << ", " << static_cast<void const*>(nodeStack.top())
//               << ")\n";
        linkSat.id2obj.add(id0, nodeStack.top());
      }
    }

    // attribute other than '.id'
    else {
      switch(*kindStack.top()) {
      default: xfailure("illegal kind");
//        // INSERT per ast node
//        case XTOK_TranslationUnit:
//          registerAttr_TranslationUnit((TranslationUnit*)nodeStack.top(), attr, lexer.YYText());
#include "astxml_parse1_3regc.gen.cc"
      }
    }
  }
  if (!nodeStack.isEmpty()) {
    userError("missing closing tags at eof");
  }
  // stacks should not be out of sync
  xassert(kindStack.isEmpty());
}

void LinkSatisfier::satisfyLinks() {
  // Nodes
  FOREACH_ASTLIST(UnsatLink, unsatLinks, iter) {
    UnsatLink const *ul = iter.data();
    void *obj = id2obj.queryif(ul->id);
    if (obj) {
      *(ul->ptr) = obj;         // ahhhh!
    } else {
      // no satisfaction was provided for this link; for now we just
      // skip it, but if you wanted to report that in some way, here
      // is the place to do it
//        cout << "unsatisfied node link: " << ul->id << endl;
    }
  }

  // FakeLists
  FOREACH_ASTLIST(UnsatLink, unsatLinks_FakeList, iter) {
    UnsatLink const *ul = iter.data();
    void *obj = id2obj.queryif(ul->id);
    if (obj) {
      *(ul->ptr) = obj;         // ahhhh!
    } else {
      // no satisfaction was provided for this link; for now we just
      // skip it, but if you wanted to report that in some way, here
      // is the place to do it
//        cout << "unsatisfied FakeList link: " << ul->id << endl;
    }
  }

  // ASTLists
  FOREACH_ASTLIST(UnsatLink, unsatLinks_ASTList, iter) {
    UnsatLink const *ul = iter.data();
    // NOTE: I rely on the fact that all ASTLists just contain
    // pointers; otherwise this cast would cause problems; Note that I
    // have to use char instead of void because you can't delete a
    // pointer to void; see the note in the if below.
    ASTList<char> *obj = reinterpret_cast<ASTList<char>*>(id2obj.queryif(ul->id));
    if (obj) {
      ASTList<char> *ptr = reinterpret_cast<ASTList<char>*>(ul->ptr);
      xassert(ptr->isEmpty());
      // this is particularly tricky because the steal contains a
      // delete, however there is nothing to delete, so we should be
      // ok.  If there were something to delete, we would be in
      // trouble because you can't call delete on a pointer to an
      // object the size of which is different from the size you think
      // it is due to a type you cast it too.
      ptr->steal(obj);          // ahhhh!
    } else {
      // no satisfaction was provided for this link; for now we just
      // skip it, but if you wanted to report that in some way, here
      // is the place to do it
//        cout << "unsatisfied ASTList link: " << ul->id << endl;
    }
  }
}

TranslationUnit *astxmlparse(StringTable &strTable, char const *inputFname)
{
  LinkSatisfier linkSatisifier;

  ifstream in(inputFname);
  AstXmlLexer lexer(inputFname);
  lexer.yyrestart(&in);
  ReadXml reader(inputFname, lexer, strTable, linkSatisifier);

  // this is going to parse one top-level tag
  bool sawEof = reader.parse();
  xassert(!sawEof);
  TranslationUnit *tunit = (TranslationUnit*) reader.getLastNode();

  // look for the eof
  sawEof = reader.parse();
  xassert(sawEof);

  // complete the link graph
  linkSatisifier.satisfyLinks();

  return tunit;
}
