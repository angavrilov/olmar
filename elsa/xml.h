// xml.h            see license.txt for copyright and terms of use

// Serialization and de-serialization support.

#ifndef XML_H
#define XML_H

#include "strsobjdict.h"        // StringSObjDict
#include "sobjstack.h"          // SObjStack
#include "objstack.h"           // ObjStack
#include "astlist.h"            // ASTList
#include "strtable.h"           // StringRef
#include "strmap.h"             // StringRefMap

class AstXmlLexer;
class StringTable;

// forwards in this file
class ReadXml;



// there are 3 categories of kinds of Tags
enum KindCategory {
  KC_Node,                      // a normal node
  // list
  KC_ASTList,
  KC_FakeList,
  KC_ObjList,
  KC_SObjList,
  // name map
  KC_StringRefMap,
  KC_StringSObjDict,
  // a name entry in a name map
  KC_Name,
};

// the <__Name> </__Name> tag is parsed into this class to hold the
// name while the value contained by it is being parsed.  Then it is
// deleted.
struct Name {
  StringRef name;

  Name() {}
  Name(StringRef name0) : name(name0) {}
  // FIX: do I destruct/free() the name when I destruct the object?
};


// -------------------- LinkSatisfier -------------------

// datastructures for dealing with unsatisified links; FIX: we can
// do the in-place recording of a lot of these unsatisified links
// (not the ast links)
struct UnsatLink {
  void **ptr;
  string id;
  int kind;
  UnsatLink(void **ptr0, string id0, int kind0 = -1);
};

// datastructures for dealing with unsatisified links where neither
// party wants to know about the other
struct UnsatBiLink {
  char const *from;
  char const *to;
  UnsatBiLink() : from(NULL), to(NULL) {}
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
  ASTList<UnsatLink> unsatLinks;
  ASTList<UnsatLink> unsatLinks_List;
  ASTList<UnsatLink> unsatLinks_NameMap;
  ASTList<UnsatBiLink> unsatBiLinks;

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
  void convertList2SObjList(ASTList<char> *list, int listKind, void **target);
  void convertList2ObjList (ASTList<char> *list, int listKind, void **target);

  void convertNameMap2StringRefMap  (StringRefMap<char> *map, int listKind, void *target);
  void convertNameMap2StringSObjDict(StringRefMap<char> *map, int listKind, void *target);

  bool kind2kindCat(int kind, KindCategory *kindCat);

  void satisfyLinks();

  private:
  void satisfyLinks_Nodes();
  void satisfyLinks_Lists();
  void satisfyLinks_Maps();
//    void satisfyLinks_Bidirectional();

  void userError(char const *msg);
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
  {
    reset();
  }
  virtual ~ReadXml() {}

  // reset the internal state
  void reset();

  // FIX: this API is borked; really you should be able to push the
  // EOF back onto the lexer so it can be read again in the client,
  // rather than having to return it in this API.  Since flex doesn't
  // let you do that, we should wrap flex in an object that does.
  //
  // parse one top-level tag; return true if we also reached the end
  // of the file
  void parseOneTopLevelTag();
  // parse one tag; return false if we reached the end of a top-level
  // tag; sawEof set to true if we also reached the end of the file
  void parseOneTag();

  // are we at the top level during parsing?
  bool atTopLevel() {return nodeStack.isEmpty();}

  // report an error to the user with source location information
  void userError(char const *msg) NORETURN;

  // return the top of the stack: the one tag that was parsed
  void *getLastNode() {return lastNode;}
  int getLastKind() {return lastKind;}

  protected:
  bool readAttributes();

  // **** subclass fills these in
  public:

  // insert elements into containers
  virtual void append2List(void *list, int listKind, void *datum, int datumKind) = 0;
  virtual void insertIntoNameMap
    (void *map, int mapKind, StringRef name, void *datum, int datumKind) = 0;

  // map a kind to its kind category
  virtual bool kind2kindCat(int kind, KindCategory *ret) = 0;

  // all lists are stored as ASTLists; these convert to the real list
  virtual bool convertList2FakeList(ASTList<char> *list, int listKind, void **target) = 0;
  virtual bool convertList2SObjList(ASTList<char> *list, int listKind, void **target) = 0;
  virtual bool convertList2ObjList (ASTList<char> *list, int listKind, void **target) = 0;

  // all name maps are stored as StringRefMaps; these convert to the real name maps
  virtual bool convertNameMap2StringRefMap
    (StringRefMap<char> *map, int mapKind, void *target) = 0;
  virtual bool convertNameMap2StringSObjDict
    (StringRefMap<char> *map, int mapKind, void *target) = 0;

  // construct a node for a tag; returns true if it was a closeTag
  virtual bool ctorNodeFromTag(int tag, void *&topTemp) = 0;
  // register an attribute into the current node
  virtual void registerAttribute(void *target, int kind, int attr, char const *yytext0) = 0;
};

#endif // XML_H
