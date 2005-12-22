// xml_type_writer.cc            see license.txt for copyright and terms of use

#include "xml_type_writer.h"    // this module
#include "variable.h"           // Variable
#include "cc_flags.h"           // toXml(DeclFlags &out, rostring str)
#include "asthelp.h"            // xmlPrintPointer
#include "xmlhelp.h"            // toXml_int() etc.
#include "strutil.h"            // DelimStr
#include "cc_ast.h"             // AST nodes only for AST sub-traversals
#include "strtokp.h"            // StrtokParse


// toXml for enums

char const *toXml(CompoundType::Keyword id) {
  switch(id) {
  default: xfailure("bad enum"); break;
    PRINTENUM(CompoundType::K_STRUCT);
    PRINTENUM(CompoundType::K_CLASS);
    PRINTENUM(CompoundType::K_UNION);
  }
}

string toXml(FunctionFlags id) {
  if (id == FF_NONE) return "FF_NONE";
  DelimStr b('|');
  PRINTFLAG(FF_METHOD);
  PRINTFLAG(FF_VARARGS);
  PRINTFLAG(FF_CONVERSION);
  PRINTFLAG(FF_CTOR);
  PRINTFLAG(FF_DTOR);
  PRINTFLAG(FF_BUILTINOP);
  PRINTFLAG(FF_NO_PARAM_INFO);
  PRINTFLAG(FF_DEFAULT_ALLOC);
  PRINTFLAG(FF_KANDR_DEFN);
  return b.sb;
}

char const *toXml(ScopeKind id) {
  switch(id) {
  default: xfailure("bad enum"); break;
  PRINTENUM(SK_UNKNOWN);
  PRINTENUM(SK_GLOBAL);
  PRINTENUM(SK_PARAMETER);
  PRINTENUM(SK_FUNCTION);
  PRINTENUM(SK_CLASS);
  PRINTENUM(SK_TEMPLATE_PARAMS);
  PRINTENUM(SK_TEMPLATE_ARGS);
  PRINTENUM(SK_NAMESPACE);
  }
}

char const *toXml(STemplateArgument::Kind id) {
  switch(id) {
  default: xfailure("bad enum"); break;
  PRINTENUM(STemplateArgument::STA_NONE);
  PRINTENUM(STemplateArgument::STA_TYPE);
  PRINTENUM(STemplateArgument::STA_INT);
  PRINTENUM(STemplateArgument::STA_ENUMERATOR);
  PRINTENUM(STemplateArgument::STA_REFERENCE);
  PRINTENUM(STemplateArgument::STA_POINTER);
  PRINTENUM(STemplateArgument::STA_MEMBER);
  PRINTENUM(STemplateArgument::STA_DEPEXPR);
  PRINTENUM(STemplateArgument::STA_TEMPLATE);
  PRINTENUM(STemplateArgument::STA_ATOMIC);
  }
}


// printing of types is idempotent
SObjSet<void const *> printedSetTY;
SObjSet<void const *> printedSetBC;
SObjSet<void const *> printedSetOL;
SObjSet<void const *> printedSetNM;

identity_defn(TY, Type)
identity_defn(TY, CompoundType)
identity_defn(TY, FunctionType::ExnSpec)
identity_defn(TY, EnumType::Value)
identity_defn(BC, BaseClass)
identity_defn(TY, Variable)
identity_defn(TY, OverloadSet)
identity_defn(TY, STemplateArgument)
identity_defn(TY, TemplateInfo)
identity_defn(TY, InheritedTemplateParams)
identityTempl_defn(OL, ObjList<T>)
identityTempl_defn(OL, SObjList<T>)
identityTempl_defn(NM, StringRefMap<T>)
identityTempl_defn(NM, StringObjDict<T>)

#define identityCpdSuper(PREFIX, NAME) \
char const *idPrefix(NAME const * const obj) { \
  if (CompoundType const * const cpd = dynamic_cast<CompoundType const * const>(obj)) { \
    return idPrefix(cpd); \
  } \
  return #PREFIX; \
} \
void const *addr(NAME const * const obj) { \
  if (CompoundType const * const cpd = dynamic_cast<CompoundType const * const>(obj)) { \
    return addr(cpd); \
  } \
  return reinterpret_cast<void const *>(obj); \
} \
bool printed(NAME const * const obj) { \
  if (CompoundType const * const cpd = dynamic_cast<CompoundType const * const>(obj)) { \
    return printed(cpd); \
  } \
  if (printedSet ##PREFIX.contains(obj)) return true; \
  printedSet ##PREFIX.add(obj); \
  return false; \
}

// AtomicType and Scope are special because they both can be a
// CompoundType sometimes and so have to change their notion of
// identity when they do
identityCpdSuper(TY, AtomicType)
identityCpdSuper(TY, Scope)

XmlTypeWriter::XmlTypeWriter
  (ASTVisitor *astVisitor0, ostream &out0, int &depth0, bool indent0)
  : XmlWriter(out0, depth0, indent0)
  , astVisitor(astVisitor0)
{}

// This one occurs in the AST, so it has to have its own first-class
// method.
void XmlTypeWriter::toXml(ObjList<STemplateArgument> *list) {
  travObjList_standalone(*list, PseudoInstantiation, args, STemplateArgument);
}

void XmlTypeWriter::toXml(Type *t) {
  // idempotency
  if (printed(t)) return;

  switch(t->getTag()) {
  default: xfailure("illegal tag");

  case Type::T_ATOMIC: {
    CVAtomicType *atom = t->asCVAtomicType();
    openTag(CVAtomicType, atom);
    // **** attributes
    printPtr(atom, atomic);
    printXml(cv, atom->cv);
    tagEnd;
    // **** subtags
    trav(atom->atomic);
    break;
  }

  case Type::T_POINTER: {
    PointerType *ptr = t->asPointerType();
    openTag(PointerType, ptr);
    // **** attributes
    printXml(cv, ptr->cv);
    printPtr(ptr, atType);
    tagEnd;
    // **** subtags
    trav(ptr->atType);
    break;
  }

  case Type::T_REFERENCE: {
    ReferenceType *ref = t->asReferenceType();
    openTag(ReferenceType, ref);
    // **** attributes
    printPtr(ref, atType);
    tagEnd;
    // **** subtags
    trav(ref->atType);
    break;
  }

  case Type::T_FUNCTION: {
    FunctionType *func = t->asFunctionType();
    openTag(FunctionType, func);
    // **** attributes
    printXml(flags, func->flags);
    printPtr(func, retType);
    printEmbed(func, params);
    printPtr(func, exnSpec);
    tagEnd;
    // **** subtags
    trav(func->retType);
    travObjList_S(func, FunctionType, params, Variable);
    // exnSpec
    if (func->exnSpec) {
      toXml_FunctionType_ExnSpec(func->exnSpec);
    }
    break;
  }

  case Type::T_ARRAY: {
    ArrayType *arr = t->asArrayType();
    openTag(ArrayType, arr);
    // **** attributes
    printPtr(arr, eltType);
    printXml_int(size, arr->size);
    tagEnd;
    // **** subtags
    trav(arr->eltType);
    break;
  }

  case Type::T_POINTERTOMEMBER: {
    PointerToMemberType *ptm = t->asPointerToMemberType();
    openTag(PointerToMemberType, ptm);
    // **** attributes
    printPtr(ptm, inClassNAT);
    printXml(cv, ptm->cv);
    printPtr(ptm, atType);
    tagEnd;
    // **** subtags
    trav(ptm->inClassNAT);
    trav(ptm->atType);
    break;
  }

  }
}

void XmlTypeWriter::toXml(AtomicType *atom) {
  // idempotency done in each sub-type as it is not done for
  // CompoundType here.
  switch(atom->getTag()) {
  default: xfailure("illegal tag");

  case AtomicType::T_SIMPLE: {
    // idempotency
    if (printed(atom)) return;
    SimpleType *simple = atom->asSimpleType();
    openTag(SimpleType, simple);
    // **** attributes
    printXml(type, simple->type);
    tagEnd;
    break;
  }

  case AtomicType::T_COMPOUND: {
    // NO!  Do NOT do this here:
//      // idempotency
//      if (printed(atom)) return;
    CompoundType *cpd = atom->asCompoundType();
    toXml(cpd);
    break;
  }

  case AtomicType::T_ENUM: {
    // idempotency
    if (printed(atom)) return;
    EnumType *e = atom->asEnumType();
    openTag(EnumType, e);
    // **** attributes
    // * superclasses
    toXml_NamedAtomicType_properties(e);
    // * members
    printEmbed(e, valueIndex);
    printXml_int(nextValue, e->nextValue);
    tagEnd;
    // **** subtags
    // * superclasses
    toXml_NamedAtomicType_subtags(e);
    // * members
    // valueIndex
    if (!printed(&e->valueIndex)) {
      openTagWhole(NameMap_EnumType_valueIndex, &e->valueIndex);
      if (sortNameMapDomainWhenSerializing) {
        for(StringObjDict<EnumType::Value>::SortedKeyIter iter(e->valueIndex);
            !iter.isDone(); iter.next()) {
          char const *name = iter.key();
          // dsw: do you know how bad it gets if I don't put a
          // const-cast here?
          EnumType::Value *eValue = const_cast<EnumType::Value*>(iter.value());
          openTag_NameMap_Item(name, eValue);
          toXml_EnumType_Value(eValue);
        }
      } else {
        for(StringObjDict<EnumType::Value>::Iter iter(e->valueIndex);
            !iter.isDone(); iter.next()) {
          rostring name = iter.key();
          // dsw: do you know how bad it gets if I don't put a
          // const-cast here?
          EnumType::Value *eValue = const_cast<EnumType::Value*>(iter.value());
          openTag_NameMap_Item(name, eValue);
          toXml_EnumType_Value(eValue);
        }
      }
    }
    break;
  }

  case AtomicType::T_TYPEVAR: {
    // idempotency
    if (printed(atom)) return;
    TypeVariable *tvar = atom->asTypeVariable();
    openTag(TypeVariable, tvar);
    // **** attributes
    // * superclasses
    toXml_NamedAtomicType_properties(tvar);
    tagEnd;
    // **** subtags
    // * superclasses
    toXml_NamedAtomicType_subtags(tvar);
    break;
  }

  case AtomicType::T_PSEUDOINSTANTIATION: {
    // idempotency
    if (printed(atom)) return;
    PseudoInstantiation *pseudo = atom->asPseudoInstantiation();
    openTag(PseudoInstantiation, pseudo);
    // **** attributes
    // * superclasses
    toXml_NamedAtomicType_properties(pseudo);
    // * members
    printPtr(pseudo, primary);
    printEmbed(pseudo, args);
    tagEnd;
    // **** subtags
    // * superclasses
    toXml_NamedAtomicType_subtags(pseudo);
    // * members
    trav(pseudo->primary);
    travObjList(pseudo, PseudoInstantiation, args, STemplateArgument);
    break;
  }

  case AtomicType::T_DEPENDENTQTYPE: {
    // idempotency
    if (printed(atom)) return;
    DependentQType *dep = atom->asDependentQType();
    openTag(DependentQType, dep);
    // **** attributes
    // * superclasses
    toXml_NamedAtomicType_properties(dep);
    // * members
    printPtr(dep, first);
    printPtrAST(dep, rest);
    tagEnd;
    // **** subtags
    // * superclasses
    toXml_NamedAtomicType_subtags(dep);
    // * members
    trav(dep->first);
    travAST(dep->rest);
    break;
  }

  }
}

void XmlTypeWriter::toXml(CompoundType *cpd) {
  // idempotency
  if (printed(cpd)) return;
  openTag(CompoundType, cpd);
  // **** attributes
  // * superclasses
  toXml_NamedAtomicType_properties(cpd);
  toXml_Scope_properties(cpd);
  // * members
  printXml_bool(forward, cpd->forward);
  printXml(keyword, cpd->keyword);
  printEmbed(cpd, dataMembers);
  printEmbed(cpd, bases);
  printEmbed(cpd, virtualBases);
  printEmbed(cpd, subobj);
  printEmbed(cpd, conversionOperators);
  printStrRef(instName, cpd->instName);
  printPtrAST(cpd, syntax);
  printPtr(cpd, parameterizingScope);
  printPtr(cpd, selfType);
  tagEnd;
  // **** subtags
  // * superclasses
  toXml_NamedAtomicType_subtags(cpd);
  toXml_Scope_subtags(cpd);
  // * members
  travObjList_S(cpd, CompoundType, dataMembers, Variable);
  travObjList(cpd, CompoundType, bases, BaseClass);
  travObjList(cpd, CompoundType, virtualBases, BaseClassSubobj);
  toXml(&cpd->subobj);          // embedded
  travObjList_S(cpd, CompoundType, conversionOperators, Variable);
  travAST(cpd->syntax);
  trav(cpd->parameterizingScope);
  trav(cpd->selfType);
}

void XmlTypeWriter::toXml_Variable_properties(Variable *var) {
  printXml_SourceLoc(loc, var->loc);
  printStrRef(name, var->name);
  printPtr(var, type);
  printXml(flags, var->flags);
  printPtrAST(var, value);
  printPtr(var, defaultParamType);
  printPtrAST(var, funcDefn);
  printPtr(var, overload);
  printPtr(var, scope);

  // these three fields are an abstraction; here we pretend they are
  // real
  AccessKeyword access = var->getAccess();
  printXml(access, access);
  ScopeKind scopeKind = var->getScopeKind();
  printXml(scopeKind, scopeKind);
  int parameterOrdinal = var->getParameterOrdinal();
  printXml_int(parameterOrdinal, parameterOrdinal);

  printPtr(var, usingAlias_or_parameterizedEntity);
  printPtr(var, templInfo);

  if (var->linkerVisibleName()) {
    newline();
    out << "fullyQualifiedMangledName=" << xmlAttrQuote(var->fullyQualifiedMangledName0());
  }
}

void XmlTypeWriter::toXml_Variable_subtags(Variable *var) {
  trav(var->type);
  travAST(var->value);
  trav(var->defaultParamType);
  travAST(var->funcDefn);
  trav(var->overload);
  trav(var->scope);
  trav(var->usingAlias_or_parameterizedEntity);
  trav(var->templInfo);
}

void XmlTypeWriter::toXml(Variable *var) {
  // idempotency
  if (printed(var)) return;
  openTag(Variable, var);
  // **** attributes
  toXml_Variable_properties(var);
  tagEnd;
  // **** subtags
  toXml_Variable_subtags(var);
}

void XmlTypeWriter::toXml_FunctionType_ExnSpec(void /*FunctionType::ExnSpec*/ *exnSpec0) {
  FunctionType::ExnSpec *exnSpec = static_cast<FunctionType::ExnSpec *>(exnSpec0);
  // idempotency
  if (printed(exnSpec)) return;
  openTag(FunctionType_ExnSpec, exnSpec);
  // **** attributes
  printEmbed(exnSpec, types);
  tagEnd;
  // **** subtags
  travObjList_S(exnSpec, ExnSpec, types, Type);
}

void XmlTypeWriter::toXml_EnumType_Value(void /*EnumType::Value*/ *eValue0) {
  EnumType::Value *eValue = static_cast<EnumType::Value *>(eValue0);
  // idempotency
  if (printed(eValue)) return;
  openTag(EnumType_Value, eValue);
  // **** attributes
  printStrRef(name, eValue->name);
  printPtr(eValue, type);
  printXml_int(value, eValue->value);
  printPtr(eValue, decl);
  tagEnd;
  // **** subtags
  trav(eValue->type);
  trav(eValue->decl);
}

void XmlTypeWriter::toXml_NamedAtomicType_properties(NamedAtomicType *nat) {
  printStrRef(name, nat->name);
  printPtr(nat, typedefVar);
  printXml(access, nat->access);
}

void XmlTypeWriter::toXml_NamedAtomicType_subtags(NamedAtomicType *nat) {
  trav(nat->typedefVar);
}

void XmlTypeWriter::toXml(OverloadSet *oload) {
  // idempotency
  if (printed(oload)) return;
  openTag(OverloadSet, oload);
  // **** attributes
  printEmbed(oload, set);
  tagEnd;
  // **** subtags
  travObjList_S(oload, OverloadSet, set, Variable);
}

void XmlTypeWriter::toXml(BaseClass *bc) {
  // Since BaseClass objects are never manipulated polymorphically,
  // that is, every BaseClass pointer's static type equals its dynamic
  // type, 'bc' cannot actually be a BaseClassSubobj.

  // idempotency
  if (printed(bc)) return;
  openTag(BaseClass, bc);
  // **** attributes
  toXml_BaseClass_properties(bc);
  tagEnd;
  // **** subtags
  toXml_BaseClass_subtags(bc);
}

void XmlTypeWriter::toXml_BaseClass_properties(BaseClass *bc) {
  printPtr(bc, ct);
  printXml(access, bc->access);
  printXml_bool(isVirtual, bc->isVirtual);
}

void XmlTypeWriter::toXml_BaseClass_subtags(BaseClass *bc) {
  trav(bc->ct);
}

void XmlTypeWriter::toXml(BaseClassSubobj *bc) {
  // idempotency
  if (printed(bc)) return;
  openTag(BaseClassSubobj, bc);
  // **** attributes
  // * superclass
  toXml_BaseClass_properties(bc);
  // * members
  printEmbed(bc, parents);
  tagEnd;
  // **** subtags
  // * superclass
  toXml_BaseClass_subtags(bc);
  // * members
  travObjList_S(bc, BaseClassSubobj, parents, BaseClassSubobj);
}

void XmlTypeWriter::toXml(Scope *scope) {
  // are we really a CompoundType?
  if (CompoundType *cpd = dynamic_cast<CompoundType*>(scope)) {
    toXml(cpd);
    return;
  }
  // idempotency
  if (printed(scope)) return;
  openTag(Scope, scope);
  // **** attributes
  toXml_Scope_properties(scope);
  tagEnd;
  // **** subtags
  toXml_Scope_subtags(scope);
}

void XmlTypeWriter::toXml_Scope_properties(Scope *scope) {
  printEmbed(scope, variables);
  printEmbed(scope, typeTags);
  printXml_bool(canAcceptNames, scope->canAcceptNames);
  printPtr(scope, parentScope);
  printXml(scopeKind, scope->scopeKind);
  printPtr(scope, namespaceVar);
  printEmbed(scope, templateParams);
  printPtr(scope, curCompound);
  printXml_SourceLoc(curLoc, scope->curLoc);
}

void XmlTypeWriter::toXml_Scope_subtags(Scope *scope) {
  travPtrMap(scope, Scope, variables, Variable);
  travPtrMap(scope, Scope, typeTags, Variable);
  trav(scope->parentScope);
  trav(scope->namespaceVar);
  travObjList_S(scope, Scope, templateParams, Variable);
  trav(scope->curCompound);
}

void XmlTypeWriter::toXml(STemplateArgument *sta) {
  // idempotency
  if (printed(sta)) return;
  openTag(STemplateArgument, sta);

  // **** attributes
  printXml(kind, sta->kind);

  newline();
  switch(sta->kind) {
  default: xfailure("illegal STemplateArgument kind"); break;

  case STemplateArgument::STA_TYPE:
    printPtrUnion(sta, value.t, t);
    break;

  case STemplateArgument::STA_INT:
    printXml_int(i, sta->value.i);
    break;

  case STemplateArgument::STA_ENUMERATOR:
  case STemplateArgument::STA_REFERENCE:
  case STemplateArgument::STA_POINTER:
  case STemplateArgument::STA_MEMBER:
    printPtrUnion(sta, value.v, v);
    break;

  case STemplateArgument::STA_DEPEXPR:
    printPtrASTUnion(sta, value.e, e);
    break;

  case STemplateArgument::STA_TEMPLATE:
    xfailure("template template arguments not implemented");
    break;

  case STemplateArgument::STA_ATOMIC:
    printPtrUnion(sta, value.at, at);
    break;
  }
  tagEnd;

  // **** subtags

  // NOTE: I don't use the trav() macro here because it would be weird
  // to test the member of a union for being NULL; it should have a
  // well-defined value if it is the selected type of the tag.
  switch(sta->kind) {
  default: xfailure("illegal STemplateArgument kind"); break;
  case STemplateArgument::STA_TYPE:
    toXml(sta->value.t);
    break;

  case STemplateArgument::STA_INT:
    // nothing to do
    break;

  case STemplateArgument::STA_ENUMERATOR:
  case STemplateArgument::STA_REFERENCE:
  case STemplateArgument::STA_POINTER:
  case STemplateArgument::STA_MEMBER:
    toXml(sta->value.v);
    break;

  case STemplateArgument::STA_DEPEXPR:
    sta->value.e->traverse(*astVisitor);
    break;

  case STemplateArgument::STA_TEMPLATE:
    xfailure("template template arguments not implemented");
    break;

  case STemplateArgument::STA_ATOMIC:
    toXml(const_cast<AtomicType*>(sta->value.at));
    break;
  }
}

void XmlTypeWriter::toXml(TemplateInfo *ti) {
  // idempotency
  if (printed(ti)) return;
  openTag(TemplateInfo, ti);
  // **** attributes
  // * superclass
  toXml_TemplateParams_properties(ti);
  // * members
  printPtr(ti, var);
  printEmbed(ti, inheritedParams);
  printPtr(ti, instantiationOf);
  printEmbed(ti, instantiations);
  printPtr(ti, specializationOf);
  printEmbed(ti, specializations);
  printEmbed(ti, arguments);
  printXml_SourceLoc(instLoc, ti->instLoc);
  printPtr(ti, partialInstantiationOf);
  printEmbed(ti, partialInstantiations);
  printEmbed(ti, argumentsToPrimary);
  printPtr(ti, defnScope);
  printPtr(ti, definitionTemplateInfo);
  tagEnd;
  // **** subtags
  // * superclass
  toXml_TemplateParams_subtags(ti);
  // * members
  trav(ti->var);
  travObjList(ti, TemplateInfo, inheritedParams, InheritedTemplateParams);
  trav(ti->instantiationOf);
  travObjList_S(ti, TemplateInfo, instantiations, Variable);
  trav(ti->specializationOf);
  travObjList_S(ti, TemplateInfo, specializations, Variable);
  travObjList(ti, TemplateInfo, arguments, STemplateArgument);
  trav(ti->partialInstantiationOf);
  travObjList_S(ti, TemplateInfo, partialInstantiations, Variable);
  travObjList(ti, TemplateInfo, argumentsToPrimary, STemplateArgument);
  trav(ti->defnScope);
  trav(ti->definitionTemplateInfo);
}

void XmlTypeWriter::toXml(InheritedTemplateParams *itp) {
  // idempotency
  if (printed(itp)) return;
  openTag(InheritedTemplateParams, itp);
  // **** attributes
  // * superclass
  toXml_TemplateParams_properties(itp);
  // * members
  printPtr(itp, enclosing);
  tagEnd;
  // **** subtags
  // * superclass
  toXml_TemplateParams_subtags(itp);
  // * members
  trav(itp->enclosing);
}

void XmlTypeWriter::toXml_TemplateParams_properties(TemplateParams *tp) {
  printEmbed(tp, params);
}

void XmlTypeWriter::toXml_TemplateParams_subtags(TemplateParams *tp) {
  travObjList_S(tp, TemplateParams, params, Variable);
}


// **** class XmlTypeWriter_AstVisitor
XmlTypeWriter_AstVisitor::XmlTypeWriter_AstVisitor
  (XmlTypeWriter &ttx0,
   ostream &out0,
   int &depth0,
   bool indent0,
   bool ensureOneVisit0)
    : XmlAstWriter_AstVisitor(out0, depth0, indent0, ensureOneVisit0)
    , ttx(ttx0)
{}

// Note that idempotency is handled in XmlTypeWriter
#define PRINT_ANNOT(A)   \
    if (A) {               \
      ttx.toXml(A); \
    }

  // this was part of the macro
//    printASTBiLink((void**)&(A), (A));

  // print the link between the ast node and the annotating node
//    void printASTBiLink(void **astField, void *annotation) {
//      out << "<__Link from=\"";
//      // this is not from an ast *node* but from the *field* of one
//      xmlPrintPointer(out, "FLD", astField);
//      out << "\" to=\"";
//      xmlPrintPointer(out, "TY", annotation);
//      out << "\"/>\n";
//    }

bool XmlTypeWriter_AstVisitor::visitTypeSpecifier(TypeSpecifier *ts) {
  if (!XmlAstWriter_AstVisitor::visitTypeSpecifier(ts)) return false;
  if (ts->isTS_type()) {
    PRINT_ANNOT(ts->asTS_type()->type);
  } else if (ts->isTS_name()) {
    PRINT_ANNOT(ts->asTS_name()->var);
    PRINT_ANNOT(ts->asTS_name()->nondependentVar);
  } else if (ts->isTS_elaborated()) {
    PRINT_ANNOT(ts->asTS_elaborated()->atype);
  } else if (ts->isTS_classSpec()) {
    PRINT_ANNOT(ts->asTS_classSpec()->ctype);
  } else if (ts->isTS_enumSpec()) {
    PRINT_ANNOT(ts->asTS_enumSpec()->etype);
  }
  return true;
}

bool XmlTypeWriter_AstVisitor::visitFunction(Function *f) {
  if (!XmlAstWriter_AstVisitor::visitFunction(f)) return false;
  PRINT_ANNOT(f->funcType);
  PRINT_ANNOT(f->receiver);
  return true;
}

bool XmlTypeWriter_AstVisitor::visitMemberInit(MemberInit *memberInit) {
  if (!XmlAstWriter_AstVisitor::visitMemberInit(memberInit)) return false;
  PRINT_ANNOT(memberInit->member);
  PRINT_ANNOT(memberInit->base);
  PRINT_ANNOT(memberInit->ctorVar);
  return true;
}

bool XmlTypeWriter_AstVisitor::visitBaseClassSpec(BaseClassSpec *bcs) {
  if (!XmlAstWriter_AstVisitor::visitBaseClassSpec(bcs)) return false;
  PRINT_ANNOT(bcs->type);
  return true;
}

bool XmlTypeWriter_AstVisitor::visitDeclarator(Declarator *d) {
  if (!XmlAstWriter_AstVisitor::visitDeclarator(d)) return false;
  PRINT_ANNOT(d->var);
  PRINT_ANNOT(d->type);
  return true;
}

bool XmlTypeWriter_AstVisitor::visitExpression(Expression *e) {
  if (!XmlAstWriter_AstVisitor::visitExpression(e)) return false;
  PRINT_ANNOT(e->type);
  if (e->isE_this()) {
    PRINT_ANNOT(e->asE_this()->receiver);
  } else if (e->isE_variable()) {
    PRINT_ANNOT(e->asE_variable()->var);
    PRINT_ANNOT(e->asE_variable()->nondependentVar);
  } else if (e->isE_constructor()) {
    PRINT_ANNOT(e->asE_constructor()->ctorVar);
  } else if (e->isE_fieldAcc()) {
    PRINT_ANNOT(e->asE_fieldAcc()->field);
  } else if (e->isE_new()) {
    PRINT_ANNOT(e->asE_new()->ctorVar);
  }
  return true;
}

#ifdef GNU_EXTENSION
bool XmlTypeWriter_AstVisitor::visitASTTypeof(ASTTypeof *a) {
  if (!XmlAstWriter_AstVisitor::visitASTTypeof(a)) return false;
  PRINT_ANNOT(a->type);
  return true;
}
#endif // GNU_EXTENSION

bool XmlTypeWriter_AstVisitor::visitPQName(PQName *pqn) {
  if (!XmlAstWriter_AstVisitor::visitPQName(pqn)) return false;
  if (pqn->isPQ_qualifier()) {
    PRINT_ANNOT(pqn->asPQ_qualifier()->qualifierVar);
    ttx.toXml(&(pqn->asPQ_qualifier()->sargs));
  } else if (pqn->isPQ_template()) {
    ttx.toXml(&(pqn->asPQ_template()->sargs));
  } else if (pqn->isPQ_variable()) {
    PRINT_ANNOT(pqn->asPQ_variable()->var);
  }
  return true;
}

bool XmlTypeWriter_AstVisitor::visitEnumerator(Enumerator *e) {
  if (!XmlAstWriter_AstVisitor::visitEnumerator(e)) return false;
  PRINT_ANNOT(e->var);
  return true;
}

bool XmlTypeWriter_AstVisitor::visitInitializer(Initializer *e) {
  if (!XmlAstWriter_AstVisitor::visitInitializer(e)) return false;
  if (e->isIN_ctor()) {
    PRINT_ANNOT(e->asIN_ctor()->ctorVar);
  }
  return true;
}

#undef PRINT_ANNOT
