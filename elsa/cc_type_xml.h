// cc_type_xml.h            see license.txt for copyright and terms of use

// Serialization and de-serialization for the type system, template
// system, and variables.

#ifndef CC_TYPE_XML_H
#define CC_TYPE_XML_H

#include "cc_type.h"            // Types, TypeVisitor
#include "template.h"           // Template stuff is only forward-declared in cc_type.h
//  #include "sobjstack.h"          // SObjStack
//  #include "objstack.h"           // ObjStack
#include "xml.h"                // ReadXml

class LinkSatisfier;
class AstXmlLexer;

string toXml(CompoundType::Keyword id);
void fromXml(CompoundType::Keyword &out, string str);

string toXml(FunctionFlags id);
void fromXml(FunctionFlags &out, string str);

// -------------------- ToXMLTypeVisitor -------------------

// print the Type tree out as XML
class ToXMLTypeVisitor : public TypeVisitor {
  protected:
  ostream &out;
  int depth;
  bool indent;

  // printing of types is idempotent
  SObjSet<Type*> printedTypes;
  SObjSet<AtomicType*> printedAtomicTypes;

  public:
  ToXMLTypeVisitor(ostream &out0, bool indent0=true)
    : out(out0)
    , depth(0)
    , indent(indent0)
  {}
  virtual ~ToXMLTypeVisitor() {}

  private:
  void printIndentation();

  // **** TypeVisitor API methods
  public:
  virtual bool visitType(Type *obj);
  virtual void postvisitType(Type *obj);

  virtual bool visitFuncParamsList(SObjList<Variable> &params);
  virtual void postvisitFuncParamsList(SObjList<Variable> &params);

  virtual bool visitVariable(Variable *var);
  virtual void postvisitVariable(Variable *var);

  virtual bool visitAtomicType(AtomicType *obj);
  virtual void postvisitAtomicType(AtomicType *obj);

  virtual void toXml_Scope_properties(Scope *scope);
  virtual void toXml_Scope_subtags(Scope *scope);
  virtual bool visitScope(Scope *obj);
  virtual void postvisitScope(Scope *obj);

  virtual bool visitScope_variables_Map(StringRefMap<Variable> &variables);
  virtual void visitScope_variables_Map_entry(StringRef name, Variable *var);
  virtual void postvisitScope_variables_Map(StringRefMap<Variable> &variables);

  virtual bool visitScope_typeTags_Map(StringRefMap<Variable> &typeTags);
  virtual void visitScope_typeTags_Map_entry(StringRef name, Variable *var);
  virtual void postvisitScope_typeTags_Map(StringRefMap<Variable> &typeTags);

//    virtual bool visitScopeTemplateParams(SObjList<Variable> &templateParams);
//    virtual void postvisitScopeTemplateParams(SObjList<Variable> &templateParams);

  virtual bool visitBaseClass(BaseClass *bc);
  virtual void postvisitBaseClass(BaseClass *bc);

  virtual bool visitBaseClassSubobj(BaseClassSubobj *bc);
  virtual void postvisitBaseClassSubobj(BaseClassSubobj *bc);

  virtual bool visitBaseClassSubobjParents(SObjList<BaseClassSubobj> &parents);
  virtual void postvisitBaseClassSubobjParents(SObjList<BaseClassSubobj> &parents);

  // factor out the commonality of the atomic types that inherit from
  // NamedAtomicType
  virtual void toXml_NamedAtomicType_properties(NamedAtomicType *nat);
  virtual void toXml_NamedAtomicType_subtags(NamedAtomicType *nat);


  // fail in these for now
  virtual bool visitSTemplateArgument(STemplateArgument *obj) {
    xfailure("implement this");
    // FIX: make this idempotent
    ++depth;                    // at the start
  }
//    virtual void postvisitSTemplateArgument(STemplateArgument *obj);
//      --depth;                    // at the end

  virtual bool visitExpression(Expression *obj) {
    xfailure("implement this");
    // FIX: make this idempotent
    ++depth;                    // at the start
  }
//    virtual void postvisitExpression(Expression *obj);
//      --depth;                    // at the end

  // same as for types; print the name also
//    virtual bool preVisitVariable(Variable *var);
//      ++depth;                    // at the start
//    virtual void postVisitVariable(Variable *var);
//      --depth;                    // at the end
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

  private:
  // map a kind to its kind category
  KindCategory kind2kindCat(int kind);

  // operate on lists
  void *prepend2FakeList(void *list, int listKind, void *datum, int datumKind);
  void *reverseFakeList(void *list, int listKind);

  void append2ASTList(void *list, int listKind, void *datum, int datumKind);

  void prepend2ObjList(void *list, int listKind, void *datum, int datumKind);
  void reverseObjList(void *list, int listKind);

  void prepend2SObjList(void *list, int listKind, void *datum, int datumKind);
  void reverseSObjList(void *list, int listKind);

  // construct a node for a tag
  bool ctorNodeFromTag(int tag, void *&topTemp);
  // register an attribute into the current node
  void registerAttribute(void *target, int kind, int attr, char const *yytext0);

  // Types
  void registerAttr_CVAtomicType       (CVAtomicType *obj,        int attr, char const *strValue);
  void registerAttr_PointerType        (PointerType *obj,         int attr, char const *strValue);
  void registerAttr_ReferenceType      (ReferenceType *obj,       int attr, char const *strValue);
  void registerAttr_FunctionType       (FunctionType *obj,        int attr, char const *strValue);
  void registerAttr_ArrayType          (ArrayType *obj,           int attr, char const *strValue);
  void registerAttr_PointerToMemberType(PointerToMemberType *obj, int attr, char const *strValue);

  // AtomicTypes
  void registerAttr_SimpleType         (SimpleType *obj,          int attr, char const *strValue);
  void registerAttr_CompoundType       (CompoundType *obj,        int attr, char const *strValue);
  void registerAttr_EnumType           (EnumType *obj,            int attr, char const *strValue);
  void registerAttr_TypeVariable       (TypeVariable *obj,        int attr, char const *strValue);
  void registerAttr_PseudoInstantiation(PseudoInstantiation *obj, int attr, char const *strValue);
  void registerAttr_DependentQType     (DependentQType *obj,      int attr, char const *strValue);
  // attempt to parse the attributes in common for any NamedAtomicType
  bool registerAttr_NamedAtomicType    (NamedAtomicType *obj,     int attr, char const *strValue);
  
//  #include "astxml_parse1_0decl.gen.cc"
};

#endif // CC_TYPE_XML_H
