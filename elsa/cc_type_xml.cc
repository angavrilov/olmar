// cc_type_xml.cc            see license.txt for copyright and terms of use

#include "cc_type_xml.h"        // this module
#include "variable.h"           // Variable
#include "cc_flags.h"           // fromXml(DeclFlags &out, rostring str)
#include "asthelp.h"            // xmlPrintPointer
#include "xmlhelp.h"            // toXml_int() etc.

#include "strutil.h"            // parseQuotedString
#include "astxml_lexer.h"       // AstXmlLexer


// this bizarre function is actually useful for something
inline void const *addr(void const *x) {
  return x;
}

// ast
string idPrefixAST(void const * const) {return "AST";}

// type annotations
string idPrefix(Type const * const)                    {return "TY";}
string idPrefix(AtomicType const * const)              {return "TY";}
string idPrefix(CompoundType const * const)            {return "TY";}
string idPrefix(FunctionType::ExnSpec const * const)   {return "TY";}
string idPrefix(EnumType::Value const * const)         {return "TY";}
string idPrefix(BaseClass const * const)               {return "TY";}
string idPrefix(Scope const * const)                   {return "TY";}
string idPrefix(Variable const * const)                {return "TY";}
string idPrefix(OverloadSet const * const)             {return "TY";}
string idPrefix(STemplateArgument const * const)       {return "TY";}
string idPrefix(TemplateInfo const * const)            {return "TY";}
string idPrefix(InheritedTemplateParams const * const) {return "TY";}

// containers
template <class T> string idPrefix(ObjList<T> const * const)       {return "OL";}
template <class T> string idPrefix(SObjList<T> const * const)      {return "OL";}
template <class T> string idPrefix(StringRefMap<T> const * const)  {return "NM";}
template <class T> string idPrefix(StringObjDict<T> const * const) {return "NM";}

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

// indent and print something when exiting the scope
class TypeToXml_CloseTagPrinter {
  string s;                     // NOTE: don't make into a string ref; it must make a copy
  TypeToXml &ttx;
  public:
  explicit TypeToXml_CloseTagPrinter(string s0, TypeToXml &ttx0)
    : s(s0), ttx(ttx0)
  {}
  private:
  explicit TypeToXml_CloseTagPrinter(TypeToXml_CloseTagPrinter &); // prohibit
  public:
  ~TypeToXml_CloseTagPrinter() {
    ttx.newline();
    ttx.out << "</" << s << ">";
  }
};


#define printThing0(NAME, PREFIX, VALUE, FUNC) \
do { \
  out << #NAME "=\"" << PREFIX << FUNC(VALUE) << "\""; \
} while(0)

#define printThing(NAME, PREFIX, VALUE, FUNC) \
do { \
  if (VALUE) { \
    newline(); \
    printThing0(NAME, PREFIX, VALUE, FUNC); \
  } \
} while(0)

#define printPtr(NAME, VALUE)    printThing(NAME, idPrefix(VALUE),    VALUE, addr)
#define printPtrAST(NAME, VALUE) printThing(NAME, idPrefixAST(VALUE), VALUE, addr)

#define printXml(NAME, VALUE) \
do { \
  newline(); \
  printThing0(NAME, "", VALUE, ::toXml); \
} while(0)

#define printXml_bool(NAME, VALUE) \
do { \
  newline(); \
  printThing0(NAME, "", VALUE, ::toXml_bool); \
} while(0)

#define printXml_int(NAME, VALUE) \
do { \
  newline(); \
  printThing0(NAME, "", VALUE, ::toXml_int); \
} while(0)

#define printXml_SourceLoc(NAME, VALUE) \
do { \
  newline(); \
  printThing0(NAME, "", VALUE, ::toXml_SourceLoc); \
} while(0)

#define printStrRef(FIELD, TARGET) \
do { \
  if (TARGET) { \
    newline(); \
    out << #FIELD "=" << quoted(TARGET); \
  } \
} while(0)

// NOTE: you must not wrap this one in a 'do {} while(0)': the dtor
// for the TypeToXml_CloseTagPrinter fires too early.
#define openTag0(NAME, OBJ, SUFFIX) \
  newline(); \
  out << "<" #NAME << " _id=\"" << idPrefix(OBJ) << addr(OBJ) << "\"" SUFFIX; \
  TypeToXml_CloseTagPrinter tagCloser(#NAME, *this); \
  IncDec depthManager(this->depth)

#define openTag(NAME, OBJ)      openTag0(NAME, OBJ, "")
#define openTagWhole(NAME, OBJ) openTag0(NAME, OBJ, ">")

// NOTE: you must not wrap this one in a 'do {} while(0)': the dtor
// for the TypeToXml_CloseTagPrinter fires too early.
#define openTag_NameMap_Item(NAME, TARGET) \
  newline(); \
  out << "<_NameMap_Item" \
      << " name=" << quoted(NAME) \
      << " item=\"" << idPrefix(TARGET) << addr(TARGET) \
      << "\">"; \
  TypeToXml_CloseTagPrinter tagCloser("_NameMap_Item", *this); \
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
// for the TypeToXml_CloseTagPrinter fires too early.
#define travListItem(TARGET) \
  newline(); \
  out << "<_List_Item item=\"" << idPrefix(TARGET) << addr(TARGET) << "\">"; \
  TypeToXml_CloseTagPrinter tagCloser("_List_Item", *this); \
  IncDec depthManager(this->depth); \
  trav(TARGET)


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

// **** printing idepotency

bool TypeToXml::printedType(void const * const obj) {
  if (printedTypes.contains(obj)) return true;
  printedTypes.add(obj);
  return false;
}

bool TypeToXml::printedScope(Scope const * const obj) {
  if (printedScopes.contains(obj)) return true;
  printedScopes.add(obj);
  return false;
}

bool TypeToXml::printedVariable(Variable const * const obj) {
  if (printedVariables.contains(obj)) return true;
  printedVariables.add(obj);
  return false;
}

bool TypeToXml::printedOL(void const * const obj) {
  if (printedOLs.contains(obj)) return true;
  printedOLs.add(obj);
  return false;
}

bool TypeToXml::printedSM(void const * const obj) {
  if (printedSMs.contains(obj)) return true;
  printedSMs.add(obj);
  return false;
}

// **** printing routines for each type system annotation

void TypeToXml::toXml(Type *obj) {
  // idempotency
  if (printedType(obj)) return;

  switch(obj->getTag()) {
  default: xfailure("illegal tag");

  case Type::T_ATOMIC: {
    CVAtomicType *atom = obj->asCVAtomicType();
    openTag(CVAtomicType, atom);
    // **** attributes
    printPtr(atomic, atom->atomic);
    printXml(cv, atom->cv);
    tagEnd;
    // **** subtags
    trav(atom->atomic);
    break;
  }

  case Type::T_POINTER: {
    PointerType *ptr = obj->asPointerType();
    openTag(PointerType, ptr);
    // **** attributes
    printXml(cv, ptr->cv);
    printPtr(atType, ptr->atType);
    tagEnd;
    // **** subtags
    trav(ptr->atType);
    break;
  }

  case Type::T_REFERENCE: {
    ReferenceType *ref = obj->asReferenceType();
    openTag(ReferenceType, ref);
    // **** attributes
    printPtr(atType, ref->atType);
    tagEnd;
    // **** subtags
    trav(ref->atType);
    break;
  }

  case Type::T_FUNCTION: {
    FunctionType *func = obj->asFunctionType();
    openTag(FunctionType, func);
    // **** attributes
    printXml(flags, func->flags);
    printPtr(retType, func->retType);
    printPtr(params, &func->params);
    printPtr(exnSpec, func->exnSpec);
    tagEnd;
    // **** subtags
    trav(func->retType);
    // params
    if (!printedOL(&func->params)) {
      openTagWhole(List_FunctionType_params,  &func->params);
      SFOREACH_OBJLIST_NC(Variable, func->params, iter) {
        travListItem(iter.data());
      }
    }
    // exnSpec
    if (func->exnSpec) {
      toXml_FunctionType_ExnSpec(func->exnSpec);
    }
    break;
  }

  case Type::T_ARRAY: {
    ArrayType *arr = obj->asArrayType();
    openTag(ArrayType, arr);
    // **** attributes
    printPtr(eltType, arr->eltType);
    printXml_int(size, arr->size);
    tagEnd;
    // **** subtags
    trav(arr->eltType);
    break;
  }

  case Type::T_POINTERTOMEMBER: {
    PointerToMemberType *ptm = obj->asPointerToMemberType();
    openTag(PointerToMemberType, ptm);
    // **** attributes
    printPtr(inClassNAT, ptm->inClassNAT);
    printXml(cv, ptm->cv);
    printPtr(atType, ptm->atType);
    tagEnd;
    // **** subtags
    trav(ptm->inClassNAT);
    trav(ptm->atType);
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
    openTag(SimpleType, simple);
    // **** attributes
    printXml(type, simple->type);
    tagEnd;
    break;
  }

  case AtomicType::T_COMPOUND: {
    CompoundType *cpd = obj->asCompoundType();
    openTag(CompoundType, cpd);
    // **** attributes
    // * superclasses
    toXml_NamedAtomicType_properties(cpd);
    toXml_Scope_properties(cpd);
    // * members
    printXml_bool(forward, cpd->forward);
    printXml(keyword, cpd->keyword);
    printPtr(dataMembers, &cpd->dataMembers);
    printPtr(bases, &cpd->bases);
    printPtr(virtualBases, &cpd->virtualBases);
    printPtr(subobj, &cpd->subobj); // embedded object, not a container
    printPtr(conversionOperators, &cpd->conversionOperators);
    printStrRef(instName, cpd->instName);
    printPtrAST(syntax, cpd->syntax);
    printPtr(parameterizingScope, cpd->parameterizingScope);
    printPtr(selfType, cpd->selfType);
    tagEnd;
    // **** subtags
    // * superclasses
    toXml_NamedAtomicType_subtags(cpd);
    toXml_Scope_subtags(cpd);
    // * members
    // dataMembers
    if (!printedOL(&cpd->dataMembers)) {
      openTagWhole(List_CompoundType_dataMembers, &cpd->dataMembers);
      SFOREACH_OBJLIST_NC(Variable, cpd->dataMembers, iter) {
        travListItem(iter.data());
      }
    }
    // bases
    if (!printedOL(&cpd->bases)) {
      openTagWhole(List_CompoundType_bases, &cpd->bases);
      FOREACH_OBJLIST_NC(BaseClass, const_cast<ObjList<BaseClass>&>(cpd->bases), iter) {
        travListItem(iter.data());
      }
    }
    // virtual bases
    if (!printedOL(&cpd->virtualBases)) {
      openTagWhole(List_CompoundType_virtualBases, &cpd->virtualBases);
      FOREACH_OBJLIST_NC(BaseClassSubobj,
                         const_cast<ObjList<BaseClassSubobj>&>(cpd->virtualBases),
                         iter) {
        travListItem(iter.data());
      }
    }
    // subobj
    trav(&cpd->subobj);
    // conversionOperators
    if (!printedOL(&cpd->conversionOperators)) {
      openTagWhole(List_CompoundType_conversionOperators, &cpd->conversionOperators);
      SFOREACH_OBJLIST_NC(Variable, cpd->conversionOperators, iter) {
        travListItem(iter.data());
      }
    }
    // parameterizingScope
    trav(cpd->parameterizingScope);
    // selfType
    trav(cpd->selfType);
    break;
  }

  case AtomicType::T_ENUM: {
    EnumType *e = obj->asEnumType();
    openTag(EnumType, e);
    // **** attributes
    // * superclasses
    toXml_NamedAtomicType_properties(e);
    // * members
    printPtr(valueIndex, &e->valueIndex);
    printXml_int(nextValue, e->nextValue);
    tagEnd;
    // **** subtags
    // * superclasses
    toXml_NamedAtomicType_subtags(e);
    // * members
    // valueIndex
    if (!printedOL(&e->valueIndex)) {
      openTagWhole(NameMap_EnumType_valueIndex, &e->valueIndex);
      for(StringObjDict<EnumType::Value>::Iter iter(e->valueIndex);
          !iter.isDone(); iter.next()) {
        string const &name = iter.key();
        // dsw: do you know how bad it gets if I don't put a
        // const-cast here?
        EnumType::Value *eValue = const_cast<EnumType::Value*>(iter.value());
        openTag_NameMap_Item(name, eValue);
        toXml_EnumType_Value(eValue);
      }
    }
    break;
  }

  case AtomicType::T_TYPEVAR: {
    TypeVariable *tvar = obj->asTypeVariable();
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
    PseudoInstantiation *pseudo = obj->asPseudoInstantiation();
    openTag(PseudoInstantiation, pseudo);
    // **** attributes
    // * superclasses
    toXml_NamedAtomicType_properties(pseudo);
    // * members
    printPtr(primary, pseudo->primary);
    printPtr(args, &pseudo->args);
    tagEnd;
    // **** subtags
    // * superclasses
    toXml_NamedAtomicType_subtags(pseudo);
    // * members
    trav(pseudo->primary);
    if (!printedOL(&pseudo->args)) {
      openTagWhole(List_PseudoInstantiation_args, &pseudo->args);
      FOREACH_OBJLIST_NC(STemplateArgument, pseudo->args, iter) {
        travListItem(iter.data());
      }
    }
    break;
  }

  case AtomicType::T_DEPENDENTQTYPE: {
    DependentQType *dep = obj->asDependentQType();
    openTag(DependentQType, dep);
    // **** attributes
    // * superclasses
    toXml_NamedAtomicType_properties(dep);
    // * members
    printPtr(first, dep->first);
    printPtrAST(rest, dep->rest);
    tagEnd;
    // **** subtags
    // * superclasses
    toXml_NamedAtomicType_subtags(dep);
    // * members
    trav(dep->first);
    // FIX: traverse this AST
//    PQName *rest;
    break;
  }

  }
}

void TypeToXml::toXml(CompoundType *obj) {
  toXml(static_cast<AtomicType*>(obj)); // just disambiguate the overloading
}

void TypeToXml::toXml(Variable *var) {
  // idempotency
  if (printedVariable(var)) return;
  openTag(Variable, var);
  // **** attributes
  printXml_SourceLoc(loc, var->loc);
  printStrRef(name, var->name);
  printPtr(type, var->type);
  printXml(flags, var->flags);
  printPtrAST(value, var->value);
  printPtr(defaultParamType, var->defaultParamType);
  printPtrAST(funcDefn, var->funcDefn);
  printPtr(overload, var->overload);
  printPtr(scope, var->scope);
//    // bits 0-7: result of 'getAccess()'
//    // bits 8-15: result of 'getScopeKind()'
//    // bits 16-31: result of 'getParameterOrdinal()'
//    unsigned intData;
//    Ugh.  Break into 3 parts eventually, but for now serialize as an int.
  newline();
  out << "intData=\"" << toXml_Variable_intData(var->intData) << "\"";
  printPtr(usingAlias_or_parameterizedEntity, var->usingAlias_or_parameterizedEntity);
  printPtr(templInfo, var->templInfo);
  tagEnd;
  // **** subtags
  trav(var->type);
//    trav(var->value);             // FIX: AST so make sure is serialized
  trav(var->defaultParamType);
//    trav(var->funcDefn);          // FIX: AST so make sure is serialized
  // skipping 'overload'; see above
  trav(var->scope);
  trav(var->usingAlias_or_parameterizedEntity);
  trav(var->templInfo);
}

void TypeToXml::toXml_FunctionType_ExnSpec(void /*FunctionType::ExnSpec*/ *exnSpec0) {
  FunctionType::ExnSpec *exnSpec = static_cast<FunctionType::ExnSpec *>(exnSpec0);
  // idempotency
  if (printedType(exnSpec)) return;
  openTag(FunctionType_ExnSpec, exnSpec);
  // **** attributes
  printPtr(types, &exnSpec->types);
  tagEnd;
  // **** subtags
  if (!printedOL(&exnSpec->types)) {
    openTagWhole(List_ExnSpec_types, &exnSpec->types);
    SFOREACH_OBJLIST_NC(Type, exnSpec->types, iter) {
      travListItem(iter.data());
    }
  }
}

void TypeToXml::toXml_EnumType_Value(void /*EnumType::Value*/ *eValue0) {
  EnumType::Value *eValue = static_cast<EnumType::Value *>(eValue0);
  // idempotency
  if (printedType(eValue)) return;
  openTag(EnumType_Value, eValue);
  // **** attributes
  printStrRef(name, eValue->name);
  printPtr(type, eValue->type);
  printXml_int(value, eValue->value);
  printPtr(decl, eValue->decl);
  tagEnd;
  // **** subtags
  trav(eValue->type);
  trav(eValue->decl);
}

void TypeToXml::toXml_NamedAtomicType_properties(NamedAtomicType *nat) {
  printStrRef(name, nat->name);
  printPtr(typedefVar, nat->typedefVar);
  printXml(access, nat->access);
}

void TypeToXml::toXml_NamedAtomicType_subtags(NamedAtomicType *nat) {
  trav(nat->typedefVar);
}

void TypeToXml::toXml(BaseClass *bc) {
  // idempotency
  if (printedType(bc)) return;
  openTag(BaseClass, bc);
  // **** attributes
  toXml_BaseClass_properties(bc);
  tagEnd;
  // **** subtags
  toXml_BaseClass_subtags(bc);
}

void TypeToXml::toXml_BaseClass_properties(BaseClass *bc) {
  printPtr(ct, bc->ct);
  printXml(access, bc->access);
  printXml_bool(isVirtual, bc->isVirtual);
}

void TypeToXml::toXml_BaseClass_subtags(BaseClass *bc) {
  trav(bc->ct);
}

void TypeToXml::toXml(BaseClassSubobj *bc) {
  // idempotency
  if (printedType(bc)) return;
  openTag(BaseClassSubobj, bc);
  // **** attributes
  // * superclass
  toXml_BaseClass_properties(bc);
  // * members
  printPtr(parents, &bc->parents);
  tagEnd;
  // **** subtags
  // * superclass
  toXml_BaseClass_subtags(bc);
  // * members
  if (!printedOL(&bc->parents)) {
    openTagWhole(List_BaseClassSubobj_parents, &bc->parents);
    SFOREACH_OBJLIST_NC(BaseClassSubobj, bc->parents, iter) {
      travListItem(iter.data());
    }
  }
}

void TypeToXml::toXml(Scope *scope) {
  // idempotency
  if (printedScope(scope)) return;
  openTag(Scope, scope);
  // **** attributes
  toXml_Scope_properties(scope);
  tagEnd;
  // **** subtags
  toXml_Scope_subtags(scope);
}

void TypeToXml::toXml_Scope_properties(Scope *scope) {
  printPtr(variables, &scope->variables);
  printPtr(typeTags, &scope->typeTags);
  printXml_bool(canAcceptNames, scope->canAcceptNames);
  printPtr(parentScope, scope->parentScope);
  printXml(scopeKind, scope->scopeKind);
  printPtr(namespaceVar, scope->namespaceVar);
  printPtr(templateParams, &scope->templateParams);
  printPtr(curCompound, scope->curCompound);
  printXml_SourceLoc(curLoc, scope->curLoc);
}

void TypeToXml::toXml_Scope_subtags(Scope *scope) {
  // variables
  if (!printedSM(&scope->variables)) {
    openTagWhole(NameMap_Scope_variables, &scope->variables);
    for(PtrMap<char const, Variable>::Iter iter(scope->variables);
        !iter.isDone();
        iter.adv()) {
      StringRef name = iter.key();
      Variable *var = iter.value();
      openTag_NameMap_Item(name, var);
      trav(var);
    }
  }
  // typeTags
  if (!printedSM(&scope->typeTags)) {
    openTagWhole(NameMap_Scope_typeTags, &scope->typeTags);
    for(PtrMap<char const, Variable>::Iter iter(scope->typeTags);
        !iter.isDone();
        iter.adv()) {
      StringRef name = iter.key();
      Variable *var = iter.value();
      openTag_NameMap_Item(name, var);
      trav(var);
    }
  }
  // parentScope
  trav(scope->parentScope);
  // namespaceVar
  trav(scope->namespaceVar);
  // templateParams
  if (!printedOL(&scope->templateParams)) {
    openTagWhole(List_Scope_templateParams, &scope->templateParams);
    SFOREACH_OBJLIST_NC(Variable, scope->templateParams, iter) {
      travListItem(iter.data());
    }
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

  // curCompound
  trav(scope->curCompound);
}

void TypeToXml::toXml(STemplateArgument *obj) {
  // idempotency
  if (printedType(obj)) return;
  openTag(STemplateArgument, obj);

  // **** attributes
  printXml(kind, obj->kind);
  switch(obj->kind) {
  default: xfailure("illegal STemplateArgument kind"); break;

  case STemplateArgument::STA_TYPE:
    printPtr(t, obj->value.t);
    break;

  case STemplateArgument::STA_INT:
    printXml_int(i, obj->value.i);
    break;

  case STemplateArgument::STA_REFERENCE:
  case STemplateArgument::STA_POINTER:
  case STemplateArgument::STA_MEMBER:
    printPtr(v, obj->value.v);
    break;

  case STemplateArgument::STA_DEPEXPR:
    printPtrAST(e, obj->value.e);
    break;

  case STemplateArgument::STA_TEMPLATE:
    xfailure("template template arguments not implemented");
    break;

  case STemplateArgument::STA_ATOMIC:
    printPtr(at, obj->value.at);
    break;
  }
  tagEnd;

  // **** subtags

  // NOTE: I don't use the trav() macro here because it would be weird
  // to test the member of a union for being NULL; it should have a
  // well-defined value if it is the selected type of the tag.
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
    toXml(const_cast<AtomicType*>(obj->value.at));
    break;
  }
}

void TypeToXml::toXml(TemplateInfo *ti) {
  // idempotency
  if (printedType(ti)) return;
  openTag(TemplateInfo, ti);
  // **** attributes
  // * superclass
  toXml_TemplateParams_properties(ti);
  // * members
  printPtr(var, ti->var);
  printPtr(inheritedParams, &ti->inheritedParams);
  printPtr(instantiationOf, ti->instantiationOf);
  printPtr(instantiations, &ti->instantiations);
  printPtr(specializationOf, ti->specializationOf);
  printPtr(specializations, &ti->specializations);
  printPtr(arguments, &ti->arguments);
  printXml_SourceLoc(instLoc, ti->instLoc);
  printPtr(partialInstantiationOf, ti->partialInstantiationOf);
  printPtr(partialInstantiations, &ti->partialInstantiations);
  printPtr(argumentsToPrimary, &ti->argumentsToPrimary);
  printPtr(defnScope, ti->defnScope);
  printPtr(definitionTemplateInfo, ti->definitionTemplateInfo);
  tagEnd;
  // **** subtags
  // * superclass
  toXml_TemplateParams_subtags(ti);
  // * members
  // var
  trav(ti->var);
  // inheritedParams
  if (!printedOL(&ti->inheritedParams)) {
    openTagWhole(List_TemplateInfo_inheritedParams, &ti->inheritedParams);
    FOREACH_OBJLIST_NC(InheritedTemplateParams, ti->inheritedParams, iter) {
      travListItem(iter.data());
    }
  }
  // instantiationOf
  trav(ti->instantiationOf);
  // instantiations
  if (!printedOL(&ti->instantiations)) {
    openTagWhole(List_TemplateInfo_instantiations, &ti->instantiations);
    SFOREACH_OBJLIST_NC(Variable, ti->instantiations, iter) {
      travListItem(iter.data());
    }
  }
  // specializationOf
  trav(ti->specializationOf);
  // specializations
  if (!printedOL(&ti->specializations)) {
    openTagWhole(List_TemplateInfo_specializations, &ti->specializations);
    SFOREACH_OBJLIST_NC(Variable, ti->specializations, iter) {
      travListItem(iter.data());
    }
  }
  // arguments
  if (!printedOL(&ti->arguments)) {
    openTagWhole(List_TemplateInfo_arguments, &ti->arguments);
    FOREACH_OBJLIST_NC(STemplateArgument, ti->arguments, iter) {
      travListItem(iter.data());
    }
  }
  // partialInstantiationOf
  trav(ti->partialInstantiationOf);
  // partialInstantiations
  if (!printedOL(&ti->partialInstantiations)) {
    openTagWhole(List_TemplateInfo_partialInstantiations, &ti->partialInstantiations);
    SFOREACH_OBJLIST_NC(Variable, ti->partialInstantiations, iter) {
      travListItem(iter.data());
    }
  }
  // argumentsToPrimary
  if (!printedOL(&ti->argumentsToPrimary)) {
    openTagWhole(List_TemplateInfo_argumentsToPrimary, &ti->argumentsToPrimary);
    FOREACH_OBJLIST_NC(STemplateArgument, ti->argumentsToPrimary, iter) {
      travListItem(iter.data());
    }
  }
  // defnScope
  trav(ti->defnScope);
  // definitionTemplateInfo
  trav(ti->definitionTemplateInfo);
}

void TypeToXml::toXml(InheritedTemplateParams *itp) {
  // idempotency
  if (printedType(itp)) return;
  openTag(TemplateInfo, itp);
  // **** attributes
  // * superclass
  toXml_TemplateParams_properties(itp);
  // * members
  printPtr(enclosing, itp->enclosing);
  // **** subtags
  // * superclass
  toXml_TemplateParams_subtags(itp);
  // * members
  trav(itp->enclosing);
}

void TypeToXml::toXml_TemplateParams_properties(TemplateParams *tp) {
  printPtr(params, &tp->params);
}

void TypeToXml::toXml_TemplateParams_subtags(TemplateParams *tp) {
  if (!printedOL(&tp->params)) {
    openTagWhole(List_TemplateParams_params, &tp->params);
    SFOREACH_OBJLIST_NC(Variable, tp->params, iter) {
      travListItem(iter.data());
    }
  }
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
  case XTOK_NameMap_EnumType_valueIndex:           *kindCat = KC_StringSObjDict;break;
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
  case XTOK_loc:                // throw it away for now; FIX: parse it
    break;
  case XTOK_name: obj->name = strTable(parseQuotedString(strValue)); break; 
  case XTOK_type: ul(type); break; 
  case XTOK_flags:
    fromXml(const_cast<DeclFlags&>(obj->flags), parseQuotedString(strValue)); break; 
  case XTOK_value: ul(value); break; 
  case XTOK_defaultParamType: ul(defaultParamType); break; 
  case XTOK_funcDefn: ul(funcDefn); break; 
  case XTOK_overload: ul(overload); break; 
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
  case XTOK_curLoc:             // throw it away for now; FIX: parse it
    break;
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
