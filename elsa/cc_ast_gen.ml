(* cc_ast_gen.ml *)
(* *** DO NOT EDIT *** *)
(* generated automatically by astgen, from cc.ast *)
(* active extension modules: gnu_attribute_hack.ast cc_tcheck.ast cc_print.ast cfg.ast cc_elaborate.ast gnu.ast kandr.ast ml_ctype.ast *)

open Cc_ast_gen_type;;   (* ast type definition *)

(* *********************************************************************
 * *********** Constructor callbacks ******************
 * ********************************************************************* *)


(* *********************************************************************
 * *********** callbacks for TranslationUnit ******************
 * ********************************************************************* *)

let create_TranslationUnit_tuple a0 a1 a2 = (a0, a1, a2)


(* *********************************************************************
 * *********** callbacks for TopForm ******************
 * ********************************************************************* *)

let create_TF_decl_constructor a0 a1 a2 = TF_decl(a0, a1, a2)

let create_TF_func_constructor a0 a1 a2 = TF_func(a0, a1, a2)

let create_TF_template_constructor a0 a1 a2 = TF_template(a0, a1, a2)

let create_TF_explicitInst_constructor a0 a1 a2 a3 = TF_explicitInst(a0, a1, a2, a3)

let create_TF_linkage_constructor a0 a1 a2 a3 = TF_linkage(a0, a1, a2, a3)

let create_TF_one_linkage_constructor a0 a1 a2 a3 = TF_one_linkage(a0, a1, a2, a3)

let create_TF_asm_constructor a0 a1 a2 = TF_asm(a0, a1, a2)

let create_TF_namespaceDefn_constructor a0 a1 a2 a3 = TF_namespaceDefn(a0, a1, a2, a3)

let create_TF_namespaceDecl_constructor a0 a1 a2 = TF_namespaceDecl(a0, a1, a2)


(* *********************************************************************
 * *********** callbacks for Function ******************
 * ********************************************************************* *)

let create_Function_tuple a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 a10 a11 = (a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11)


(* *********************************************************************
 * *********** callbacks for MemberInit ******************
 * ********************************************************************* *)

let create_MemberInit_tuple a0 a1 a2 a3 a4 a5 a6 a7 = (a0, a1, a2, a3, a4, a5, a6, a7)


(* *********************************************************************
 * *********** callbacks for Declaration ******************
 * ********************************************************************* *)

let create_Declaration_tuple a0 a1 a2 a3 = (a0, a1, a2, a3)


(* *********************************************************************
 * *********** callbacks for ASTTypeId ******************
 * ********************************************************************* *)

let create_ASTTypeId_tuple a0 a1 a2 = (a0, a1, a2)


(* *********************************************************************
 * *********** callbacks for PQName ******************
 * ********************************************************************* *)

let create_PQ_qualifier_constructor a0 a1 a2 a3 a4 a5 a6 = PQ_qualifier(a0, a1, a2, a3, a4, a5, a6)

let create_PQ_name_constructor a0 a1 a2 = PQ_name(a0, a1, a2)

let create_PQ_operator_constructor a0 a1 a2 a3 = PQ_operator(a0, a1, a2, a3)

let create_PQ_template_constructor a0 a1 a2 a3 a4 = PQ_template(a0, a1, a2, a3, a4)

let create_PQ_variable_constructor a0 a1 a2 = PQ_variable(a0, a1, a2)


(* *********************************************************************
 * *********** callbacks for TypeSpecifier ******************
 * ********************************************************************* *)

let create_TS_name_constructor a0 a1 a2 a3 a4 a5 a6 = TS_name(a0, a1, a2, a3, a4, a5, a6)

let create_TS_simple_constructor a0 a1 a2 a3 = TS_simple(a0, a1, a2, a3)

let create_TS_elaborated_constructor a0 a1 a2 a3 a4 a5 = TS_elaborated(a0, a1, a2, a3, a4, a5)

let create_TS_classSpec_constructor a0 a1 a2 a3 a4 a5 a6 a7 = TS_classSpec(a0, a1, a2, a3, a4, a5, a6, a7)

let create_TS_enumSpec_constructor a0 a1 a2 a3 a4 a5 = TS_enumSpec(a0, a1, a2, a3, a4, a5)

let create_TS_type_constructor a0 a1 a2 a3 = TS_type(a0, a1, a2, a3)

let create_TS_typeof_constructor a0 a1 a2 a3 = TS_typeof(a0, a1, a2, a3)


(* *********************************************************************
 * *********** callbacks for BaseClassSpec ******************
 * ********************************************************************* *)

let create_BaseClassSpec_tuple a0 a1 a2 a3 a4 = (a0, a1, a2, a3, a4)


(* *********************************************************************
 * *********** callbacks for Enumerator ******************
 * ********************************************************************* *)

let create_Enumerator_tuple a0 a1 a2 a3 a4 a5 = (a0, a1, a2, a3, a4, a5)


(* *********************************************************************
 * *********** callbacks for MemberList ******************
 * ********************************************************************* *)

let create_MemberList_tuple a0 a1 = (a0, a1)


(* *********************************************************************
 * *********** callbacks for Member ******************
 * ********************************************************************* *)

let create_MR_decl_constructor a0 a1 a2 = MR_decl(a0, a1, a2)

let create_MR_func_constructor a0 a1 a2 = MR_func(a0, a1, a2)

let create_MR_access_constructor a0 a1 a2 = MR_access(a0, a1, a2)

let create_MR_usingDecl_constructor a0 a1 a2 = MR_usingDecl(a0, a1, a2)

let create_MR_template_constructor a0 a1 a2 = MR_template(a0, a1, a2)


(* *********************************************************************
 * *********** callbacks for ExceptionSpec ******************
 * ********************************************************************* *)

let create_ExceptionSpec_tuple a0 a1 = (a0, a1)


(* *********************************************************************
 * *********** callbacks for OperatorName ******************
 * ********************************************************************* *)

let create_ON_newDel_constructor a0 a1 a2 = ON_newDel(a0, a1, a2)

let create_ON_operator_constructor a0 a1 = ON_operator(a0, a1)

let create_ON_conversion_constructor a0 a1 = ON_conversion(a0, a1)


(* *********************************************************************
 * *********** callbacks for Statement ******************
 * ********************************************************************* *)

let create_S_skip_constructor a0 a1 = S_skip(a0, a1)

let create_S_label_constructor a0 a1 a2 a3 = S_label(a0, a1, a2, a3)

let create_S_case_constructor a0 a1 a2 a3 a4 = S_case(a0, a1, a2, a3, a4)

let create_S_default_constructor a0 a1 a2 = S_default(a0, a1, a2)

let create_S_expr_constructor a0 a1 a2 = S_expr(a0, a1, a2)

let create_S_compound_constructor a0 a1 a2 = S_compound(a0, a1, a2)

let create_S_if_constructor a0 a1 a2 a3 a4 = S_if(a0, a1, a2, a3, a4)

let create_S_switch_constructor a0 a1 a2 a3 = S_switch(a0, a1, a2, a3)

let create_S_while_constructor a0 a1 a2 a3 = S_while(a0, a1, a2, a3)

let create_S_doWhile_constructor a0 a1 a2 a3 = S_doWhile(a0, a1, a2, a3)

let create_S_for_constructor a0 a1 a2 a3 a4 a5 = S_for(a0, a1, a2, a3, a4, a5)

let create_S_break_constructor a0 a1 = S_break(a0, a1)

let create_S_continue_constructor a0 a1 = S_continue(a0, a1)

let create_S_return_constructor a0 a1 a2 a3 = S_return(a0, a1, a2, a3)

let create_S_goto_constructor a0 a1 a2 = S_goto(a0, a1, a2)

let create_S_decl_constructor a0 a1 a2 = S_decl(a0, a1, a2)

let create_S_try_constructor a0 a1 a2 a3 = S_try(a0, a1, a2, a3)

let create_S_asm_constructor a0 a1 a2 = S_asm(a0, a1, a2)

let create_S_namespaceDecl_constructor a0 a1 a2 = S_namespaceDecl(a0, a1, a2)

let create_S_function_constructor a0 a1 a2 = S_function(a0, a1, a2)

let create_S_rangeCase_constructor a0 a1 a2 a3 a4 a5 a6 = S_rangeCase(a0, a1, a2, a3, a4, a5, a6)

let create_S_computedGoto_constructor a0 a1 a2 = S_computedGoto(a0, a1, a2)


(* *********************************************************************
 * *********** callbacks for Condition ******************
 * ********************************************************************* *)

let create_CN_expr_constructor a0 a1 = CN_expr(a0, a1)

let create_CN_decl_constructor a0 a1 = CN_decl(a0, a1)


(* *********************************************************************
 * *********** callbacks for Handler ******************
 * ********************************************************************* *)

let create_Handler_tuple a0 a1 a2 a3 a4 a5 a6 = (a0, a1, a2, a3, a4, a5, a6)


(* *********************************************************************
 * *********** callbacks for Expression ******************
 * ********************************************************************* *)

let create_E_boolLit_constructor a0 a1 a2 = E_boolLit(a0, a1, a2)

let create_E_intLit_constructor a0 a1 a2 a3 = E_intLit(a0, a1, a2, a3)

let create_E_floatLit_constructor a0 a1 a2 a3 = E_floatLit(a0, a1, a2, a3)

let create_E_stringLit_constructor a0 a1 a2 a3 a4 = E_stringLit(a0, a1, a2, a3, a4)

let create_E_charLit_constructor a0 a1 a2 a3 = E_charLit(a0, a1, a2, a3)

let create_E_this_constructor a0 a1 a2 = E_this(a0, a1, a2)

let create_E_variable_constructor a0 a1 a2 a3 a4 = E_variable(a0, a1, a2, a3, a4)

let create_E_funCall_constructor a0 a1 a2 a3 a4 = E_funCall(a0, a1, a2, a3, a4)

let create_E_constructor_constructor a0 a1 a2 a3 a4 a5 a6 = E_constructor(a0, a1, a2, a3, a4, a5, a6)

let create_E_fieldAcc_constructor a0 a1 a2 a3 a4 = E_fieldAcc(a0, a1, a2, a3, a4)

let create_E_sizeof_constructor a0 a1 a2 a3 = E_sizeof(a0, a1, a2, a3)

let create_E_unary_constructor a0 a1 a2 a3 = E_unary(a0, a1, a2, a3)

let create_E_effect_constructor a0 a1 a2 a3 = E_effect(a0, a1, a2, a3)

let create_E_binary_constructor a0 a1 a2 a3 a4 = E_binary(a0, a1, a2, a3, a4)

let create_E_addrOf_constructor a0 a1 a2 = E_addrOf(a0, a1, a2)

let create_E_deref_constructor a0 a1 a2 = E_deref(a0, a1, a2)

let create_E_cast_constructor a0 a1 a2 a3 a4 = E_cast(a0, a1, a2, a3, a4)

let create_E_cond_constructor a0 a1 a2 a3 a4 = E_cond(a0, a1, a2, a3, a4)

let create_E_sizeofType_constructor a0 a1 a2 a3 a4 = E_sizeofType(a0, a1, a2, a3, a4)

let create_E_assign_constructor a0 a1 a2 a3 a4 = E_assign(a0, a1, a2, a3, a4)

let create_E_new_constructor a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 = E_new(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9)

let create_E_delete_constructor a0 a1 a2 a3 a4 a5 = E_delete(a0, a1, a2, a3, a4, a5)

let create_E_throw_constructor a0 a1 a2 a3 a4 = E_throw(a0, a1, a2, a3, a4)

let create_E_keywordCast_constructor a0 a1 a2 a3 a4 = E_keywordCast(a0, a1, a2, a3, a4)

let create_E_typeidExpr_constructor a0 a1 a2 = E_typeidExpr(a0, a1, a2)

let create_E_typeidType_constructor a0 a1 a2 = E_typeidType(a0, a1, a2)

let create_E_grouping_constructor a0 a1 a2 = E_grouping(a0, a1, a2)

let create_E_arrow_constructor a0 a1 a2 a3 = E_arrow(a0, a1, a2, a3)

let create_E_statement_constructor a0 a1 a2 = E_statement(a0, a1, a2)

let create_E_compoundLit_constructor a0 a1 a2 a3 = E_compoundLit(a0, a1, a2, a3)

let create_E___builtin_constant_p_constructor a0 a1 a2 a3 = E___builtin_constant_p(a0, a1, a2, a3)

let create_E___builtin_va_arg_constructor a0 a1 a2 a3 a4 = E___builtin_va_arg(a0, a1, a2, a3, a4)

let create_E_alignofType_constructor a0 a1 a2 a3 = E_alignofType(a0, a1, a2, a3)

let create_E_alignofExpr_constructor a0 a1 a2 a3 = E_alignofExpr(a0, a1, a2, a3)

let create_E_gnuCond_constructor a0 a1 a2 a3 = E_gnuCond(a0, a1, a2, a3)

let create_E_addrOfLabel_constructor a0 a1 a2 = E_addrOfLabel(a0, a1, a2)


(* *********************************************************************
 * *********** callbacks for FullExpression ******************
 * ********************************************************************* *)

let create_FullExpression_tuple a0 a1 a2 = (a0, a1, a2)


(* *********************************************************************
 * *********** callbacks for ArgExpression ******************
 * ********************************************************************* *)

let create_ArgExpression_tuple a0 a1 = (a0, a1)


(* *********************************************************************
 * *********** callbacks for ArgExpressionListOpt ******************
 * ********************************************************************* *)

let create_ArgExpressionListOpt_tuple a0 a1 = (a0, a1)


(* *********************************************************************
 * *********** callbacks for Initializer ******************
 * ********************************************************************* *)

let create_IN_expr_constructor a0 a1 a2 a3 = IN_expr(a0, a1, a2, a3)

let create_IN_compound_constructor a0 a1 a2 a3 = IN_compound(a0, a1, a2, a3)

let create_IN_ctor_constructor a0 a1 a2 a3 a4 a5 = IN_ctor(a0, a1, a2, a3, a4, a5)

let create_IN_designated_constructor a0 a1 a2 a3 a4 = IN_designated(a0, a1, a2, a3, a4)


(* *********************************************************************
 * *********** callbacks for TemplateDeclaration ******************
 * ********************************************************************* *)

let create_TD_func_constructor a0 a1 a2 = TD_func(a0, a1, a2)

let create_TD_decl_constructor a0 a1 a2 = TD_decl(a0, a1, a2)

let create_TD_tmember_constructor a0 a1 a2 = TD_tmember(a0, a1, a2)


(* *********************************************************************
 * *********** callbacks for TemplateParameter ******************
 * ********************************************************************* *)

let create_TP_type_constructor a0 a1 a2 a3 a4 a5 = TP_type(a0, a1, a2, a3, a4, a5)

let create_TP_nontype_constructor a0 a1 a2 a3 a4 = TP_nontype(a0, a1, a2, a3, a4)


(* *********************************************************************
 * *********** callbacks for TemplateArgument ******************
 * ********************************************************************* *)

let create_TA_type_constructor a0 a1 a2 = TA_type(a0, a1, a2)

let create_TA_nontype_constructor a0 a1 a2 = TA_nontype(a0, a1, a2)

let create_TA_templateUsed_constructor a0 a1 = TA_templateUsed(a0, a1)


(* *********************************************************************
 * *********** callbacks for NamespaceDecl ******************
 * ********************************************************************* *)

let create_ND_alias_constructor a0 a1 a2 = ND_alias(a0, a1, a2)

let create_ND_usingDecl_constructor a0 a1 = ND_usingDecl(a0, a1)

let create_ND_usingDir_constructor a0 a1 = ND_usingDir(a0, a1)


(* *********************************************************************
 * *********** callbacks for Declarator ******************
 * ********************************************************************* *)

let create_Declarator_tuple a0 a1 a2 a3 a4 a5 a6 a7 = (a0, a1, a2, a3, a4, a5, a6, a7)


(* *********************************************************************
 * *********** callbacks for IDeclarator ******************
 * ********************************************************************* *)

let create_D_name_constructor a0 a1 a2 = D_name(a0, a1, a2)

let create_D_pointer_constructor a0 a1 a2 a3 = D_pointer(a0, a1, a2, a3)

let create_D_reference_constructor a0 a1 a2 = D_reference(a0, a1, a2)

let create_D_func_constructor a0 a1 a2 a3 a4 a5 a6 a7 = D_func(a0, a1, a2, a3, a4, a5, a6, a7)

let create_D_array_constructor a0 a1 a2 a3 a4 = D_array(a0, a1, a2, a3, a4)

let create_D_bitfield_constructor a0 a1 a2 a3 a4 = D_bitfield(a0, a1, a2, a3, a4)

let create_D_ptrToMember_constructor a0 a1 a2 a3 a4 = D_ptrToMember(a0, a1, a2, a3, a4)

let create_D_grouping_constructor a0 a1 a2 = D_grouping(a0, a1, a2)


(* *********************************************************************
 * *********** callbacks for FullExpressionAnnot ******************
 * ********************************************************************* *)

let create_FullExpressionAnnot_tuple a0 a1 = (a0, a1)


(* *********************************************************************
 * *********** callbacks for ASTTypeof ******************
 * ********************************************************************* *)

let create_TS_typeof_expr_constructor a0 a1 a2 = TS_typeof_expr(a0, a1, a2)

let create_TS_typeof_type_constructor a0 a1 a2 = TS_typeof_type(a0, a1, a2)


(* *********************************************************************
 * *********** callbacks for Designator ******************
 * ********************************************************************* *)

let create_FieldDesignator_constructor a0 a1 a2 = FieldDesignator(a0, a1, a2)

let create_SubscriptDesignator_constructor a0 a1 a2 a3 a4 a5 = SubscriptDesignator(a0, a1, a2, a3, a4, a5)


(* *********************************************************************
 * *********** callbacks for AttributeSpecifierList ******************
 * ********************************************************************* *)

let create_AttributeSpecifierList_cons_constructor a0 a1 a2 = AttributeSpecifierList_cons(a0, a1, a2)


(* *********************************************************************
 * *********** callbacks for AttributeSpecifier ******************
 * ********************************************************************* *)

let create_AttributeSpecifier_cons_constructor a0 a1 a2 = AttributeSpecifier_cons(a0, a1, a2)


(* *********************************************************************
 * *********** callbacks for Attribute ******************
 * ********************************************************************* *)

let create_AT_empty_constructor a0 a1 = AT_empty(a0, a1)

let create_AT_word_constructor a0 a1 a2 = AT_word(a0, a1, a2)

let create_AT_func_constructor a0 a1 a2 a3 = AT_func(a0, a1, a2, a3)


(* *********************************************************************
 * *********** Register callbacks ******************
 * ********************************************************************* *)

let register_cc_ast_callbacks () =
  Callback.register
    "create_TranslationUnit_tuple"
    create_TranslationUnit_tuple;
  Callback.register
    "create_TF_decl_constructor"
    create_TF_decl_constructor;
  Callback.register
    "create_TF_func_constructor"
    create_TF_func_constructor;
  Callback.register
    "create_TF_template_constructor"
    create_TF_template_constructor;
  Callback.register
    "create_TF_explicitInst_constructor"
    create_TF_explicitInst_constructor;
  Callback.register
    "create_TF_linkage_constructor"
    create_TF_linkage_constructor;
  Callback.register
    "create_TF_one_linkage_constructor"
    create_TF_one_linkage_constructor;
  Callback.register
    "create_TF_asm_constructor"
    create_TF_asm_constructor;
  Callback.register
    "create_TF_namespaceDefn_constructor"
    create_TF_namespaceDefn_constructor;
  Callback.register
    "create_TF_namespaceDecl_constructor"
    create_TF_namespaceDecl_constructor;
  Callback.register
    "create_Function_tuple"
    create_Function_tuple;
  Callback.register
    "create_MemberInit_tuple"
    create_MemberInit_tuple;
  Callback.register
    "create_Declaration_tuple"
    create_Declaration_tuple;
  Callback.register
    "create_ASTTypeId_tuple"
    create_ASTTypeId_tuple;
  Callback.register
    "create_PQ_qualifier_constructor"
    create_PQ_qualifier_constructor;
  Callback.register
    "create_PQ_name_constructor"
    create_PQ_name_constructor;
  Callback.register
    "create_PQ_operator_constructor"
    create_PQ_operator_constructor;
  Callback.register
    "create_PQ_template_constructor"
    create_PQ_template_constructor;
  Callback.register
    "create_PQ_variable_constructor"
    create_PQ_variable_constructor;
  Callback.register
    "create_TS_name_constructor"
    create_TS_name_constructor;
  Callback.register
    "create_TS_simple_constructor"
    create_TS_simple_constructor;
  Callback.register
    "create_TS_elaborated_constructor"
    create_TS_elaborated_constructor;
  Callback.register
    "create_TS_classSpec_constructor"
    create_TS_classSpec_constructor;
  Callback.register
    "create_TS_enumSpec_constructor"
    create_TS_enumSpec_constructor;
  Callback.register
    "create_TS_type_constructor"
    create_TS_type_constructor;
  Callback.register
    "create_TS_typeof_constructor"
    create_TS_typeof_constructor;
  Callback.register
    "create_BaseClassSpec_tuple"
    create_BaseClassSpec_tuple;
  Callback.register
    "create_Enumerator_tuple"
    create_Enumerator_tuple;
  Callback.register
    "create_MemberList_tuple"
    create_MemberList_tuple;
  Callback.register
    "create_MR_decl_constructor"
    create_MR_decl_constructor;
  Callback.register
    "create_MR_func_constructor"
    create_MR_func_constructor;
  Callback.register
    "create_MR_access_constructor"
    create_MR_access_constructor;
  Callback.register
    "create_MR_usingDecl_constructor"
    create_MR_usingDecl_constructor;
  Callback.register
    "create_MR_template_constructor"
    create_MR_template_constructor;
  Callback.register
    "create_ExceptionSpec_tuple"
    create_ExceptionSpec_tuple;
  Callback.register
    "create_ON_newDel_constructor"
    create_ON_newDel_constructor;
  Callback.register
    "create_ON_operator_constructor"
    create_ON_operator_constructor;
  Callback.register
    "create_ON_conversion_constructor"
    create_ON_conversion_constructor;
  Callback.register
    "create_S_skip_constructor"
    create_S_skip_constructor;
  Callback.register
    "create_S_label_constructor"
    create_S_label_constructor;
  Callback.register
    "create_S_case_constructor"
    create_S_case_constructor;
  Callback.register
    "create_S_default_constructor"
    create_S_default_constructor;
  Callback.register
    "create_S_expr_constructor"
    create_S_expr_constructor;
  Callback.register
    "create_S_compound_constructor"
    create_S_compound_constructor;
  Callback.register
    "create_S_if_constructor"
    create_S_if_constructor;
  Callback.register
    "create_S_switch_constructor"
    create_S_switch_constructor;
  Callback.register
    "create_S_while_constructor"
    create_S_while_constructor;
  Callback.register
    "create_S_doWhile_constructor"
    create_S_doWhile_constructor;
  Callback.register
    "create_S_for_constructor"
    create_S_for_constructor;
  Callback.register
    "create_S_break_constructor"
    create_S_break_constructor;
  Callback.register
    "create_S_continue_constructor"
    create_S_continue_constructor;
  Callback.register
    "create_S_return_constructor"
    create_S_return_constructor;
  Callback.register
    "create_S_goto_constructor"
    create_S_goto_constructor;
  Callback.register
    "create_S_decl_constructor"
    create_S_decl_constructor;
  Callback.register
    "create_S_try_constructor"
    create_S_try_constructor;
  Callback.register
    "create_S_asm_constructor"
    create_S_asm_constructor;
  Callback.register
    "create_S_namespaceDecl_constructor"
    create_S_namespaceDecl_constructor;
  Callback.register
    "create_S_function_constructor"
    create_S_function_constructor;
  Callback.register
    "create_S_rangeCase_constructor"
    create_S_rangeCase_constructor;
  Callback.register
    "create_S_computedGoto_constructor"
    create_S_computedGoto_constructor;
  Callback.register
    "create_CN_expr_constructor"
    create_CN_expr_constructor;
  Callback.register
    "create_CN_decl_constructor"
    create_CN_decl_constructor;
  Callback.register
    "create_Handler_tuple"
    create_Handler_tuple;
  Callback.register
    "create_E_boolLit_constructor"
    create_E_boolLit_constructor;
  Callback.register
    "create_E_intLit_constructor"
    create_E_intLit_constructor;
  Callback.register
    "create_E_floatLit_constructor"
    create_E_floatLit_constructor;
  Callback.register
    "create_E_stringLit_constructor"
    create_E_stringLit_constructor;
  Callback.register
    "create_E_charLit_constructor"
    create_E_charLit_constructor;
  Callback.register
    "create_E_this_constructor"
    create_E_this_constructor;
  Callback.register
    "create_E_variable_constructor"
    create_E_variable_constructor;
  Callback.register
    "create_E_funCall_constructor"
    create_E_funCall_constructor;
  Callback.register
    "create_E_constructor_constructor"
    create_E_constructor_constructor;
  Callback.register
    "create_E_fieldAcc_constructor"
    create_E_fieldAcc_constructor;
  Callback.register
    "create_E_sizeof_constructor"
    create_E_sizeof_constructor;
  Callback.register
    "create_E_unary_constructor"
    create_E_unary_constructor;
  Callback.register
    "create_E_effect_constructor"
    create_E_effect_constructor;
  Callback.register
    "create_E_binary_constructor"
    create_E_binary_constructor;
  Callback.register
    "create_E_addrOf_constructor"
    create_E_addrOf_constructor;
  Callback.register
    "create_E_deref_constructor"
    create_E_deref_constructor;
  Callback.register
    "create_E_cast_constructor"
    create_E_cast_constructor;
  Callback.register
    "create_E_cond_constructor"
    create_E_cond_constructor;
  Callback.register
    "create_E_sizeofType_constructor"
    create_E_sizeofType_constructor;
  Callback.register
    "create_E_assign_constructor"
    create_E_assign_constructor;
  Callback.register
    "create_E_new_constructor"
    create_E_new_constructor;
  Callback.register
    "create_E_delete_constructor"
    create_E_delete_constructor;
  Callback.register
    "create_E_throw_constructor"
    create_E_throw_constructor;
  Callback.register
    "create_E_keywordCast_constructor"
    create_E_keywordCast_constructor;
  Callback.register
    "create_E_typeidExpr_constructor"
    create_E_typeidExpr_constructor;
  Callback.register
    "create_E_typeidType_constructor"
    create_E_typeidType_constructor;
  Callback.register
    "create_E_grouping_constructor"
    create_E_grouping_constructor;
  Callback.register
    "create_E_arrow_constructor"
    create_E_arrow_constructor;
  Callback.register
    "create_E_statement_constructor"
    create_E_statement_constructor;
  Callback.register
    "create_E_compoundLit_constructor"
    create_E_compoundLit_constructor;
  Callback.register
    "create_E___builtin_constant_p_constructor"
    create_E___builtin_constant_p_constructor;
  Callback.register
    "create_E___builtin_va_arg_constructor"
    create_E___builtin_va_arg_constructor;
  Callback.register
    "create_E_alignofType_constructor"
    create_E_alignofType_constructor;
  Callback.register
    "create_E_alignofExpr_constructor"
    create_E_alignofExpr_constructor;
  Callback.register
    "create_E_gnuCond_constructor"
    create_E_gnuCond_constructor;
  Callback.register
    "create_E_addrOfLabel_constructor"
    create_E_addrOfLabel_constructor;
  Callback.register
    "create_FullExpression_tuple"
    create_FullExpression_tuple;
  Callback.register
    "create_ArgExpression_tuple"
    create_ArgExpression_tuple;
  Callback.register
    "create_ArgExpressionListOpt_tuple"
    create_ArgExpressionListOpt_tuple;
  Callback.register
    "create_IN_expr_constructor"
    create_IN_expr_constructor;
  Callback.register
    "create_IN_compound_constructor"
    create_IN_compound_constructor;
  Callback.register
    "create_IN_ctor_constructor"
    create_IN_ctor_constructor;
  Callback.register
    "create_IN_designated_constructor"
    create_IN_designated_constructor;
  Callback.register
    "create_TD_func_constructor"
    create_TD_func_constructor;
  Callback.register
    "create_TD_decl_constructor"
    create_TD_decl_constructor;
  Callback.register
    "create_TD_tmember_constructor"
    create_TD_tmember_constructor;
  Callback.register
    "create_TP_type_constructor"
    create_TP_type_constructor;
  Callback.register
    "create_TP_nontype_constructor"
    create_TP_nontype_constructor;
  Callback.register
    "create_TA_type_constructor"
    create_TA_type_constructor;
  Callback.register
    "create_TA_nontype_constructor"
    create_TA_nontype_constructor;
  Callback.register
    "create_TA_templateUsed_constructor"
    create_TA_templateUsed_constructor;
  Callback.register
    "create_ND_alias_constructor"
    create_ND_alias_constructor;
  Callback.register
    "create_ND_usingDecl_constructor"
    create_ND_usingDecl_constructor;
  Callback.register
    "create_ND_usingDir_constructor"
    create_ND_usingDir_constructor;
  Callback.register
    "create_Declarator_tuple"
    create_Declarator_tuple;
  Callback.register
    "create_D_name_constructor"
    create_D_name_constructor;
  Callback.register
    "create_D_pointer_constructor"
    create_D_pointer_constructor;
  Callback.register
    "create_D_reference_constructor"
    create_D_reference_constructor;
  Callback.register
    "create_D_func_constructor"
    create_D_func_constructor;
  Callback.register
    "create_D_array_constructor"
    create_D_array_constructor;
  Callback.register
    "create_D_bitfield_constructor"
    create_D_bitfield_constructor;
  Callback.register
    "create_D_ptrToMember_constructor"
    create_D_ptrToMember_constructor;
  Callback.register
    "create_D_grouping_constructor"
    create_D_grouping_constructor;
  Callback.register
    "create_FullExpressionAnnot_tuple"
    create_FullExpressionAnnot_tuple;
  Callback.register
    "create_TS_typeof_expr_constructor"
    create_TS_typeof_expr_constructor;
  Callback.register
    "create_TS_typeof_type_constructor"
    create_TS_typeof_type_constructor;
  Callback.register
    "create_FieldDesignator_constructor"
    create_FieldDesignator_constructor;
  Callback.register
    "create_SubscriptDesignator_constructor"
    create_SubscriptDesignator_constructor;
  Callback.register
    "create_AttributeSpecifierList_cons_constructor"
    create_AttributeSpecifierList_cons_constructor;
  Callback.register
    "create_AttributeSpecifier_cons_constructor"
    create_AttributeSpecifier_cons_constructor;
  Callback.register
    "create_AT_empty_constructor"
    create_AT_empty_constructor;
  Callback.register
    "create_AT_word_constructor"
    create_AT_word_constructor;
  Callback.register
    "create_AT_func_constructor"
    create_AT_func_constructor;
  ()


