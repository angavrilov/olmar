// xml_type_writer.h            see license.txt for copyright and terms of use

// Serialization for the type system, template system, and variables.

#ifndef XML_TYPE_WRITER_H
#define XML_TYPE_WRITER_H

#include "cc_type.h"            // Type
#include "template.h"           // Template stuff is only forward-declared in cc_type.h
#include "xml_writer.h"         // XmlWriter

class OverloadSet;
class ASTVisitor;

//  char const *toXml(CompoundType::Keyword id);
//  string toXml(FunctionFlags id);
//  char const *toXml(ScopeKind id);
//  char const *toXml(STemplateArgument::Kind id);


identityB(Type);
identityB(AtomicType);
identityB(CompoundType);
identityB(FunctionType::ExnSpec);
identityB(EnumType::Value);
identityB(BaseClass);
identityB(Scope);
identityB(Variable);
identityB(OverloadSet);
identityB(STemplateArgument);
identityB(TemplateInfo);
identityB(InheritedTemplateParams);

class XmlTypeWriter : public XmlWriter {
  public:
  ASTVisitor *astVisitor;       // for launching sub-traversals of AST we encounter in the Types

  public:
  XmlTypeWriter(ostream &out0, int &depth0, bool indent0=false);
  virtual ~XmlTypeWriter() {}

  public:
  // in the AST
  void toXml(ObjList<STemplateArgument> *list);

  void toXml(Type *t);
  void toXml(AtomicType *atom);
  void toXml(CompoundType *ct); // disambiguates the overloading
  void toXml_Variable_properties(Variable *var);
  void toXml_Variable_subtags(Variable *var);
  void toXml(Variable *var);

  private:
  void toXml_FunctionType_ExnSpec(void /*FunctionType::ExnSpec*/ *exnSpec);

  void toXml_EnumType_Value(void /*EnumType::Value*/ *eValue0);
  void toXml_NamedAtomicType_properties(NamedAtomicType *nat);
  void toXml_NamedAtomicType_subtags(NamedAtomicType *nat);

  void toXml(OverloadSet *oload);

  void toXml(BaseClass *bc);
  void toXml_BaseClass_properties(BaseClass *bc);
  void toXml_BaseClass_subtags(BaseClass *bc);
  void toXml(BaseClassSubobj *bc);

  void toXml(Scope *scope);
  void toXml_Scope_properties(Scope *scope);
  void toXml_Scope_subtags(Scope *scope);

  void toXml(STemplateArgument *sta);
  void toXml(TemplateInfo *ti);
  void toXml(InheritedTemplateParams *itp);
  void toXml_TemplateParams_properties(TemplateParams *tp);
  void toXml_TemplateParams_subtags(TemplateParams *tp);
};

#endif // XML_TYPE_WRITER_H
