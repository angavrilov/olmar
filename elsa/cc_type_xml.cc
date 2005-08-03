// cc_type_xml.cc            see license.txt for copyright and terms of use

#include "cc_type_xml.h"        // this module
#include "variable.h"           // Variable
#include "cc_flags.h"           // fromXml(DeclFlags &out, rostring str)
#include "asthelp.h"            // xmlPrintPointer
#include "xmlhelp.h"            // toXml_int() etc.

#include "strutil.h"            // parseQuotedString
#include "astxml_lexer.h"       // AstXmlLexer


//  #define addr(x) (static_cast<void const*>(x))
inline void const *addr(void const *x) {
  return x;
}


#define printThing0(NAME, VALUE, PREFIX, SUFFIX, FUNC) \
do { \
  out << #NAME "=\"" PREFIX << FUNC(VALUE) << "\"" SUFFIX; \
} while(0)

#define printThing(NAME, VALUE, PREFIX, FUNC) \
do { \
  if (VALUE) { \
    newline(); \
    printThing0(NAME, VALUE, PREFIX, "", FUNC); \
  } \
} while(0)

#define printPtr(NAME, VALUE, PREFIX) printThing(NAME, VALUE, PREFIX, addr)

#define printXml(NAME, VALUE) \
  newline(); \
  printThing0(NAME, VALUE, "", "", ::toXml)

#define printXml_bool(NAME, VALUE) \
  newline(); \
  printThing0(NAME, VALUE, "", "", ::toXml_bool)

#define printXml_int(NAME, VALUE) \
  newline(); \
  printThing0(NAME, VALUE, "", "", ::toXml_int)

#define printStrRef(FIELD, TARGET) \
do { \
  if (TARGET) { \
    newline(); \
    out << #FIELD "=" << quoted(TARGET); \
  } \
} while(0)

#define openTag0(NAME, PREFIX, OBJ, SUFFIX) \
do { \
  newline(); \
  out << "<" #NAME; \
  out << " _id=\"" PREFIX << addr(OBJ) << "\"" SUFFIX; \
  ++depth; \
} while(0)

#define openTag(NAME, PREFIX, OBJ) openTag0(NAME, PREFIX, OBJ, "")
#define openTagWhole(NAME, PREFIX, OBJ) openTag0(NAME, PREFIX, OBJ, ">")

#define openTag_NameMap_Item(NAME, TARGET) \
do { \
  newline(); \
  out << "<_NameMap_Item" \
      << " name=" << quoted(NAME) \
      << " item=\"TY" << addr(TARGET) \
      << "\">"; \
  ++depth; \
} while(0)

#define tagEnd \
do { \
  out << ">"; \
} while(0)

#define closeTag(NAME) \
do { \
  --depth; \
  newline(); \
  out << "</" #NAME ">"; \
} while(0)

#define trav(TARGET) \
do { \
  if (TARGET) { \
    toXml(TARGET); \
  } \
} while(0)

#define travListItem(PREFIX, TARGET) \
do { \
  newline(); \
  out << "<_List_Item item=\"" << PREFIX << addr(TARGET) << "\">"; \
  ++depth; \
  trav(TARGET); \
  closeTag(_List_Item); \
} while(0)


//  #if 0                           // refinement possibilities ...
//  #define address(x) static_cast<void const*>(&(x))

//  string ref(FunctionType::ExnSpec &spec)
//  {
//    return stringc << "\"TY" << address(&spec) << "\"";
//  }

//  template <class T>
//    string ref(SObjList<T> &list)
//  {
//    return stringc << "\"OL" << address(&list) << "\"";
//  }
//  #endif // 0


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

string toXml(STemplateArgument::Kind id) {
  return stringc << static_cast<int>(id);
}
void fromXml(STemplateArgument::Kind &out, rostring str) {
  out = static_cast<STemplateArgument::Kind>(atoi(str));
}


// -------------------- TypeToXml -------------------

void TypeToXml::newline() {
  out << "\n";
  if (indent) {
    for (int i=0; i<depth; ++i) cout << " ";
  }
}

// ****************

bool TypeToXml::printedType(void const *obj) {
  if (printedTypes.contains(obj)) return true;
  printedTypes.add(obj);
  return false;
}

bool TypeToXml::printedScope(void const *obj) {
  if (printedScopes.contains(obj)) return true;
  printedScopes.add(obj);
  return false;
}

bool TypeToXml::printedVariable(void const *obj) {
  if (printedVariables.contains(obj)) return true;
  printedVariables.add(obj);
  return false;
}

bool TypeToXml::printedOL(void const *obj) {
  if (printedOLs.contains(obj)) return true;
  printedOLs.add(obj);
  return false;
}

bool TypeToXml::printedSM(void const *obj) {
  if (printedSMs.contains(obj)) return true;
  printedSMs.add(obj);
  return false;
}

// ****************

void TypeToXml::toXml(Type *obj) {
  // idempotency
  if (printedType(obj)) return;

  switch(obj->getTag()) {
  default: xfailure("illegal tag");

  case Type::T_ATOMIC: {
    CVAtomicType *atom = obj->asCVAtomicType();
    // **** attributes
    openTag(CVAtomicType, "TY", obj);
    printPtr(atomic, atom->atomic, "TY");
    printXml(cv, atom->cv);
    tagEnd;
    // **** subtags
    trav(atom->atomic);
    closeTag(CVAtomicType);
    break;
  }

  case Type::T_POINTER: {
    PointerType *ptr = obj->asPointerType();
    // **** attributes
    openTag(PointerType, "TY", obj);
    printXml(cv, ptr->cv);
    printPtr(atType, ptr->atType, "TY");
    tagEnd;
    // **** subtags
    trav(ptr->atType);
    closeTag(PointerType);
    break;
  }

  case Type::T_REFERENCE: {
    ReferenceType *ref = obj->asReferenceType();
    // **** attributes
    openTag(ReferenceType, "TY", obj);
    printPtr(atType, ref->atType, "TY");
    tagEnd;
    // **** subtags
    trav(ref->atType);
    closeTag(ReferenceType);
    break;
  }

  case Type::T_FUNCTION: {
    FunctionType *func = obj->asFunctionType();
    // **** attributes
    openTag(FunctionType, "TY", obj);
    printXml(flags, func->flags);
    printPtr(retType, func->retType, "TY");
    printPtr(params, &func->params, "OL");
    printPtr(exnSpec, func->exnSpec, "TY");
    tagEnd;
    // **** subtags
    trav(func->retType);
    // params
    if (!printedOL(&func->params)) {
      openTagWhole(List_FunctionType_params, "OL", &func->params);
      SFOREACH_OBJLIST_NC(Variable, func->params, iter) {
        travListItem("TY", iter.data());
      }
      closeTag(List_FunctionType_params);
    }
    // exnSpec
    if (func->exnSpec) {
      toXml_FunctionType_ExnSpec(func->exnSpec);
    }
    closeTag(FunctionType);
    break;
  }

  case Type::T_ARRAY: {
    ArrayType *arr = obj->asArrayType();
    // **** attributes
    openTag(ArrayType, "TY", obj);
    printPtr(eltType, arr->eltType, "TY");
    newline();
    out << "size=\"" << arr->size << "\"";
    tagEnd;
    // **** subtags
    trav(arr->eltType);
    closeTag(ArrayType);
    break;
  }

  case Type::T_POINTERTOMEMBER: {
    PointerToMemberType *ptm = obj->asPointerToMemberType();
    // **** attributes
    openTag(PointerToMemberType, "TY", obj);
    printPtr(inClassNAT, ptm->inClassNAT, "TY");
    printXml(cv, ptm->cv);
    printPtr(atType, ptm->atType, "TY");
    tagEnd;
    // **** subtags
    trav(ptm->inClassNAT);
    trav(ptm->atType);
    closeTag(PointerToMemberType);
    break;
  }

  }
}

void TypeToXml::toXml(AtomicType *obj) {
  // idempotency
  if (printedType(obj)) return;

  switch(obj->getTag()) {
  default: xfailure("illegal tag");

  case AtomicType::T_SIMPLE: {
    SimpleType *simple = obj->asSimpleType();
    // **** attributes
    openTag(SimpleType, "TY", obj);
    printXml(type, simple->type);
    tagEnd;
    closeTag(SimpleType);
    break;
  }

  case AtomicType::T_COMPOUND: {
    CompoundType *cpd = obj->asCompoundType();
    // **** attributes
    openTag(CompoundType, "TY", obj);
    // superclasses
    toXml_NamedAtomicType_properties(cpd);
    toXml_Scope_properties(cpd);
    printXml_bool(forward, cpd->forward);
    printXml(keyword, cpd->keyword);
    printPtr(dataMembers, &cpd->dataMembers, "TY");
    printPtr(bases, &cpd->bases, "TY");
    printPtr(virtualBases, &cpd->virtualBases, "TY");
    printPtr(subobj, &cpd->subobj, "TY");
    printPtr(conversionOperators, &cpd->conversionOperators, "TY");
    printStrRef(instName, cpd->instName);
    printPtr(syntax, cpd->syntax, "AST"); // FIX: AST so make sure is serialized
    printPtr(parameterizingScope, cpd->parameterizingScope, "TY");
    printPtr(selfType, cpd->selfType, "TY");
    tagEnd;
    // **** subtags
    toXml_NamedAtomicType_subtags(cpd);
    toXml_Scope_subtags(cpd);
    // data members
    if (!printedOL(&cpd->dataMembers)) {
      openTagWhole(List_CompoundType_dataMembers, "OL", &cpd->dataMembers);
      SFOREACH_OBJLIST_NC(Variable, cpd->dataMembers, iter) {
        travListItem("TY", iter.data());
      }
      closeTag(List_CompoundType_dataMembers);
    }
    // bases
    if (!printedOL(&cpd->bases)) {
      openTagWhole(List_CompoundType_bases, "OL", &cpd->bases);
      FOREACH_OBJLIST_NC(BaseClass, const_cast<ObjList<BaseClass>&>(cpd->bases), iter) {
        travListItem("TY", iter.data());
      }
      closeTag(List_CompoundType_bases);
    }
    // virtual bases
    if (!printedOL(&cpd->virtualBases)) {
      openTagWhole(List_CompoundType_virtualBases, "OL", &cpd->virtualBases);
      FOREACH_OBJLIST_NC(BaseClassSubobj,
                         const_cast<ObjList<BaseClassSubobj>&>(cpd->virtualBases),
                         iter) {
        travListItem("TY", iter.data());
      }
      closeTag(List_CompoundType_virtualBases);
    }
    // subobj
    trav(&cpd->subobj);
    // conversionOperators
    if (!printedOL(&cpd->conversionOperators)) {
      openTagWhole(List_CompoundType_conversionOperators, "OL", &cpd->conversionOperators);
      SFOREACH_OBJLIST_NC(Variable, cpd->conversionOperators, iter) {
        travListItem("TY", iter.data());
      }
      closeTag(List_CompoundType_conversionOperators);
    }
    // parameterizingScope
    trav(cpd->parameterizingScope);
    closeTag(CompoundType);
    break;
  }

  case AtomicType::T_ENUM: {
    EnumType *e = obj->asEnumType();
    // **** attributes
    openTag(EnumType, "TY", e);
    toXml_NamedAtomicType_properties(e);
    printPtr(valueIndex, &e->valueIndex, "TY");
    printXml_int(nextValue, e->nextValue);
    tagEnd;
    // **** subtags
    toXml_NamedAtomicType_subtags(e);
    // valueIndex
    if (!printedOL(&e->valueIndex)) {
      openTagWhole(NameMap_EnumType_valueIndex, "OL", &e->valueIndex);
      for(StringObjDict<EnumType::Value>::Iter iter(e->valueIndex);
          !iter.isDone(); iter.next()) {
        string const &name = iter.key();
        // dsw: do you know how bad it gets if I don't put a
        // const-cast here?
        EnumType::Value *eValue = const_cast<EnumType::Value*>(iter.value());
        openTag_NameMap_Item(name, eValue);
        toXml_EnumType_Value(eValue);
        closeTag(_NameMap_Item);
      }
      closeTag(NameMap_EnumType_valueIndex);
    }
    closeTag(EnumType);
    break;
  }

  case AtomicType::T_TYPEVAR: {
    TypeVariable *tvar = obj->asTypeVariable();
    // **** attributes
    openTag(TypeVariable, "TY", obj);
    toXml_NamedAtomicType_properties(tvar);
    tagEnd;
    // **** subtags
    toXml_NamedAtomicType_subtags(tvar);
    closeTag(TypeVariable);
    break;
  }

  case AtomicType::T_PSEUDOINSTANTIATION: {
    PseudoInstantiation *pseudo = obj->asPseudoInstantiation();
    // **** attributes
    openTag(PseudoInstantiation, "TY", obj);
    toXml_NamedAtomicType_properties(pseudo);
    printPtr(primary, pseudo->primary, "TY");
    printPtr(args, &pseudo->args, "OL");
    tagEnd;
    // **** subtags
    toXml_NamedAtomicType_subtags(pseudo);
    // NOTE: without the cast, the overloading is ambiguous
    trav(static_cast<AtomicType*>(pseudo->primary));
    if (!printedOL(&pseudo->args)) {
      openTagWhole(List_PseudoInstantiation_args, "OL", &pseudo->args);
      FOREACH_OBJLIST_NC(STemplateArgument, pseudo->args, iter) {
        travListItem("TY", iter.data());
      }
      closeTag(List_PseudoInstantiation_args);
    }
    closeTag(PseudoInstantiation);
    break;
  }

  case AtomicType::T_DEPENDENTQTYPE: {
    DependentQType *dep = obj->asDependentQType();
    // **** attributes
    openTag(DependentQType, "TY", obj);
    toXml_NamedAtomicType_properties(dep);
    printPtr(first, dep->first, "TY");
    printPtr(rest, dep->rest, "AST");
    tagEnd;
    // **** subtags
    toXml_NamedAtomicType_subtags(dep);
    trav(dep->first);
    // FIX: traverse this AST
//    PQName *rest;
    closeTag(DependentQType);
    break;
  }

  }
}

void TypeToXml::toXml(Variable *var) {
  // idempotency
  if (printedVariable(var)) return;
  // **** attributes
  openTag(Variable, "TY", var);
//    SourceLoc loc;
//    I'm skipping these for now, but source locations will be serialized
//    as file:line:col when I serialize the internals of the Source Loc
//    Manager.
  printStrRef(name, var->name);
  printPtr(type, var->type, "TY");
  printXml(flags, var->flags);
  printPtr(value, var->value, "AST"); // FIX: this is AST make sure gets serialized
  printPtr(defaultParamType, var->defaultParamType, "TY");
  printPtr(funcDefn, var->funcDefn, "AST"); // FIX: this is AST make sure gets serialized
//    OverloadSet *overload;  // (nullable serf)
//    I don't think we need to serialize this because we are done with
//    overloading after typechecking.  Will have to eventually be done if
//    an analysis wants to analyze uninstantiate templates.
  printPtr(scope, var->scope, "TY");
//    // bits 0-7: result of 'getAccess()'
//    // bits 8-15: result of 'getScopeKind()'
//    // bits 16-31: result of 'getParameterOrdinal()'
//    unsigned intData;
//    Ugh.  Break into 3 parts eventually, but for now serialize as an int.
  newline();
  out << "intData=\"" << toXml_Variable_intData(var->intData) << "\"";
  printPtr(usingAlias_or_parameterizedEntity, var->usingAlias_or_parameterizedEntity, "TY");
  printPtr(templInfo, var->templInfo, "TY");
  tagEnd;
  // **** subtags
  trav(var->type);
//    trav(var->value);             // FIX: AST so make sure is serialized
  trav(var->defaultParamType);
//    trav(var->funcDefn);          // FIX: AST so make sure is serialized
  // skipping 'overload'; see above
  trav(var->scope);
  trav(var->usingAlias_or_parameterizedEntity);
//    trav(var->templInfo);
  closeTag(Variable);
}

void TypeToXml::toXml_FunctionType_ExnSpec(void /*FunctionType::ExnSpec*/ *exnSpec0) {
  FunctionType::ExnSpec *exnSpec = static_cast<FunctionType::ExnSpec *>(exnSpec0);
  // idempotency
  if (printedType(exnSpec)) return;
  // **** attributes
  openTag(FunctionType_ExnSpec, "TY", exnSpec);
  printPtr(types, &exnSpec->types, "OL");
  tagEnd;
  // **** FunctionType::ExnSpec subtags
  if (!printedOL(&exnSpec->types)) {
    openTagWhole(List_ExnSpec_types, "OL", &exnSpec->types);
    SFOREACH_OBJLIST_NC(Type, exnSpec->types, iter) {
      travListItem("TY", iter.data());
    }
    closeTag(List_ExnSpec_types);
  }
  closeTag(FunctionType_ExnSpec);
}

void TypeToXml::toXml_EnumType_Value(void /*EnumType::Value*/ *eValue0) {
  EnumType::Value *eValue = static_cast<EnumType::Value *>(eValue0);
  // idempotency
  if (printedType(eValue)) return;
  // **** attributes
  openTag(EnumType_Value, "TY", eValue);
  printStrRef(name, eValue->name);
  printPtr(type, &eValue->type, "TY");
  printXml_int(value, eValue->value);
  printPtr(decl, &eValue->decl, "TY");
  tagEnd;
  // **** subtags
  trav(eValue->type);
  trav(eValue->decl);
  closeTag(EnumType_Value);
}

void TypeToXml::toXml_NamedAtomicType_properties(NamedAtomicType *nat) {
  printStrRef(name, nat->name);
  printPtr(typedefVar, nat->typedefVar, "TY");
  printXml(access, nat->access);
}

void TypeToXml::toXml_NamedAtomicType_subtags(NamedAtomicType *nat) {
  trav(nat->typedefVar);
}

void TypeToXml::toXml(BaseClass *bc) {
  // idempotency
  if (printedType(bc)) return;
  // **** attributes
  openTag(BaseClass, "TY", bc);
  toXml_BaseClass_properties(bc);
  tagEnd;
  // **** subtags
  // NOTE: without the cast, the overloading is ambiguous
  trav(static_cast<AtomicType*>(bc->ct));
  closeTag(BaseClass);
}

void TypeToXml::toXml_BaseClass_properties(BaseClass *bc) {
  printPtr(ct, bc->ct, "TY");
  printXml(access, bc->access);
  printXml_bool(isVirtual, bc->isVirtual);
}

void TypeToXml::toXml(BaseClassSubobj *bc) {
  // idempotency
  if (printedType(bc)) return;
  // **** attributes
  openTag(BaseClassSubobj, "TY", bc);
  toXml_BaseClass_properties(bc);
  printPtr(parents, &bc->parents, "OL");
  tagEnd;
  // **** subtags
  if (!printedOL(&bc->parents)) {
    openTagWhole(List_BaseClassSubobj_parents, "OL", &bc->parents);
    SFOREACH_OBJLIST_NC(BaseClassSubobj, bc->parents, iter) {
      travListItem("TY", iter.data());
    }
    closeTag(List_BaseClassSubobj_parents);
  }
  closeTag(BaseClassSubobj);
}

void TypeToXml::toXml(Scope *scope) {
  // idempotency
  if (printedScope(scope)) return;
  // **** attributes
  openTag(Scope, "TY", scope);
  toXml_Scope_properties(scope);
  tagEnd;
  // **** subtags
  toXml_Scope_subtags(scope);
  closeTag(Scope);
}

void TypeToXml::toXml_Scope_properties(Scope *scope) {
  printPtr(variables, &scope->variables, "SM");
  printPtr(typeTags, &scope->typeTags, "SM");
  printXml_bool(canAcceptNames, scope->canAcceptNames);
  printPtr(parentScope, scope, "TY");
  printXml(scopeKind, scope->scopeKind);
  printPtr(namespaceVar, scope->namespaceVar, "TY");
  printPtr(templateParams, &scope->templateParams, "OL");
  printPtr(curCompound, scope->curCompound, "TY");
}

void TypeToXml::toXml_Scope_subtags(Scope *scope) {
  // variables
  if (!printedSM(&scope->variables)) {
    openTagWhole(NameMap_Scope_variables, "SM", &scope->variables);
    for(PtrMap<char const, Variable>::Iter iter(scope->variables);
        !iter.isDone();
        iter.adv()) {
      StringRef name = iter.key();
      Variable *var = iter.value();
      openTag_NameMap_Item(name, var);
      trav(var);
      closeTag(_NameMap_Item);
    }
    closeTag(NameMap_Scope_variables);
  }
  // typeTags
  if (!printedSM(&scope->typeTags)) {
    openTagWhole(NameMap_Scope_typeTags, "SM", &scope->typeTags);
    for(PtrMap<char const, Variable>::Iter iter(scope->typeTags);
        !iter.isDone();
        iter.adv()) {
      StringRef name = iter.key();
      Variable *var = iter.value();
      openTag_NameMap_Item(name, var);
      trav(var);
      closeTag(_NameMap_Item);
    }
    closeTag(NameMap_Scope_typeTags);
  }
  // parentScope
  trav(scope->parentScope);
  // namespaceVar
  trav(scope->namespaceVar);
  // templateParams
  if (!printedOL(&scope->templateParams)) {
    openTagWhole(List_Scope_templateParams, "OL", &scope->templateParams);
    SFOREACH_OBJLIST_NC(Variable, scope->templateParams, iter) {
      travListItem("TY", iter.data());
    }
    closeTag(List_Scope_templateParams);
  }
  // I don't think I need this; see Scott's comments in the scope
  // class
//    Variable *parameterizedEntity;          // (nullable serf)
  // --------------- for using-directives ----------------
  // Scott says that I don't need these
  //
  // it is basically a bug that we need to serialize this but we do so
  // there it is.
//    CompoundType *curCompound;          // (serf) CompoundType we're building
//    Should not be being used after typechecking, but in theory could omit.
//    if (curCompound) {
//      curCompound->traverse(vis);
//    }

  // NOTE: without the cast, the overloading is ambiguous
  trav(static_cast<AtomicType*>(scope->curCompound));
}

void TypeToXml::toXml(STemplateArgument *obj) {
  openTag(STemplateArgument, "TY", obj);
  printXml(kind, obj->kind);

  // **** attributes
  switch(obj->kind) {
  default: xfailure("illegal STemplateArgument kind"); break;

  case STemplateArgument::STA_TYPE:
    printPtr(t, obj->value.t, "TY");
    break;

  case STemplateArgument::STA_INT:
    newline();
    out << "i=\"" << toXml_int(obj->value.i) << "\"";
    break;

  case STemplateArgument::STA_REFERENCE:
  case STemplateArgument::STA_POINTER:
  case STemplateArgument::STA_MEMBER:
    printPtr(v, obj->value.v, "TY");
    break;

  case STemplateArgument::STA_DEPEXPR:
    printPtr(e, obj->value.e, "AST");
    break;

  case STemplateArgument::STA_TEMPLATE:
    xfailure("template template arguments not implemented");
    break;

  case STemplateArgument::STA_ATOMIC:
    printPtr(at, obj->value.at, "TY");
    break;
  }
  tagEnd;

  // **** subtags
  switch(obj->kind) {
  default: xfailure("illegal STemplateArgument kind"); break;

  case STemplateArgument::STA_TYPE:
    toXml(obj->value.t);
    break;

  case STemplateArgument::STA_INT:
    // nothing to do
    break;

  case STemplateArgument::STA_REFERENCE:
  case STemplateArgument::STA_POINTER:
  case STemplateArgument::STA_MEMBER:
    toXml(obj->value.v);
    break;

  case STemplateArgument::STA_DEPEXPR:
    // FIX: what the hell should we do?  the same remark is made in
    // the traverse() method code at this point
    break;

  case STemplateArgument::STA_TEMPLATE:
    xfailure("template template arguments not implemented");
    break;

  case STemplateArgument::STA_ATOMIC:
    trav(const_cast<AtomicType*>(obj->value.at));
    break;
  }

  closeTag(STemplateArgument);
}

// -------------------- ReadXml_Type -------------------

void ReadXml_Type::append2List(void *list0, int listKind, void *datum0) {
  xassert(list0);
  ASTList<char> *list = static_cast<ASTList<char>*>(list0);
  char *datum = (char*)datum0;
  list->append(datum);
}

void ReadXml_Type::insertIntoNameMap(void *map0, int mapKind, StringRef name, void *datum) {
  xassert(map0);
  StringRefMap<char> *map = static_cast<StringRefMap<char>*>(map0);
  if (map->get(name)) {
    userError(stringc << "duplicate name " << name << " in map");
  }
  map->add(name, (char*)datum);
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
      ret->prepend(iter.data());
    }
    ret->reverse();
    break;
  }

  case XTOK_List_BaseClassSubobj_parents: {
    SObjList<BaseClassSubobj> *ret = reinterpret_cast<SObjList<BaseClassSubobj>*>(target);
    xassert(ret->isEmpty());
    FOREACH_ASTLIST_NC(BaseClassSubobj, reinterpret_cast<ASTList<BaseClassSubobj>&>(*list), iter) {
      ret->prepend(iter.data());
    }
    ret->reverse();
    break;
  }

  case XTOK_List_ExnSpec_types: {
    SObjList<Type> *ret = reinterpret_cast<SObjList<Type>*>(target);
    xassert(ret->isEmpty());
    FOREACH_ASTLIST_NC(Type, reinterpret_cast<ASTList<Type>&>(*list), iter) {
      ret->prepend(iter.data());
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
      ret->prepend(iter.data());
    }
    ret->reverse();
    break;
  }

  case XTOK_List_CompoundType_virtualBases: {
    ObjList<BaseClassSubobj> *ret = reinterpret_cast<ObjList<BaseClassSubobj>*>(target);
    xassert(ret->isEmpty());
    FOREACH_ASTLIST_NC(BaseClassSubobj, reinterpret_cast<ASTList<BaseClassSubobj>&>(*list), iter) {
      ret->prepend(iter.data());
    }
    ret->reverse();
    break;
  }

  }
  return true;
}

bool ReadXml_Type::convertNameMap2StringRefMap
  (StringRefMap<char> *map, int mapKind, void *target) {
  xassert(map);
  switch(mapKind) {
  default: return false;        // we did not find a matching tag

  case XTOK_NameMap_Scope_variables:
  case XTOK_NameMap_Scope_typeTags: {
    StringRefMap<Variable> *ret = reinterpret_cast<StringRefMap<Variable>*>(target);
    xassert(ret->isEmpty());
    for(StringRefMap<Variable>::Iter iter(reinterpret_cast<StringRefMap<Variable>&>(*map));
        !iter.isDone(); iter.adv()) {
      ret->add(iter.key(), iter.value());
    }
    break;
  }

  }
  return true;
}

bool ReadXml_Type::convertNameMap2StringSObjDict
  (StringRefMap<char> *map, int mapKind, void *target) {
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
      ret->add(iter.key(), iter.value());
    }
    break;
  }

  }
  return true;
}

void *ReadXml_Type::ctorNodeFromTag(int tag) {
  switch(tag) {
  default: userError("unexpected token while looking for an open tag name");
  case 0: userError("unexpected file termination while looking for an open tag name");

  // **** Types
  case XTOK_CVAtomicType: return new CVAtomicType((AtomicType*)0, (CVFlags)0);
  case XTOK_PointerType: return new PointerType((CVFlags)0, (Type*)0);
  case XTOK_ReferenceType: return new ReferenceType((Type*)0);
  case XTOK_FunctionType: return new FunctionType((Type*)0);
  case XTOK_FunctionType_ExnSpec: return new FunctionType::ExnSpec();
  case XTOK_ArrayType: return new ArrayType((ReadXML&)*this); // call the special ctor
  case XTOK_PointerToMemberType:
    return new PointerToMemberType((NamedAtomicType*)0, (CVFlags)0, (Type*)0);

  // **** Atomic Types
  // NOTE: this really should go through the SimpleTyp::fixed array
  case XTOK_SimpleType: return new SimpleType((SimpleTypeId)0);
  case XTOK_CompoundType: return new CompoundType((CompoundType::Keyword)0, (StringRef)0);
  case XTOK_EnumType: return new EnumType((StringRef)0);
  case XTOK_EnumType_Value:
    return new EnumType::Value((StringRef)0, (EnumType*)0, (int)0, (Variable*)0);
  case XTOK_TypeVariable: return new TypeVariable((StringRef)0);
  case XTOK_PseudoInstantiation: return new PseudoInstantiation((CompoundType*)0);
  case XTOK_DependentQType: return new DependentQType((AtomicType*)0);

  // **** Other
  case XTOK_Variable: return new Variable((ReadXML&)*this);// call the special ctor
  case XTOK_Scope: return new Scope((ReadXML&)*this); // call the special ctor
  case XTOK_BaseClass: return new BaseClass((CompoundType*)0, (AccessKeyword)0, (bool)0);
  case XTOK_BaseClassSubobj:
    // NOTE: special; FIX: should I make the BaseClass on the heap and
    // then delete it?  I'm not sure if the compiler is going to be
    // able to tell that even though it is passed by reference to the
    // BaseClassSubobj that it is not kept there and therefore can be
    // deleted at the end of the full expression.
    return new BaseClassSubobj(BaseClass((CompoundType*)0, (AccessKeyword)0, (bool)0));

  // **** Containers
  // ObjList
  case XTOK_List_CompoundType_bases: return new ASTList<BaseClass>();
  case XTOK_List_CompoundType_virtualBases: return new ASTList<BaseClassSubobj>();

  // SObjList
  case XTOK_List_FunctionType_params: return new ASTList<Variable>();
  case XTOK_List_CompoundType_dataMembers: return new ASTList<Variable>();
  case XTOK_List_CompoundType_conversionOperators: return new ASTList<Variable>();
  case XTOK_List_BaseClassSubobj_parents: return new ASTList<BaseClassSubobj>();
  case XTOK_List_ExnSpec_types: return new ASTList<Type>();
  case XTOK_List_Scope_templateParams: return new ASTList<Variable>();

  // StringRefMap
  case XTOK_NameMap_Scope_variables: return new StringRefMap<Variable>();
  case XTOK_NameMap_Scope_typeTags: return new StringRefMap<Variable>();
  case XTOK_NameMap_EnumType_valueIndex: return new StringRefMap<EnumType::Value>();
  }
}

// **************** registerAttribute

#define regAttr(TYPE) \
  registerAttr_##TYPE((TYPE*)target, attr, yytext0)

void ReadXml_Type::registerAttribute(void *target, int kind, int attr, char const *yytext0) {
  switch(kind) {
  default: xfailure("illegal kind");

  // **** Types
  case XTOK_CVAtomicType: regAttr(CVAtomicType); break; 
  case XTOK_PointerType: regAttr(PointerType); break; 
  case XTOK_ReferenceType: regAttr(ReferenceType); break; 
  case XTOK_FunctionType: regAttr(FunctionType); break; 
  case XTOK_FunctionType_ExnSpec:
    registerAttr_FunctionType_ExnSpec((FunctionType::ExnSpec*)target, attr, yytext0); break;
  case XTOK_ArrayType: regAttr(ArrayType); break; 
  case XTOK_PointerToMemberType: regAttr(PointerToMemberType); break; 

  // **** Atomic Types
  case XTOK_SimpleType: regAttr(SimpleType); break; 
  case XTOK_CompoundType: regAttr(CompoundType); break; 
  case XTOK_EnumType: regAttr(EnumType); break; 
  case XTOK_EnumType_Value:
    registerAttr_EnumType_Value((EnumType::Value*)target, attr, yytext0); break;
  case XTOK_TypeVariable: regAttr(TypeVariable); break; 
  case XTOK_PseudoInstantiation: regAttr(PseudoInstantiation); break; 
  case XTOK_DependentQType: regAttr(DependentQType); break; 

  // **** Other
  case XTOK_Variable: regAttr(Variable); break; 
  case XTOK_Scope: regAttr(Scope); break; 
  case XTOK_BaseClass: regAttr(BaseClass); break; 
  case XTOK_BaseClassSubobj: regAttr(BaseClassSubobj); break; 
  }
}

#define ul(FIELD) \
  linkSat.unsatLinks.append \
    (new UnsatLink((void**) &(obj->FIELD), \
                   parseQuotedString(strValue)))

#define ulList(LIST, FIELD, KIND) \
  linkSat.unsatLinks##LIST.append \
    (new UnsatLink((void**) &(obj->FIELD), \
                   parseQuotedString(strValue), \
                   (KIND)))

void ReadXml_Type::registerAttr_CVAtomicType(CVAtomicType *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a CVAtomicType"); break;
  case XTOK_cv: fromXml(obj->cv, parseQuotedString(strValue)); break;
  case XTOK_atomic: ul(atomic); break; 
  }
}

void ReadXml_Type::registerAttr_PointerType(PointerType *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a PointerType"); break; 
  case XTOK_cv: fromXml(obj->cv, parseQuotedString(strValue)); break; 
  case XTOK_atType: ul(atType); break; 
  }
}

void ReadXml_Type::registerAttr_ReferenceType(ReferenceType *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a ReferenceType"); break; 
  case XTOK_atType: ul(atType); break; 
  }
}

void ReadXml_Type::registerAttr_FunctionType(FunctionType *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a FunctionType"); break;
  case XTOK_flags: fromXml(obj->flags, parseQuotedString(strValue)); break;
  case XTOK_retType: ul(retType); break;
  case XTOK_params: ulList(_List, params, XTOK_List_FunctionType_params); break;
  case XTOK_exnSpec: ul(exnSpec); break;
  }
}

void ReadXml_Type::registerAttr_FunctionType_ExnSpec
  (FunctionType::ExnSpec *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a FunctionType_ExnSpec"); break;
  case XTOK_types: ulList(_List, types, XTOK_List_ExnSpec_types); break;
  }
}

void ReadXml_Type::registerAttr_ArrayType(ArrayType *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a ArrayType"); break; 
  case XTOK_eltType: ul(eltType); break; 
  case XTOK_size: obj->size = atoi(parseQuotedString(strValue)); break; 
  }
}

void ReadXml_Type::registerAttr_PointerToMemberType
  (PointerToMemberType *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a PointerToMemberType"); break; 
  case XTOK_inClassNAT: ul(inClassNAT); break; 
  case XTOK_cv: fromXml(obj->cv, parseQuotedString(strValue)); break; 
  case XTOK_atType: ul(atType); break; 
  }
}

void ReadXml_Type::registerAttr_Variable(Variable *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a Variable"); break;
  // FIX: SourceLoc loc
  case XTOK_name: obj->name = strTable(parseQuotedString(strValue)); break; 
  case XTOK_type: ul(type); break; 
  case XTOK_flags:
    fromXml(const_cast<DeclFlags&>(obj->flags), parseQuotedString(strValue)); break; 
  case XTOK_value: ul(value); break; 
  case XTOK_defaultParamType: ul(defaultParamType); break; 
  case XTOK_funcDefn: ul(funcDefn); break; 
  case XTOK_scope: ul(scope); break; 
  case XTOK_intData: fromXml_Variable_intData(obj->intData, parseQuotedString(strValue)); break; 
  case XTOK_usingAlias_or_parameterizedEntity: ul(usingAlias_or_parameterizedEntity); break; 
  // FIX: templInfo
  }
}

bool ReadXml_Type::registerAttr_NamedAtomicType_super
  (NamedAtomicType *obj, int attr, char const *strValue) {
  switch(attr) {
  default: return false;        // we didn't find it
  case XTOK_name: obj->name = strTable(parseQuotedString(strValue)); break; 
  case XTOK_typedefVar: ul(typedefVar); break; 
  case XTOK_access: fromXml(obj->access, parseQuotedString(strValue)); break; 
  }
  return true;                  // found it
}

void ReadXml_Type::registerAttr_SimpleType(SimpleType *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a SimpleType"); break; 
  case XTOK_type:
    // NOTE: this 'type' is not a type node, but basically an enum,
    // and thus is handled more like a flag would be.
    fromXml(const_cast<SimpleTypeId&>(obj->type), parseQuotedString(strValue));
    break;
  }
}

void ReadXml_Type::registerAttr_CompoundType(CompoundType *obj, int attr, char const *strValue) {
  // superclasses
  if (registerAttr_NamedAtomicType_super(obj, attr, strValue)) return;
  if (registerAttr_Scope_super(obj, attr, strValue)) return;

  switch(attr) {
  default: userError("illegal attribute for a CompoundType"); break; 
  case XTOK_forward: fromXml_bool(obj->forward, parseQuotedString(strValue)); break; 
  case XTOK_keyword: fromXml(obj->keyword, parseQuotedString(strValue)); break; 
  case XTOK_dataMembers: ulList(_List, dataMembers, XTOK_List_CompoundType_dataMembers); break; 
  case XTOK_bases: ulList(_List, bases, XTOK_List_CompoundType_bases); break; 
  case XTOK_virtualBases: ulList(_List, virtualBases, XTOK_List_CompoundType_virtualBases); break; 
  case XTOK_subobj: ul(subobj); break; 
  case XTOK_conversionOperators:
    ulList(_List, conversionOperators, XTOK_List_CompoundType_conversionOperators); break; 
  case XTOK_instName: obj->instName = strTable(parseQuotedString(strValue)); break; 
  case XTOK_syntax: ul(syntax); break; 
  case XTOK_parameterizingScope: ul(parameterizingScope); break; 
  case XTOK_selfType: ul(selfType); break; 
  }
}

void ReadXml_Type::registerAttr_EnumType(EnumType *obj, int attr, char const *strValue) {
  // superclass
  if (registerAttr_NamedAtomicType_super(obj, attr, strValue)) return;

  switch(attr) {
  default: userError("illegal attribute for a EnumType"); break; 
  case XTOK_valueIndex: ulList(_NameMap, valueIndex, XTOK_NameMap_EnumType_valueIndex); break; 
  case XTOK_nextValue: obj->nextValue = atoi(parseQuotedString(strValue)); break; 
  }
}

void ReadXml_Type::registerAttr_EnumType_Value
  (EnumType::Value *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a EnumType"); break; 
  case XTOK_name: obj->name = strTable(parseQuotedString(strValue)); break; 
  case XTOK_type: ul(type); break; // NOTE: 'type' here is actually an atomic type
  case XTOK_value: obj->value = atoi(parseQuotedString(strValue)); break; 
  case XTOK_decl: ul(decl); break; 
  }
}

void ReadXml_Type::registerAttr_TypeVariable(TypeVariable *obj, int attr, char const *strValue) {
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
  default: userError("illegal attribute for a PsuedoInstantiation"); break; 
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
  default: userError("illegal attribute for a DependentQType"); break; 
//    AtomicType *first;            // (serf) TypeVariable or PseudoInstantiation
//    // After the first component comes whatever name components followed
//    // in the original syntax.  All template arguments have been
//    // tcheck'd.
//    PQName *rest;
  }
}

bool ReadXml_Type::registerAttr_Scope_super(Scope *obj, int attr, char const *strValue) {
  switch(attr) {
  default: return false;        // we didn't find it break; 
  case XTOK_variables: ulList(_NameMap, variables, XTOK_NameMap_Scope_variables); break; 
  case XTOK_typeTags: ulList(_NameMap, typeTags, XTOK_NameMap_Scope_typeTags); break; 
  case XTOK_canAcceptNames: fromXml_bool(obj->canAcceptNames, parseQuotedString(strValue)); break; 
  case XTOK_parentScope: ul(parentScope); break; 
  case XTOK_scopeKind: fromXml(obj->scopeKind, parseQuotedString(strValue)); break; 
  case XTOK_namespaceVar: ul(namespaceVar); break; 
  case XTOK_templateParams: ul(templateParams); break; 
  case XTOK_curCompound: ul(curCompound); break; 
  }
  return true;                  // found it
}

void ReadXml_Type::registerAttr_Scope(Scope *obj, int attr, char const *strValue) {
  // "superclass": just re-use our own superclass code for ourself
  if (registerAttr_Scope_super(obj, attr, strValue)) return;
  // shouldn't get here
  userError("illegal attribute for a Scope");
}

bool ReadXml_Type::registerAttr_BaseClass_super(BaseClass *obj, int attr, char const *strValue) {
  switch(attr) {
  default: return false; break; 
  case XTOK_ct: ul(ct); break; 
  case XTOK_access: fromXml(obj->access, parseQuotedString(strValue)); break; 
  case XTOK_isVirtual: fromXml_bool(obj->isVirtual, parseQuotedString(strValue)); break; 
  }
  return true;
}

void ReadXml_Type::registerAttr_BaseClass(BaseClass *obj, int attr, char const *strValue) {
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
  default: userError("illegal attribute for a BaseClassSubobj"); break; 
  case XTOK_parents: ulList(_List, parents, XTOK_List_BaseClassSubobj_parents); break; 
  }
}
