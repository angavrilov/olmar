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

// -------------------- ToXMLTypeVisitor -------------------

// print the Type tree out as XML
class ToXMLTypeVisitor : public TypeVisitor {
  protected:
  ostream &out;
  int depth;
  bool indent;

  // printing of types is idempotent
  SObjSet<void*> printedObjects;

  public:
  ToXMLTypeVisitor(ostream &out0, bool indent0=true)
    : out(out0)
    , depth(0)
    , indent(indent0)
  {}
  virtual ~ToXMLTypeVisitor() {}

  private:
  void printIndentation();
  void startItem(rostring prefix, void const *ptr);
  void stopItem();

  // **** TypeVisitor API methods
  public:
  virtual bool visitType(Type *obj);
  virtual void postvisitType(Type *obj);

  virtual bool visitFuncParamsList(SObjList<Variable> &params);
  virtual void postvisitFuncParamsList(SObjList<Variable> &params);
  virtual bool visitFuncParamsList_item(Variable *param);
  virtual void postvisitFuncParamsList_item(Variable *param);

  virtual bool visitVariable(Variable *var);
  virtual void postvisitVariable(Variable *var);

  virtual bool visitAtomicType(AtomicType *obj);
  virtual void postvisitAtomicType(AtomicType *obj);

  virtual bool visitEnumType_Value(void /*EnumType::Value*/ *eValue0);
  virtual void postvisitEnumType_Value(void /*EnumType::Value*/ *eValue0);

  virtual void toXml_Scope_properties(Scope *scope);
  virtual void toXml_Scope_subtags(Scope *scope);
  virtual bool visitScope(Scope *obj);
  virtual void postvisitScope(Scope *obj);

  virtual bool visitScopeVariables(StringRefMap<Variable> &variables);
  virtual void postvisitScopeVariables(StringRefMap<Variable> &variables);
  virtual bool visitScopeVariables_entry(StringRef name, Variable *var);
  virtual void postvisitScopeVariables_entry(StringRef name, Variable *var);

  virtual bool visitScopeTypeTags(StringRefMap<Variable> &typeTags);
  virtual void postvisitScopeTypeTags(StringRefMap<Variable> &typeTags);
  virtual bool visitScopeTypeTags_entry(StringRef name, Variable *var);
  virtual void postvisitScopeTypeTags_entry(StringRef name, Variable *var);

  virtual bool visitScopeTemplateParams(SObjList<Variable> &templateParams);
  virtual void postvisitScopeTemplateParams(SObjList<Variable> &templateParams);
  virtual bool visitScopeTemplateParams_item(Variable *var);
  virtual void postvisitScopeTemplateParams_item(Variable *var);

  virtual void toXml_BaseClass_properties(BaseClass *bc);

  virtual bool visitBaseClass(BaseClass *bc);
  virtual void postvisitBaseClass(BaseClass *bc);

  virtual bool visitBaseClassSubobj(BaseClassSubobj *bc);
  virtual void postvisitBaseClassSubobj(BaseClassSubobj *bc);

  virtual bool visitBaseClassSubobjParentsList(SObjList<BaseClassSubobj> &parents);
  virtual void postvisitBaseClassSubobjParentsList(SObjList<BaseClassSubobj> &parents);
  virtual bool visitBaseClassSubobjParentsList_item(BaseClassSubobj *parents);
  virtual void postvisitBaseClassSubobjParentsList_item(BaseClassSubobj *parents);

  // factor out the commonality of the atomic types that inherit from
  // NamedAtomicType
  virtual void toXml_NamedAtomicType_properties(NamedAtomicType *nat);
  virtual void toXml_NamedAtomicType_subtags(NamedAtomicType *nat);

  virtual bool visitSTemplateArgument(STemplateArgument *obj);
  virtual void postvisitSTemplateArgument(STemplateArgument *obj);
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

  void ctorNodeFromTag(int tag, void *&topTemp);
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
