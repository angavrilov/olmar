// cc_type_xml.cc            see license.txt for copyright and terms of use

#include "cc_type_xml.h"        // this module
#include "variable.h"           // Variable
#include "asthelp.h"            // xmlPrintPointer

#include "strutil.h"            // parseQuotedString
#include "astxml_lexer.h"       // AstXmlLexer



string toXml(CompoundType::Keyword id) {
  return stringc << static_cast<int>(id);
}

void fromXml(CompoundType::Keyword &out, string str) {
  out = static_cast<CompoundType::Keyword>(atoi(str));
}


string toXml(FunctionFlags id) {
  return stringc << static_cast<int>(id);
}

void fromXml(FunctionFlags &out, string str) {
  out = static_cast<FunctionFlags>(atoi(str));
}


string toXml(ScopeKind id) {
  return stringc << static_cast<int>(id);
}

void fromXml(ScopeKind &out, string str) {
  out = static_cast<ScopeKind>(atoi(str));
}

// -------------------- ToXMLTypeVisitor -------------------

void ToXMLTypeVisitor::printIndentation() {
  if (indent) {
    for (int i=0; i<depth; ++i) cout << " ";
  }
}

bool ToXMLTypeVisitor::visitType(Type *obj) {
  if (printedTypes.contains(obj)) return false;
  printIndentation();

  switch(obj->getTag()) {
  default: xfailure("illegal tag");
  case Type::T_ATOMIC: {
    CVAtomicType *atom = obj->asCVAtomicType();
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
    out << "<ReferenceType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;
    printIndentation();
    out << "atType=\"TY" << static_cast<void const*>(ref->atType) << "\">\n";
    break;
  }
  case Type::T_FUNCTION: {
    FunctionType *func = obj->asFunctionType();
    out << "<FunctionType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;
    printIndentation();
    out << "flags=\"" << toXml(func->flags) << "\"\n";
    printIndentation();
    out << "retType=\"TY" << static_cast<void const*>(func->retType) << "\"\n";
    printIndentation();
    out << "params=\"SO" << static_cast<void const*>(&(func->params)) << "\"\n";
    // FIX: skip this for now
    // ExnSpec *exnSpec;                  // (nullable owner)

    // FIX: remove this when we do the above attributes
    printIndentation();
    cout << ">\n";

    break;
  }
  case Type::T_ARRAY: {
    ArrayType *arr = obj->asArrayType();
    out << "<ArrayType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;
    printIndentation();
    out << "eltType=\"TY" << static_cast<void const*>(arr->eltType) << "\"\n";
    printIndentation();
    out << "size=\"" << arr->size << "\">";
    break;
  }
  case Type::T_POINTERTOMEMBER: {
    PointerToMemberType *ptm = obj->asPointerToMemberType();
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

  printedTypes.add(obj);
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
  out << "<FunctionType_params_List";
  out << " .id=\"SO" << static_cast<void const*>(&params) << "\">\n";
  ++depth;
  return true;
}

void ToXMLTypeVisitor::postvisitFuncParamsList(SObjList<Variable> &params) {
  --depth;
  printIndentation();
  out << "</FunctionType_params_List>\n";
}

bool ToXMLTypeVisitor::visitVariable(Variable *var) {
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
  xmlPrintPointer(out, "ND", var->value);
  out << "\"\n";
  // FIX: this is AST so we should make sure it gets printed out

  // this value is nullable
  if (var->defaultParamType) {
    printIndentation();
    out << "type=\"TY" << static_cast<void const*>(var->defaultParamType) << "\">\n";
  }

  if (var->funcDefn) {
    printIndentation();
    out << "funcDefn=\"";
    xmlPrintPointer(out, "ND", var->funcDefn);
    out << "\"";
    // FIX: this is AST so we should make sure it gets printed out
  }

//    OverloadSet *overload;  // (nullable serf)
//    I don't think we need to serialize this because we are done with
//    overloading after typechecking.  Will have to eventually be done if
//    an analysis wants to analyze uninstantiate templates.

//    Scope *scope;           // (nullable serf)
//    FIX: I think we do need this.

//    // bits 0-7: result of 'getAccess()'
//    // bits 8-15: result of 'getScopeKind()'
//    // bits 16-31: result of 'getParameterOrdinal()'
//    unsigned intData;
//    Ugh.  Break into 3 parts eventually, but for now serialize as an int.
  printIndentation();
  // FIX: split this up into 3
  out << "intData=\"" << toXml_intData(var->intData) << "\"\n";

  if (var->usingAlias_or_parameterizedEntity) {
    printIndentation();
    out << "type=\"TY" << static_cast<void const*>(var->usingAlias_or_parameterizedEntity)
        << "\">\n";
  }

//    TemplateInfo *templInfo;      // (owner)
//    FIX: Ugh.

  // FIX: remove this when we do the above attributes
  printIndentation();
  cout << ">\n";

  // **** subtags

  // this value is nullable
  if (var->defaultParamType) {
    // This is not visited by default by the type visitor, so we not
    // only have to print the id we have to print the tree.
    var->defaultParamType->traverse(*this);
  }

  if (var->usingAlias_or_parameterizedEntity) {
    // This is not visited by default by the type visitor, so we not
    // only have to print the id we have to print the tree.
    var->usingAlias_or_parameterizedEntity->traverse(*this);
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
  out << "typedefVar=\"TY" << static_cast<void const*>(nat->typedefVar) << "\">\n";

  printIndentation();
  out << "access=\"" << toXml(nat->access) << "\"\n";
}

void ToXMLTypeVisitor::toXml_NamedAtomicType_subtags(NamedAtomicType *nat) {
  // This is not visited by default by the type visitor, so we not
  // only have to print the id we have to print the tree.
  nat->typedefVar->traverse(*this);
}

bool ToXMLTypeVisitor::visitAtomicType(AtomicType *obj) {
  if (printedAtomicTypes.contains(obj)) return false;
  printIndentation();

  switch(obj->getTag()) {
  default: xfailure("illegal tag");
  case AtomicType::T_SIMPLE: {
    SimpleType *simple = obj->asSimpleType();
    out << "<SimpleType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;
    printIndentation();
    out << "type=\"" << toXml(simple->type) << "\">\n";
    break;
  }

  case AtomicType::T_COMPOUND: {
    CompoundType *cpd = obj->asCompoundType();
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

    //    // classes from which this one inherits; 'const' so you have to
    //    // use 'addBaseClass', but public to make traversal easy
    //    const ObjList<BaseClass> bases;
    printIndentation();
    out << "bases=\"TY" << static_cast<void const*>(&(cpd->bases)) << "\"\n";

    //    // collected virtual base class subobjects
    //    ObjList<BaseClassSubobj> virtualBases;
    printIndentation();
    out << "virtualBases=\"TY" << static_cast<void const*>(&(cpd->virtualBases)) << "\"\n";

    //    // this is the root of the subobject hierarchy diagram
    //    // invariant: subobj.ct == this
    //    BaseClassSubobj subobj;
    printIndentation();
    out << "subobj=\"TY" << static_cast<void const*>(&(cpd->subobj)) << "\"\n";

    //    // list of all conversion operators this class has, including
    //    // those that have been inherited but not then hidden
    //    SObjList<Variable> conversionOperators;
    printIndentation();
    out << "conversionOperators=\"TY" << static_cast<void const*>(&(cpd->conversionOperators))
        << "\"\n";

    printIndentation();
    out << "instName=" << quoted(cpd->instName) << "\n";

    //    // AST node that describes this class; used for implementing
    //    // templates (AST pointer)
    //    // dsw: used for other purposes also
    //    TS_classSpec *syntax;               // (nullable serf)
    if (cpd->syntax) {
      printIndentation();
      out << "syntax=\"";
      xmlPrintPointer(out, "ND", cpd->syntax);
      out << "\"";
      // FIX: this is AST so we should make sure it gets printed out
    }

    //    // template parameter scope that is consulted for lookups after
    //    // this scope and its base classes; this changes over time as
    //    // the class is added to and removed from the scope stack; it
    //    // is NULL whenever it is not on the scope stack
    //    Scope *parameterizingScope;         // (nullable serf)
    if (cpd->parameterizingScope) {
      printIndentation();
      out << "parameterizingScope=\"";
      xmlPrintPointer(out, "TY", cpd->parameterizingScope);
      out << "\"";
    }

    printIndentation();
    out << "selfType=\"TY" << static_cast<void const*>(cpd->selfType) << "\">\n";

    // **** subtags

    toXml_NamedAtomicType_subtags(cpd);
    toXml_Scope_subtags(cpd);

    out << "<CompoundType_dataMembers_List";
    out << " .id=\"SO" << static_cast<void const*>(&(cpd->dataMembers)) << "\">\n";
    ++depth;
    SFOREACH_OBJLIST_NC(Variable, cpd->dataMembers, iter) {
      Variable *var = iter.data();
      // The usual traversal rountine will not go down into here, so
      // we have to.
      var->traverse(*this);
    }
    --depth;
    printIndentation();
    out << "</CompoundType_dataMembers_List>\n";

    out << "<CompoundType_bases_List";
    out << " .id=\"OJ" << static_cast<void const*>(&(cpd->bases)) << "\">\n";
    ++depth;
    FOREACH_OBJLIST_NC(BaseClass, const_cast<ObjList<BaseClass>&>(cpd->bases), iter) {
      BaseClass *base = iter.data();
      // The usual traversal rountine will not go down into here, so
      // we have to.
      base->traverse(*this);
    }
    --depth;
    printIndentation();
    out << "</CompoundType_bases_List>\n";

    out << "<CompoundType_virtualBases_List";
    out << " .id=\"OJ" << static_cast<void const*>(&(cpd->virtualBases)) << "\">\n";
    ++depth;
    FOREACH_OBJLIST_NC(BaseClassSubobj,
                       const_cast<ObjList<BaseClassSubobj>&>(cpd->virtualBases),
                       iter) {
      BaseClassSubobj *baseSubobj = iter.data();
      // The usual traversal rountine will not go down into here, so
      // we have to.
      baseSubobj->traverse(*this);
    }
    --depth;
    printIndentation();
    out << "</CompoundType_virtualBases_List>\n";

    cpd->subobj.traverse(*this);

    out << "<CompoundType_conversionOperators_List";
    out << " .id=\"SO" << static_cast<void const*>(&(cpd->conversionOperators)) << "\">\n";
    ++depth;
    SFOREACH_OBJLIST_NC(Variable, cpd->conversionOperators, iter) {
      Variable *var = iter.data();
      // The usual traversal rountine will not go down into here, so
      // we have to.
      var->traverse(*this);
    }
    --depth;
    printIndentation();
    out << "</CompoundType_conversionOperators_List>\n";

    if (cpd->parameterizingScope) {
      cpd->parameterizingScope->traverse(*this);
    }

    break;
  }

  case AtomicType::T_ENUM: {
    EnumType *e = obj->asEnumType();
    out << "<EnumType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;
    toXml_NamedAtomicType_properties(e);
//      printIndentation();
    //    StringObjDict<Value> valueIndex;    // values in this enumeration

    printIndentation();
    out << "nextValue=\"" << e->nextValue << "\">";

    // **** subtags

    toXml_NamedAtomicType_subtags(e);
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

  printedAtomicTypes.add(obj);
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
//    out << "params=\"SO" << static_cast<void const*>(&(scope->templateParams)) << "\"\n";

  printIndentation();
  out << "curCompound=\"TY" << static_cast<void const*>(scope->curCompound) << "\"\n";
}

void ToXMLTypeVisitor::toXml_Scope_subtags(Scope *scope)
{
}

bool ToXMLTypeVisitor::visitScope(Scope *scope)
{
  out << "<Scope";
  out << " .id=\"TY" << static_cast<void const*>(scope) << "\"\n";
  ++depth;
  toXml_Scope_properties(scope);
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

bool ToXMLTypeVisitor::visitScope_variables_Map(StringRefMap<Variable> &variables)
{
  printIndentation();
  out << "<Scope_variables_Map";
  out << " .id=\"SM" << static_cast<void const*>(&variables) << "\">\n";
  ++depth;
  return true;
}
void ToXMLTypeVisitor::visitScope_variables_Map_entry(StringRef name, Variable *var)
{
  printIndentation();
  out << "<__NameValuePair";
  out << " name=" << quoted(name) << "";
  out << " var=\"TY" << static_cast<void const*>(var) << "\">\n";
}
void ToXMLTypeVisitor::postvisitScope_variables_Map(StringRefMap<Variable> &variables)
{
  --depth;
  printIndentation();
  out << "</Scope_variables_Map>\n";
}

bool ToXMLTypeVisitor::visitScope_typeTags_Map(StringRefMap<Variable> &typeTags)
{
  printIndentation();
  out << "<Scope_typeTags_Map";
  out << " .id=\"SM" << static_cast<void const*>(&typeTags) << "\">\n";
  ++depth;
  return true;
}
void ToXMLTypeVisitor::visitScope_typeTags_Map_entry(StringRef name, Variable *var)
{
  printIndentation();
  out << "<__NameValuePair";
  out << " name=" << quoted(name) << "";
  out << " var=\"TY" << static_cast<void const*>(var) << "\">\n";
}
void ToXMLTypeVisitor::postvisitScope_typeTags_Map(StringRefMap<Variable> &typeTags)
{
  --depth;
  printIndentation();
  out << "</Scope_typeTags_Map>\n";
}

bool ToXMLTypeVisitor::visitBaseClass(BaseClass *bc)
{
  out << "<BaseClass";
  out << " .id=\"TY" << static_cast<void const*>(bc) << "\"\n";
  ++depth;

  printIndentation();
  out << "ct=\"TY" << static_cast<void const*>(bc->ct) << "\"\n";

  printIndentation();
  out << "access=\"" << toXml(bc->access) << "\"\n";

  printIndentation();
  out << "isVirtual=\"" << toXml_bool(bc->isVirtual) << "\">\n";

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
  out << "<BaseClassSubobj";
  out << " .id=\"TY" << static_cast<void const*>(bc) << "\"\n";
  ++depth;

  printIndentation();
  out << "parents=\"SO" << static_cast<void const*>(&(bc->parents)) << "\">\n";

  return true;
}
void ToXMLTypeVisitor::postvisitBaseClassSubobj(BaseClassSubobj *bc)
{
  --depth;
  printIndentation();
  out << "</BaseClassSubobj>\n";
}

bool ToXMLTypeVisitor::visitBaseClassSubobjParents(SObjList<BaseClassSubobj> &parents)
{
  printIndentation();
  out << "<BaseClassSubobj_Parents";
  out << " .id=\"SO" << static_cast<void const*>(&parents) << "\">\n";
  ++depth;
  return true;
}
void ToXMLTypeVisitor::postvisitBaseClassSubobjParents(SObjList<BaseClassSubobj> &parents)
{
  --depth;
  printIndentation();
  out << "</BaseClassSubobj_Parents>\n";
}

// -------------------- ReadXml_Type -------------------

KindCategory ReadXml_Type::kind2kindCat(int kind) {
  switch(kind) {
  default: xfailure("illegal token kind");
  // Types
  case XTOK_CVAtomicType: return KC_Node;
  case XTOK_PointerType: return KC_Node;
  case XTOK_ReferenceType: return KC_Node;
  case XTOK_FunctionType: return KC_Node;
  case XTOK_ArrayType: return KC_Node;
  case XTOK_PointerToMemberType: return KC_Node;
  // AtomicTypes
  case XTOK_SimpleType: return KC_Node;
  case XTOK_CompoundType: return KC_Node;
  case XTOK_EnumType: return KC_Node;
  case XTOK_TypeVariable: return KC_Node;
  case XTOK_PseudoInstantiation: return KC_Node;
  case XTOK_DependentQType: return KC_Node;
  // ObjList
  case XTOK_CompoundType_bases_List: return KC_ObjList;
  case XTOK_CompoundType_virtualBases_List: return KC_ObjList;
  // SObjList
  case XTOK_FunctionType_params_List: return KC_SObjList;
  case XTOK_CompoundType_dataMembers_List: return KC_SObjList;
  case XTOK_CompoundType_conversionOperators_List: return KC_SObjList;
  // StringRefMap
  case XTOK_Scope_variables_Map: return KC_StringRefMap;
  case XTOK_Scope_typeTags_Map: return KC_StringRefMap;
  }
}

void *ReadXml_Type::prepend2FakeList(void *list, int listKind, void *datum, int datumKind) {
  switch(listKind) {
  default: xfailure("attempt to prepend to a non-FakeList token kind");
//    case XTOK_FakeList_MemberInit:
//      if (!datumKind == XTOK_MemberInit) {
//        userError("can't put that onto a FakeList of MemberInit");
//      }
//      return ((FakeList<MemberInit>*)list)->prepend((MemberInit*)datum);
//      break;
  }
}

void *ReadXml_Type::reverseFakeList(void *list, int listKind) {
  switch(listKind) {
  default: xfailure("attempt to reverse a non-FakeList token kind");
//    case XTOK_FakeList_MemberInit:
//      return ((FakeList<MemberInit>*)list)->reverse();
//      break;
  }
}

void ReadXml_Type::append2ASTList(void *list, int listKind, void *datum, int datumKind) {
  switch(listKind) {
  default: xfailure("attempt to append to a non-ASTList token kind");
//    case XTOK_ASTList_TopForm:
//      if (!datumKind == XTOK_TopForm) {
//        userError("can't put that onto an ASTList of TopForm");
//      }
//      ((ASTList<TopForm>*)list)->append((TopForm*)datum);
//      break;
  }
}

void ReadXml_Type::prepend2ObjList(void *list, int listKind, void *datum, int datumKind) {
  switch(listKind) {
  default: xfailure("attempt to prepend to a non-ObjList token kind");
//    case XTOK_FakeList_MemberInit:
//      if (!datumKind == XTOK_MemberInit) {
//        userError("can't put that onto a FakeList of MemberInit");
//      }
//      return ((FakeList<MemberInit>*)list)->prepend((MemberInit*)datum);
//      break;
  }
}

void ReadXml_Type::reverseObjList(void *list, int listKind) {
  switch(listKind) {
  default: xfailure("attempt to reverse a non-ObjList token kind");
//    case XTOK_FakeList_MemberInit:
//      return ((FakeList<MemberInit>*)list)->reverse();
//      break;
  }
}

void ReadXml_Type::prepend2SObjList(void *list, int listKind, void *datum, int datumKind) {
  switch(listKind) {
  default: xfailure("attempt to prepend to a non-SObjList token kind");
//    case XTOK_FakeList_MemberInit:
//      if (!datumKind == XTOK_MemberInit) {
//        userError("can't put that onto a FakeList of MemberInit");
//      }
//      return ((FakeList<MemberInit>*)list)->prepend((MemberInit*)datum);
//      break;
  }
}

void ReadXml_Type::reverseSObjList(void *list, int listKind) {
  switch(listKind) {
  default: xfailure("attempt to reverse a non-SObjList token kind");
//    case XTOK_FakeList_MemberInit:
//      return ((FakeList<MemberInit>*)list)->reverse();
//      break;
  }
}

bool ReadXml_Type::ctorNodeFromTag(int tag, void *&topTemp) {
  switch(tag) {
  default: userError("unexpected token while looking for an open tag name");
  case 0: userError("unexpected file termination while looking for an open tag name");
  case XTOK_SLASH:
    return true;
    break;

  // **** Types
  case XTOK_CVAtomicType:
    topTemp = tFac.makeCVAtomicType((AtomicType*)0, (CVFlags)0);
    break;

  case XTOK_PointerType:
    topTemp = tFac.makePointerType((CVFlags)0, (Type*)0);
    break;

  case XTOK_ReferenceType:
    topTemp = tFac.makeReferenceType((Type*)0);
    break;

  case XTOK_FunctionType:
    topTemp = tFac.makeFunctionType((Type*)0);
    break;

  case XTOK_ArrayType:
    topTemp = tFac.makeArrayType((Type*)0, (int)0);
    break;

  case XTOK_PointerToMemberType:
    topTemp = tFac.makePointerToMemberType((NamedAtomicType*)0, (CVFlags)0, (Type*)0);
    break;

  // **** Atomic Types
  case XTOK_SimpleType:
    // NOTE: this really should go through the SimpleTyp::fixed array
    topTemp = new SimpleType((SimpleTypeId)0);
    break;

  case XTOK_CompoundType:
    topTemp = tFac.makeCompoundType((CompoundType::Keyword)0, (StringRef)0);
    break;

  case XTOK_EnumType:
    topTemp = new EnumType((StringRef)0);
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

  // **** Container types
  case XTOK_FunctionType_params_List:
    topTemp = new SObjList<Variable>();
    break;

  case XTOK_CompoundType_dataMembers_List:
    topTemp = new SObjList<Variable>();
    break;

  case XTOK_CompoundType_bases_List:
    topTemp = new ObjList<BaseClass>();
    break;

  case XTOK_CompoundType_virtualBases_List:
    topTemp = new ObjList<BaseClassSubobj>();
    break;

  case XTOK_CompoundType_conversionOperators_List:
    topTemp = new SObjList<Variable>();
    break;

  case XTOK_Scope_variables_Map:
    topTemp = new StringRefMap<Variable>();
    break;

  case XTOK_Scope_typeTags_Map:
    topTemp = new StringRefMap<Variable>();
    break;

//  #include "astxml_parse1_2ctrc.gen.cc"
  }
  return false;
}

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

  case XTOK_TypeVariable:
    registerAttr_TypeVariable((TypeVariable*)target, attr, yytext0);
    break;

  case XTOK_PseudoInstantiation:
    registerAttr_PseudoInstantiation((PseudoInstantiation*)target, attr, yytext0);
    break;

  case XTOK_DependentQType:
    registerAttr_DependentQType((DependentQType*)target, attr, yytext0);
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
    linkSat.unsatLinks.append(new UnsatLink((void**) &(obj->atomic), parseQuotedString(strValue)));
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
    linkSat.unsatLinks.append(new UnsatLink((void**) &(obj->atType), parseQuotedString(strValue)));
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
    linkSat.unsatLinks.append(new UnsatLink((void**) &(obj->atType), parseQuotedString(strValue)));
    break;
  }
}

void ReadXml_Type::registerAttr_FunctionType
  (FunctionType *obj, int attr, char const *strValue) {
  switch(attr) {
  default:
    userError("illegal attribute for a FunctionType");
    break;
  case XTOK_retType:
    linkSat.unsatLinks.append(new UnsatLink((void**) &(obj->retType), parseQuotedString(strValue)));
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
    linkSat.unsatLinks.append(new UnsatLink((void**) &(obj->eltType), parseQuotedString(strValue)));
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
  case XTOK_cv:
    fromXml(obj->cv, parseQuotedString(strValue));
    break;
  case XTOK_atType:
    linkSat.unsatLinks.append(new UnsatLink((void**) &(obj->atType), parseQuotedString(strValue)));
    break;
  case XTOK_inClassNAT:
    linkSat.unsatLinks.append(new UnsatLink((void**) &(obj->inClassNAT), parseQuotedString(strValue)));
    break;
  }
}


bool ReadXml_Type::registerAttr_NamedAtomicType
  (NamedAtomicType *obj, int attr, char const *strValue) {
  switch(attr) {
  default:
    return false;               // we didn't find it
    break;
  case XTOK_name:
    obj->name = strTable(parseQuotedString(strValue));
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
    fromXml(const_cast<SimpleTypeId&>(obj->type), parseQuotedString(strValue));
    break;
  }
}

void ReadXml_Type::registerAttr_CompoundType
  (CompoundType *obj, int attr, char const *strValue) {
  if (registerAttr_NamedAtomicType(obj, attr, strValue)) return;
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
  case XTOK_instName:
    obj->instName = strTable(parseQuotedString(strValue));
    break;
  case XTOK_selfType:
    linkSat.unsatLinks.append(new UnsatLink((void**) &(obj->selfType), parseQuotedString(strValue)));
    break;
  }
}

void ReadXml_Type::registerAttr_EnumType
  (EnumType *obj, int attr, char const *strValue) {
  if (registerAttr_NamedAtomicType(obj, attr, strValue)) return;
  switch(attr) {
  default:
    userError("illegal attribute for a EnumType");
    break;
  case XTOK_nextValue:
    obj->nextValue = atoi(parseQuotedString(strValue));
    break;
  }
}

void ReadXml_Type::registerAttr_TypeVariable
  (TypeVariable *obj, int attr, char const *strValue) {
  if (registerAttr_NamedAtomicType(obj, attr, strValue)) return;
  switch(attr) {
  default:
    userError("illegal attribute for a TypeVariable");
    break;
  }
}

void ReadXml_Type::registerAttr_PseudoInstantiation
  (PseudoInstantiation *obj, int attr, char const *strValue) {
  if (registerAttr_NamedAtomicType(obj, attr, strValue)) return;
  switch(attr) {
  default:
    userError("illegal attribute for a PsuedoInstantiation");
    break;
  }
}

void ReadXml_Type::registerAttr_DependentQType
  (DependentQType *obj, int attr, char const *strValue) {
  if (registerAttr_NamedAtomicType(obj, attr, strValue)) return;
  switch(attr) {
  default:
    userError("illegal attribute for a DependentQType");
    break;
  }
}
