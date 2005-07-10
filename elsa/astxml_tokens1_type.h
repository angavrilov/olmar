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

  /* Some containers; I no longer care about order */
  XTOK_CompoundType_bases_List,
  XTOK_CompoundType_virtualBases_List,
  XTOK_FunctionType_params_List,
  XTOK_CompoundType_dataMembers_List,
  XTOK_CompoundType_conversionOperators_List,
  XTOK_Scope_variables_Map,
  XTOK_Scope_typeTags_Map,
