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


// I have manually mangled the name to include "_bool" as otherwise
// what happens is that if a toXml() for some enum flag is missing
// then the C++ compiler will just use the toXml(bool) instead, which
// is a bug.
string toXml_bool(bool b);
void fromXml_bool(bool &b, string str);



// -------------------- LinkSatisfier -------------------

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


// -------------------- ReadXml -------------------

// Framework for reading an XML file.  A subclass must fill in the
// methods that need to know about individual tags.

// there are 3 categories of kinds of Tags
enum KindCategory {
  KC_Node,                      // a normal node
  KC_ASTList,                   // an ast list
  KC_FakeList,                  // a fake list
  KC_ObjList,                   // an ObjList
  KC_SObjList,                  // an SObjList
};

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

  // map a kind to its kind category
  virtual KindCategory kind2kindCat(int kind) = 0;

  // operate on kinds of lists
  virtual void *prepend2FakeList(void *list, int listKind, void *datum, int datumKind) = 0;
  virtual void *reverseFakeList(void *list, int listKind) = 0;

  virtual void append2ASTList(void *list, int listKind, void *datum, int datumKind) = 0;

  virtual void prepend2ObjList(void *list, int listKind, void *datum, int datumKind) = 0;
  virtual void reverseObjList(void *list, int listKind) = 0;

  virtual void prepend2SObjList(void *list, int listKind, void *datum, int datumKind) = 0;
  virtual void reverseSObjList(void *list, int listKind) = 0;

  // construct a node for a tag; returns true if it was a closeTag
  virtual bool ctorNodeFromTag(int tag, void *&topTemp) = 0;
  // register an attribute into the current node
  virtual void registerAttribute(void *target, int kind, int attr, char const *yytext0) = 0;
};

#endif // XML_H
