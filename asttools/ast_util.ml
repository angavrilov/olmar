
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
let baseClass_annotation baseClass = baseClass.poly_base
let compound_info_annotation info = info.compound_info_poly

let atomicType_annotation = function
  | SimpleType(annot, _)
  | PseudoInstantiation(annot, _, _, _, _, _)
  | EnumType(annot, _, _, _, _)
  | TypeVariable(annot, _, _, _)
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

let translationUnit_annotation ((annot, _) : 'a translationUnit_type) =
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

let func_annotation ((annot, _, _, _, _, _, _, _, _) : 'a function_type) = annot

let memberInit_annotation ((annot, _, _, _) : 'a memberInit_type) = annot

let declaration_annotation ((annot, _, _, _) : 'a declaration_type) = annot

let aSTTypeId_annotation ((annot, _, _) : 'a aSTTypeId_type) = annot

let pQName_annotation = function
  | PQ_qualifier(annot, _, _, _, _)
  | PQ_name(annot, _, _)
  | PQ_operator(annot, _, _, _)
  | PQ_template(annot, _, _, _)
  | PQ_variable(annot, _, _)
    -> annot

let typeSpecifier_annotation = function
  | TS_name(annot, _, _, _, _)
  | TS_simple(annot, _, _, _)
  | TS_elaborated(annot, _, _, _, _)
  | TS_classSpec(annot, _, _, _, _, _, _)
  | TS_enumSpec(annot, _, _, _, _)
  | TS_type(annot, _, _, _)
  | TS_typeof(annot, _, _, _)
    -> annot

let baseClassSpec_annotation ((annot, _, _, _) : 'a baseClassSpec_type) = annot

let enumerator_annotation ((annot, _, _, _) : 'a enumerator_type) = annot

let memberList_annotation ((annot, _) : 'a memberList_type) = annot

let member_annotation = function
  | MR_decl(annot, _, _)
  | MR_func(annot, _, _)
  | MR_access(annot, _, _)
  | MR_usingDecl(annot, _, _)
  | MR_template(annot, _, _)
    -> annot

let declarator_annotation ((annot, _, _, _, _) : 'a declarator_type) = annot

let iDeclarator_annotation = function
  | D_name(annot, _, _)
  | D_pointer(annot, _, _, _)
  | D_reference(annot, _, _)
  | D_func(annot, _, _, _, _, _, _)
  | D_array(annot, _, _, _)
  | D_bitfield(annot, _, _, _)
  | D_ptrToMember(annot, _, _, _, _)
  | D_grouping(annot, _, _)
    -> annot

let exceptionSpec_annotation ((annot, _) : 'a exceptionSpec_type) = annot

let operatorName_annotation = function
  | ON_newDel(annot, _, _)
  | ON_operator(annot, _)
  | ON_conversion(annot, _)
    -> annot

let statement_annotation = function
  | S_skip(annot, _)
  | S_label(annot, _, _, _)
  | S_case(annot, _, _, _)
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
  | S_rangeCase(annot, _, _, _, _)
  | S_computedGoto(annot, _, _)
    -> annot

let condition_annotation = function
  | CN_expr(annot, _)
  | CN_decl(annot, _)
    -> annot

let handler_annotation ((annot, _, _, _, _) : 'a handler_type) = annot

let expression_annotation = function
  | E_boolLit(annot, _)
  | E_intLit(annot, _)
  | E_floatLit(annot, _)
  | E_stringLit(annot, _, _)
  | E_charLit(annot, _)
  | E_this annot
  | E_variable(annot, _)
  | E_funCall(annot, _, _, _)
  | E_constructor(annot, _, _, _, _)
  | E_fieldAcc(annot, _, _)
  | E_sizeof(annot, _)
  | E_unary(annot, _, _)
  | E_effect(annot, _, _)
  | E_binary(annot, _, _, _)
  | E_addrOf(annot, _)
  | E_deref(annot, _)
  | E_cast(annot, _, _)
  | E_cond(annot, _, _, _)
  | E_sizeofType(annot, _)
  | E_assign(annot, _, _, _)
  | E_new(annot, _, _, _, _, _)
  | E_delete(annot, _, _, _, _)
  | E_throw(annot, _, _)
  | E_keywordCast(annot, _, _, _)
  | E_typeidExpr(annot, _)
  | E_typeidType(annot, _)
  | E_grouping(annot, _)
  | E_arrow(annot, _, _)
  | E_statement(annot, _)
  | E_compoundLit(annot, _, _)
  | E___builtin_constant_p(annot, _, _)
  | E___builtin_va_arg(annot, _, _, _)
  | E_alignofType(annot, _)
  | E_alignofExpr(annot, _)
  | E_gnuCond(annot, _, _)
  | E_addrOfLabel(annot, _)
    -> annot

let fullExpression_annotation ((annot, _) : 'a fullExpression_type) = annot

let argExpression_annotation ((annot, _) : 'a argExpression_type) = annot

let argExpressionListOpt_annotation 
    ((annot, _) : 'a argExpressionListOpt_type) = annot

let init_annotation = function
  | IN_expr(annot, _, _)
  | IN_compound(annot, _, _)
  | IN_ctor(annot, _, _, _)
  | IN_designated(annot, _, _, _)
    -> annot

let templateDeclaration_annotation = function
  | TD_func(annot, _, _)
  | TD_decl(annot, _, _)
  | TD_tmember(annot, _, _)
    -> annot

let templateParameter_annotation = function
  | TP_type(annot, _, _, _, _)
  | TP_nontype(annot, _, _, _)
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
  | TS_typeof_expr(annot, _)
  | TS_typeof_type(annot, _)
    -> annot

let designator_annotation = function
  | FieldDesignator(annot, _, _)
  | SubscriptDesignator(annot, _, _, _)
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
  | PQ_qualifier(_, loc, _, _, _)
  | PQ_name(_, loc, _)
  | PQ_operator(_, loc, _, _)
  | PQ_template(_, loc, _, _)
  | PQ_variable(_, loc, _)
    -> loc

let typeSpecifier_loc = function
  | TS_name(_, loc, _, _, _)
  | TS_simple(_, loc, _, _)
  | TS_elaborated(_, loc, _, _, _)
  | TS_classSpec(_, loc, _, _, _, _, _)
  | TS_enumSpec(_, loc, _, _, _)
  | TS_type(_, loc, _, _)
  | TS_typeof(_, loc, _, _)
    -> loc

let enumerator_loc ((_, loc, _, _) : 'a enumerator_type) = loc

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
  | D_func(_, loc, _, _, _, _, _)
  | D_array(_, loc, _, _)
  | D_bitfield(_, loc, _, _)
  | D_ptrToMember(_, loc, _, _, _)
  | D_grouping(_, loc, _)
    -> loc

let statement_loc = function
  | S_skip(_, loc)
  | S_label(_, loc, _, _)
  | S_case(_, loc, _, _)
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
  | S_rangeCase(_, loc, _, _, _)
  | S_computedGoto(_, loc, _)
    -> loc

let init_loc = function
  | IN_expr(_, loc, _)
  | IN_compound(_, loc, _)
  | IN_ctor(_, loc, _, _)
  | IN_designated(_, loc, _, _)
    -> loc

let templateParameter_loc = function
  | TP_type(_, loc, _, _, _)
  | TP_nontype(_, loc, _, _)
    -> loc

let designator_loc = function
  | FieldDesignator(_, loc, _)
  | SubscriptDesignator(_, loc, _, _)
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
  | TS_name(_, _, cv, _, _)
  | TS_simple(_, _, cv, _)
  | TS_elaborated(_, _, cv, _, _)
  | TS_classSpec(_, _, cv, _, _, _, _)
  | TS_enumSpec(_, _, cv, _, _)
  | TS_type(_, _, cv, _)
  | TS_typeof(_, _, cv, _)
    -> cv
