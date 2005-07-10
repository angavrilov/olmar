  /* Type nodes */
"CVAtomicType" return tok(XTOK_CVAtomicType);
"PointerType" return tok(XTOK_PointerType);
"ReferenceType" return tok(XTOK_ReferenceType);
"FunctionType" return tok(XTOK_FunctionType);
"ArrayType" return tok(XTOK_ArrayType);
"PointerToMemberType" return tok(XTOK_PointerToMemberType);
"atomic" return tok(XTOK_atomic);
"atType" return tok(XTOK_atType);
"retType" return tok(XTOK_retType);
"eltType" return tok(XTOK_eltType);
"inClassNAT" return tok(XTOK_inClassNAT);
  /* these two are duplicated with the AST tokens; this is a bit of a hack */
  /*  "cv" return tok(XTOK_cv); */
  /*  "size" return tok(XTOK_size); */

  /* AtomicType nodes */
"SimpleType" return tok(XTOK_SimpleType);
"CompoundType" return tok(XTOK_CompoundType);
"EnumType" return tok(XTOK_EnumType);
"TypeVariable" return tok(XTOK_TypeVariable);
"PseudoInstantiation" return tok(XTOK_PseudoInstantiation);
"DependentQType" return tok(XTOK_DependentQType);
"typedefVar" return tok(XTOK_typedefVar);
"forward" return tok(XTOK_forward);
"dataMembers" return tok(XTOK_dataMembers);
"virtualBases" return tok(XTOK_virtualBases);
"subobj" return tok(XTOK_subobj);
"conversionOperators" return tok(XTOK_conversionOperators);
"instName" return tok(XTOK_instName);
"syntax" return tok(XTOK_syntax);
"parameterizingScope" return tok(XTOK_parameterizingScope);
"selfType" return tok(XTOK_selfType);
"valueIndex" return tok(XTOK_valueIndex);
"nextValue" return tok(XTOK_nextValue);
"primary" return tok(XTOK_primary);
"first" return tok(XTOK_first);
  /* these are already defined */
  /*  "name" return tok(XTOK_name); */
  /*  "access" return tok(XTOK_access); */
  /*  "type" return tok(XTOK_type); */
  /*  "bases" return tok(XTOK_bases); */
  /*  "args" return tok(XTOK_args); */
  /*  "rest" return tok(XTOK_rest); */

  /* Some containers; I no longer care about order */
"CompoundType_bases_List" return tok(XTOK_CompoundType_bases_List);
"CompoundType_virtualBases_List" return tok(XTOK_CompoundType_virtualBases_List);
"FunctionType_params_List" return tok(XTOK_FunctionType_params_List);
"CompoundType_dataMembers_List" return tok(XTOK_CompoundType_dataMembers_List);
"CompoundType_conversionOperators_List" return tok(XTOK_CompoundType_conversionOperators_List);
"Scope_variables_Map" return tok(XTOK_Scope_variables_Map);
"Scope_typeTags_Map" return tok(XTOK_Scope_typeTags_Map);
