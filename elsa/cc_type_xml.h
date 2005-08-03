// cc_type_xml.h            see license.txt for copyright and terms of use

// Serialization and de-serialization for the type system, template
// system, and variables.

#ifndef CC_TYPE_XML_H
#define CC_TYPE_XML_H

#include "cc_type.h"            // Types, TypeVisitor
#include "template.h"           // Template stuff is only forward-declared in cc_type.h
#include "sobjset.h"            // SObjSet
#include "xml.h"                // ReadXml

class LinkSatisfier;
class AstXmlLexer;

string toXml(CompoundType::Keyword id);
void fromXml(CompoundType::Keyword &out, rostring str);

string toXml(FunctionFlags id);
void fromXml(FunctionFlags &out, rostring str);

// -------------------- TypeToXml -------------------

// print the Type tree out as XML
class TypeToXml {
  protected:
  ostream &out;
  int depth;
  bool indent;

  // printing of types is idempotent
  SObjSet<void const *> printedTypes;
  SObjSet<void const *> printedScopes;
  SObjSet<void const *> printedVariables;
  SObjSet<void const *> printedOLs;
  SObjSet<void const *> printedSMs;

  public:
  TypeToXml(ostream &out0, bool indent0=true)
    : out(out0)
    , depth(0)
    , indent(indent0)
  {}
  virtual ~TypeToXml() {}

  private:
  // print a newline and indent if the user wants indentation; NOTE:
  // the convention is that you don't print a newline until you are
  // *sure* you have something to print that goes onto the next line;
  // that is, most lines do *not* end in a newline
  void newline();
  friend class TypeToXml_CloseTagPrinter;

  // register the object and check if it has been printed
  bool printedType(void const *obj);
  bool printedScope(void const *obj);
  bool printedVariable(void const *obj);
  bool printedOL(void const *obj);
  bool printedSM(void const *obj);

  public:
  void toXml(Type *obj);
  void toXml(AtomicType *obj);
  void toXml(Variable *var);

  private:
  void toXml_FunctionType_ExnSpec(void /*FunctionType::ExnSpec*/ *exnSpec);

  void toXml_EnumType_Value(void /*EnumType::Value*/ *eValue0);
  void toXml_NamedAtomicType_properties(NamedAtomicType *nat);
  void toXml_NamedAtomicType_subtags(NamedAtomicType *nat);

  void toXml(BaseClass *bc);
  void toXml_BaseClass_properties(BaseClass *bc);
  void toXml(BaseClassSubobj *bc);

  void toXml(Scope *obj);
  void toXml_Scope_properties(Scope *scope);
  void toXml_Scope_subtags(Scope *scope);

  void toXml(STemplateArgument *obj);
};


// -------------------- ReadXml_Type -------------------

// Specialization of the ReadXml framework that reads in XML for
// serialized types.

// parse Types and Variables serialized as XML
class ReadXml_Type : public ReadXml {
  BasicTypeFactory &tFac;

  public:
  ReadXml_Type(char const *inputFname0,
               AstXmlLexer &lexer0,
               StringTable &strTable0,
               LinkSatisfier &linkSat0,
               BasicTypeFactory &tFac0)
    : ReadXml(inputFname0, lexer0, strTable0, linkSat0)
    , tFac(tFac0)
  {}

  public:
  void append2List(void *list, int listKind, void *datum);
  void insertIntoNameMap(void *map0, int mapKind, StringRef name, void *datum);
  bool kind2kindCat0(int kind, KindCategory *kindCat);

  bool convertList2FakeList(ASTList<char> *list, int listKind, void **target);
  bool convertList2SObjList(ASTList<char> *list, int listKind, void **target);
  bool convertList2ObjList (ASTList<char> *list, int listKind, void **target);

  bool convertNameMap2StringRefMap
    (StringRefMap<char>   *map, int mapKind, void *target);
  bool convertNameMap2StringSObjDict
    (StringRefMap<char> *map, int mapKind, void *target);

  void *ctorNodeFromTag(int tag);
  void registerAttribute(void *target, int kind, int attr, char const *yytext0);

  private:
  // Types
  void registerAttr_CVAtomicType       (CVAtomicType *obj,        int attr, char const *strValue);
  void registerAttr_PointerType        (PointerType *obj,         int attr, char const *strValue);
  void registerAttr_ReferenceType      (ReferenceType *obj,       int attr, char const *strValue);
  void registerAttr_FunctionType       (FunctionType *obj,        int attr, char const *strValue);
  void registerAttr_FunctionType_ExnSpec
    (FunctionType::ExnSpec *obj, int attr, char const *strValue);
  void registerAttr_ArrayType          (ArrayType *obj,           int attr, char const *strValue);
  void registerAttr_PointerToMemberType(PointerToMemberType *obj, int attr, char const *strValue);

  // AtomicTypes
  bool registerAttr_NamedAtomicType_super(NamedAtomicType *obj,   int attr, char const *strValue);
  void registerAttr_SimpleType         (SimpleType *obj,          int attr, char const *strValue);
  void registerAttr_CompoundType       (CompoundType *obj,        int attr, char const *strValue);
  void registerAttr_EnumType           (EnumType *obj,            int attr, char const *strValue);
  void registerAttr_EnumType_Value     (EnumType::Value *obj,     int attr, char const *strValue);
  void registerAttr_TypeVariable       (TypeVariable *obj,        int attr, char const *strValue);
  void registerAttr_PseudoInstantiation(PseudoInstantiation *obj, int attr, char const *strValue);
  void registerAttr_DependentQType     (DependentQType *obj,      int attr, char const *strValue);

  // other
  void registerAttr_Variable           (Variable *obj,            int attr, char const *strValue);
  bool registerAttr_Scope_super        (Scope *obj,               int attr, char const *strValue);
  void registerAttr_Scope              (Scope *obj,               int attr, char const *strValue);
  bool registerAttr_BaseClass_super    (BaseClass *obj,           int attr, char const *strValue);
  void registerAttr_BaseClass          (BaseClass *obj,           int attr, char const *strValue);
  void registerAttr_BaseClassSubobj    (BaseClassSubobj *obj,     int attr, char const *strValue);

//  #include "astxml_parse1_0decl.gen.cc"
};

#endif // CC_TYPE_XML_H
