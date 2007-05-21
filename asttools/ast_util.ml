(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* various simple utility functions on C++ ast's
 *
 * contains
 *   *_annotation : * -> annotation
 *   *_loc : * -> sourceLoc
 *   typeSpecifier_cv : typeSpecifier -> cVFlags
 *)

open Cc_ml_types
open Cc_ast_gen_type
open Ml_ctype
open Ast_annotation

(**************************************************************************
 **************************************************************************
 **************************************************************************
 **************************************************************************
 **************************************************************************
 *
 * annotation accessor
 *
 **************************************************************************
 **************************************************************************
 **************************************************************************
 **************************************************************************
 **************************************************************************)

let variable_annotation(v : 'a variable) = v.poly_var
let templ_info_annotation(ti : 'a templateInfo) = ti.poly_templ
let inherited_templ_params_annotation(itp : 'a inheritedTemplateParams) =
  itp.poly_inherited_templ
let baseClass_annotation baseClass = baseClass.poly_base
let compound_info_annotation info = info.compound_info_poly
let scope_annotation scope = scope.poly_scope

let enum_value_annotation (annot, _, _) = annot

let atomicType_annotation = function
  | SimpleType(annot, _)
  | PseudoInstantiation(annot, _, _, _, _, _)
  | EnumType(annot, _, _, _, _, _)
  | TypeVariable(annot, _, _, _)
  | DependentQType(annot, _, _, _, _, _)
    -> annot
  | CompoundType(compound_info) -> compound_info.compound_info_poly

let cType_annotation = function
  | CVAtomicType(annot, _, _)
  | PointerType(annot, _, _)
  | ReferenceType(annot, _)
  | FunctionType(annot, _, _, _, _)
  | ArrayType(annot, _, _)
  | PointerToMemberType(annot, _, _, _)
    -> annot

let sTemplateArgument_annotation = function
  | STA_NONE annot
  | STA_TYPE(annot, _)
  | STA_INT(annot, _)
  | STA_ENUMERATOR(annot, _)
  | STA_REFERENCE(annot, _)
  | STA_POINTER(annot, _)
  | STA_MEMBER(annot, _)
  | STA_DEPEXPR(annot, _)
  | STA_TEMPLATE annot
  | STA_ATOMIC(annot, _) ->
      annot

let translationUnit_annotation ((annot, _, _) : 'a translationUnit_type) =
  annot

let topForm_annotation = function
  | TF_decl(annot, _, _)
  | TF_func(annot, _, _)
  | TF_template(annot, _, _)
  | TF_explicitInst(annot, _, _, _)
  | TF_linkage(annot, _, _, _)
  | TF_one_linkage(annot, _, _, _)
  | TF_asm(annot, _, _)
  | TF_namespaceDefn(annot, _, _, _)
  | TF_namespaceDecl(annot, _, _)
    -> annot

let func_annotation 
    ((annot, _, _, _, _, _, _, _, _, _, _, _) : 'a function_type) = annot

let memberInit_annotation ((annot, _, _, _, _, _, _, _) : 'a memberInit_type) = annot

let declaration_annotation ((annot, _, _, _) : 'a declaration_type) = annot

let aSTTypeId_annotation ((annot, _, _) : 'a aSTTypeId_type) = annot

let pQName_annotation = function
  | PQ_qualifier(annot, _, _, _, _, _, _)
  | PQ_name(annot, _, _)
  | PQ_operator(annot, _, _, _)
  | PQ_template(annot, _, _, _, _)
  | PQ_variable(annot, _, _)
    -> annot

let typeSpecifier_annotation = function
  | TS_name(annot, _, _, _, _, _, _)
  | TS_simple(annot, _, _, _)
  | TS_elaborated(annot, _, _, _, _, _)
  | TS_classSpec(annot, _, _, _, _, _, _, _)
  | TS_enumSpec(annot, _, _, _, _, _)
  | TS_type(annot, _, _, _)
  | TS_typeof(annot, _, _, _)
    -> annot

let baseClassSpec_annotation
    ((annot, _, _, _, _) : 'a baseClassSpec_type) = annot

let enumerator_annotation ((annot, _, _, _, _, _) : 'a enumerator_type) = annot

let memberList_annotation ((annot, _) : 'a memberList_type) = annot

let member_annotation = function
  | MR_decl(annot, _, _)
  | MR_func(annot, _, _)
  | MR_access(annot, _, _)
  | MR_usingDecl(annot, _, _)
  | MR_template(annot, _, _)
    -> annot

let declarator_annotation 
    ((annot, _, _, _, _, _, _, _) : 'a declarator_type) = annot

let iDeclarator_annotation = function
  | D_name(annot, _, _)
  | D_pointer(annot, _, _, _)
  | D_reference(annot, _, _)
  | D_func(annot, _, _, _, _, _, _, _)
  | D_array(annot, _, _, _, _)
  | D_bitfield(annot, _, _, _, _)
  | D_ptrToMember(annot, _, _, _, _)
  | D_grouping(annot, _, _)
  | D_attribute(annot, _, _, _)
    -> annot

let fullExpressionAnnot_annotation ((annot, _) : 'a fullExpressionAnnot_type) =
  annot

let exceptionSpec_annotation ((annot, _) : 'a exceptionSpec_type) = annot

let operatorName_annotation = function
  | ON_newDel(annot, _, _)
  | ON_operator(annot, _)
  | ON_conversion(annot, _)
    -> annot

let statement_annotation = function
  | S_skip(annot, _)
  | S_label(annot, _, _, _)
  | S_case(annot, _, _, _, _)
  | S_default(annot, _, _)
  | S_expr(annot, _, _)
  | S_compound(annot, _, _)
  | S_if(annot, _, _, _, _)
  | S_switch(annot, _, _, _)
  | S_while(annot, _, _, _)
  | S_doWhile(annot, _, _, _)
  | S_for(annot, _, _, _, _, _)
  | S_break(annot, _)
  | S_continue(annot, _)
  | S_return(annot, _, _, _)
  | S_goto(annot, _, _)
  | S_decl(annot, _, _)
  | S_try(annot, _, _, _)
  | S_asm(annot, _, _)
  | S_namespaceDecl(annot, _, _)
  | S_function(annot, _, _)
  | S_rangeCase(annot, _, _, _, _, _, _)
  | S_computedGoto(annot, _, _)
    -> annot

let condition_annotation = function
  | CN_expr(annot, _)
  | CN_decl(annot, _)
    -> annot

let handler_annotation ((annot, _, _, _, _, _, _) : 'a handler_type) = annot

let expression_annotation = function
  | E_boolLit(annot, _, _)
  | E_intLit(annot, _, _, _)
  | E_floatLit(annot, _, _, _)
  | E_stringLit(annot, _, _, _, _)
  | E_charLit(annot, _, _, _)
  | E_this (annot, _, _)
  | E_variable(annot, _, _, _, _)
  | E_funCall(annot, _, _, _, _)
  | E_constructor(annot, _, _, _, _, _, _)
  | E_fieldAcc(annot, _, _, _, _)
  | E_sizeof(annot, _, _, _)
  | E_unary(annot, _, _, _)
  | E_effect(annot, _, _, _)
  | E_binary(annot, _, _, _, _)
  | E_addrOf(annot, _, _)
  | E_deref(annot, _, _)
  | E_cast(annot, _, _, _, _)
  | E_cond(annot, _, _, _, _)
  | E_sizeofType(annot, _, _, _, _)
  | E_assign(annot, _, _, _, _)
  | E_new(annot, _, _, _, _, _, _, _, _, _)
  | E_delete(annot, _, _, _, _, _)
  | E_throw(annot, _, _, _, _)
  | E_keywordCast(annot, _, _, _, _)
  | E_typeidExpr(annot, _, _)
  | E_typeidType(annot, _, _)
  | E_grouping(annot, _, _)
  | E_arrow(annot, _, _, _)
  | E_statement(annot, _, _)
  | E_compoundLit(annot, _, _, _)
  | E___builtin_constant_p(annot, _, _, _)
  | E___builtin_va_arg(annot, _, _, _, _)
  | E_alignofType(annot, _, _, _)
  | E_alignofExpr(annot, _, _, _)
  | E_gnuCond(annot, _, _, _)
  | E_addrOfLabel(annot, _, _)
    -> annot

let fullExpression_annotation ((annot, _, _) : 'a fullExpression_type) = annot

let argExpression_annotation ((annot, _) : 'a argExpression_type) = annot

let argExpressionListOpt_annotation
    ((annot, _) : 'a argExpressionListOpt_type) = annot

let init_annotation = function
  | IN_expr(annot, _, _, _)
  | IN_compound(annot, _, _, _)
  | IN_ctor(annot, _, _, _, _, _)
  | IN_designated(annot, _, _, _, _)
    -> annot

let templateDeclaration_annotation = function
  | TD_func(annot, _, _)
  | TD_decl(annot, _, _)
  | TD_tmember(annot, _, _)
    -> annot

let templateParameter_annotation = function
  | TP_type(annot, _, _, _, _, _)
  | TP_nontype(annot, _, _, _, _)
    -> annot

let templateArgument_annotation = function
  | TA_type(annot, _, _)
  | TA_nontype(annot, _, _)
  | TA_templateUsed(annot, _)
    -> annot

let namespaceDecl_annotation = function
  | ND_alias(annot, _, _)
  | ND_usingDecl(annot, _)
  | ND_usingDir(annot, _)
    -> annot

let aSTTypeof_annotation = function
  | TS_typeof_expr(annot, _, _)
  | TS_typeof_type(annot, _, _)
    -> annot

let designator_annotation = function
  | FieldDesignator(annot, _, _)
  | SubscriptDesignator(annot, _, _, _, _, _)
    -> annot

let attributeSpecifierList_annotation = function
  | AttributeSpecifierList_cons(annot, _, _)
    -> annot

let attributeSpecifier_annotation = function
  | AttributeSpecifier_cons(annot, _, _)
    -> annot

let attribute_annotation = function
  | AT_empty(annot, _)
  | AT_word(annot, _, _)
  | AT_func(annot, _, _, _)
    -> annot


(**************************************************************************
 **************************************************************************
 **************************************************************************
 **************************************************************************
 **************************************************************************
 *
 * sourceLoc accessor
 *
 **************************************************************************
 **************************************************************************
 **************************************************************************
 **************************************************************************
 **************************************************************************)

let variable_loc(v : annotated variable) = v.loc

let topForm_loc = function
  | TF_decl(_, loc, _)
  | TF_func(_, loc, _)
  | TF_template(_, loc, _)
  | TF_explicitInst(_, loc, _, _)
  | TF_linkage(_, loc, _, _)
  | TF_one_linkage(_, loc, _, _)
  | TF_asm(_, loc, _)
  | TF_namespaceDefn(_, loc, _, _)
  | TF_namespaceDecl(_, loc, _)
    -> loc

let pQName_loc = function
  | PQ_qualifier(_, loc, _, _, _, _, _)
  | PQ_name(_, loc, _)
  | PQ_operator(_, loc, _, _)
  | PQ_template(_, loc, _, _, _)
  | PQ_variable(_, loc, _)
    -> loc

let typeSpecifier_loc = function
  | TS_name(_, loc, _, _, _, _, _)
  | TS_simple(_, loc, _, _)
  | TS_elaborated(_, loc, _, _, _, _)
  | TS_classSpec(_, loc, _, _, _, _, _, _)
  | TS_enumSpec(_, loc, _, _, _, _)
  | TS_type(_, loc, _, _)
  | TS_typeof(_, loc, _, _)
    -> loc

let enumerator_loc ((_, loc, _, _, _, _) : 'a enumerator_type) = loc

let member_loc = function
  | MR_decl(_, loc, _)
  | MR_func(_, loc, _)
  | MR_access(_, loc, _)
  | MR_usingDecl(_, loc, _)
  | MR_template(_, loc, _)
    -> loc

let iDeclarator_loc = function
  | D_name(_, loc, _)
  | D_pointer(_, loc, _, _)
  | D_reference(_, loc, _)
  | D_func(_, loc, _, _, _, _, _, _)
  | D_array(_, loc, _, _, _)
  | D_bitfield(_, loc, _, _, _)
  | D_ptrToMember(_, loc, _, _, _)
  | D_grouping(_, loc, _)
  | D_attribute(_, loc, _, _)
    -> loc

let statement_loc = function
  | S_skip(_, loc)
  | S_label(_, loc, _, _)
  | S_case(_, loc, _, _, _)
  | S_default(_, loc, _)
  | S_expr(_, loc, _)
  | S_compound(_, loc, _)
  | S_if(_, loc, _, _, _)
  | S_switch(_, loc, _, _)
  | S_while(_, loc, _, _)
  | S_doWhile(_, loc, _, _)
  | S_for(_, loc, _, _, _, _)
  | S_break(_, loc)
  | S_continue(_, loc)
  | S_return(_, loc, _, _)
  | S_goto(_, loc, _)
  | S_decl(_, loc, _)
  | S_try(_, loc, _, _)
  | S_asm(_, loc, _)
  | S_namespaceDecl(_, loc, _)
  | S_function(_, loc, _)
  | S_rangeCase(_, loc, _, _, _, _, _)
  | S_computedGoto(_, loc, _)
    -> loc

let init_loc = function
  | IN_expr(_, loc, _, _)
  | IN_compound(_, loc, _, _)
  | IN_ctor(_, loc, _, _, _, _)
  | IN_designated(_, loc, _, _, _)
    -> loc

let templateParameter_loc = function
  | TP_type(_, loc, _, _, _, _)
  | TP_nontype(_, loc, _, _, _)
    -> loc

let designator_loc = function
  | FieldDesignator(_, loc, _)
  | SubscriptDesignator(_, loc, _, _, _, _)
    -> loc

let attribute_loc = function
  | AT_empty(_, loc)
  | AT_word(_, loc, _)
  | AT_func(_, loc, _, _)
    -> loc


(**************************************************************************
 **************************************************************************
 **************************************************************************
 **************************************************************************
 **************************************************************************
 *
 * other accessors
 *
 **************************************************************************
 **************************************************************************
 **************************************************************************
 **************************************************************************
 **************************************************************************)

let typeSpecifier_cv = function
  | TS_name(_, _, cv, _, _, _, _)
  | TS_simple(_, _, cv, _)
  | TS_elaborated(_, _, cv, _, _, _)
  | TS_classSpec(_, _, cv, _, _, _, _, _)
  | TS_enumSpec(_, _, cv, _, _, _)
  | TS_type(_, _, cv, _)
  | TS_typeof(_, _, cv, _)
    -> cv


let expression_type = function
  | E_boolLit(_, ex_type, _)
  | E_intLit(_, ex_type, _, _)
  | E_floatLit(_, ex_type, _, _)
  | E_stringLit(_, ex_type, _, _, _)
  | E_charLit(_, ex_type, _, _)
  | E_this (_, ex_type, _)
  | E_variable(_, ex_type, _, _, _)
  | E_funCall(_, ex_type, _, _, _)
  | E_constructor(_, ex_type, _, _, _, _, _)
  | E_fieldAcc(_, ex_type, _, _, _)
  | E_sizeof(_, ex_type, _, _)
  | E_unary(_, ex_type, _, _)
  | E_effect(_, ex_type, _, _)
  | E_binary(_, ex_type, _, _, _)
  | E_addrOf(_, ex_type, _)
  | E_deref(_, ex_type, _)
  | E_cast(_, ex_type, _, _, _)
  | E_cond(_, ex_type, _, _, _)
  | E_sizeofType(_, ex_type, _, _, _)
  | E_assign(_, ex_type, _, _, _)
  | E_new(_, ex_type, _, _, _, _, _, _, _, _)
  | E_delete(_, ex_type, _, _, _, _)
  | E_throw(_, ex_type, _, _, _)
  | E_keywordCast(_, ex_type, _, _, _)
  | E_typeidExpr(_, ex_type, _)
  | E_typeidType(_, ex_type, _)
  | E_grouping(_, ex_type, _)
  | E_arrow(_, ex_type, _, _)
  | E_statement(_, ex_type, _)
  | E_compoundLit(_, ex_type, _, _)
  | E___builtin_constant_p(_, ex_type, _, _)
  | E___builtin_va_arg(_, ex_type, _, _, _)
  | E_alignofType(_, ex_type, _, _)
  | E_alignofExpr(_, ex_type, _, _)
  | E_gnuCond(_, ex_type, _, _)
  | E_addrOfLabel(_, ex_type, _)
    -> ex_type
