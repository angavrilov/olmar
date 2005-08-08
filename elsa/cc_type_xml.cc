// cc_type_xml.cc            see license.txt for copyright and terms of use

#include "cc_type_xml.h"        // this module
#include "variable.h"           // Variable
#include "cc_flags.h"           // fromXml(DeclFlags &out, rostring str)
#include "asthelp.h"            // xmlPrintPointer
#include "xmlhelp.h"            // toXml_int() etc.

#include "strutil.h"            // parseQuotedString
#include "astxml_lexer.h"       // AstXmlLexer


// to/from Xml for enums
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


// **** macros and functions to assist in serializing Type System
// annotations

string idPrefixAST(void const * const) {return "AST";}

#define identity0(PREFIX, NAME, TEMPL) \
TEMPL char const *idPrefix(NAME const * const) {return #PREFIX;} \
TEMPL bool TypeToXml::printed(NAME const * const obj) { \
  if (printedSet ##PREFIX.contains(obj)) return true; \
  printedSet ##PREFIX.add(obj); \
  return false; \
}
#define identity(PREFIX, NAME) identity0(PREFIX, NAME, )
#define identityTempl(PREFIX, NAME) identity0(PREFIX, NAME, template<class T>)

identity(TY, Type)
identity(TY, AtomicType)
identity(TY, CompoundType)
identity(TY, FunctionType::ExnSpec)
identity(TY, EnumType::Value)
identity(TY, BaseClass)
identity(TY, Scope)
identity(TY, Variable)
identity(TY, OverloadSet)
identity(TY, STemplateArgument)
identity(TY, TemplateInfo)
identity(TY, InheritedTemplateParams)
identityTempl(OL, ObjList<T>)
identityTempl(OL, SObjList<T>)
identityTempl(NM, StringRefMap<T>)
identityTempl(NM, StringObjDict<T>)

#undef identity0
#undef identity
#undef identityTempl

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


// this bizarre function is actually useful for something
inline void const *addr(void const *x) {
  return x;
}

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

#define printPtr(BASE, MEM)    printThing(MEM, idPrefix((BASE)->MEM),     (BASE)->MEM,  addr)
#define printPtrAST(BASE, MEM) printThing(MEM, idPrefixAST((BASE)->MEM),  (BASE)->MEM,  addr)
// print an embedded thing
#define printEmbed(BASE, MEM)  printThing(MEM, idPrefix(&((BASE)->MEM)),&((BASE)->MEM), addr)

// for unions where the member name does not match the xml name and we
// don't want the 'if'
#define printPtrUnion(BASE, MEM, NAME) printThing0(NAME, idPrefix((BASE)->MEM), (BASE)->MEM, addr)
// this is only used in one place
#define printPtrASTUnion(BASE, MEM, NAME) printThing0(NAME, "AST", (BASE)->MEM, addr)

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

#define travObjList0(BASE, BASETYPE, FIELD, FIELDTYPE, ITER_MACRO, LISTKIND) \
do { \
  if (!printed(&BASE->FIELD)) { \
    openTagWhole(List_ ##BASETYPE ##_ ##FIELD, &BASE->FIELD); \
    ITER_MACRO(FIELDTYPE, const_cast<LISTKIND<FIELDTYPE>&>(BASE->FIELD), iter) { \
      travListItem(iter.data()); \
    } \
  } \
} while(0)

#define travObjList_S(BASE, BASETYPE, FIELD, FIELDTYPE) \
travObjList0(BASE, BASETYPE, FIELD, FIELDTYPE, SFOREACH_OBJLIST_NC, SObjList)
#define travObjList(BASE, BASETYPE, FIELD, FIELDTYPE) \
travObjList0(BASE, BASETYPE, FIELD, FIELDTYPE, FOREACH_OBJLIST_NC, ObjList)

#define travPtrMap(BASE, BASETYPE, FIELD, FIELDTYPE) \
do { \
  if (!printed(&BASE->FIELD)) { \
    openTagWhole(NameMap_ ##BASETYPE ##_ ##FIELD, &BASE->FIELD); \
    for(PtrMap<char const, FIELDTYPE>::Iter iter(BASE->FIELD); \
        !iter.isDone(); \
        iter.adv()) { \
      StringRef name = iter.key(); \
      FIELDTYPE *var = iter.value(); \
      openTag_NameMap_Item(name, var); \
      trav(var); \
    } \
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


// -------------------- TypeToXml -------------------

void TypeToXml::newline() {
  out << "\n";
  if (indent) {
    for (int i=0; i<depth; ++i) cout << " ";
  }
}

void TypeToXml::toXml(Type *t) {
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

void TypeToXml::toXml(AtomicType *atom) {
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
    // FIX: traverse this AST
//    PQName *rest;
    break;
  }

  }
}

void TypeToXml::toXml(CompoundType *cpd) {
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
  printPtr(cpd, subobj);
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
  trav(cpd->subobj);
  travObjList_S(cpd, CompoundType, conversionOperators, Variable);
  trav(cpd->parameterizingScope);
  trav(cpd->selfType);
}

void TypeToXml::toXml(Variable *var) {
  // idempotency
  if (printed(var)) return;
  openTag(Variable, var);
  // **** attributes
  printXml_SourceLoc(loc, var->loc);
  printStrRef(name, var->name);
  printPtr(var, type);
  printXml(flags, var->flags);
  printPtrAST(var, value);
  printPtr(var, defaultParamType);
  printPtrAST(var, funcDefn);
  printPtr(var, overload);
  printPtr(var, scope);
//    // bits 0-7: result of 'getAccess()'
//    // bits 8-15: result of 'getScopeKind()'
//    // bits 16-31: result of 'getParameterOrdinal()'
//    unsigned intData;
//    Ugh.  Break into 3 parts eventually, but for now serialize as an int.
  newline();
  out << "intData=\"" << toXml_Variable_intData(var->intData) << "\"";
  printPtr(var, usingAlias_or_parameterizedEntity);
  printPtr(var, templInfo);
  tagEnd;
  // **** subtags
  trav(var->type);
//    trav(var->value);             // FIX: AST so make sure is serialized
  trav(var->defaultParamType);
//    trav(var->funcDefn);          // FIX: AST so make sure is serialized
  trav(var->overload);
  trav(var->scope);
  trav(var->usingAlias_or_parameterizedEntity);
  trav(var->templInfo);
}

void TypeToXml::toXml_FunctionType_ExnSpec(void /*FunctionType::ExnSpec*/ *exnSpec0) {
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

void TypeToXml::toXml_EnumType_Value(void /*EnumType::Value*/ *eValue0) {
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

void TypeToXml::toXml_NamedAtomicType_properties(NamedAtomicType *nat) {
  printStrRef(name, nat->name);
  printPtr(nat, typedefVar);
  printXml(access, nat->access);
}

void TypeToXml::toXml_NamedAtomicType_subtags(NamedAtomicType *nat) {
  trav(nat->typedefVar);
}

void TypeToXml::toXml(OverloadSet *oload) {
  // idempotency
  if (printed(oload)) return;
  openTag(OverloadSet, oload);
  // **** attributes
  printEmbed(oload, set);
  tagEnd;
  // **** subtags
  travObjList_S(oload, OverloadSet, set, Variable);
}

void TypeToXml::toXml(BaseClass *bc) {
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

void TypeToXml::toXml_BaseClass_properties(BaseClass *bc) {
  printPtr(bc, ct);
  printXml(access, bc->access);
  printXml_bool(isVirtual, bc->isVirtual);
}

void TypeToXml::toXml_BaseClass_subtags(BaseClass *bc) {
  trav(bc->ct);
}

void TypeToXml::toXml(BaseClassSubobj *bc) {
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

void TypeToXml::toXml(Scope *scope) {
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

void TypeToXml::toXml_Scope_properties(Scope *scope) {
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

void TypeToXml::toXml_Scope_subtags(Scope *scope) {
  travPtrMap(scope, Scope, variables, Variable);
  travPtrMap(scope, Scope, typeTags, Variable);
  trav(scope->parentScope);
  trav(scope->namespaceVar);
  travObjList_S(scope, Scope, templateParams, Variable);
  trav(scope->curCompound);
}

void TypeToXml::toXml(STemplateArgument *sta) {
  // idempotency
  if (printed(sta)) return;
  openTag(STemplateArgument, sta);

  // **** attributes
  printXml(kind, sta->kind);
  switch(sta->kind) {
  default: xfailure("illegal STemplateArgument kind"); break;

  case STemplateArgument::STA_TYPE:
    printPtrUnion(sta, value.t, t);
    break;

  case STemplateArgument::STA_INT:
    printXml_int(i, sta->value.i);
    break;

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

  case STemplateArgument::STA_REFERENCE:
  case STemplateArgument::STA_POINTER:
  case STemplateArgument::STA_MEMBER:
    toXml(sta->value.v);
    break;

  case STemplateArgument::STA_DEPEXPR:
    // FIX: what the hell should we do?  the same remark is made in
    // the traverse() method code at this point
    break;

  case STemplateArgument::STA_TEMPLATE:
    xfailure("template template arguments not implemented");
    break;

  case STemplateArgument::STA_ATOMIC:
    toXml(const_cast<AtomicType*>(sta->value.at));
    break;
  }
}

void TypeToXml::toXml(TemplateInfo *ti) {
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

void TypeToXml::toXml(InheritedTemplateParams *itp) {
  // idempotency
  if (printed(itp)) return;
  openTag(TemplateInfo, itp);
  // **** attributes
  // * superclass
  toXml_TemplateParams_properties(itp);
  // * members
  printPtr(itp, enclosing);
  // **** subtags
  // * superclass
  toXml_TemplateParams_subtags(itp);
  // * members
  trav(itp->enclosing);
}

void TypeToXml::toXml_TemplateParams_properties(TemplateParams *tp) {
  printEmbed(tp, params);
}

void TypeToXml::toXml_TemplateParams_subtags(TemplateParams *tp) {
  travObjList_S(tp, TemplateParams, params, Variable);
}


// -------------------- ReadXml_Type -------------------

#define convertList(LISTTYPE, ITEMTYPE) \
do { \
  LISTTYPE<ITEMTYPE> *ret = reinterpret_cast<LISTTYPE<ITEMTYPE>*>(target); \
  xassert(ret->isEmpty()); \
  FOREACH_ASTLIST_NC(ITEMTYPE, reinterpret_cast<ASTList<ITEMTYPE>&>(*list), iter) { \
    ret->prepend(iter.data()); \
  } \
  ret->reverse(); \
} while(0)

#define convertNameMap(MAPTYPE, ITEMTYPE) \
do { \
  MAPTYPE<ITEMTYPE> *ret = reinterpret_cast<MAPTYPE<ITEMTYPE>*>(target); \
  xassert(ret->isEmpty()); \
  for(StringRefMap<ITEMTYPE>::Iter \
        iter(reinterpret_cast<StringRefMap<ITEMTYPE>&>(*map)); \
      !iter.isDone(); iter.adv()) { \
    ret->add(iter.key(), iter.value()); \
  } \
} while(0)

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
  case XTOK_CVAtomicType:
  case XTOK_PointerType:
  case XTOK_ReferenceType:
  case XTOK_FunctionType:
  case XTOK_FunctionType_ExnSpec:
  case XTOK_ArrayType:
  case XTOK_PointerToMemberType:
  // AtomicTypes
  case XTOK_SimpleType:
  case XTOK_CompoundType:
  case XTOK_EnumType:
  case XTOK_TypeVariable:
  case XTOK_PseudoInstantiation:
  case XTOK_DependentQType:
  // Other
  case XTOK_Variable:
  case XTOK_Scope:
  case XTOK_BaseClass:
  case XTOK_BaseClassSubobj:
  case XTOK_OverloadSet:
  case XTOK_STemplateArgument:
  case XTOK_TemplateInfo:
  case XTOK_InheritedTemplateParams:
    *kindCat = KC_Node;
    break;

  // **** Containers

  //   ObjList
  case XTOK_List_CompoundType_bases:
  case XTOK_List_CompoundType_virtualBases:
  case XTOK_List_TemplateInfo_inheritedParams:
  case XTOK_List_TemplateInfo_arguments:
  case XTOK_List_TemplateInfo_argumentsToPrimary:
    *kindCat = KC_ObjList;
    break;

  //   SObjList
  case XTOK_List_FunctionType_params:
  case XTOK_List_CompoundType_dataMembers:
  case XTOK_List_CompoundType_conversionOperators:
  case XTOK_List_BaseClassSubobj_parents:
  case XTOK_List_ExnSpec_types:
  case XTOK_List_Scope_templateParams:
  case XTOK_List_OverloadSet_set:
  case XTOK_List_TemplateInfo_instantiations:
  case XTOK_List_TemplateInfo_specializations:
  case XTOK_List_TemplateInfo_partialInstantiations:
  case XTOK_List_TemplateParams_params:
    *kindCat = KC_SObjList;
    break;

  //   StringRefMap
  case XTOK_NameMap_Scope_variables:
  case XTOK_NameMap_Scope_typeTags:
    *kindCat = KC_StringRefMap;
    break;

  //   StringSObjDict
  case XTOK_NameMap_EnumType_valueIndex:
    *kindCat = KC_StringSObjDict;
    break;
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
  case XTOK_List_CompoundType_conversionOperators:
  case XTOK_List_Scope_templateParams:
  case XTOK_List_OverloadSet_set:
  case XTOK_List_TemplateInfo_instantiations:
  case XTOK_List_TemplateInfo_specializations:
  case XTOK_List_TemplateInfo_partialInstantiations:
  case XTOK_List_TemplateParams_params:
    convertList(SObjList, Variable);
    break;

  case XTOK_List_BaseClassSubobj_parents:
    convertList(SObjList, BaseClassSubobj);
    break;

  case XTOK_List_ExnSpec_types:
    convertList(SObjList, Type);
    break;

  }
  return true;
}

bool ReadXml_Type::convertList2ObjList(ASTList<char> *list, int listKind, void **target) {
  // NOTE: ObjList only has constant-time prepend, not constant-time
  // append, hence the prepend() and reverse().
  xassert(list);

  switch(listKind) {
  default: return false;        // we did not find a matching tag

  case XTOK_List_CompoundType_bases:
    convertList(ObjList, BaseClass);
    break;

  case XTOK_List_CompoundType_virtualBases:
    convertList(ObjList, BaseClassSubobj);
    break;

  case XTOK_List_TemplateInfo_inheritedParams:
    convertList(ObjList, InheritedTemplateParams);
    break;

  case XTOK_List_TemplateInfo_arguments:
  case XTOK_List_TemplateInfo_argumentsToPrimary:
    convertList(ObjList, STemplateArgument);
    break;

  }
  return true;
}

bool ReadXml_Type::convertNameMap2StringRefMap
  (StringRefMap<char> *map, int mapKind, void *target) {
  xassert(map);
  switch(mapKind) {
  default: return false;        // we did not find a matching tag

  case XTOK_NameMap_Scope_variables:
  case XTOK_NameMap_Scope_typeTags:
    convertNameMap(StringRefMap, Variable);
    break;

  }
  return true;
}

bool ReadXml_Type::convertNameMap2StringSObjDict
  (StringRefMap<char> *map, int mapKind, void *target) {
  xassert(map);
  switch(mapKind) {
  default: return false;        // we did not find a matching tag

  case XTOK_NameMap_EnumType_valueIndex:
    convertNameMap(StringSObjDict, EnumType::Value);
    break;

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
  case XTOK_OverloadSet: return new OverloadSet();
  case XTOK_STemplateArgument: return new STemplateArgument();
  case XTOK_TemplateInfo: return new TemplateInfo((SourceLoc)0);
  case XTOK_InheritedTemplateParams: return new InheritedTemplateParams((CompoundType*)0);

  // **** Containers
  // ObjList
  case XTOK_List_CompoundType_bases:
    return new ASTList<BaseClass>();
  case XTOK_List_CompoundType_virtualBases:
    return new ASTList<BaseClassSubobj>();
  case XTOK_List_TemplateInfo_inheritedParams:
    return new ObjList<InheritedTemplateParams>();
  case XTOK_List_TemplateInfo_arguments:
  case XTOK_List_TemplateInfo_argumentsToPrimary:
    return new ObjList<STemplateArgument>();

  // SObjList
  case XTOK_List_BaseClassSubobj_parents:
    return new ASTList<BaseClassSubobj>();
  case XTOK_List_ExnSpec_types:
    return new ASTList<Type>();
  case XTOK_List_FunctionType_params:
  case XTOK_List_CompoundType_dataMembers:
  case XTOK_List_CompoundType_conversionOperators:
  case XTOK_List_Scope_templateParams:
  case XTOK_List_OverloadSet_set:
  case XTOK_List_TemplateInfo_instantiations:
  case XTOK_List_TemplateInfo_specializations:
  case XTOK_List_TemplateInfo_partialInstantiations:
  case XTOK_List_TemplateParams_params:
    return new ASTList<Variable>();

  // StringRefMap
  case XTOK_NameMap_Scope_variables:
  case XTOK_NameMap_Scope_typeTags:
    return new StringRefMap<Variable>();
  case XTOK_NameMap_EnumType_valueIndex:
    return new StringRefMap<EnumType::Value>();
  }
}

// **************** registerAttribute

#define regAttr(TYPE) \
  registerAttr_##TYPE((TYPE*)target, attr, yytext0)

void ReadXml_Type::registerAttribute(void *target, int kind, int attr, char const *yytext0) {
  switch(kind) {
  default: xfailure("illegal kind");

  // **** Types
  case XTOK_CVAtomicType: regAttr(CVAtomicType);               break;
  case XTOK_PointerType: regAttr(PointerType);                 break;
  case XTOK_ReferenceType: regAttr(ReferenceType);             break;
  case XTOK_FunctionType: regAttr(FunctionType);               break;
  case XTOK_ArrayType: regAttr(ArrayType);                     break;
  case XTOK_PointerToMemberType: regAttr(PointerToMemberType); break;
  case XTOK_FunctionType_ExnSpec:
    registerAttr_FunctionType_ExnSpec
      ((FunctionType::ExnSpec*)target, attr, yytext0);         break;

  // **** Atomic Types
  case XTOK_SimpleType: regAttr(SimpleType);                   break;
  case XTOK_CompoundType: regAttr(CompoundType);               break;
  case XTOK_EnumType: regAttr(EnumType);                       break;
  case XTOK_TypeVariable: regAttr(TypeVariable);               break;
  case XTOK_PseudoInstantiation: regAttr(PseudoInstantiation); break;
  case XTOK_DependentQType: regAttr(DependentQType);           break;
  case XTOK_EnumType_Value:
    registerAttr_EnumType_Value
      ((EnumType::Value*)target, attr, yytext0);               break;

  // **** Other
  case XTOK_Variable: regAttr(Variable);                       break;
  case XTOK_Scope: regAttr(Scope);                             break;
  case XTOK_BaseClass: regAttr(BaseClass);                     break;
  case XTOK_BaseClassSubobj: regAttr(BaseClassSubobj);         break;
  case XTOK_OverloadSet: regAttr(OverloadSet);                 break;
  case XTOK_STemplateArgument: regAttr(STemplateArgument);     break;
  case XTOK_TemplateInfo: regAttr(TemplateInfo);               break;
  case XTOK_InheritedTemplateParams:
    regAttr(InheritedTemplateParams);                          break;
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
  case XTOK_size: fromXml_int(obj->size, parseQuotedString(strValue)); break; 
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
  case XTOK_templInfo: ul(templInfo); break; 
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
  case XTOK_nextValue: fromXml_int(obj->nextValue, parseQuotedString(strValue)); break; 
  }
}

void ReadXml_Type::registerAttr_EnumType_Value
  (EnumType::Value *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a EnumType"); break; 
  case XTOK_name: obj->name = strTable(parseQuotedString(strValue)); break; 
  case XTOK_type: ul(type); break; // NOTE: 'type' here is actually an atomic type
  case XTOK_value: fromXml_int(obj->value, parseQuotedString(strValue)); break; 
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

void ReadXml_Type::registerAttr_OverloadSet(OverloadSet *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a OverloadSet"); break; 
  case XTOK_set: ulList(_List, set, XTOK_List_OverloadSet_set); break; 
  }
}

void ReadXml_Type::registerAttr_STemplateArgument
  (STemplateArgument *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a STemplateArgument"); break; 
  case XTOK_kind:
  case XTOK_t: ul(value.t); break;
  case XTOK_i: fromXml_int(obj->value.i, parseQuotedString(strValue)); break; 
  case XTOK_v: ul(value.v); break;
  case XTOK_e: ul(value.e); break;
  case XTOK_at: ul(value.at); break;
  }
}

void ReadXml_Type::registerAttr_TemplateInfo(TemplateInfo *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a TemplateInfo"); break; 
  case XTOK_var: ul(var); break;
  case XTOK_inheritedParams:
    ulList(_List, inheritedParams, XTOK_List_TemplateInfo_inheritedParams); break;
  case XTOK_instantiationOf:
    ul(instantiationOf); break;
  case XTOK_instantiations:
    ulList(_List, instantiations, XTOK_List_TemplateInfo_instantiations); break;
  case XTOK_specializationOf:
    ul(specializationOf); break;
  case XTOK_specializations:
    ulList(_List, specializations, XTOK_List_TemplateInfo_specializations); break;
  case XTOK_arguments:
    ulList(_List, arguments, XTOK_List_TemplateInfo_arguments); break;
  case XTOK_instLoc:            // throw it away for now; FIX: parse it
    break;
  case XTOK_partialInstantiationOf:
    ul(partialInstantiationOf); break;
  case XTOK_partialInstantiations:
    ulList(_List, partialInstantiations, XTOK_List_TemplateInfo_partialInstantiations); break;
  case XTOK_argumentsToPrimary:
    ulList(_List, argumentsToPrimary, XTOK_List_TemplateInfo_argumentsToPrimary); break;
  case XTOK_defnScope:
    ul(defnScope); break;
  case XTOK_definitionTemplateInfo:
    ul(definitionTemplateInfo); break;
  case XTOK_params:
    ulList(_List, params, XTOK_List_TemplateParams_params); break;
  }
}

void ReadXml_Type::registerAttr_InheritedTemplateParams
  (InheritedTemplateParams *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a InheritedTemplateParams"); break; 
  case XTOK_params:
    ulList(_List, params, XTOK_List_TemplateParams_params); break;
  case XTOK_enclosing:
    ul(enclosing); break;
  }
}
