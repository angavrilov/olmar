// xml.h            see license.txt for copyright and terms of use

// Generic serialization and de-serialization support.

#ifndef XML_H
#define XML_H

#include "strsobjdict.h"        // StringSObjDict
#include "sobjstack.h"          // SObjStack
#include "objstack.h"           // ObjStack
#include "astlist.h"            // ASTList

class AstXmlLexer;
class StringTable;

// forwards in this file
class ReadXml;


// I have manually mangled the name to include "_bool" as otherwise
// what happens is that if a toXml() for some enum flag is missing
// then the C++ compiler will just use the toXml(bool) instead, which
// is a bug.
string toXml_bool(bool b);
void fromXml_bool(bool &b, string str);


// there are 3 categories of kinds of Tags
enum KindCategory {
  KC_Node,                      // a normal node
  KC_ASTList,                   // an ast list
  KC_FakeList,                  // a fake list
  KC_ObjList,                   // an ObjList
  KC_SObjList,                  // an SObjList
  KC_StringRefMap,              // a StringRefMap
};


// -------------------- LinkSatisfier -------------------

// datastructures for dealing with unsatisified links; FIX: we can
// do the in-place recording of a lot of these unsatisified links
// (not the ast links)
struct UnsatLink {
  void **ptr;
  string id;
  int kind;
  UnsatLink(void **ptr0, string id0, int kind0);
};

// manages recording unsatisfied links and satisfying them later
class LinkSatisfier {
  // I need this because it know how to get the FakeLists out of the
  // generic (AST) lists.  This is a bit of a kludge.
  ASTList<ReadXml> readers;

  public:
  // Since AST nodes are embedded, we have to put this on to a
  // different list than the ususal pointer unsatisfied links.  I just
  // separate out all lists.
  ASTList<UnsatLink> unsatLinks_List;
  ASTList<UnsatLink> unsatLinks;

  // map object ids to the actual object
  StringSObjDict<void> id2obj;

  public:
  LinkSatisfier() {}
  ~LinkSatisfier() {
    // FIX: don't delete these for now so that when we destruct we
    // won't take them with us
    readers.removeAll_dontDelete();
  }

  void registerReader(ReadXml *reader);
  void *convertList2FakeList(ASTList<char> *list, int listKind);
  bool kind2kindCat(int kind, KindCategory *kindCat);
  void satisfyLinks();
};


// -------------------- ReadXml -------------------

// Framework for reading an XML file.  A subclass must fill in the
// methods that need to know about individual tags.

class ReadXml {
  protected:
  char const *inputFname;       // just for error messages
  AstXmlLexer &lexer;           // a lexer on a stream already opened from the file
  StringTable &strTable;        // for canonicalizing the StringRef's in the input file
  LinkSatisfier &linkSat;

  private:
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
  virtual ~ReadXml() {}

  // reset the internal state
  void reset();
  // parse one top-level tag
  bool parse();
  // return the top of the stack: the one tag that was parsed
  void *getLastNode() {return lastNode;}

  private:
  void readAttributes();

  protected:
  void userError(char const *msg) NORETURN;

  // **** subclass fills these in
  public:
  // append a list element to a list
  virtual void append2List(void *list, int listKind, void *datum, int datumKind) = 0;
  // map a kind to its kind category
  virtual bool kind2kindCat(int kind, KindCategory *ret) = 0;
  // all lists are stored as ASTLists; this converts to FakeLists
  virtual bool convertList2FakeList(ASTList<char> *list, int listKind, void **target) = 0;
  // construct a node for a tag; returns true if it was a closeTag
  virtual bool ctorNodeFromTag(int tag, void *&topTemp) = 0;
  // register an attribute into the current node
  virtual void registerAttribute(void *target, int kind, int attr, char const *yytext0) = 0;
};

#endif // XML_H
