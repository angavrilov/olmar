// cc_type_xml.cc            see license.txt for copyright and terms of use

#include "cc_type_xml.h"        // this module

#include "strutil.h"            // parseQuotedString
#include "astxml_lexer.h"       // AstXmlLexer


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
    out << "atomic=\"" << static_cast<void const*>(atom->atomic) << "\"\n";
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

    // FIX: skip this for now
    // SObjList<Variable> params;

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

void ToXMLTypeVisitor::toXml_NamedAtomicType(NamedAtomicType *nat) {
  printIndentation();
  out << "name=" << quoted(nat->name) << "\n";
//    Variable *typedefVar;       // (owner) implicit typedef variable
  printIndentation();
  out << "access=\"" << toXml(nat->access) << "\"\n";
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
    toXml_NamedAtomicType(cpd);
    printIndentation();
    out << "forward=\"" << toXml_bool(cpd->forward) << "\"\n";
    printIndentation();
    out << "keyword=\"" << toXml(cpd->keyword) << "\"\n";
    //    SObjList<Variable> dataMembers;

    //    // classes from which this one inherits; 'const' so you have to
    //    // use 'addBaseClass', but public to make traversal easy
    //    const ObjList<BaseClass> bases;

    //    // collected virtual base class subobjects
    //    ObjList<BaseClassSubobj> virtualBases;

    //    // this is the root of the subobject hierarchy diagram
    //    // invariant: subobj.ct == this
    //    BaseClassSubobj subobj;

    //    // list of all conversion operators this class has, including
    //    // those that have been inherited but not then hidden
    //    SObjList<Variable> conversionOperators;

    printIndentation();
    out << "instName=" << quoted(cpd->instName) << "\n";

    //    // AST node that describes this class; used for implementing
    //    // templates (AST pointer)
    //    // dsw: used for other purposes also
    //    TS_classSpec *syntax;               // (nullable serf)

    //    // template parameter scope that is consulted for lookups after
    //    // this scope and its base classes; this changes over time as
    //    // the class is added to and removed from the scope stack; it
    //    // is NULL whenever it is not on the scope stack
    //    Scope *parameterizingScope;         // (nullable serf)

    printIndentation();
    out << "selfType=\"TY" << static_cast<void const*>(cpd->selfType) << "\">\n";
    break;
  }

  case AtomicType::T_ENUM: {
    EnumType *e = obj->asEnumType();
    out << "<EnumType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;
    toXml_NamedAtomicType(e);
//      printIndentation();
    //    StringObjDict<Value> valueIndex;    // values in this enumeration

    printIndentation();
    out << "nextValue=\"" << e->nextValue << "\">";
    break;
  }

  case AtomicType::T_TYPEVAR: {
    TypeVariable *tvar = obj->asTypeVariable();
    out << "<TypeVariable";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;
    toXml_NamedAtomicType(tvar);
    break;
  }

  case AtomicType::T_PSEUDOINSTANTIATION: {
    PseudoInstantiation *pseudo = obj->asPseudoInstantiation();
    out << "<PseudoInstantiation";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;
    toXml_NamedAtomicType(pseudo);

//    CompoundType *primary;

//    // the arguments, some of which contain type variables
//    ObjList<STemplateArgument> args;
    break;
  }

  case AtomicType::T_DEPENDENTQTYPE: {
    DependentQType *dep = obj->asDependentQType();
    out << "<DependentQType";
    out << " .id=\"TY" << static_cast<void const*>(obj) << "\"\n";
    ++depth;
    toXml_NamedAtomicType(dep);

//    AtomicType *first;            // (serf) TypeVariable or PseudoInstantiation

//    // After the first component comes whatever name components followed
//    // in the original syntax.  All template arguments have been
//    // tcheck'd.
//    PQName *rest;
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

//  bool ToXMLToXMLTypeVisitor::preVisitVariable(Variable *var) {
//    xassert(var);
//    out << "<Variable";

//    if (var->name) {
//      // FIX: it is not clear that we want to ask for the fully
//      // qualified name of variables that aren't linker visible, as it
//      // is not well defined.
//      out << " fqname='" << var->fullyQualifiedName() << "'";
//    }
//    // FIX: do I want the mangled name?

//    out << ">";
    
//    return true;
//  }

//  void ToXMLToXMLTypeVisitor::postVisitVariable(Variable *var) {
//    out << "</Variable>";

//  }


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
