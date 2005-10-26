// xml.h            see license.txt for copyright and terms of use

// Serialization and de-serialization support.

// FIX: this module should eventually go into the ast repository

#ifndef XML_H
#define XML_H

#include "strsobjdict.h"        // StringSObjDict
#include "sobjstack.h"          // SObjStack
#include "objstack.h"           // ObjStack
#include "astlist.h"            // ASTList
#include "strtable.h"           // StringRef
#include "strmap.h"             // StringRefMap
#include "xmlhelp.h"            // toXml_int etc.

class AstXmlLexer;
class StringTable;

// forwards in this file
class XmlReaderManager;


// **** Support for XML serialization

// to Xml for enums
#define PRINTENUM(X) case X: return #X
#define PRINTFLAG(X) if (id & (X)) b << #X

// manage identity of AST
char const *idPrefixAST(void const * const);
void const *addrAST(void const * const obj);

// manage identity; definitions
#define identity0(PREFIX, NAME, TEMPL) \
TEMPL char const *idPrefix(NAME const * const) {return #PREFIX;} \
TEMPL void const *addr(NAME const * const obj) {return reinterpret_cast<void const *>(obj);} \
TEMPL bool printed(NAME const * const obj) { \
  if (printedSet ##PREFIX.contains(obj)) return true; \
  printedSet ##PREFIX.add(obj); \
  return false; \
}
#define identity(PREFIX, NAME) identity0(PREFIX, NAME, )
#define identityTempl(PREFIX, NAME) identity0(PREFIX, NAME, template<class T>)
// declarations
#define identityB0(NAME, TEMPL) TEMPL bool printed(NAME const * const obj)
#define identityB(NAME) identityB0(NAME, )
#define identityBTempl(NAME) identityB0(NAME, template<class T>)

// manage the output stream
class ToXml {
  public:
  ostream &out;                 // output stream to which to print

  protected:
  int &depth;                   // ref so we can share our indentation depth with other printers
  bool indent;                  // should we print indentation?

  public:
  ToXml(ostream &out0, int &depth0, bool indent0=false);

  protected:
  // print a newline and indent if the user wants indentation; NOTE:
  // the convention is that you don't print a newline until you are
  // *sure* you have something to print that goes onto the next line;
  // that is, most lines do *not* end in a newline
  void newline();
  friend class XmlCloseTagPrinter;
};

// manage closing tags: indent and print something when exiting the
// scope
class XmlCloseTagPrinter {
  string s;                     // NOTE: don't make into a string ref; it must make a copy
  ToXml &ttx;

  public:
  XmlCloseTagPrinter(string s0, ToXml &ttx0)
    : s(s0), ttx(ttx0)
  {}

  private:
  explicit XmlCloseTagPrinter(XmlCloseTagPrinter &); // prohibit

  public:
  ~XmlCloseTagPrinter() {
    ttx.newline();
    ttx.out << "</" << s << ">";
  }
};

// manage indentation depth
class IncDec {
  int &x;
  public:
  explicit IncDec(int &x0) : x(x0) {++x;}
  private:
  explicit IncDec(const IncDec&); // prohibit
  public:
  ~IncDec() {--x;}
};

#define printThing0(NAME, VALUE) \
do { \
  out << #NAME "=" << xmlAttrQuote(VALUE); \
} while(0)

#define printThing(NAME, RAW, VALUE) \
do { \
  if (RAW) { \
    newline(); \
    printThing0(NAME, VALUE); \
  } \
} while(0)

#define printPtr(BASE, MEM)    printThing(MEM, (BASE)->MEM, xmlPrintPointer(idPrefix((BASE)->MEM), addr((BASE)->MEM)))
#define printPtrAST(BASE, MEM) printThing(MEM, (BASE)->MEM, xmlPrintPointer(idPrefixAST((BASE)->MEM), addrAST((BASE)->MEM)))
// print an embedded thing
#define printEmbed(BASE, MEM)  printThing(MEM, (&((BASE)->MEM)), xmlPrintPointer(idPrefix(&((BASE)->MEM)), addr(&((BASE)->MEM))))

// for unions where the member name does not match the xml name and we
// don't want the 'if'
#define printPtrUnion(BASE, MEM, NAME) printThing0(NAME, xmlPrintPointer(idPrefix((BASE)->MEM), addr((BASE)->MEM)))
// this is only used in one place
#define printPtrASTUnion(BASE, MEM, NAME) printThing0(NAME, xmlPrintPointer("AST", addrAST((BASE)->MEM)))

#define printXml(NAME, VALUE) \
do { \
  newline(); \
  printThing0(NAME, ::toXml(VALUE)); \
} while(0)

#define printXml_bool(NAME, VALUE) \
do { \
  newline(); \
  printThing0(NAME, ::toXml_bool(VALUE)); \
} while(0)

#define printXml_int(NAME, VALUE) \
do { \
  newline(); \
  printThing0(NAME, ::toXml_int(VALUE)); \
} while(0)

#define printXml_SourceLoc(NAME, VALUE) \
do { \
  newline(); \
  printThing0(NAME, ::toXml_SourceLoc(VALUE)); \
} while(0)

#define printStrRef(FIELD, TARGET) \
do { \
  if (TARGET) { \
    newline(); \
    out << #FIELD "=" << xmlAttrQuote(TARGET); \
  } \
} while(0)

// FIX: rename this; it also works for ArrayStacks
#define travObjList0(OBJ, BASETYPE, FIELD, FIELDTYPE, ITER_MACRO, LISTKIND) \
do { \
  if (!printed(&OBJ)) { \
    openTagWhole(List_ ##BASETYPE ##_ ##FIELD, &OBJ); \
    ITER_MACRO(FIELDTYPE, const_cast<LISTKIND<FIELDTYPE>&>(OBJ), iter) { \
      travListItem(iter.data()); \
    } \
  } \
} while(0)

#define travObjList_S(BASE, BASETYPE, FIELD, FIELDTYPE) \
travObjList0(BASE->FIELD, BASETYPE, FIELD, FIELDTYPE, SFOREACH_OBJLIST_NC, SObjList)

#define travObjList(BASE, BASETYPE, FIELD, FIELDTYPE) \
travObjList0(BASE->FIELD, BASETYPE, FIELD, FIELDTYPE, FOREACH_OBJLIST_NC, ObjList)

// Not tested; put the backslash back after the first line
//  #define travArrayStack(BASE, BASETYPE, FIELD, FIELDTYPE)
//  travObjList0(BASE->FIELD, BASETYPE, FIELD, FIELDTYPE, FOREACH_ARRAYSTACK_NC, ArrayStack)

#define travObjList_standalone(OBJ, BASETYPE, FIELD, FIELDTYPE) \
travObjList0(OBJ, BASETYPE, FIELD, FIELDTYPE, FOREACH_OBJLIST_NC, ObjList)

#define travPtrMap(BASE, BASETYPE, FIELD, FIELDTYPE) \
do { \
  if (!printed(&BASE->FIELD)) { \
    openTagWhole(NameMap_ ##BASETYPE ##_ ##FIELD, &BASE->FIELD); \
    for(PtrMap<char const, FIELDTYPE>::Iter iter(BASE->FIELD); \
        !iter.isDone(); \
        iter.adv()) { \
      StringRef name = iter.key(); \
      FIELDTYPE *var = iter.value(); \
      openTag_NameMap_Item(name, var); \
      trav(var); \
    } \
  } \
} while(0)

// NOTE: you must not wrap this one in a 'do {} while(0)': the dtor
// for the XmlCloseTagPrinter fires too early.
#define openTag0(NAME, OBJ, SUFFIX) \
  newline(); \
  out << "<" #NAME << " _id=" \
    << xmlAttrQuote(xmlPrintPointer(idPrefix(OBJ), addr(OBJ))) \
    << SUFFIX; \
  XmlCloseTagPrinter tagCloser(#NAME, *this); \
  IncDec depthManager(this->depth)

#define openTag(NAME, OBJ)      openTag0(NAME, OBJ, "")
#define openTagWhole(NAME, OBJ) openTag0(NAME, OBJ, ">")

// NOTE: you must not wrap this one in a 'do {} while(0)': the dtor
// for the XmlCloseTagPrinter fires too early.
#define openTag_NameMap_Item(NAME, TARGET) \
  newline(); \
  out << "<_NameMap_Item" \
      << " name=" << xmlAttrQuote(NAME) \
      << " item=" << xmlAttrQuote(xmlPrintPointer(idPrefix(TARGET), addr(TARGET))) \
      << ">"; \
  XmlCloseTagPrinter tagCloser("_NameMap_Item", *this); \
  IncDec depthManager(this->depth)

#define tagEnd \
do { \
  out << ">"; \
} while(0)

#define trav(TARGET) \
do { \
  if (TARGET) { \
    toXml(TARGET); \
  } \
} while(0)

// NOTE: you must not wrap this one in a 'do {} while(0)': the dtor
// for the XmlCloseTagPrinter fires too early.
#define travListItem(TARGET) \
  newline(); \
  out << "<_List_Item item=" \
    << xmlAttrQuote(xmlPrintPointer(idPrefix(TARGET), addr(TARGET))) \
    << ">"; \
  XmlCloseTagPrinter tagCloser("_List_Item", *this); \
  IncDec depthManager(this->depth); \
  trav(TARGET)

#define travAST(TARGET) \
do { \
  if (TARGET) { \
    (TARGET)->traverse(*astVisitor); \
  } \
} while(0)


// **** Support for XML de-serialization

// from Xml for enums
#define READENUM(X) else if (streq(str, #X)) out = (X)
#define READFLAG(X) else if (streq(token, #X)) out |= (X)

#define ul(FIELD, KIND) \
  manager->unsatLinks.append \
    (new UnsatLink((void*) &(obj->FIELD), \
                   xmlAttrDeQuote(strValue), \
                   (KIND), \
                   false))

#define ulEmbed(FIELD, KIND) \
  manager->unsatLinks.append \
    (new UnsatLink((void*) &(obj->FIELD), \
                   xmlAttrDeQuote(strValue), \
                   (KIND), \
                   true))

#define ulList(LIST, FIELD, KIND) \
  manager->unsatLinks##LIST.append \
    (new UnsatLink((void*) &(obj->FIELD), \
                   xmlAttrDeQuote(strValue), \
                   (KIND), \
                   true))

#define convertList(LISTTYPE, ITEMTYPE) \
do { \
  LISTTYPE<ITEMTYPE> *ret = reinterpret_cast<LISTTYPE<ITEMTYPE>*>(target); \
  xassert(ret->isEmpty()); \
  FOREACH_ASTLIST_NC(ITEMTYPE, reinterpret_cast<ASTList<ITEMTYPE>&>(*list), iter) { \
    ret->prepend(iter.data()); \
  } \
  ret->reverse(); \
} while(0)

#define convertArrayStack(LISTTYPE, ITEMTYPE) \
do { \
  LISTTYPE<ITEMTYPE> *ret = reinterpret_cast<LISTTYPE<ITEMTYPE>*>(target); \
  xassert(ret->isEmpty()); \
  FOREACH_ASTLIST_NC(ITEMTYPE, reinterpret_cast<ASTList<ITEMTYPE>&>(*list), iter) { \
    ret->push(*iter.data()); \
  } \
} while(0)

#define convertNameMap(MAPTYPE, ITEMTYPE) \
do { \
  MAPTYPE<ITEMTYPE> *ret = reinterpret_cast<MAPTYPE<ITEMTYPE>*>(target); \
  xassert(ret->isEmpty()); \
  for(StringRefMap<ITEMTYPE>::Iter \
        iter(reinterpret_cast<StringRefMap<ITEMTYPE>&>(*map)); \
      !iter.isDone(); iter.adv()) { \
    ret->add(iter.key(), iter.value()); \
  } \
} while(0)

// there are 3 categories of kinds of Tags
enum KindCategory {
  // normal node
  KC_Node,

  // list
  KC_ASTList,
  KC_FakeList,
  KC_ObjList,
  KC_SObjList,
  KC_ArrayStack,
  KC_Item,                      // an item entry in a list

  // name map
  KC_StringRefMap,
  KC_StringSObjDict,
  KC_Name,                      // a name entry in a name map
};

// the <_List_Item> </_List_Item> tag is parsed into this class to
// hold the name while the value contained by it is being parsed.
// Then it is deleted.
struct ListItem {
  StringRef to;
  ListItem() : to(NULL) {}
};

// the <_NameMap_Item> </_NameMap_Item> tag is parsed into this class
// to hold the name while the value contained by it is being parsed.
// Then it is deleted.
struct NameMapItem {
  StringRef from;
  StringRef to;
  NameMapItem() : from(NULL), to(NULL) {}
  // FIX: do I destruct/free() the name when I destruct the object?
};

// datastructures for dealing with unsatisified links; FIX: we can
// do the in-place recording of a lot of these unsatisified links
// (not the ast links)
struct UnsatLink {
  void *ptr;
  string id;
  int kind;
  bool embedded;
  UnsatLink(void *ptr0, string id0, int kind0, bool embedded0);
};

//  // datastructures for dealing with unsatisified links where neither
//  // party wants to know about the other
//  struct UnsatBiLink {
//    char const *from;
//    char const *to;
//    UnsatBiLink() : from(NULL), to(NULL) {}
//  };

// A subclass fills-in the methods that need to know about individual
// tags.
class XmlReader {
  protected:
  XmlReaderManager *manager;

  public:
  XmlReader() : manager(NULL) {}
  virtual ~XmlReader() {}

  public:
  void setManager(XmlReaderManager *manager0);

  protected:
  void userError(char const *msg) NORETURN;

  // **** virtual API
  public:

  // Parse a tag: construct a node for a tag
  virtual void *ctorNodeFromTag(int tag) = 0;

  // Parse an attribute: register an attribute into the current node
  virtual bool registerAttribute(void *target, int kind, int attr, char const *yytext0) = 0;

  // Parse raw data: register some raw data into the current node
  virtual bool registerStringToken(void *target, int kind, char const *yytext0) = 0;

  // implement an eq-relation on tag kinds by mapping a tag kind to a
  // category
  virtual bool kind2kindCat(int kind, KindCategory *kindCat) = 0;

  // **** Generic Convert

  // note: return whether we know the answer, not the answer which
  // happens to also be a bool
  virtual bool recordKind(int kind, bool& answer) = 0;

  // cast a pointer to the pointer type we need it to be; this is only
  // needed because of multiple inheritance
  virtual bool callOpAssignToEmbeddedObj(void *obj, int kind, void *target) = 0;
  virtual bool upcastToWantedType(void *obj, int kind, void **target, int targetKind) = 0;
  // all lists are stored as ASTLists; convert to the real list
  virtual bool convertList2FakeList  (ASTList<char> *list, int listKind, void **target) = 0;
  virtual bool convertList2SObjList  (ASTList<char> *list, int listKind, void **target) = 0;
  virtual bool convertList2ObjList   (ASTList<char> *list, int listKind, void **target) = 0;
  virtual bool convertList2ArrayStack(ASTList<char> *list, int listKind, void **target) = 0;
  // all name maps are stored as StringRefMaps; convert to the real name maps
  virtual bool convertNameMap2StringRefMap
    (StringRefMap<char> *map, int mapKind, void *target) = 0;
  virtual bool convertNameMap2StringSObjDict
    (StringRefMap<char> *map, int mapKind, void *target) = 0;
};

// XmlReader-s register themselves with the Manager which tries them
// one at a time while handling incoming xml tags.
class XmlReaderManager {
  // the readers we are managing
  ASTList<XmlReader> readers;

  // **** Parsing
  char const *inputFname;       // just for error messages
  AstXmlLexer &lexer;           // a lexer on a stream already opened from the file
  public:
  StringTable &strTable;        // for canonicalizing the StringRef's in the input file

  private:
  // the node (and its kind) for the last closing tag we saw; useful
  // for extracting the top of the tree
  void *lastNode;
  int lastKind;
  // parsing stack
  SObjStack<void> nodeStack;
  ObjStack<int> kindStack;

  // **** Satisfying links

  public:
  // Since AST nodes are embedded, we have to put this on to a
  // different list than the ususal pointer unsatisfied links.
  ASTList<UnsatLink> unsatLinks;
  ASTList<UnsatLink> unsatLinks_List;
  ASTList<UnsatLink> unsatLinks_NameMap;
//    ASTList<UnsatBiLink> unsatBiLinks;

  // map object ids to the actual object
  StringSObjDict<void> id2obj;
  // map object ids to their kind ONLY IF there is a non-trivial
  // upcast to make at the link satisfaction point
  StringSObjDict<int> id2kind;

  public:
  XmlReaderManager(char const *inputFname0,
                   AstXmlLexer &lexer0,
                   StringTable &strTable0)
    : inputFname(inputFname0)
    , lexer(lexer0)
    , strTable(strTable0)
    , lastNode(NULL)            // also done in reset()
    , lastKind(0)               // also done in reset()
  {
    reset();
  }
  virtual ~XmlReaderManager() {
    readers.removeAll_dontDelete();
  }

  // **** initialization
  public:
  void registerReader(XmlReader *reader);
  void reset();

  // **** parsing
  public:
  void parseOneTopLevelTag();
  // read until we get a translation unit tag; FIX: not sure what
  // happens if the last tag is not a TranslationUnit
  void readUntilTUnitTag();

  private:
  void parseOneTagOrDatum();
  bool readAttributes();

  // disjunctive dispatch to the list of readers
  void kind2kindCat(int kind, KindCategory *kindCat);
  void *ctorNodeFromTag(int tag);
  void registerAttribute(void *target, int kind, int attr, char const *yytext0);
  void registerStringToken(void *target, int kind, char const *yytext0);

  void append2List(void *list, int listKind, void *datum);
  void insertIntoNameMap(void *map, int mapKind, StringRef name, void *datum);

  // **** parsing result
  public:
  // report an error to the user with source location information
  void userError(char const *msg) NORETURN;
  // are we at the top level during parsing?
  bool atTopLevel() {return nodeStack.isEmpty();}
  // return the top of the stack: the one tag that was parsed
  void *getLastNode() {return lastNode;}
  int getLastKind() {return lastKind;}

  // **** satisfying links
  public:
  void satisfyLinks();

  private:
  void satisfyLinks_Nodes();
  void satisfyLinks_Lists();
  void satisfyLinks_Maps();
//    void satisfyLinks_Bidirectional();

  public:
  bool recordKind(int kind);

  private:
  // convert nodes
  void callOpAssignToEmbeddedObj(void *obj, int kind, void *target);
  void *upcastToWantedType(void *obj, int kind, int targetKind);
  // convert lists
  void *convertList2FakeList (ASTList<char> *list, int listKind);
  void convertList2SObjList  (ASTList<char> *list, int listKind, void **target);
  void convertList2ObjList   (ASTList<char> *list, int listKind, void **target);
  void convertList2ArrayStack(ASTList<char> *list, int listKind, void **target);
  // convert maps
  void convertNameMap2StringRefMap  (StringRefMap<char> *map, int listKind, void *target);
  void convertNameMap2StringSObjDict(StringRefMap<char> *map, int listKind, void *target);
};

#endif // XML_H
