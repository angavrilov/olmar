// cc_type_xml.cc            see license.txt for copyright and terms of use

#include "cc_type_xml.h"        // this module
#include "variable.h"           // Variable
#include "cc_flags.h"           // fromXml(DeclFlags &out, rostring str)
#include "asthelp.h"            // xmlPrintPointer
#include "xmlhelp.h"            // toXml_int() etc.

#include "strutil.h"            // parseQuotedString
#include "astxml_lexer.h"       // AstXmlLexer


string toXml(CompoundType::Keyword id) {
  return stringc << static_cast<int>(id);
}

void fromXml(CompoundType::Keyword &out, rostring str) {
  out = static_cast<CompoundType::Keyword>(atoi(str));
}


string toXml(FunctionFlags id) {
  return stringc << static_cast<int>(id);
}

void fromXml(FunctionFlags &out, rostring str) {
  out = static_cast<FunctionFlags>(atoi(str));
}


string toXml(ScopeKind id) {
  return stringc << static_cast<int>(id);
}

void fromXml(ScopeKind &out, rostring str) {
  out = static_cast<ScopeKind>(atoi(str));
}


// -------------------- ToXMLTypeVisitor -------------------

void ToXMLTypeVisitor::printIndentation() {
  if (indent) {
    for (int i=0; i<depth; ++i) cout << " ";
  }
}

void ToXMLTypeVisitor::startItem(rostring prefix, void const *ptr) {
  printIndentation();
  out << "<__Item item=\"" << prefix << static_cast<void const*>(ptr) << "\">\n";
  ++depth;
}

void ToXMLTypeVisitor::stopItem() {
  --depth;
  printIndentation();
  out << "</__Item>\n";
}

bool ToXMLTypeVisitor::visitType(Type *obj) {
  if (printedObjects.contains(obj)) return false;
  printedObjects.add(obj);

  switch(obj->getTag()) {
  default: xfailure("illegal tag");
  case Type::T_ATOMIC: {
    CVAtomicType *atom = obj->asCVAtomicType();
    printIndentation();
    out << "<CVAtomicType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;

    printIndentation();
    out << "atomic=\"TY" << static_cast<void const*>(atom->atomic) << "\"\n";

    printIndentation();
    out << "cv=\"" << toXml(atom->cv) << "\">\n";

    break;
  }
  case Type::T_POINTER: {
    PointerType *ptr = obj->asPointerType();
    printIndentation();
    out << "<PointerType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;

    printIndentation();
    out << "cv=\"" << toXml(ptr->cv) << "\"\n";

    printIndentation();
    out << "atType=\"TY" << static_cast<void const*>(ptr->atType) << "\">\n";

    break;
  }
  case Type::T_REFERENCE: {
    ReferenceType *ref = obj->asReferenceType();
    printIndentation();
    out << "<ReferenceType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;

    printIndentation();
    out << "atType=\"TY" << static_cast<void const*>(ref->atType) << "\">\n";

    break;
  }
  case Type::T_FUNCTION: {
    FunctionType *func = obj->asFunctionType();
    printIndentation();
    out << "<FunctionType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;

    printIndentation();
    out << "flags=\"" << toXml(func->flags) << "\"\n";

    printIndentation();
    out << "retType=\"TY" << static_cast<void const*>(func->retType) << "\"\n";

    printIndentation();
    out << "params=\"SO" << static_cast<void const*>(&(func->params)) << "\"\n";

    printIndentation();
    out << "exnSpec=\"TY" << static_cast<void const*>(&(func->exnSpec)) << "\">\n";

    // **** subtags

    // These are not visited by default by the type visitor, so we not
    // only have to print the id we have to print the tree.
    if (func->exnSpec) {
      // I do not want to make a non-virtual traverse() method on
      // FunctionType::ExnSpec and I don't want to make it virtual as
      // it has no virtual methods yet, so I do this manually.
      printIndentation();
      out << "<FunctionType_ExnSpec";
      out << " .id=\"TY" << static_cast<void const*>(&(func->exnSpec)) << "\"\n";
      ++depth;

      printIndentation();
      out << "types=\"SO" << static_cast<void const*>(&(func->exnSpec->types)) << "\">\n";

      // **** FunctionType::ExnSpec subtags

      printIndentation();
      out << "<List_ExnSpec_types";
      out << " .id=\"SO" << static_cast<void const*>(&func->exnSpec->types) << "\">\n";
      ++depth;

      SFOREACH_OBJLIST_NC(Type, func->exnSpec->types, iter) {
        Type *t = iter.data();
        startItem("TY", t);
        t->traverse(*this);
        stopItem();
      }

      --depth;
      printIndentation();
      out << "</List_ExnSpec_types>\n";

      --depth;
      printIndentation();
      out << "</FunctionType_ExnSpec>\n";
    }

    break;
  }
  case Type::T_ARRAY: {
    ArrayType *arr = obj->asArrayType();
    printIndentation();
    out << "<ArrayType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;

    printIndentation();
    out << "eltType=\"TY" << static_cast<void const*>(arr->eltType) << "\"\n";

    printIndentation();
    out << "size=\"" << arr->size << "\">\n";

    break;
  }
  case Type::T_POINTERTOMEMBER: {
    PointerToMemberType *ptm = obj->asPointerToMemberType();
    printIndentation();
    out << "<PointerToMemberType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;

    printIndentation();
    out << "inClassNAT=\"TY" << static_cast<void const*>(ptm->inClassNAT) << "\"\n";

    printIndentation();
    out << "cv=\"" << toXml(ptm->cv) << "\"\n";

    printIndentation();
    out << "atType=\"TY" << static_cast<void const*>(ptm->atType) << "\">";

    break;
  }
  }

  return true;
}

void ToXMLTypeVisitor::postvisitType(Type *obj) {
  --depth;

  printIndentation();

  switch(obj->getTag()) {
  default: xfailure("illegal tag");
  case Type::T_ATOMIC:
    out << "</CVAtomicType>\n";
    break;
  case Type::T_POINTER:
    out << "</PointerType>\n";
    break;
  case Type::T_REFERENCE:
    out << "</ReferenceType>\n";
    break;
  case Type::T_FUNCTION:
    out << "</FunctionType>\n";
    break;
  case Type::T_ARRAY:
    out << "</ArrayType>\n";
    break;
  case Type::T_POINTERTOMEMBER:
    out << "</PointerToMemberType>\n";
    break;
  }
}

bool ToXMLTypeVisitor::visitFuncParamsList(SObjList<Variable> &params) {
  printIndentation();
  out << "<List_FunctionType_params";
  out << " .id=\"SO" << static_cast<void const*>(&params) << "\">\n";
  ++depth;
  return true;
}

void ToXMLTypeVisitor::postvisitFuncParamsList(SObjList<Variable> &params) {
  --depth;
  printIndentation();
  out << "</List_FunctionType_params>\n";
}

bool ToXMLTypeVisitor::visitFuncParamsListItem(Variable *param) {
  startItem("TY", param);
  return true;
}

void ToXMLTypeVisitor::postvisitFuncParamsListItem(Variable *param) {
  stopItem();
}

bool ToXMLTypeVisitor::visitVariable(Variable *var) {
  if (printedObjects.contains(var)) return false;
  printedObjects.add(var);

  printIndentation();
  out << "<Variable";
  out << " .id=\"TY" << static_cast<void const*>(var) << "\"\n";
  ++depth;

//    SourceLoc loc;
//    I'm skipping these for now, but source locations will be serialized
//    as file:line:col when I serialize the internals of the Source Loc
//    Manager.

  if (var->name) {
    printIndentation();
    out << "name=" << quoted(var->name) << "\n";
  }

  printIndentation();
  out << "type=\"TY" << static_cast<void const*>(var->type) << "\"\n";
  
  printIndentation();
  out << "flags=\"" << toXml(var->flags) << "\"\n";

  printIndentation();
  out << "value=\"";
  xmlPrintPointer(out, "AST", var->value);
  out << "\"\n";
  // FIX: this is AST so we should make sure it gets printed out

  // this is nullable
  if (var->defaultParamType) {
    printIndentation();
    out << "defaultParamType=\"TY" << static_cast<void const*>(var->defaultParamType) << "\">\n";
  }

  if (var->funcDefn) {
    printIndentation();
    out << "funcDefn=\"";
    xmlPrintPointer(out, "AST", var->funcDefn);
    out << "\"\n";
    // FIX: this is AST so we should make sure it gets printed out
  }

//    OverloadSet *overload;  // (nullable serf)
//    I don't think we need to serialize this because we are done with
//    overloading after typechecking.  Will have to eventually be done if
//    an analysis wants to analyze uninstantiate templates.

  if (var->scope) {
    printIndentation();
    out << "scope=\"TY" << static_cast<void const*>(var->scope) << "\"\n";
  }

//    // bits 0-7: result of 'getAccess()'
//    // bits 8-15: result of 'getScopeKind()'
//    // bits 16-31: result of 'getParameterOrdinal()'
//    unsigned intData;
//    Ugh.  Break into 3 parts eventually, but for now serialize as an int.
  printIndentation();
  // FIX: split this up into 3
  out << "intData=\"" << toXml_Variable_intData(var->intData) << "\"\n";

  if (var->usingAlias_or_parameterizedEntity) {
    printIndentation();
    out << "usingAlias_or_parameterizedEntity=\"TY"
        << static_cast<void const*>(var->usingAlias_or_parameterizedEntity)
        << "\"\n";
  }

//    TemplateInfo *templInfo;      // (owner)
//    FIX: Ugh.

  // FIX: remove this when we do the above attributes
  printIndentation();
  cout << ">\n";

  // **** subtags

  // These are not visited by default by the type visitor, so we not
  // only have to print the id we have to print the tree.

  if (var->defaultParamType) {
    var->defaultParamType->traverse(*this);
  }

  if (var->usingAlias_or_parameterizedEntity) {
    var->usingAlias_or_parameterizedEntity->traverse(*this);
  }

  if (var->scope) {
    var->scope->traverse(*this);
  }

  return true;
}

void ToXMLTypeVisitor::postvisitVariable(Variable *var) {
  --depth;
  printIndentation();
  out << "</Variable>\n";
}

void ToXMLTypeVisitor::toXml_NamedAtomicType_properties(NamedAtomicType *nat) {
  printIndentation();
  out << "name=" << quoted(nat->name) << "\n";

  printIndentation();
  out << "typedefVar=\"TY" << static_cast<void const*>(nat->typedefVar) << "\"\n";

  printIndentation();
  out << "access=\"" << toXml(nat->access) << "\"\n";
}

void ToXMLTypeVisitor::toXml_NamedAtomicType_subtags(NamedAtomicType *nat) {
  // This is not visited by default by the type visitor, so we not
  // only have to print the id we have to print the tree.
  nat->typedefVar->traverse(*this);
}

bool ToXMLTypeVisitor::visitAtomicType(AtomicType *obj) {
  if (printedObjects.contains(obj)) return false;
  printedObjects.add(obj);

  switch(obj->getTag()) {
  default: xfailure("illegal tag");
  case AtomicType::T_SIMPLE: {
    SimpleType *simple = obj->asSimpleType();
    printIndentation();
    out << "<SimpleType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;

    printIndentation();
    out << "type=\"" << toXml(simple->type) << "\">\n";

    break;
  }

  case AtomicType::T_COMPOUND: {
    CompoundType *cpd = obj->asCompoundType();
    printIndentation();
    out << "<CompoundType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;

    // superclasses
    toXml_NamedAtomicType_properties(cpd);
    toXml_Scope_properties(cpd);

    printIndentation();
    out << "forward=\"" << toXml_bool(cpd->forward) << "\"\n";

    printIndentation();
    out << "keyword=\"" << toXml(cpd->keyword) << "\"\n";

    printIndentation();
    out << "dataMembers=\"TY" << static_cast<void const*>(&(cpd->dataMembers)) << "\"\n";

    printIndentation();
    out << "bases=\"TY" << static_cast<void const*>(&(cpd->bases)) << "\"\n";

    printIndentation();
    out << "virtualBases=\"TY" << static_cast<void const*>(&(cpd->virtualBases)) << "\"\n";

    printIndentation();
    out << "subobj=\"TY" << static_cast<void const*>(&(cpd->subobj)) << "\"\n";

    printIndentation();
    out << "conversionOperators=\"TY" << static_cast<void const*>(&(cpd->conversionOperators))
        << "\"\n";

    printIndentation();
    out << "instName=" << quoted(cpd->instName) << "\n";

    if (cpd->syntax) {
      printIndentation();
      out << "syntax=\"";
      xmlPrintPointer(out, "AST", cpd->syntax);
      out << "\"\n";
      // FIX: this is AST so we should make sure it gets printed out
    }

    if (cpd->parameterizingScope) {
      printIndentation();
      out << "parameterizingScope=\"";
      xmlPrintPointer(out, "TY", cpd->parameterizingScope);
      out << "\"\n";
    }

    printIndentation();
    out << "selfType=\"TY" << static_cast<void const*>(cpd->selfType) << "\">\n";

    // **** subtags

    toXml_NamedAtomicType_subtags(cpd);
    toXml_Scope_subtags(cpd);

    printIndentation();
    out << "<List_CompoundType_dataMembers";
    out << " .id=\"SO" << static_cast<void const*>(&(cpd->dataMembers)) << "\">\n";
    ++depth;
    SFOREACH_OBJLIST_NC(Variable, cpd->dataMembers, iter) {
      Variable *var = iter.data();
      startItem("TY", var);
      // The usual traversal rountine will not go down into here, so
      // we have to.
      var->traverse(*this);
      stopItem();
    }
    --depth;
    printIndentation();
    out << "</List_CompoundType_dataMembers>\n";

    printIndentation();
    out << "<List_CompoundType_bases";
    out << " .id=\"OJ" << static_cast<void const*>(&(cpd->bases)) << "\">\n";
    ++depth;
    FOREACH_OBJLIST_NC(BaseClass, const_cast<ObjList<BaseClass>&>(cpd->bases), iter) {
      BaseClass *base = iter.data();
      startItem("TY", base);
      // The usual traversal rountine will not go down into here, so
      // we have to.
      base->traverse(*this);
      stopItem();
    }
    --depth;
    printIndentation();
    out << "</List_CompoundType_bases>\n";

    printIndentation();
    out << "<List_CompoundType_virtualBases";
    out << " .id=\"OJ" << static_cast<void const*>(&(cpd->virtualBases)) << "\">\n";
    ++depth;
    FOREACH_OBJLIST_NC(BaseClassSubobj,
                       const_cast<ObjList<BaseClassSubobj>&>(cpd->virtualBases),
                       iter) {
      BaseClassSubobj *baseSubobj = iter.data();
      startItem("TY", baseSubobj);
      // The usual traversal rountine will not go down into here, so
      // we have to.
      baseSubobj->traverse(*this);
      stopItem();
    }
    --depth;
    printIndentation();
    out << "</List_CompoundType_virtualBases>\n";

    cpd->subobj.traverse(*this);

    printIndentation();
    out << "<List_CompoundType_conversionOperators";
    out << " .id=\"SO" << static_cast<void const*>(&(cpd->conversionOperators)) << "\">\n";
    ++depth;
    SFOREACH_OBJLIST_NC(Variable, cpd->conversionOperators, iter) {
      Variable *var = iter.data();
      startItem("TY", var);
      // The usual traversal rountine will not go down into here, so
      // we have to.
      var->traverse(*this);
      stopItem();
    }
    --depth;
    printIndentation();
    out << "</List_CompoundType_conversionOperators>\n";

    if (cpd->parameterizingScope) {
      cpd->parameterizingScope->traverse(*this);
    }

    break;
  }

  case AtomicType::T_ENUM: {
    EnumType *e = obj->asEnumType();
    printIndentation();
    out << "<EnumType";
    out << " .id=\"TY" << static_cast<void const*>(e) << "\"\n";
    ++depth;

    toXml_NamedAtomicType_properties(e);

    printIndentation();
    out << "valueIndex=\"TY" << static_cast<void const*>(&(e->valueIndex)) << "\"\n";

    printIndentation();
    out << "nextValue=\"" << e->nextValue << "\">\n";

    // **** subtags

    toXml_NamedAtomicType_subtags(e);

    printIndentation();
    out << "<NameMap_EnumType_valueIndex";
    out << " .id=\"SO" << static_cast<void const*>(&(e->valueIndex)) << "\">\n";
    ++depth;
    for(StringObjDict<EnumType::Value>::Iter iter(e->valueIndex);
        !iter.isDone(); iter.next()) {
      string const &name = iter.key();
      // dsw: do you know how bad it gets if I don't put a const-cast
      // here?
      EnumType::Value *eValue = const_cast<EnumType::Value*>(iter.value());
      // The usual traverse() rountine will not go down into here, so
      // we have to.
      //
      // NOTE: I omit putting a traverse method on EnumType::Value as
      // it should be virtual to be parallel to the other traverse()
      // methods which would add a vtable and I don't think it would
      // ever be used anyway.  So I just inline it here.
      printIndentation();
      out << "<__Name"
          << " name=" << quoted(name)
          << " item=\"TY" << static_cast<void const*>(eValue)
          << "\">\n";
      ++depth;
      bool ret = visitEnumType_Value(eValue);
      xassert(ret);
      postvisitEnumType_Value(eValue);
      --depth;
      printIndentation();
      out << "</__Name>\n";
    }
    --depth;
    printIndentation();
    out << "</NameMap_EnumType_valueIndex>\n";

    break;
  }

  case AtomicType::T_TYPEVAR: {
    TypeVariable *tvar = obj->asTypeVariable();
    printIndentation();
    out << "<TypeVariable";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;

    toXml_NamedAtomicType_properties(tvar);

    printIndentation();
    out << ">\n";

    // **** subtags

    toXml_NamedAtomicType_subtags(tvar);
    break;
  }

  case AtomicType::T_PSEUDOINSTANTIATION: {
    PseudoInstantiation *pseudo = obj->asPseudoInstantiation();
    printIndentation();
    out << "<PseudoInstantiation";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;

    toXml_NamedAtomicType_properties(pseudo);

//    CompoundType *primary;

//    // the arguments, some of which contain type variables
//    ObjList<STemplateArgument> args;

    printIndentation();
    out << ">\n";

    // **** subtags

    toXml_NamedAtomicType_subtags(pseudo);
    break;
  }

  case AtomicType::T_DEPENDENTQTYPE: {
    DependentQType *dep = obj->asDependentQType();
    printIndentation();
    out << "<DependentQType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;

    toXml_NamedAtomicType_properties(dep);

//    AtomicType *first;            // (serf) TypeVariable or PseudoInstantiation

//    // After the first component comes whatever name components followed
//    // in the original syntax.  All template arguments have been
//    // tcheck'd.
//    PQName *rest;

    printIndentation();
    out << ">\n";

    // **** subtags

    toXml_NamedAtomicType_subtags(dep);
    break;
  }
  }

  return true;
}

void ToXMLTypeVisitor::postvisitAtomicType(AtomicType *obj) {
  --depth;

  printIndentation();

  switch(obj->getTag()) {
  default: xfailure("illegal tag");
  case AtomicType::T_SIMPLE:
    out << "</SimpleType>\n";
    break;
  case AtomicType::T_COMPOUND:
    out << "</CompoundType>\n";
    break;
  case AtomicType::T_ENUM:
    out << "</EnumType>\n";
    break;
  case AtomicType::T_TYPEVAR:
    out << "</TypeVariable>\n";
    break;
  case AtomicType::T_PSEUDOINSTANTIATION:
    out << "</PseudoInstantiation>\n";
    break;
  case AtomicType::T_DEPENDENTQTYPE:
    out << "</DependentQType>\n";
    break;
  }
}


bool ToXMLTypeVisitor::visitEnumType_Value(void /*EnumType::Value*/ *eValue0) {
  EnumType::Value *eValue = static_cast<EnumType::Value *>(eValue0);
  if (printedObjects.contains(eValue)) return false;
  printedObjects.add(eValue);
  printIndentation();
  out << "<EnumType_Value";
  out << " .id=\"TY" << static_cast<void const*>(eValue) << "\"\n";
  ++depth;

  printIndentation();
  out << "name=" << quoted(eValue->name) << "\n";

  printIndentation();
  out << "type=\"TY" << static_cast<void const*>(&(eValue->type)) << "\"\n";

  printIndentation();
  out << "value=\"" << eValue->value << "\"\n";

  if (eValue->decl) {
    printIndentation();
    out << "decl=\"TY" << static_cast<void const*>(&(eValue->decl)) << "\">\n";
  }

  // **** subtags
  //
  // NOTE: the hypothetical EnumType::Value::traverse() method would
  // probably do this, so perhaps it should be inlined above where
  // said hypothetical method would go, but instead I just put it here
  // as it seems just as natural.

  eValue->type->traverse(*this);
  eValue->decl->traverse(*this);

  return true;
}

void ToXMLTypeVisitor::postvisitEnumType_Value(void /*EnumType::Value*/ *eValue0) {
//    EnumType::Value *eValue = static_cast<EnumType::Value*>(eValue0);
  --depth;
  printIndentation();
  out << "</EnumType_Value>\n";
}


void ToXMLTypeVisitor::toXml_Scope_properties(Scope *scope)
{
  printIndentation();
  out << "variables=\"SM" << static_cast<void const*>(&(scope->variables)) << "\"\n";

  printIndentation();
  out << "typeTags=\"SM" << static_cast<void const*>(&(scope->typeTags)) << "\"\n";

  printIndentation();
  out << "canAcceptNames=\"" << toXml_bool(scope->canAcceptNames) << "\"\n";

  printIndentation();
  out << "parentScope=\"TY" << static_cast<void const*>(scope->parentScope) << "\"\n";

  printIndentation();
  out << "scopeKind=\"" << toXml(scope->scopeKind) << "\"\n";

  printIndentation();
  out << "namespaceVar=\"TY" << static_cast<void const*>(scope->namespaceVar) << "\"\n";

  // FIX: do this when we do templates
//    printIndentation();
//    out << "templateParams=\"SO" << static_cast<void const*>(&(scope->templateParams)) << "\"\n";

  printIndentation();
  out << "curCompound=\"TY" << static_cast<void const*>(scope->curCompound) << "\"\n";
}

void ToXMLTypeVisitor::toXml_Scope_subtags(Scope *scope)
{
  // nothing to do as the traverse visits everything
}

bool ToXMLTypeVisitor::visitScope(Scope *scope)
{
  if (printedObjects.contains(scope)) return false;
  printedObjects.add(scope);

  printIndentation();
  out << "<Scope";
  out << " .id=\"TY" << static_cast<void const*>(scope) << "\"\n";
  ++depth;

  toXml_Scope_properties(scope);

  printIndentation();
  out << ">\n";

  // **** subtags

  toXml_Scope_subtags(scope);

  return true;
}
void ToXMLTypeVisitor::postvisitScope(Scope *scope)
{
  --depth;
  printIndentation();
  out << "</Scope>\n";
}

bool ToXMLTypeVisitor::visitScope_NameMap_variables(StringRefMap<Variable> &variables)
{
  printIndentation();
  out << "<NameMap_Scope_variables";
  out << " .id=\"SM" << static_cast<void const*>(&variables) << "\">\n";
  ++depth;
  return true;
}
void ToXMLTypeVisitor::visitScope_NameMap_variables_entry(StringRef name, Variable *var)
{
  printIndentation();
  out << "<__Name"
      << " name=" << quoted(name)
      << " item=\"TY" << static_cast<void const*>(var)
      << "\">\n";
  ++depth;
  bool ret = visitVariable(var);
  xassert(ret);
  postvisitVariable(var);
  --depth;
  out << "</__Name>\n";
}
void ToXMLTypeVisitor::postvisitScope_NameMap_variables(StringRefMap<Variable> &variables)
{
  --depth;
  printIndentation();
  out << "</NameMap_Scope_variables>\n";
}

bool ToXMLTypeVisitor::visitScope_NameMap_typeTags(StringRefMap<Variable> &typeTags)
{
  printIndentation();
  out << "<NameMap_Scope_typeTags";
  out << " .id=\"SM" << static_cast<void const*>(&typeTags) << "\">\n";
  ++depth;
  return true;
}
void ToXMLTypeVisitor::visitScope_NameMap_typeTags_entry(StringRef name, Variable *var)
{
  printIndentation();
  out << "<__Name"
      << " name=" << quoted(name)
      << " item=\"TY" << static_cast<void const*>(var)
      << "\">\n";
  ++depth;
  bool ret = visitVariable(var);
  xassert(ret);
  postvisitVariable(var);
  --depth;
  out << "</__Name>\n";
}
void ToXMLTypeVisitor::postvisitScope_NameMap_typeTags(StringRefMap<Variable> &typeTags)
{
  --depth;
  printIndentation();
  out << "</NameMap_Scope_typeTags>\n";
}

void ToXMLTypeVisitor::toXml_BaseClass_properties(BaseClass *bc)
{
  printIndentation();
  out << "ct=\"TY" << static_cast<void const*>(bc->ct) << "\"\n";

  printIndentation();
  out << "access=\"" << toXml(bc->access) << "\"\n";

  printIndentation();
  out << "isVirtual=\"" << toXml_bool(bc->isVirtual) << "\"\n";
}

bool ToXMLTypeVisitor::visitBaseClass(BaseClass *bc)
{
  if (printedObjects.contains(bc)) return false;
  printedObjects.add(bc);

  printIndentation();
  out << "<BaseClass";
  out << " .id=\"TY" << static_cast<void const*>(bc) << "\"\n";
  ++depth;

  toXml_BaseClass_properties(bc);

  printIndentation();
  out << ">\n";

  // **** subtags
  // none

  return true;
}
void ToXMLTypeVisitor::postvisitBaseClass(BaseClass *bc)
{
  --depth;
  printIndentation();
  out << "</BaseClass>\n";
}

bool ToXMLTypeVisitor::visitBaseClassSubobj(BaseClassSubobj *bc)
{
  if (printedObjects.contains(bc)) return false;
  printedObjects.add(bc);

  printIndentation();
  out << "<BaseClassSubobj";
  out << " .id=\"TY" << static_cast<void const*>(bc) << "\"\n";
  ++depth;

  toXml_BaseClass_properties(bc);

  printIndentation();
  out << "parents=\"SO" << static_cast<void const*>(&(bc->parents)) << "\">\n";

  // **** subtags
  // none

  return true;
}
void ToXMLTypeVisitor::postvisitBaseClassSubobj(BaseClassSubobj *bc)
{
  --depth;
  printIndentation();
  out << "</BaseClassSubobj>\n";
}

bool ToXMLTypeVisitor::visitBaseClassSubobjParentsList(SObjList<BaseClassSubobj> &parents)
{
  printIndentation();
  out << "<List_BaseClassSubobj_parents";
  out << " .id=\"SO" << static_cast<void const*>(&parents) << "\">\n";
  ++depth;
  return true;
}

void ToXMLTypeVisitor::postvisitBaseClassSubobjParentsList(SObjList<BaseClassSubobj> &parents)
{
  --depth;
  printIndentation();
  out << "</List_BaseClassSubobj_parents>\n";
}

bool ToXMLTypeVisitor::visitBaseClassSubobjParentsListItem(BaseClassSubobj *parent)
{
  startItem("TY", parent);
  return true;
}

void ToXMLTypeVisitor::postvisitBaseClassSubobjParentsListItem(BaseClassSubobj *parent)
{
  stopItem();
}

// -------------------- ReadXml_Type -------------------

//  #include "astxml_parse1_1defn.gen.cc"
void ReadXml_Type::append2List(void *list0, int listKind, void *datum0) {
  xassert(list0);
  ASTList<char> *list = static_cast<ASTList<char>*>(list0);
  char *datum = (char*)datum0;
  list->append(datum);
}

void ReadXml_Type::insertIntoNameMap
  (void *map0, int mapKind, StringRef name, void *datum0) {

  xassert(map0);
  StringRefMap<char> *map = static_cast<StringRefMap<char>*>(map0);
  char *datum = (char*)datum0;

  if (map->get(name)) {
    userError(stringc << "duplicate name " << name << " in map");
  }

  map->add(name, datum);
}

bool ReadXml_Type::kind2kindCat0(int kind, KindCategory *kindCat) {
  switch(kind) {
  default: return false;        // we don't know this kind

  // Types
  case XTOK_CVAtomicType:        *kindCat = KC_Node; break;
  case XTOK_PointerType:         *kindCat = KC_Node; break;
  case XTOK_ReferenceType:       *kindCat = KC_Node; break;
  case XTOK_FunctionType:        *kindCat = KC_Node; break;
  case XTOK_FunctionType_ExnSpec:*kindCat = KC_Node; break; // special
  case XTOK_ArrayType:           *kindCat = KC_Node; break;
  case XTOK_PointerToMemberType: *kindCat = KC_Node; break;

  // AtomicTypes
  case XTOK_SimpleType:          *kindCat = KC_Node; break;
  case XTOK_CompoundType:        *kindCat = KC_Node; break;
  case XTOK_EnumType:            *kindCat = KC_Node; break;
  case XTOK_TypeVariable:        *kindCat = KC_Node; break;
  case XTOK_PseudoInstantiation: *kindCat = KC_Node; break;
  case XTOK_DependentQType:      *kindCat = KC_Node; break;

  // Other
  case XTOK_Variable:            *kindCat = KC_Node; break;
  case XTOK_Scope:               *kindCat = KC_Node; break;
  case XTOK_BaseClass:           *kindCat = KC_Node; break;
  case XTOK_BaseClassSubobj:     *kindCat = KC_Node; break;

  // Containers
  //   ObjList
  case XTOK_List_CompoundType_bases:               *kindCat = KC_ObjList;       break;
  case XTOK_List_CompoundType_virtualBases:        *kindCat = KC_ObjList;       break;
  //   SObjList
  case XTOK_List_FunctionType_params:              *kindCat = KC_SObjList;      break;
  case XTOK_List_CompoundType_dataMembers:         *kindCat = KC_SObjList;      break;
  case XTOK_List_CompoundType_conversionOperators: *kindCat = KC_SObjList;      break;
  case XTOK_List_BaseClassSubobj_parents:          *kindCat = KC_SObjList;      break;
  case XTOK_List_ExnSpec_types:                    *kindCat = KC_SObjList;      break;

  //   StringRefMap
  case XTOK_NameMap_Scope_variables:               *kindCat = KC_StringRefMap;  break;
  case XTOK_NameMap_Scope_typeTags:                *kindCat = KC_StringRefMap;  break;
  case XTOK_NameMap_EnumType_valueIndex:           *kindCat = KC_StringRefMap;  break;
  }
  return true;
}

bool ReadXml_Type::convertList2FakeList(ASTList<char> *list, int listKind, void **target) {
  xfailure("should not be called during Type parsing there are no FakeLists in the Type System");
  return false;
}

bool ReadXml_Type::convertList2SObjList(ASTList<char> *list, int listKind, void **target) {
  // NOTE: SObjList only has constant-time prepend, not constant-time
  // append, hence the prepend() and reverse().
  xassert(list);

  switch(listKind) {
  default: return false;        // we did not find a matching tag

  case XTOK_List_FunctionType_params:
  case XTOK_List_CompoundType_dataMembers:
  case XTOK_List_CompoundType_conversionOperators: {
    SObjList<Variable> *ret = reinterpret_cast<SObjList<Variable>*>(target);
    xassert(ret->isEmpty());
    FOREACH_ASTLIST_NC(Variable, reinterpret_cast<ASTList<Variable>&>(*list), iter) {
      Variable *var = iter.data();
      ret->prepend(var);
    }
    ret->reverse();
    break;
  }

  case XTOK_List_BaseClassSubobj_parents: {
    SObjList<BaseClassSubobj> *ret = reinterpret_cast<SObjList<BaseClassSubobj>*>(target);
    xassert(ret->isEmpty());
    FOREACH_ASTLIST_NC(BaseClassSubobj, reinterpret_cast<ASTList<BaseClassSubobj>&>(*list), iter) {
      BaseClassSubobj *bcs = iter.data();
      ret->prepend(bcs);
    }
    ret->reverse();
    break;
  }

  case XTOK_List_ExnSpec_types: {
    SObjList<Type> *ret = reinterpret_cast<SObjList<Type>*>(target);
    xassert(ret->isEmpty());
    FOREACH_ASTLIST_NC(Type, reinterpret_cast<ASTList<Type>&>(*list), iter) {
      Type *type = iter.data();
      ret->prepend(type);
    }
    ret->reverse();
    break;
  }
  }
  return true;
}

bool ReadXml_Type::convertList2ObjList (ASTList<char> *list, int listKind, void **target) {
  // NOTE: ObjList only has constant-time prepend, not constant-time
  // append, hence the prepend() and reverse().
  xassert(list);

  switch(listKind) {
  default: return false;        // we did not find a matching tag

  case XTOK_List_CompoundType_bases: {
    ObjList<BaseClass> *ret = reinterpret_cast<ObjList<BaseClass>*>(target);
    xassert(ret->isEmpty());
    FOREACH_ASTLIST_NC(BaseClass, reinterpret_cast<ASTList<BaseClass>&>(*list), iter) {
      BaseClass *bcs = iter.data();
      ret->prepend(bcs);
    }
    ret->reverse();
    break;
  }

  case XTOK_List_CompoundType_virtualBases: {
    ObjList<BaseClassSubobj> *ret = reinterpret_cast<ObjList<BaseClassSubobj>*>(target);
    xassert(ret->isEmpty());
    FOREACH_ASTLIST_NC(BaseClassSubobj, reinterpret_cast<ASTList<BaseClassSubobj>&>(*list), iter) {
      BaseClassSubobj *bcs = iter.data();
      ret->prepend(bcs);
    }
    ret->reverse();
    break;
  }

  }
  return true;
}

bool ReadXml_Type::convertNameMap2StringRefMap
  (StringRefMap<char> *map, int mapKind, void *target)
{
  xassert(map);

  switch(mapKind) {
  default: return false;        // we did not find a matching tag

  case XTOK_NameMap_Scope_variables:
  case XTOK_NameMap_Scope_typeTags: {
    StringRefMap<Variable> *ret = reinterpret_cast<StringRefMap<Variable>*>(target);
    xassert(ret->isEmpty());
    for(StringRefMap<Variable>::Iter iter(reinterpret_cast<StringRefMap<Variable>&>(*map));
        !iter.isDone(); iter.adv()) {
      StringRef name = iter.key();
      Variable *value = iter.value();
      ret->add(name, value);
    }
    break;
  }

  }
  return true;
}

bool ReadXml_Type::convertNameMap2StringSObjDict
  (StringRefMap<char> *map, int mapKind, void *target)
{
  xassert(map);

  switch(mapKind) {
  default: return false;        // we did not find a matching tag

  case XTOK_NameMap_EnumType_valueIndex: {
    StringSObjDict<EnumType::Value> *ret =
      reinterpret_cast<StringSObjDict<EnumType::Value>*>(target);
    xassert(ret->isEmpty());
    for(StringRefMap<EnumType::Value>::Iter
          iter(reinterpret_cast<StringRefMap<EnumType::Value>&>(*map));
        !iter.isDone(); iter.adv()) {
      StringRef name = iter.key();
      EnumType::Value *value = iter.value();
      ret->add(name, value);
    }
    break;
  }

  }
  return true;
}

void ReadXml_Type::ctorNodeFromTag(int tag, void *&topTemp) {
  switch(tag) {
  default: userError("unexpected token while looking for an open tag name");
  case 0: userError("unexpected file termination while looking for an open tag name");

  // **** Types
  case XTOK_CVAtomicType:
    topTemp = new CVAtomicType((AtomicType*)0, (CVFlags)0);
    break;

  case XTOK_PointerType:
    topTemp = new PointerType((CVFlags)0, (Type*)0);
    break;

  case XTOK_ReferenceType:
    topTemp = new ReferenceType((Type*)0);
    break;

  case XTOK_FunctionType:
    topTemp = new FunctionType((Type*)0);
    break;

  case XTOK_FunctionType_ExnSpec:
    topTemp = new FunctionType::ExnSpec();
    break;

  case XTOK_ArrayType:
    topTemp = new ArrayType((ReadXML&)*this); // call the special ctor
    break;

  case XTOK_PointerToMemberType:
    topTemp = new PointerToMemberType((NamedAtomicType*)0, (CVFlags)0, (Type*)0);
    break;

  // **** Atomic Types
  case XTOK_SimpleType:
    // NOTE: this really should go through the SimpleTyp::fixed array
    topTemp = new SimpleType((SimpleTypeId)0);
    break;

  case XTOK_CompoundType:
    topTemp = new CompoundType((CompoundType::Keyword)0, (StringRef)0);
    break;

  case XTOK_EnumType:
    topTemp = new EnumType((StringRef)0);
    break;

  case XTOK_EnumType_Value:
    topTemp = new EnumType::Value((StringRef)0, (EnumType*)0, (int)0, (Variable*)0);
    break;

  case XTOK_TypeVariable:
    topTemp = new TypeVariable((StringRef)0);
    break;

  case XTOK_PseudoInstantiation:
    topTemp = new PseudoInstantiation((CompoundType*)0);
    break;

  case XTOK_DependentQType:
    topTemp = new DependentQType((AtomicType*)0);
    break;

  // **** Other
  case XTOK_Variable:
    topTemp = new Variable((ReadXML&)*this); // call the special ctor
    break;
  case XTOK_Scope:
    topTemp = new Scope((ReadXML&)*this); // call the special ctor
    break;
  case XTOK_BaseClass:
    topTemp = new BaseClass((CompoundType*)0, (AccessKeyword)0, (bool)0);
    break;
  case XTOK_BaseClassSubobj:
    // NOTE: special; FIX: should I make the BaseClass on the heap and
    // then delete it?  I'm not sure if the compiler is going to be
    // able to tell that even though it is passed by reference to the
    // BaseClassSubobj that it is not kept there and therefore can be
    // deleted at the end of the full expression.
    topTemp = new BaseClassSubobj(BaseClass((CompoundType*)0, (AccessKeyword)0, (bool)0));
    break;

  // **** Containers
  // ObjList
  case XTOK_List_CompoundType_bases:
    topTemp = new ASTList<BaseClass>();
    break;
  case XTOK_List_CompoundType_virtualBases:
    topTemp = new ASTList<BaseClassSubobj>();
    break;
  // SObjList
  case XTOK_List_FunctionType_params:
    topTemp = new ASTList<Variable>();
    break;
  case XTOK_List_CompoundType_dataMembers:
    topTemp = new ASTList<Variable>();
    break;
  case XTOK_List_CompoundType_conversionOperators:
    topTemp = new ASTList<Variable>();
    break;
  case XTOK_List_BaseClassSubobj_parents:
    topTemp = new ASTList<BaseClassSubobj>();
    break;
  case XTOK_List_ExnSpec_types:
    topTemp = new ASTList<Type>();
    break;

  // StringRefMap
  case XTOK_NameMap_Scope_variables:
    topTemp = new StringRefMap<Variable>();
    break;
  case XTOK_NameMap_Scope_typeTags:
    topTemp = new StringRefMap<Variable>();
    break;
  case XTOK_NameMap_EnumType_valueIndex:
    topTemp = new StringRefMap<EnumType::Value>();
    break;

//  #include "astxml_parse1_2ctrc.gen.cc"
  }
}

// **************** registerAttribute

void ReadXml_Type::registerAttribute(void *target, int kind, int attr, char const *yytext0) {
  switch(kind) {
  default: xfailure("illegal kind");

  // **** Types
  case XTOK_CVAtomicType:
    registerAttr_CVAtomicType((CVAtomicType*)target, attr, yytext0);
    break;

  case XTOK_PointerType:
    registerAttr_PointerType((PointerType*)target, attr, yytext0);
    break;

  case XTOK_ReferenceType:
    registerAttr_ReferenceType((ReferenceType*)target, attr, yytext0);
    break;

  case XTOK_FunctionType:
    registerAttr_FunctionType((FunctionType*)target, attr, yytext0);
    break;

  case XTOK_FunctionType_ExnSpec:
    registerAttr_FunctionType_ExnSpec((FunctionType::ExnSpec*)target, attr, yytext0);
    break;

  case XTOK_ArrayType:
    registerAttr_ArrayType((ArrayType*)target, attr, yytext0);
    break;

  case XTOK_PointerToMemberType:
    registerAttr_PointerToMemberType((PointerToMemberType*)target, attr, yytext0);
    break;

  // **** Atomic Types
  case XTOK_SimpleType:
    registerAttr_SimpleType((SimpleType*)target, attr, yytext0);
    break;

  case XTOK_CompoundType:
    registerAttr_CompoundType((CompoundType*)target, attr, yytext0);
    break;

  case XTOK_EnumType:
    registerAttr_EnumType((EnumType*)target, attr, yytext0);
    break;

  case XTOK_EnumType_Value:
    registerAttr_EnumType_Value((EnumType::Value*)target, attr, yytext0);
    break;

  case XTOK_TypeVariable:
    registerAttr_TypeVariable((TypeVariable*)target, attr, yytext0);
    break;

  case XTOK_PseudoInstantiation:
    registerAttr_PseudoInstantiation((PseudoInstantiation*)target, attr, yytext0);
    break;

  case XTOK_DependentQType:
    registerAttr_DependentQType((DependentQType*)target, attr, yytext0);
    break;

  // **** Other
  case XTOK_Variable:
    registerAttr_Variable((Variable*)target, attr, yytext0);
    break;

  case XTOK_Scope:
    registerAttr_Scope((Scope*)target, attr, yytext0);
    break;

  case XTOK_BaseClass:
    registerAttr_BaseClass((BaseClass*)target, attr, yytext0);
    break;

  case XTOK_BaseClassSubobj:
    registerAttr_BaseClassSubobj((BaseClassSubobj*)target, attr, yytext0);
    break;

//  #include "astxml_parse1_3regc.gen.cc"
  }
}

void ReadXml_Type::registerAttr_CVAtomicType
  (CVAtomicType *obj, int attr, char const *strValue) {
  switch(attr) {
  default:
    userError("illegal attribute for a CVAtomicType");
    break;

  case XTOK_cv:
    fromXml(obj->cv, parseQuotedString(strValue));
    break;

  case XTOK_atomic:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->atomic),
                     parseQuotedString(strValue)));
    break;

  }
}

void ReadXml_Type::registerAttr_PointerType
  (PointerType *obj, int attr, char const *strValue) {
  switch(attr) {
  default:
    userError("illegal attribute for a PointerType");
    break;

  case XTOK_cv:
    fromXml(obj->cv, parseQuotedString(strValue));
    break;

  case XTOK_atType:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->atType),
                     parseQuotedString(strValue)));
    break;

  }
}

void ReadXml_Type::registerAttr_ReferenceType
  (ReferenceType *obj, int attr, char const *strValue) {
  switch(attr) {
  default:
    userError("illegal attribute for a ReferenceType");
    break;

  case XTOK_atType:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->atType),
                     parseQuotedString(strValue)));
    break;

  }
}

void ReadXml_Type::registerAttr_FunctionType
  (FunctionType *obj, int attr, char const *strValue) {
  switch(attr) {
  default:
    userError("illegal attribute for a FunctionType");
    break;

  case XTOK_flags:
    fromXml(obj->flags, parseQuotedString(strValue));
    break;

  case XTOK_retType:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->retType),
                     parseQuotedString(strValue)));
    break;

  case XTOK_params:
    linkSat.unsatLinks_List.append
      (new UnsatLink((void**) &(obj->params),
                     parseQuotedString(strValue),
                     XTOK_List_FunctionType_params));
    break;

  case XTOK_exnSpec:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->exnSpec),
                     parseQuotedString(strValue)));
    break;

  }
}

void ReadXml_Type::registerAttr_FunctionType_ExnSpec
  (FunctionType::ExnSpec *obj, int attr, char const *strValue) {
  switch(attr) {
  default:
    userError("illegal attribute for a FunctionType_ExnSpec");
    break;

  case XTOK_types:
    linkSat.unsatLinks_List.append
      (new UnsatLink((void**) &(obj->types),
                     parseQuotedString(strValue),
                     XTOK_List_ExnSpec_types));
    break;
  }
}

void ReadXml_Type::registerAttr_ArrayType
  (ArrayType *obj, int attr, char const *strValue) {
  switch(attr) {
  default:
    userError("illegal attribute for a ArrayType");
    break;

  case XTOK_eltType:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->eltType),
                     parseQuotedString(strValue)));
    break;

  case XTOK_size:
    obj->size = atoi(parseQuotedString(strValue));
    break;

  }
}

void ReadXml_Type::registerAttr_PointerToMemberType
  (PointerToMemberType *obj, int attr, char const *strValue) {
  switch(attr) {
  default:
    userError("illegal attribute for a PointerToMemberType");
    break;

  case XTOK_inClassNAT:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->inClassNAT),
                     parseQuotedString(strValue)));
    break;

  case XTOK_cv:
    fromXml(obj->cv, parseQuotedString(strValue));
    break;

  case XTOK_atType:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->atType),
                     parseQuotedString(strValue)));
    break;

  }
}

void ReadXml_Type::registerAttr_Variable
  (Variable *obj, int attr, char const *strValue) {
  switch(attr) {
  default:
    userError("illegal attribute for a Variable");
    break;

  // FIX: SourceLoc loc

  case XTOK_name:
    obj->name = strTable(parseQuotedString(strValue));
    break;

  case XTOK_type:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->type),
                     parseQuotedString(strValue)));
    break;

  case XTOK_flags:
    fromXml(const_cast<DeclFlags&>(obj->flags), parseQuotedString(strValue));
    break;

  case XTOK_value:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->value),
                     parseQuotedString(strValue)));
    break;

  case XTOK_defaultParamType:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->defaultParamType),
                     parseQuotedString(strValue)));
    break;

  case XTOK_funcDefn:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->funcDefn),
                     parseQuotedString(strValue)));
    break;

  case XTOK_scope:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->scope),
                     parseQuotedString(strValue)));
    break;

  case XTOK_intData:
    fromXml_Variable_intData(obj->intData, parseQuotedString(strValue));
    break;

  case XTOK_usingAlias_or_parameterizedEntity:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->usingAlias_or_parameterizedEntity),
                     parseQuotedString(strValue)));
    break;

  // FIX: templInfo
  }
}

bool ReadXml_Type::registerAttr_NamedAtomicType_super
  (NamedAtomicType *obj, int attr, char const *strValue) {
  switch(attr) {
  default:
    return false;               // we didn't find it
    break;

  case XTOK_name:
    obj->name = strTable(parseQuotedString(strValue));
    break;

  case XTOK_typedefVar:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->typedefVar),
                     parseQuotedString(strValue)));
    break;

  case XTOK_access:
    fromXml(obj->access, parseQuotedString(strValue));
    break;

  }
  return true;                  // found it
}

void ReadXml_Type::registerAttr_SimpleType
  (SimpleType *obj, int attr, char const *strValue) {
  switch(attr) {
  default:
    userError("illegal attribute for a SimpleType");
    break;

  case XTOK_type:
    // NOTE: this 'type' is not a type node, but basically an enum,
    // and thus is handled more like a flag would be.
    fromXml(const_cast<SimpleTypeId&>(obj->type), parseQuotedString(strValue));
    break;

  }
}

void ReadXml_Type::registerAttr_CompoundType
  (CompoundType *obj, int attr, char const *strValue) {

  // superclasses
  if (registerAttr_NamedAtomicType_super(obj, attr, strValue)) return;
  if (registerAttr_Scope_super(obj, attr, strValue)) return;

  switch(attr) {
  default:
    userError("illegal attribute for a CompoundType");
    break;

  case XTOK_forward:
    fromXml_bool(obj->forward, parseQuotedString(strValue));
    break;

  case XTOK_keyword:
    fromXml(obj->keyword, parseQuotedString(strValue));
    break;

  case XTOK_dataMembers:
    linkSat.unsatLinks_List.append
      (new UnsatLink((void**) &(obj->dataMembers),
                     parseQuotedString(strValue),
                     XTOK_List_CompoundType_dataMembers));
    break;

  case XTOK_bases:
    linkSat.unsatLinks_List.append
      (new UnsatLink((void**) &(obj->bases),
                     parseQuotedString(strValue),
                     XTOK_List_CompoundType_bases));
    break;

  case XTOK_virtualBases:
    linkSat.unsatLinks_List.append
      (new UnsatLink((void**) &(obj->virtualBases),
                     parseQuotedString(strValue),
                     XTOK_List_CompoundType_virtualBases));
    break;

  case XTOK_subobj:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->subobj),
                     parseQuotedString(strValue)));
    break;

  case XTOK_conversionOperators:
    linkSat.unsatLinks_List.append
      (new UnsatLink((void**) &(obj->conversionOperators),
                     parseQuotedString(strValue),
                     XTOK_List_CompoundType_conversionOperators));
    break;

  case XTOK_instName:
    obj->instName = strTable(parseQuotedString(strValue));
    break;

  case XTOK_syntax:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->syntax),
                     parseQuotedString(strValue)));
    break;

  case XTOK_parameterizingScope:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->parameterizingScope),
                     parseQuotedString(strValue)));
    break;

  case XTOK_selfType:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->selfType),
                     parseQuotedString(strValue)));
    break;

  }
}

void ReadXml_Type::registerAttr_EnumType
  (EnumType *obj, int attr, char const *strValue) {

  // superclass
  if (registerAttr_NamedAtomicType_super(obj, attr, strValue)) return;

  switch(attr) {
  default:
    userError("illegal attribute for a EnumType");
    break;

  case XTOK_valueIndex:
    linkSat.unsatLinks_NameMap.append
      (new UnsatLink((void**) &(obj->valueIndex),
                     parseQuotedString(strValue),
                     XTOK_NameMap_EnumType_valueIndex));
    break;

  case XTOK_nextValue:
    obj->nextValue = atoi(parseQuotedString(strValue));
    break;

  }
}

void ReadXml_Type::registerAttr_EnumType_Value
  (EnumType::Value *obj, int attr, char const *strValue) {

  switch(attr) {
  default:
    userError("illegal attribute for a EnumType");
    break;

  case XTOK_name:
    obj->name = strTable(parseQuotedString(strValue));
    break;

  case XTOK_type:
    // NOTE: 'type' here is actually an atomic type
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->type),
                     parseQuotedString(strValue)));
    break;

  case XTOK_value:
    obj->value = atoi(parseQuotedString(strValue));
    break;

  case XTOK_decl:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->decl),
                     parseQuotedString(strValue)));
    break;

  }
}

void ReadXml_Type::registerAttr_TypeVariable
  (TypeVariable *obj, int attr, char const *strValue) {

  // superclass
  if (registerAttr_NamedAtomicType_super(obj, attr, strValue)) return;

  // shouldn't get here
  userError("illegal attribute for a TypeVariable");
}

void ReadXml_Type::registerAttr_PseudoInstantiation
  (PseudoInstantiation *obj, int attr, char const *strValue) {

  // superclass
  if (registerAttr_NamedAtomicType_super(obj, attr, strValue)) return;

  switch(attr) {
  default:
    userError("illegal attribute for a PsuedoInstantiation");
    break;

//    CompoundType *primary;

//    // the arguments, some of which contain type variables
//    ObjList<STemplateArgument> args;

  }
}

void ReadXml_Type::registerAttr_DependentQType
  (DependentQType *obj, int attr, char const *strValue) {

  // superclass
  if (registerAttr_NamedAtomicType_super(obj, attr, strValue)) return;

  switch(attr) {
  default:
    userError("illegal attribute for a DependentQType");
    break;

//    AtomicType *first;            // (serf) TypeVariable or PseudoInstantiation

//    // After the first component comes whatever name components followed
//    // in the original syntax.  All template arguments have been
//    // tcheck'd.
//    PQName *rest;

  }
}

bool ReadXml_Type::registerAttr_Scope_super
  (Scope *obj, int attr, char const *strValue) {
  switch(attr) {
  default:
    return false;               // we didn't find it
    break;

  case XTOK_variables:
    linkSat.unsatLinks_NameMap.append
      (new UnsatLink((void**) &(obj->variables),
                     parseQuotedString(strValue),
                     XTOK_NameMap_Scope_variables));
    break;

  case XTOK_typeTags:
    linkSat.unsatLinks_NameMap.append
      (new UnsatLink((void**) &(obj->typeTags),
                     parseQuotedString(strValue),
                     XTOK_NameMap_Scope_typeTags));
    break;

  case XTOK_canAcceptNames:
    fromXml_bool(obj->canAcceptNames, parseQuotedString(strValue));
    break;

  case XTOK_parentScope:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->parentScope),
                     parseQuotedString(strValue)));
    break;

  case XTOK_scopeKind:
    fromXml(obj->scopeKind, parseQuotedString(strValue));
    break;

  case XTOK_namespaceVar:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->namespaceVar),
                     parseQuotedString(strValue)));
    break;

  // templateParams

  case XTOK_curCompound:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->curCompound),
                     parseQuotedString(strValue)));
    break;

  }
  return true;                  // found it
}

void ReadXml_Type::registerAttr_Scope
  (Scope *obj, int attr, char const *strValue) {

  // "superclass": just re-use our own superclass code for ourself
  if (registerAttr_Scope_super(obj, attr, strValue)) return;

  // shouldn't get here
  userError("illegal attribute for a Scope");
}

bool ReadXml_Type::registerAttr_BaseClass_super
  (BaseClass *obj, int attr, char const *strValue) {
  switch(attr) {
  default:
    return false;
    break;

  case XTOK_ct:
    linkSat.unsatLinks.append
      (new UnsatLink((void**) &(obj->ct),
                     parseQuotedString(strValue)));
    break;

  case XTOK_access:
    fromXml(obj->access, parseQuotedString(strValue));
    break;

  case XTOK_isVirtual:
    fromXml_bool(obj->isVirtual, parseQuotedString(strValue));
    break;
  }
  return true;
}

void ReadXml_Type::registerAttr_BaseClass
  (BaseClass *obj, int attr, char const *strValue) {

  // "superclass": just re-use our own superclass code for ourself
  if (registerAttr_BaseClass_super(obj, attr, strValue)) return;

  // shouldn't get here
  userError("illegal attribute for a BaseClass");
}

void ReadXml_Type::registerAttr_BaseClassSubobj
  (BaseClassSubobj *obj, int attr, char const *strValue) {

  // "superclass": just re-use our own superclass code for ourself
  if (registerAttr_BaseClass_super(obj, attr, strValue)) return;

  switch(attr) {
  default:
    userError("illegal attribute for a BaseClassSubobj");
    break;

  case XTOK_parents:
    linkSat.unsatLinks_List.append
      (new UnsatLink((void**) &(obj->parents),
                     parseQuotedString(strValue),
                     XTOK_List_BaseClassSubobj_parents));
    break;

  }
}
