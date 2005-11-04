// xml_type_reader.cc            see license.txt for copyright and terms of use

#include "xml_type_reader.h"    // this module
#include "variable.h"           // Variable
#include "strtokp.h"            // StrtokParse
#include "cc_flags.h"           // fromXml(DeclFlags &out, rostring str)
#include "xmlhelp.h"            // fromXml_int() etc.
#include "xml_enum.h"           // XTOK_*


// fromXml for enums

void fromXml(CompoundType::Keyword &out, rostring str) {
  if(0) xfailure("?");
  READENUM(CompoundType::K_STRUCT);
  READENUM(CompoundType::K_CLASS);
  READENUM(CompoundType::K_UNION);
  else xfailure("bad enum string");
}

void fromXml(FunctionFlags &out, rostring str) {
  StrtokParse tok(str, "|");
  for (int i=0; i<tok; ++i) {
    char const * const token = tok[i];
    if(0) xfailure("?");
    READFLAG(FF_NONE);
    READFLAG(FF_METHOD);
    READFLAG(FF_VARARGS);
    READFLAG(FF_CONVERSION);
    READFLAG(FF_CTOR);
    READFLAG(FF_DTOR);
    READFLAG(FF_BUILTINOP);
    READFLAG(FF_NO_PARAM_INFO);
    READFLAG(FF_DEFAULT_ALLOC);
    READFLAG(FF_KANDR_DEFN);
    else xfailure("illegal flag");
  }
}

void fromXml(ScopeKind &out, rostring str) {
  if(0) xfailure("?");
  READENUM(SK_UNKNOWN);
  READENUM(SK_GLOBAL);
  READENUM(SK_PARAMETER);
  READENUM(SK_FUNCTION);
  READENUM(SK_CLASS);
  READENUM(SK_TEMPLATE_PARAMS);
  READENUM(SK_TEMPLATE_ARGS);
  READENUM(SK_NAMESPACE);
  else xfailure("bad enum string");
}

void fromXml(STemplateArgument::Kind &out, rostring str) {
  if(0) xfailure("?");
  READENUM(STemplateArgument::STA_NONE);
  READENUM(STemplateArgument::STA_TYPE);
  READENUM(STemplateArgument::STA_INT);
  READENUM(STemplateArgument::STA_ENUMERATOR);
  READENUM(STemplateArgument::STA_REFERENCE);
  READENUM(STemplateArgument::STA_POINTER);
  READENUM(STemplateArgument::STA_MEMBER);
  READENUM(STemplateArgument::STA_DEPEXPR);
  READENUM(STemplateArgument::STA_TEMPLATE);
  READENUM(STemplateArgument::STA_ATOMIC);
  else xfailure("bad enum string");
}


bool XmlTypeReader::kind2kindCat(int kind, KindCategory *kindCat) {
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
  case XTOK_List_PseudoInstantiation_args:
  case XTOK_List_TemplateInfo_inheritedParams:
  case XTOK_List_TemplateInfo_arguments:
  case XTOK_List_TemplateInfo_argumentsToPrimary:
  case XTOK_List_PQ_qualifier_sargs:
  case XTOK_List_PQ_template_sargs:
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

bool XmlTypeReader::recordKind(int kind, bool& answer) {
  switch(kind) {
  default: return false;        // we don't know this kind

  // **** record these kinds
  case XTOK_CompoundType:
    answer = true;
    return true;
    break;

  // **** do not record these

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
//    case XTOK_CompoundType: handled above
  case XTOK_EnumType:
  case XTOK_EnumType_Value:
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
  // **** Containers
  //   ObjList
  case XTOK_List_CompoundType_bases:
  case XTOK_List_CompoundType_virtualBases:
  case XTOK_List_PseudoInstantiation_args:
  case XTOK_List_TemplateInfo_inheritedParams:
  case XTOK_List_TemplateInfo_arguments:
  case XTOK_List_TemplateInfo_argumentsToPrimary:
  case XTOK_List_PQ_qualifier_sargs:
  case XTOK_List_PQ_template_sargs:
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
  //   StringRefMap
  case XTOK_NameMap_Scope_variables:
  case XTOK_NameMap_Scope_typeTags:
  //   StringSObjDict
  case XTOK_NameMap_EnumType_valueIndex:
    answer = false;
    return true;
    break;
  }
}

bool XmlTypeReader::callOpAssignToEmbeddedObj(void *obj, int kind, void *target) {
  xassert(obj);
  xassert(target);
  switch(kind) {

  default:
    // This handler conflates two situations; one is where kind is a
    // kind from the typesystem, but not an XTOK_BaseClassSubobj,
    // which is an error; the other is where kind is just not from the
    // typesystem, which should just be a return false so that another
    // XmlReader will be attempted.  However the first situation
    // should not be handled by any of the other XmlReaders either and
    // so should also result in an error, albeit perhaps not as exact
    // of one as it could have been.  I just don't want to put a huge
    // switch statement here for all the other kinds in the type
    // system.
    return false;
    break;

  case XTOK_BaseClassSubobj:
    BaseClassSubobj *obj1 = reinterpret_cast<BaseClassSubobj*>(obj);
    BaseClassSubobj *target1 = reinterpret_cast<BaseClassSubobj*>(target);
    obj1->operator=(*target1);
    return true;
    break;

  }
}

bool XmlTypeReader::upcastToWantedType(void *obj, int objKind, void **target, int targetKind) {
  xassert(obj);
  xassert(target);

  // classes where something interesting happens
  if (objKind == XTOK_CompoundType) {
    CompoundType *tmp = reinterpret_cast<CompoundType*>(obj);
    if (targetKind == XTOK_CompoundType) {
      *target = tmp;
    } else if (targetKind == XTOK_Scope) {
      // upcast to a Scope
      *target = static_cast<Scope*>(tmp);
    } else if (targetKind == XTOK_AtomicType) {
      // upcast to an AtomicType
      *target = static_cast<AtomicType*>(tmp);
    } else if (targetKind == XTOK_NamedAtomicType) {
      // upcast to an NamedAtomicType
      *target = static_cast<NamedAtomicType*>(tmp);
    }
    return true;
  } else {
    // This handler conflates two situations; one is where objKind is
    // a kind from the typesystem, but not an XTOK_CompoundType, which
    // is an error; the other is where objKind is just not from the
    // typesystem, which should just be a return false so that another
    // XmlReader will be attempted.  However the first situation
    // should not be handled by any of the other XmlReaders either and
    // so should also result in an error, albeit perhaps not as exact
    // of one as it could have been.  I just don't want to put a huge
    // switch statement here for all the other kinds in the type
    // system.
    return false;
  }
}

bool XmlTypeReader::convertList2FakeList(ASTList<char> *list, int listKind, void **target) {
  xfailure("should not be called during Type parsing there are no FakeLists in the Type System");
  return false;
}

bool XmlTypeReader::convertList2SObjList(ASTList<char> *list, int listKind, void **target) {
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

bool XmlTypeReader::convertList2ObjList(ASTList<char> *list, int listKind, void **target) {
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
  case XTOK_List_PseudoInstantiation_args:
  case XTOK_List_PQ_qualifier_sargs:
  case XTOK_List_PQ_template_sargs:
    convertList(ObjList, STemplateArgument);
    break;

  }
  return true;
}

bool XmlTypeReader::convertList2ArrayStack(ASTList<char> *list, int listKind, void **target) {
  xfailure("should not be called during Type parsing there are no ArrayStacks in the Type System");
  return false;
}

bool XmlTypeReader::convertNameMap2StringRefMap
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

bool XmlTypeReader::convertNameMap2StringSObjDict
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

void *XmlTypeReader::ctorNodeFromTag(int tag) {
  switch(tag) {
  default: return NULL; break;
  case 0: userError("unexpected file termination while looking for an open tag name");

  // **** Types
  case XTOK_CVAtomicType: return new CVAtomicType((AtomicType*)0, (CVFlags)0);
  case XTOK_PointerType: return new PointerType((CVFlags)0, (Type*)0);
  case XTOK_ReferenceType: return new ReferenceType((Type*)0);
  case XTOK_FunctionType: return new FunctionType((Type*)0);
  case XTOK_FunctionType_ExnSpec: return new FunctionType::ExnSpec();
  case XTOK_ArrayType: return new ArrayType((ReadXML&)*this); // call the special ctor
  case XTOK_PointerToMemberType:
    return new PointerToMemberType((ReadXML&)*this); // call the special ctor

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
    return new ASTList<InheritedTemplateParams>();
  case XTOK_List_TemplateInfo_arguments:
  case XTOK_List_TemplateInfo_argumentsToPrimary:
  case XTOK_List_PseudoInstantiation_args:
  case XTOK_List_PQ_qualifier_sargs:
  case XTOK_List_PQ_template_sargs:
    return new ASTList<STemplateArgument>();

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

// **************** registerStringToken

bool XmlTypeReader::registerStringToken(void *target, int kind, char const *yytext0) {
  return false;
}

// **************** registerAttribute

#define regAttr(TYPE) \
  registerAttr_##TYPE((TYPE*)target, attr, yytext0)

bool XmlTypeReader::registerAttribute(void *target, int kind, int attr, char const *yytext0) {
  switch(kind) {
  default: return false; break;

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

  return true;
}

void XmlTypeReader::registerAttr_CVAtomicType(CVAtomicType *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a CVAtomicType"); break;
  case XTOK_atomic: ul(atomic, XTOK_AtomicType); break;
  case XTOK_cv: fromXml(obj->cv, xmlAttrDeQuote(strValue)); break;
  }
}

void XmlTypeReader::registerAttr_PointerType(PointerType *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a PointerType"); break;
  case XTOK_cv: fromXml(obj->cv, xmlAttrDeQuote(strValue)); break;
  case XTOK_atType: ul(atType, XTOK_Type); break;
  }
}

void XmlTypeReader::registerAttr_ReferenceType(ReferenceType *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a ReferenceType"); break;
  case XTOK_atType: ul(atType, XTOK_Type); break;
  }
}

void XmlTypeReader::registerAttr_FunctionType(FunctionType *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a FunctionType"); break;
  case XTOK_flags: fromXml(obj->flags, xmlAttrDeQuote(strValue)); break;
  case XTOK_retType: ul(retType, XTOK_Type); break;
  case XTOK_params: ulList(_List, params, XTOK_List_FunctionType_params); break;
  case XTOK_exnSpec: ul(exnSpec, XTOK_FunctionType_ExnSpec); break;
  }
}

void XmlTypeReader::registerAttr_FunctionType_ExnSpec
  (FunctionType::ExnSpec *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a FunctionType_ExnSpec"); break;
  case XTOK_types: ulList(_List, types, XTOK_List_ExnSpec_types); break;
  }
}

void XmlTypeReader::registerAttr_ArrayType(ArrayType *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a ArrayType"); break;
  case XTOK_eltType: ul(eltType, XTOK_Type); break;
  case XTOK_size: fromXml_int(obj->size, xmlAttrDeQuote(strValue)); break;
  }
}

void XmlTypeReader::registerAttr_PointerToMemberType
  (PointerToMemberType *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a PointerToMemberType"); break;
  case XTOK_inClassNAT:
    ul(inClassNAT, XTOK_NamedAtomicType); break;
  case XTOK_cv: fromXml(obj->cv, xmlAttrDeQuote(strValue)); break;
  case XTOK_atType: ul(atType, XTOK_Type); break;
  }
}

bool XmlTypeReader::registerAttr_Variable_super(Variable *obj, int attr, char const *strValue) {
  switch(attr) {
  default: return false; break; // we didn't find it
  case XTOK_loc: fromXml_SourceLoc(obj->loc, xmlAttrDeQuote(strValue)); break;
  case XTOK_name: obj->name = manager->strTable(xmlAttrDeQuote(strValue)); break;
  case XTOK_type: ul(type, XTOK_Type); break;
  case XTOK_flags:
    fromXml(const_cast<DeclFlags&>(obj->flags), xmlAttrDeQuote(strValue)); break;
  case XTOK_value: ul(value, XTOK_Expression); break;
  case XTOK_defaultParamType: ul(defaultParamType, XTOK_Type); break;
  case XTOK_funcDefn: ul(funcDefn, XTOK_Function); break;
  case XTOK_overload: ul(overload, XTOK_OverloadSet); break;
  case XTOK_scope: ul(scope, XTOK_Scope); break;

  // these three fields are an abstraction; here we pretend they are
  // real
  case XTOK_access:
    AccessKeyword access;
    fromXml(access, xmlAttrDeQuote(strValue));
    obj->setAccess(access);
    break;
  case XTOK_scopeKind:
    ScopeKind scopeKind;
    fromXml(scopeKind, xmlAttrDeQuote(strValue));
    obj->setScopeKind(scopeKind);
    break;
  case XTOK_parameterOrdinal:
    int parameterOrdinal;
    fromXml_int(parameterOrdinal, xmlAttrDeQuote(strValue));
    obj->setParameterOrdinal(parameterOrdinal);
    break;

  case XTOK_fullyQualifiedMangledName:
    // FIX: For now throw it away; I suppose perhaps we should check
    // that it is indeed the fully qualified name, but we would have
    // to be done de-serializing the whole object heirarcy first and
    // we aren't even done de-serializing the Variable object.
    break;

  case XTOK_usingAlias_or_parameterizedEntity:
    ul(usingAlias_or_parameterizedEntity, XTOK_Variable); break;
  case XTOK_templInfo: ul(templInfo, XTOK_TemplateInfo); break;
  }

  return true;                  // found it
}

void XmlTypeReader::registerAttr_Variable(Variable *obj, int attr, char const *strValue) {
  // "superclass": just re-use our own superclass code for ourself
  if (registerAttr_Variable_super(obj, attr, strValue)) return;
  // shouldn't get here
  userError("illegal attribute for a Variable");
}

bool XmlTypeReader::registerAttr_NamedAtomicType_super
  (NamedAtomicType *obj, int attr, char const *strValue) {
  switch(attr) {
  default: return false; break; // we didn't find it
  case XTOK_name: obj->name = manager->strTable(xmlAttrDeQuote(strValue)); break;
  case XTOK_typedefVar: ul(typedefVar, XTOK_Variable); break;
  case XTOK_access: fromXml(obj->access, xmlAttrDeQuote(strValue)); break;
  }
  return true;                  // found it
}

void XmlTypeReader::registerAttr_SimpleType(SimpleType *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a SimpleType"); break;
  case XTOK_type:
    // NOTE: this 'type' is not a type node, but basically an enum,
    // and thus is handled more like a flag would be.
    fromXml(const_cast<SimpleTypeId&>(obj->type), xmlAttrDeQuote(strValue));
    break;
  }
}

void XmlTypeReader::registerAttr_CompoundType(CompoundType *obj, int attr, char const *strValue) {
  // superclasses
  if (registerAttr_NamedAtomicType_super(obj, attr, strValue)) return;
  if (registerAttr_Scope_super(obj, attr, strValue)) return;

  switch(attr) {
  default: userError("illegal attribute for a CompoundType"); break;
  case XTOK_forward: fromXml_bool(obj->forward, xmlAttrDeQuote(strValue)); break;
  case XTOK_keyword: fromXml(obj->keyword, xmlAttrDeQuote(strValue)); break;
  case XTOK_dataMembers: ulList(_List, dataMembers, XTOK_List_CompoundType_dataMembers); break;
  case XTOK_bases: ulList(_List, bases, XTOK_List_CompoundType_bases); break;
  case XTOK_virtualBases: ulList(_List, virtualBases, XTOK_List_CompoundType_virtualBases); break;
  case XTOK_subobj: ulEmbed(subobj, XTOK_BaseClassSubobj); break;
  case XTOK_conversionOperators:
    ulList(_List, conversionOperators, XTOK_List_CompoundType_conversionOperators); break;
  case XTOK_instName: obj->instName = manager->strTable(xmlAttrDeQuote(strValue)); break;
  case XTOK_syntax: ul(syntax, XTOK_TS_classSpec); break;
  case XTOK_parameterizingScope: ul(parameterizingScope, XTOK_Scope); break;
  case XTOK_selfType: ul(selfType, XTOK_Type); break;
  }
}

void XmlTypeReader::registerAttr_EnumType(EnumType *obj, int attr, char const *strValue) {
  // superclass
  if (registerAttr_NamedAtomicType_super(obj, attr, strValue)) return;

  switch(attr) {
  default: userError("illegal attribute for a EnumType"); break;
  case XTOK_valueIndex: ulList(_NameMap, valueIndex, XTOK_NameMap_EnumType_valueIndex); break;
  case XTOK_nextValue: fromXml_int(obj->nextValue, xmlAttrDeQuote(strValue)); break;
  }
}

void XmlTypeReader::registerAttr_EnumType_Value
  (EnumType::Value *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a EnumType"); break;
  case XTOK_name: obj->name = manager->strTable(xmlAttrDeQuote(strValue)); break;
  case XTOK_type: ul(type, XTOK_EnumType); break; // NOTE: 'type' here is actually an atomic type
  case XTOK_value: fromXml_int(obj->value, xmlAttrDeQuote(strValue)); break;
  case XTOK_decl: ul(decl, XTOK_Variable); break;
  }
}

void XmlTypeReader::registerAttr_TypeVariable(TypeVariable *obj, int attr, char const *strValue) {
  // superclass
  if (registerAttr_NamedAtomicType_super(obj, attr, strValue)) return;
  // shouldn't get here
  userError("illegal attribute for a TypeVariable");
}

void XmlTypeReader::registerAttr_PseudoInstantiation
  (PseudoInstantiation *obj, int attr, char const *strValue) {
  // superclass
  if (registerAttr_NamedAtomicType_super(obj, attr, strValue)) return;

  switch(attr) {
  default: userError("illegal attribute for a PsuedoInstantiation"); break;
  case XTOK_primary: ul(primary, XTOK_CompoundType); break;
  case XTOK_args: ulList(_List, args, XTOK_List_PseudoInstantiation_args); break;
  }
}

void XmlTypeReader::registerAttr_DependentQType
  (DependentQType *obj, int attr, char const *strValue) {
  // superclass
  if (registerAttr_NamedAtomicType_super(obj, attr, strValue)) return;

  switch(attr) {
  default: userError("illegal attribute for a DependentQType"); break;
  case XTOK_first: ul(first, XTOK_AtomicType);
  case XTOK_rest: ul(rest, XTOK_PQName);
  }
}

bool XmlTypeReader::registerAttr_Scope_super(Scope *obj, int attr, char const *strValue) {
  switch(attr) {
  default: return false; break; // we didn't find it
  case XTOK_variables: ulList(_NameMap, variables, XTOK_NameMap_Scope_variables); break;
  case XTOK_typeTags: ulList(_NameMap, typeTags, XTOK_NameMap_Scope_typeTags); break;
  case XTOK_canAcceptNames: fromXml_bool(obj->canAcceptNames, xmlAttrDeQuote(strValue)); break;
  case XTOK_parentScope: ul(parentScope, XTOK_Scope); break;
  case XTOK_scopeKind: fromXml(obj->scopeKind, xmlAttrDeQuote(strValue)); break;
  case XTOK_namespaceVar: ul(namespaceVar, XTOK_Variable); break;
  case XTOK_templateParams: ulList(_List, templateParams, XTOK_List_Scope_templateParams); break;
  case XTOK_curCompound: ul(curCompound, XTOK_CompoundType); break;
  case XTOK_curLoc: fromXml_SourceLoc(obj->curLoc, xmlAttrDeQuote(strValue)); break;
  }
  return true;                  // found it
}

void XmlTypeReader::registerAttr_Scope(Scope *obj, int attr, char const *strValue) {
  // "superclass": just re-use our own superclass code for ourself
  if (registerAttr_Scope_super(obj, attr, strValue)) return;
  // shouldn't get here
  userError("illegal attribute for a Scope");
}

bool XmlTypeReader::registerAttr_BaseClass_super(BaseClass *obj, int attr, char const *strValue) {
  switch(attr) {
  default: return false; break;
  case XTOK_ct: ul(ct, XTOK_CompoundType); break;
  case XTOK_access: fromXml(obj->access, xmlAttrDeQuote(strValue)); break;
  case XTOK_isVirtual: fromXml_bool(obj->isVirtual, xmlAttrDeQuote(strValue)); break;
  }
  return true;
}

void XmlTypeReader::registerAttr_BaseClass(BaseClass *obj, int attr, char const *strValue) {
  // "superclass": just re-use our own superclass code for ourself
  if (registerAttr_BaseClass_super(obj, attr, strValue)) return;
  // shouldn't get here
  userError("illegal attribute for a BaseClass");
}

void XmlTypeReader::registerAttr_BaseClassSubobj
  (BaseClassSubobj *obj, int attr, char const *strValue) {
  // "superclass": just re-use our own superclass code for ourself
  if (registerAttr_BaseClass_super(obj, attr, strValue)) return;

  switch(attr) {
  default: userError("illegal attribute for a BaseClassSubobj"); break;
  case XTOK_parents: ulList(_List, parents, XTOK_List_BaseClassSubobj_parents); break;
  }
}

void XmlTypeReader::registerAttr_OverloadSet(OverloadSet *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a OverloadSet"); break;
  case XTOK_set: ulList(_List, set, XTOK_List_OverloadSet_set); break;
  }
}

void XmlTypeReader::registerAttr_STemplateArgument
  (STemplateArgument *obj, int attr, char const *strValue) {
  switch(attr) {
  default: userError("illegal attribute for a STemplateArgument"); break;
  case XTOK_kind: fromXml(obj->kind, xmlAttrDeQuote(strValue)); break;
  // exactly one of these must show up as it is a union; I don't check that though
  case XTOK_t: ul(value.t, XTOK_Type); break;
  case XTOK_i: fromXml_int(obj->value.i, xmlAttrDeQuote(strValue)); break;
  case XTOK_v: ul(value.v, XTOK_Variable); break;
  case XTOK_e: ul(value.e, XTOK_Expression); break;
  case XTOK_at: ul(value.at, XTOK_AtomicType); break;
  }
}

bool XmlTypeReader::registerAttr_TemplateParams_super
  (TemplateParams *obj, int attr, char const *strValue) {
  switch(attr) {
  default: return false; break; // we didn't find it
  case XTOK_params: ulList(_List, params, XTOK_List_TemplateParams_params); break;
  }
  return true;
}

void XmlTypeReader::registerAttr_TemplateInfo(TemplateInfo *obj, int attr, char const *strValue) {
  // superclass
  if (registerAttr_TemplateParams_super(obj, attr, strValue)) return;

  switch(attr) {
  default: userError("illegal attribute for a TemplateInfo"); break;
  case XTOK_var: ul(var, XTOK_Variable); break;
  case XTOK_inheritedParams:
    ulList(_List, inheritedParams, XTOK_List_TemplateInfo_inheritedParams); break;
  case XTOK_instantiationOf:
    ul(instantiationOf, XTOK_Variable); break;
  case XTOK_instantiations:
    ulList(_List, instantiations, XTOK_List_TemplateInfo_instantiations); break;
  case XTOK_specializationOf:
    ul(specializationOf, XTOK_Variable); break;
  case XTOK_specializations:
    ulList(_List, specializations, XTOK_List_TemplateInfo_specializations); break;
  case XTOK_arguments:
    ulList(_List, arguments, XTOK_List_TemplateInfo_arguments); break;
  case XTOK_instLoc:
    fromXml_SourceLoc(obj->instLoc, xmlAttrDeQuote(strValue)); break;
  case XTOK_partialInstantiationOf:
    ul(partialInstantiationOf, XTOK_Variable); break;
  case XTOK_partialInstantiations:
    ulList(_List, partialInstantiations, XTOK_List_TemplateInfo_partialInstantiations); break;
  case XTOK_argumentsToPrimary:
    ulList(_List, argumentsToPrimary, XTOK_List_TemplateInfo_argumentsToPrimary); break;
  case XTOK_defnScope:
    ul(defnScope, XTOK_Scope); break;
  case XTOK_definitionTemplateInfo:
    ul(definitionTemplateInfo, XTOK_TemplateInfo); break;
  }
}

void XmlTypeReader::registerAttr_InheritedTemplateParams
  (InheritedTemplateParams *obj, int attr, char const *strValue) {
  // superclass
  if (registerAttr_TemplateParams_super(obj, attr, strValue)) return;

  switch(attr) {
  default: userError("illegal attribute for a InheritedTemplateParams"); break;
  case XTOK_enclosing:
    ul(enclosing, XTOK_CompoundType); break;
  }
}
