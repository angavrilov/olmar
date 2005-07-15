  /* Type nodes */
  XTOK_CVAtomicType, // "CVAtomicType"
  XTOK_PointerType, // "PointerType"
  XTOK_ReferenceType, // "ReferenceType"
  XTOK_FunctionType, // "FunctionType"
  XTOK_ArrayType, // "ArrayType"
  XTOK_PointerToMemberType, // "PointerToMemberType"
    XTOK_atomic, // "atomic"
  XTOK_atType, // "atType"
  XTOK_retType, // "retType"
  XTOK_eltType, // "eltType"
  XTOK_inClassNAT, // "inClassNAT"
    // these two are already in the AST tokens; this is a bit of a
    // hack
//    XTOK_cv, // "cv"
//    XTOK_size, // "size"

  /* AtomicType nodes */
  XTOK_SimpleType, // "SimpleType"
  XTOK_CompoundType, // "CompoundType"
  XTOK_EnumType, // "EnumType"
  XTOK_EnumType_Value, // "EnumType_Value"
  XTOK_TypeVariable, // "TypeVariable"
  XTOK_PseudoInstantiation, // "PseudoInstantiation"
  XTOK_DependentQType, // "DependentQType"
  XTOK_typedefVar, // "typedefVar"
  XTOK_forward, // "forward"
  XTOK_dataMembers, // "dataMembers"
  XTOK_virtualBases, // "virtualBases"
  XTOK_subobj, // "subobj"
  XTOK_conversionOperators, // "conversionOperators"
  XTOK_instName, // "instName"
  XTOK_syntax, // "syntax"
  XTOK_parameterizingScope, // "parameterizingScope"
  XTOK_selfType, // "selfType"
  XTOK_valueIndex, // "valueIndex"
  XTOK_nextValue, // "nextValue"
  XTOK_primary, // "primary"
  XTOK_first, // "first"
    // these are already defined somewhere above
//    XTOK_name, // "name"
//    XTOK_access, // "access"
//    XTOK_type, // "type"
//    XTOK_bases, // "bases"
//    XTOK_args, // "args"
//    XTOK_rest, // "rest"

    /* Other */
    XTOK_Variable, // "Variable"
    XTOK_Scope, // "Scope"
    XTOK_BaseClass, // "BaseClass"
    XTOK_BaseClassSubobj, // "BaseClassSubobj"
    XTOK_flags, // "flags"
    XTOK_value, // "value"
    XTOK_defaultParamType, // "defaultParamType"
    XTOK_funcDefn, // "funcDefn"
    XTOK_scope, // "scope"
    XTOK_intData, // "intData"
    XTOK_canAcceptNames, // "canAcceptNames"
    XTOK_parentScope, // "parentScope"
    XTOK_scopeKind, // "scopeKind"
    XTOK_namespaceVar, // "namespaceVar"
    XTOK_curCompound, // "curCompound"
    XTOK_ct, // "ct"
    XTOK_variables, // "variables"
    XTOK_typeTags, // "typeTags"
    XTOK_parents, // "parents"

  /* Some containers; I no longer care about order */
  XTOK_List_CompoundType_bases,
  XTOK_List_CompoundType_virtualBases,

  XTOK_List_FunctionType_params,
  XTOK_List_CompoundType_dataMembers,
  XTOK_List_CompoundType_conversionOperators,
  XTOK_List_BaseClassSubobj_parents,

  XTOK_NameMap_Scope_variables,
  XTOK_NameMap_Scope_typeTags,
  XTOK_NameMap_EnumType_valueIndex,
  XTOK_Name, // "__Name"
