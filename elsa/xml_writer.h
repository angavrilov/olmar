// xml_writer.h            see license.txt for copyright and terms of use

// Support for XML serialization

// FIX: this module should eventually go into the ast repository.

// FIX: many of these macros could be changed into function templates.

#ifndef XML_WRITER_H
#define XML_WRITER_H

#include "strtable.h"           // StringRef
#include "xmlhelp.h"            // toXml_int etc.
#include "strmap.h"             // StringRefMap

// FIX: this is a hack; I want some place to set a flag to tell me if
// I want the xml serialization of name-maps to be canonical (names in
// sorted order)
extern bool sortNameMapDomainWhenSerializing;


// to Xml for enums
#define PRINTENUM(X) case X: return #X
#define PRINTFLAG(X) if (id & (X)) b << #X

// manage identity; definitions; FIX: I don't like how printed() is
// still using the object address instead of its unique id, but those
// are one-to-one so I suppose its ok for now
#define identity_defn0(PREFIX, NAME, TEMPL) \
TEMPL char const *idPrefix(NAME const * const) {return #PREFIX;} \
TEMPL xmlUniqueId_t uniqueId(NAME const * const obj) {return mapAddrToUniqueId(obj);} \
TEMPL bool printed(NAME const * const obj) { \
  if (printedSet ##PREFIX.contains(obj)) return true; \
  printedSet ##PREFIX.add(obj); \
  return false; \
}
#define identity_defn(PREFIX, NAME) identity_defn0(PREFIX, NAME, )
#define identityTempl_defn(PREFIX, NAME) identity_defn0(PREFIX, NAME, template<class T>)

// declarations
#define identity_decl0(NAME, TEMPL) \
TEMPL char const *idPrefix(NAME const * const); \
TEMPL xmlUniqueId_t uniqueId(NAME const * const obj); \
TEMPL bool printed(NAME const * const obj)
#define identity_decl(NAME) identity_decl0(NAME, )
// NOTE: it makes no sense to declare a template like this, so do not
// do this
//  #define identityTempl_decl(NAME) identity_decl0(NAME, template<class T>)

// manage the output stream
class XmlWriter {
  public:
  ostream &out;                 // output stream to which to print

  protected:
  int &depth;                   // ref so we can share our indentation depth with other printers
  bool indent;                  // should we print indentation?

  public:
  XmlWriter(ostream &out0, int &depth0, bool indent0);

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
  XmlWriter &ttx;

  public:
  XmlCloseTagPrinter(string s0, XmlWriter &ttx0)
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
  if ((RAW) && shouldSerialize(RAW)) { \
    newline(); \
    printThing0(NAME, VALUE); \
  } \
} while(0)

#define printThingAST(NAME, RAW, VALUE) \
do { \
  if (astVisitor && RAW) { \
    newline(); \
    printThing0(NAME, VALUE); \
  } \
} while(0)

#define printPtr0(NAME, VALUE) printThing(NAME, VALUE, xmlPrintPointer(idPrefix(VALUE), uniqueId(VALUE)))
#define printPtr(BASE, MEM)    printPtr0(MEM, (BASE)->MEM)
#define printPtrAST(BASE, MEM) printThingAST(MEM, (BASE)->MEM, xmlPrintPointer("AST", uniqueIdAST((BASE)->MEM)))
// print an embedded thing
#define printEmbed(BASE, MEM)  printThing(MEM, (&((BASE)->MEM)), xmlPrintPointer(idPrefix(&((BASE)->MEM)), uniqueId(&((BASE)->MEM))))

// for unions where the member name does not match the xml name and we
// don't want the 'if'
#define printPtrUnion(BASE, MEM, NAME) printThing0(NAME, xmlPrintPointer(idPrefix((BASE)->MEM), uniqueId((BASE)->MEM)))
// this is only used in one place
#define printPtrASTUnion(BASE, MEM, NAME) if (astVisitor) printThing0(NAME, xmlPrintPointer("AST", uniqueIdAST((BASE)->MEM)))

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
#define travObjList0(OBJ, TAGNAME, FIELDTYPE, ITER_MACRO, LISTKIND) \
do { \
  if (!printed(&OBJ)) { \
    openTagWhole(List_ ##TAGNAME, &OBJ); \
    ITER_MACRO(FIELDTYPE, const_cast<LISTKIND<FIELDTYPE>&>(OBJ), iter) { \
      FIELDTYPE *obj0 = iter.data(); \
      if (shouldSerialize(obj0)) { \
        travListItem(obj0); \
      } \
    } \
  } \
} while(0)
#define travObjList1(OBJ, BASETYPE, FIELD, FIELDTYPE, ITER_MACRO, LISTKIND) \
  travObjList0(OBJ, BASETYPE ##_ ##FIELD, FIELDTYPE, ITER_MACRO, LISTKIND) 

#define travObjList_S(BASE, BASETYPE, FIELD, FIELDTYPE) \
  travObjList1(BASE->FIELD, BASETYPE, FIELD, FIELDTYPE, SFOREACH_OBJLIST_NC, SObjList)

#define travObjListPtr_S(BASE, BASETYPE, FIELD, FIELDTYPE) \
  travObjList1(*(BASE->FIELD), BASETYPE, FIELD, FIELDTYPE, SFOREACH_OBJLIST_NC, SObjList)

#define travObjList(BASE, BASETYPE, FIELD, FIELDTYPE) \
  travObjList1(BASE->FIELD, BASETYPE, FIELD, FIELDTYPE, FOREACH_OBJLIST_NC, ObjList)

// Not tested; put the backslash back after the first line
//  #define travArrayStack(BASE, BASETYPE, FIELD, FIELDTYPE)
//  travObjList1(BASE->FIELD, BASETYPE, FIELD, FIELDTYPE, FOREACH_ARRAYSTACK_NC, ArrayStack)

#define travObjList_standalone(OBJ, BASETYPE, FIELD, FIELDTYPE) \
  travObjList1(OBJ, BASETYPE, FIELD, FIELDTYPE, FOREACH_OBJLIST_NC, ObjList)

#define travStringRefMap0(OBJ, BASETYPE, FIELD, RANGETYPE) \
do { \
  if (!printed(OBJ)) { \
    openTagWhole(NameMap_ ##BASETYPE ##_ ##FIELD, OBJ); \
    if (sortNameMapDomainWhenSerializing) { \
      for(StringRefMap<RANGETYPE>::SortedKeyIter iter(*(OBJ)); \
          !iter.isDone(); iter.adv()) { \
        RANGETYPE *obj = iter.value(); \
        if (shouldSerialize(obj)) { \
          openTag_NameMap_Item(iter.key(), obj); \
          trav(obj); \
        } \
      } \
    } else { \
      for(PtrMap<char const, RANGETYPE>::Iter iter(*(OBJ)); \
          !iter.isDone(); iter.adv()) { \
        RANGETYPE *obj = iter.value(); \
        if (shouldSerialize(obj)) { \
          openTag_NameMap_Item(iter.key(), obj); \
          trav(obj); \
        } \
      } \
    } \
  } \
} while(0)

#define travStringRefMap(BASE, BASETYPE, FIELD, RANGETYPE) \
  travStringRefMap0(&((BASE)->FIELD), BASETYPE, FIELD, RANGETYPE)

#define travPtrMap0(OBJ, BASETYPE, FIELD, DOMTYPE, RANGETYPE) \
do { \
  if (!printed(OBJ)) { \
    openTagWhole(Map_ ##BASETYPE ##_ ##FIELD, OBJ); \
    for(PtrMap<DOMTYPE, RANGETYPE>::Iter iter(*(OBJ)); \
        !iter.isDone(); iter.adv()) { \
      DOMTYPE *key = iter.key(); \
      RANGETYPE *value = iter.value(); \
      bool shouldSrzDom = shouldSerialize(key); \
      bool shouldSrzRange = shouldSerialize(value); \
      if (/*dsw: NOTE: This must be an 'OR' for at least one situation in Oink*/ \
          /*perhaps the semantics should be speical-cased.*/ \
        shouldSrzDom || shouldSrzRange) { \
        openTag_Map_Item(key, value); \
        trav(key); \
        trav(value); \
      } \
    } \
  } \
} while(0)

// NOTE: you must not wrap this one in a 'do {} while(0)': the dtor
// for the XmlCloseTagPrinter fires too early.
#define openTag0(NAME, OBJ, SUFFIX) \
  newline(); \
  char const * const name = NAME; \
  out << "<" << name << " _id=" \
    << xmlAttrQuote(xmlPrintPointer(idPrefix(OBJ), uniqueId(OBJ))) \
    << SUFFIX; \
  XmlCloseTagPrinter tagCloser(name, *this); \
  IncDec depthManager(this->depth)

#define openTag(NAME, OBJ)             openTag0(#NAME, OBJ, "" )
#define openTagVirtual(NAME, OBJ)      openTag0( NAME, OBJ, "" )
#define openTagWhole(NAME, OBJ)        openTag0(#NAME, OBJ, ">")
#define openTagWholeVirtual(NAME, OBJ) openTag0( NAME, OBJ, ">")

// NOTE: you must not wrap this one in a 'do {} while(0)': the dtor
// for the XmlCloseTagPrinter fires too early.
#define openTag_NameMap_Item(NAME, TARGET) \
  newline(); \
  out << "<_NameMap_Item" \
      << " name=" << xmlAttrQuote(NAME) \
      << " item=" << xmlAttrQuote(xmlPrintPointer(idPrefix(TARGET), uniqueId(TARGET))) \
      << ">"; \
  XmlCloseTagPrinter tagCloser("_NameMap_Item", *this); \
  IncDec depthManager(this->depth)

// NOTE: you must not wrap this one in a 'do {} while(0)': the dtor
// for the XmlCloseTagPrinter fires too early.
#define openTag_Map_Item(NAME, TARGET) \
  newline(); \
  out << "<_Map_Item" \
      << " key=" << xmlAttrQuote(xmlPrintPointer(idPrefix(NAME), uniqueId(NAME))) \
      << " item=" << xmlAttrQuote(xmlPrintPointer(idPrefix(TARGET), uniqueId(TARGET))) \
      << ">"; \
  XmlCloseTagPrinter tagCloser("_Map_Item", *this); \
  IncDec depthManager(this->depth)

#define tagEnd \
do { \
  out << ">"; \
} while(0)

#define trav(TARGET) \
do { \
  if (TARGET && shouldSerialize(TARGET)) { \
    toXml(TARGET); \
  } \
} while(0)

// NOTE: you must not wrap this one in a 'do {} while(0)': the dtor
// for the XmlCloseTagPrinter fires too early.
#define travListItem(TARGET) \
  newline(); \
  out << "<_List_Item item=" \
    << xmlAttrQuote(xmlPrintPointer(idPrefix(TARGET), uniqueId(TARGET))) \
    << ">"; \
  XmlCloseTagPrinter tagCloser("_List_Item", *this); \
  IncDec depthManager(this->depth); \
  trav(TARGET)

#define travAST(TARGET) \
do { \
  if (TARGET) { \
    if (astVisitor) (TARGET)->traverse(*astVisitor); \
  } \
} while(0)

#endif // XML_WRITER_H
