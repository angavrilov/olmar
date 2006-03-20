// xml_writer.h            see license.txt for copyright and terms of use

// Support for XML serialization

// FIX: this module should eventually go into the ast repository

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

// manage identity of AST
char const *idPrefixAST(void const * const);
void const *addrAST(void const * const obj);

// manage identity; definitions
#define identity_defn0(PREFIX, NAME, TEMPL) \
TEMPL char const *idPrefix(NAME const * const) {return #PREFIX;} \
TEMPL void const *addr(NAME const * const obj) {return reinterpret_cast<void const *>(obj);} \
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
TEMPL void const *addr(NAME const * const obj); \
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
#define travObjList0(OBJ, TAGNAME, FIELDTYPE, ITER_MACRO, LISTKIND) \
do { \
  if (!printed(&OBJ)) { \
    openTagWhole(List_ ##TAGNAME, &OBJ); \
    ITER_MACRO(FIELDTYPE, const_cast<LISTKIND<FIELDTYPE>&>(OBJ), iter) { \
      travListItem(iter.data()); \
    } \
  } \
} while(0)
#define travObjList1(OBJ, BASETYPE, FIELD, FIELDTYPE, ITER_MACRO, LISTKIND) \
  travObjList0(OBJ, BASETYPE ##_ ##FIELD, FIELDTYPE, ITER_MACRO, LISTKIND) 

#define travObjList_S(BASE, BASETYPE, FIELD, FIELDTYPE) \
travObjList1(BASE->FIELD, BASETYPE, FIELD, FIELDTYPE, SFOREACH_OBJLIST_NC, SObjList)

#define travObjList(BASE, BASETYPE, FIELD, FIELDTYPE) \
travObjList1(BASE->FIELD, BASETYPE, FIELD, FIELDTYPE, FOREACH_OBJLIST_NC, ObjList)

// Not tested; put the backslash back after the first line
//  #define travArrayStack(BASE, BASETYPE, FIELD, FIELDTYPE)
//  travObjList1(BASE->FIELD, BASETYPE, FIELD, FIELDTYPE, FOREACH_ARRAYSTACK_NC, ArrayStack)

#define travObjList_standalone(OBJ, BASETYPE, FIELD, FIELDTYPE) \
travObjList1(OBJ, BASETYPE, FIELD, FIELDTYPE, FOREACH_OBJLIST_NC, ObjList)

#define travPtrMap(BASE, BASETYPE, FIELD, FIELDTYPE) \
do { \
  if (!printed(&BASE->FIELD)) { \
    openTagWhole(NameMap_ ##BASETYPE ##_ ##FIELD, &BASE->FIELD); \
    if (sortNameMapDomainWhenSerializing) { \
      for(StringRefMap<FIELDTYPE>::SortedKeyIter iter(BASE->FIELD); \
          !iter.isDone(); iter.adv()) { \
        FIELDTYPE *var = iter.value(); \
        openTag_NameMap_Item(iter.key(), var); \
        trav(var); \
      } \
    } else { \
      for(PtrMap<char const, FIELDTYPE>::Iter iter(BASE->FIELD); \
          !iter.isDone(); iter.adv()) { \
        FIELDTYPE *var = iter.value(); \
        openTag_NameMap_Item(iter.key(), var); \
        trav(var); \
      } \
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
    if (astVisitor) (TARGET)->traverse(*astVisitor); \
  } \
} while(0)

#endif // XML_WRITER_H
