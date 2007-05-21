(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* blueprint for a mapping function for the whole ast 
 * 
 * need to add support for cycles and maybe a few other things
 *)

open Cc_ml_types
open Cc_ast_gen_type
open Ml_ctype
open Ast_annotation

(**************************************************************************
 *
 * contents of astmap.ml 
 *
 **************************************************************************)

let annotation_fun (a : annotated) = a

let opt_map f = function
  | None -> None
  | Some x -> Some(f x)

let string_fun s = (s : string)

let bool_fun b = (b : bool)

let int_fun i = (i : int)

let nativeint_fun i = (i : nativeint)

let sourceLoc_fun((file : string), (line : int), (char : int) as loc) = 
  (loc : sourceLoc)

let declFlags_fun(l : declFlag list) = l

let simpleTypeId_fun(id : simpleTypeId) = id

let typeIntr_fun(keywort : typeIntr) = keywort

let accessKeyword_fun(keyword : accessKeyword) = keyword

let cVFlags_fun(fl : cVFlag list) = fl

let overloadableOp_fun(op :overloadableOp) = op

let unaryOp_fun(op : unaryOp) = op

let effectOp_fun(op : effectOp) = op

let binaryOp_fun(op : binaryOp) = op

let castKeyword_fun(keyword : castKeyword) = keyword

let function_flags_fun(flags : function_flags) = flags


let array_size_fun = function
  | NO_SIZE -> NO_SIZE
  | DYN_SIZE -> DYN_SIZE
  | FIXED_SIZE(int) -> FIXED_SIZE(int_fun int)

let compoundType_Keyword_fun = function
  | K_STRUCT -> K_STRUCT
  | K_CLASS -> K_CLASS
  | K_UNION -> K_UNION


(***************** variable ***************************)

let rec variable_fun(v : annotated variable) =
  {
    poly_var = annotation_fun v.poly_var;

    loc = sourceLoc_fun v.loc;

    var_name = opt_map string_fun v.var_name;

    var_type = (* POSSIBLY CIRCULAR *)
      ref(opt_map cType_fun !(v.var_type));

    flags = declFlags_fun v.flags;

    value = opt_map expression_fun v.value;

    defaultParam = opt_map cType_fun v.defaultParam;

    funcDefn = (* POSSIBLY CIRCULAR *)
      ref(opt_map func_fun !(v.funcDefn));
  }


(***************** cType ******************************)

and baseClass_fun baseClass =
  {
    poly_base = annotation_fun baseClass.poly_base;

    compound = compound_info_fun baseClass.compound;

    bc_access = accessKeyword_fun baseClass.bc_access;

    is_virtual = bool_fun baseClass.is_virtual;
  }


and compound_info_fun info = 
  {
    compound_info_poly = annotation_fun info.compound_info_poly;

    compound_name = string_fun info.compound_name;

    typedef_var = variable_fun info.typedef_var;

    ci_access = accessKeyword_fun info.ci_access;

    is_forward_decl = bool_fun info.is_forward_decl;

    keyword = compoundType_Keyword_fun info.keyword;

    data_members = List.map variable_fun info.data_members;

    bases = List.map baseClass_fun info.bases;

    conversion_operators = List.map variable_fun info.conversion_operators;

    friends = List.map variable_fun info.friends;

    inst_name = string_fun info.inst_name;

    self_type = (* POSSIBLY CIRCULAR *)
      ref(opt_map cType_fun !(info.self_type));
  }



and atomicType_fun = function
  | SimpleType(annot, simpleTypeId) ->
      SimpleType(annotation_fun annot,
		 simpleTypeId_fun simpleTypeId)

  | CompoundType(compound_info) ->
      CompoundType(compound_info_fun compound_info)

  | PseudoInstantiation(annot, str, variable_opt, accessKeyword, 
			compound_info, sTemplateArgument_list) ->
      PseudoInstantiation(annotation_fun annot,
			  string_fun str,
			  opt_map variable_fun variable_opt,
			  accessKeyword_fun accessKeyword,
			  compound_info_fun compound_info,
			  List.map sTemplateArgument_fun sTemplateArgument_list)

  | EnumType(annot, string, variable, accessKeyword, string_nativeint_list) ->
      EnumType(annotation_fun annot,
	       string_fun string,
	       variable_fun variable,
	       accessKeyword_fun accessKeyword,
	       List.map (fun (string,nativeint) -> 
			   (string_fun string, nativeint_fun nativeint))
		 string_nativeint_list)

  | TypeVariable(annot, string, variable, accessKeyword) ->
      TypeVariable(annotation_fun annot,
		   string_fun string,
		   variable_fun variable,
		   accessKeyword_fun accessKeyword)


and cType_fun = function
  | CVAtomicType(annot, cVFlags, atomicType) ->
      CVAtomicType(annotation_fun annot,
		   cVFlags_fun cVFlags,
		   atomicType_fun atomicType)

  | PointerType(annot, cVFlags, cType) ->
      PointerType(annotation_fun annot,
		  cVFlags_fun cVFlags,
		  cType_fun cType)

  | ReferenceType(annot, cType) ->
      ReferenceType(annotation_fun annot,
		    cType_fun cType)

  | FunctionType(annot, function_flags, cType, variable_list, cType_list_opt) ->
      FunctionType(annotation_fun annot,
		   function_flags_fun function_flags,
		   cType_fun cType,
		   List.map variable_fun variable_list,
		   opt_map (List.map cType_fun) cType_list_opt)

  | ArrayType(annot, cType, array_size) ->
      ArrayType(annotation_fun annot,
		cType_fun cType,
		array_size_fun array_size)

  | PointerToMemberType(annot, atomicType (* = NamedAtomicType *), 
			cVFlags, cType) ->
      assert(match atomicType with 
	       | SimpleType _ -> false
	       | CompoundType _
	       | PseudoInstantiation _
	       | EnumType _
	       | TypeVariable _ -> true);
      PointerToMemberType(annotation_fun annot,
			  atomicType_fun atomicType,
			  cVFlags_fun cVFlags,
			  cType_fun cType)

and sTemplateArgument_fun = function
  | STA_NONE annot -> 
      STA_NONE(annotation_fun annot)

  | STA_TYPE(annot, cType) -> 
      STA_TYPE(annotation_fun annot, cType_fun cType)

  | STA_INT(annot, int) -> 
      STA_INT(annotation_fun annot, int_fun int)

  | STA_ENUMERATOR(annot, variable) -> 
      STA_ENUMERATOR(annotation_fun annot, variable_fun variable)

  | STA_REFERENCE(annot, variable) -> 
      STA_REFERENCE(annotation_fun annot, variable_fun variable)

  | STA_POINTER(annot, variable) -> 
      STA_POINTER(annotation_fun annot, variable_fun variable)

  | STA_MEMBER(annot, variable) -> 
      STA_MEMBER(annotation_fun annot, variable_fun variable)

  | STA_DEPEXPR(annot, expression) -> 
      STA_DEPEXPR(annotation_fun annot, expression_fun expression)

  | STA_TEMPLATE annot -> 
      STA_TEMPLATE(annotation_fun annot)

  | STA_ATOMIC(annot, atomicType) -> 
      STA_ATOMIC(annotation_fun annot, atomicType_fun atomicType)



(***************** generated ast nodes ****************)

and translationUnit_fun 
                      ((annot, topForm_list) : annotated translationUnit_type) =
  (annotation_fun annot, 
   List.map topForm_fun topForm_list)


and topForm_fun = function
  | TF_decl(annot, sourceLoc, declaration) -> 
      TF_decl(annotation_fun annot,
	      sourceLoc_fun sourceLoc,
	      declaration_fun declaration)

  | TF_func(annot, sourceLoc, func) -> 
      TF_func(annotation_fun annot,
	      sourceLoc_fun sourceLoc,
	      func_fun func)

  | TF_template(annot, sourceLoc, templateDeclaration) -> 
      TF_template(annotation_fun annot,
		  sourceLoc_fun sourceLoc,
		  templateDeclaration_fun templateDeclaration)

  | TF_explicitInst(annot, sourceLoc, declFlags, declaration) -> 
      TF_explicitInst(annotation_fun annot,
		      sourceLoc_fun sourceLoc,
		      declFlags_fun declFlags,
		      declaration_fun declaration)

  | TF_linkage(annot, sourceLoc, stringRef, translationUnit) -> 
      TF_linkage(annotation_fun annot,
		 sourceLoc_fun sourceLoc,
		 string_fun stringRef,
		 translationUnit_fun translationUnit)

  | TF_one_linkage(annot, sourceLoc, stringRef, topForm) -> 
      TF_one_linkage(annotation_fun annot,
		     sourceLoc_fun sourceLoc,
		     string_fun stringRef,
		     topForm_fun topForm)

  | TF_asm(annot, sourceLoc, e_stringLit) -> 
      assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
      TF_asm(annotation_fun annot,
	     sourceLoc_fun sourceLoc,
	     expression_fun e_stringLit)

  | TF_namespaceDefn(annot, sourceLoc, stringRef_opt, topForm_list) -> 
      TF_namespaceDefn(annotation_fun annot,
		       sourceLoc_fun sourceLoc,
		       opt_map string_fun stringRef_opt,
		       List.map topForm_fun topForm_list)

  | TF_namespaceDecl(annot, sourceLoc, namespaceDecl) -> 
      TF_namespaceDecl(annotation_fun annot,
		       sourceLoc_fun sourceLoc,
		       namespaceDecl_fun namespaceDecl)



and func_fun(annot, declFlags, typeSpecifier, declarator, memberInit_list, 
	 s_compound_opt, handler_list, statement_opt, bool) =
  assert(match s_compound_opt with
	   | None -> true
	   | Some s_compound ->
	       match s_compound with 
		 | S_compound _ -> true 
		 | _ -> false);
  (annotation_fun annot,
   declFlags_fun declFlags,
   typeSpecifier_fun typeSpecifier,
   declarator_fun declarator,
   List.map memberInit_fun memberInit_list,
   opt_map statement_fun s_compound_opt,
   List.map handler_fun handler_list,
   opt_map statement_fun statement_opt,
   bool_fun bool)



and memberInit_fun(annot, pQName, argExpression_list, statement_opt) =
  (annotation_fun annot,
   pQName_fun pQName,
   List.map argExpression_fun argExpression_list,
   opt_map statement_fun statement_opt)



and declaration_fun(annot, declFlags, typeSpecifier, declarator_list) =
  (annotation_fun annot,
   declFlags_fun declFlags,
   typeSpecifier_fun typeSpecifier,
   List.map declarator_fun declarator_list)


and aSTTypeId_fun(annot, typeSpecifier, declarator) =
  (annotation_fun annot,
   typeSpecifier_fun typeSpecifier,
   declarator_fun declarator)



and pQName_fun = function
  | PQ_qualifier(annot, sourceLoc, stringRef_opt, 
		 templateArgument_opt, pQName) -> 
      PQ_qualifier(annotation_fun annot,
		   sourceLoc_fun sourceLoc,
		   opt_map string_fun stringRef_opt,
		   opt_map templateArgument_fun templateArgument_opt,
		   pQName_fun pQName)

  | PQ_name(annot, sourceLoc, stringRef) -> 
      PQ_name(annotation_fun annot,
	      sourceLoc_fun sourceLoc,
	      string_fun stringRef)

  | PQ_operator(annot, sourceLoc, operatorName, stringRef) -> 
      PQ_operator(annotation_fun annot,
		  sourceLoc_fun sourceLoc,
		  operatorName_fun operatorName,
		  string_fun stringRef)

  | PQ_template(annot, sourceLoc, stringRef, templateArgument_opt) -> 
      PQ_template(annotation_fun annot,
		  sourceLoc_fun sourceLoc,
		  string_fun stringRef,
		  opt_map templateArgument_fun templateArgument_opt)

  | PQ_variable(annot, sourceLoc, variable) -> 
      PQ_variable(annotation_fun annot,
		  sourceLoc_fun sourceLoc,
		  variable_fun variable)



and typeSpecifier_fun = function
  | TS_name(annot, sourceLoc, cVFlags, pQName, bool) -> 
      TS_name(annotation_fun annot,
	      sourceLoc_fun sourceLoc,
	      cVFlags_fun cVFlags,
	      pQName_fun pQName,
	      bool_fun bool)

  | TS_simple(annot, sourceLoc, cVFlags, simpleTypeId) -> 
      TS_simple(annotation_fun annot,
		sourceLoc_fun sourceLoc,
		cVFlags_fun cVFlags,
		simpleTypeId_fun simpleTypeId)

  | TS_elaborated(annot, sourceLoc, cVFlags, typeIntr, pQName) -> 
      TS_elaborated(annotation_fun annot,
		    sourceLoc_fun sourceLoc,
		    cVFlags_fun cVFlags,
		    typeIntr_fun typeIntr,
		    pQName_fun pQName)

  | TS_classSpec(annot, sourceLoc, cVFlags, typeIntr, pQName_opt, 
		 baseClassSpec_list, memberList) -> 
      TS_classSpec(annotation_fun annot,
		   sourceLoc_fun sourceLoc,
		   cVFlags_fun cVFlags,
		   typeIntr_fun typeIntr,
		   opt_map pQName_fun pQName_opt,
		   List.map baseClassSpec_fun baseClassSpec_list,
		   memberList_fun memberList      )

  | TS_enumSpec(annot, sourceLoc, cVFlags, stringRef_opt, enumerator_list) -> 
      TS_enumSpec(annotation_fun annot,
		  sourceLoc_fun sourceLoc,
		  cVFlags_fun cVFlags,
		  opt_map string_fun stringRef_opt,
		  List.map enumerator_fun enumerator_list)

  | TS_type(annot, sourceLoc, cVFlags, cType) -> 
      TS_type(annotation_fun annot,
	      sourceLoc_fun sourceLoc,
	      cVFlags_fun cVFlags,
	      cType_fun cType)

  | TS_typeof(annot, sourceLoc, cVFlags, aSTTypeof) -> 
      TS_typeof(annotation_fun annot,
		sourceLoc_fun sourceLoc,
		cVFlags_fun cVFlags,
		aSTTypeof_fun aSTTypeof)


and baseClassSpec_fun(annot, bool, accessKeyword, pQName) =
  (annotation_fun annot,
   bool_fun bool,
   accessKeyword_fun accessKeyword,
   pQName_fun pQName)


and enumerator_fun(annot, sourceLoc, stringRef, expression_opt) =
  (annotation_fun annot,
   sourceLoc_fun sourceLoc,
   string_fun stringRef,
   opt_map expression_fun expression_opt)


and memberList_fun(annot, member_list) =
  (annotation_fun annot,
   List.map member_fun member_list)


and member_fun = function
  | MR_decl(annot, sourceLoc, declaration) -> 
      MR_decl(annotation_fun annot,
	      sourceLoc_fun sourceLoc,
	      declaration_fun declaration)

  | MR_func(annot, sourceLoc, func) -> 
      MR_func(annotation_fun annot,
	      sourceLoc_fun sourceLoc,
	      func_fun func)

  | MR_access(annot, sourceLoc, accessKeyword) -> 
      MR_access(annotation_fun annot,
		sourceLoc_fun sourceLoc,
		accessKeyword_fun accessKeyword)

  | MR_usingDecl(annot, sourceLoc, nd_usingDecl) -> 
      assert(match nd_usingDecl with ND_usingDecl _ -> true | _ -> false);
      MR_usingDecl(annotation_fun annot,
		   sourceLoc_fun sourceLoc,
		   namespaceDecl_fun nd_usingDecl)

  | MR_template(annot, sourceLoc, templateDeclaration) -> 
      MR_template(annotation_fun annot,
		  sourceLoc_fun sourceLoc,
		  templateDeclaration_fun templateDeclaration)


and declarator_fun(annot, iDeclarator, init_opt, 
		   statement_opt_ctor, statement_opt_dtor) =
  (annotation_fun annot,
   iDeclarator_fun iDeclarator,
   opt_map init_fun init_opt,
   opt_map statement_fun statement_opt_ctor,
   opt_map statement_fun statement_opt_dtor)


and iDeclarator_fun = function
  | D_name(annot, sourceLoc, pQName_opt) -> 
      D_name(annotation_fun annot,
	     sourceLoc_fun sourceLoc,
	     opt_map pQName_fun pQName_opt)

  | D_pointer(annot, sourceLoc, cVFlags, iDeclarator) -> 
      D_pointer(annotation_fun annot,
		sourceLoc_fun sourceLoc,
		cVFlags_fun cVFlags,
		iDeclarator_fun iDeclarator)

  | D_reference(annot, sourceLoc, iDeclarator) -> 
      D_reference(annotation_fun annot,
		  sourceLoc_fun sourceLoc,
		  iDeclarator_fun iDeclarator)

  | D_func(annot, sourceLoc, iDeclarator, aSTTypeId_list, cVFlags, 
	   exceptionSpec_opt, pq_name_list) -> 
      assert(List.for_all (function | PQ_name _ -> true | _ -> false) 
	       pq_name_list);
      D_func(annotation_fun annot,
	     sourceLoc_fun sourceLoc,
	     iDeclarator_fun iDeclarator,
	     List.map aSTTypeId_fun aSTTypeId_list,
	     cVFlags_fun cVFlags,
	     opt_map exceptionSpec_fun exceptionSpec_opt,
	     List.map pQName_fun pq_name_list)

  | D_array(annot, sourceLoc, iDeclarator, expression_opt) -> 
      D_array(annotation_fun annot,
	      sourceLoc_fun sourceLoc,
	      iDeclarator_fun iDeclarator,
	      opt_map expression_fun expression_opt)

  | D_bitfield(annot, sourceLoc, pQName_opt, expression) -> 
      D_bitfield(annotation_fun annot,
		 sourceLoc_fun sourceLoc,
		 opt_map pQName_fun pQName_opt,
		 expression_fun expression)

  | D_ptrToMember(annot, sourceLoc, pQName, cVFlags, iDeclarator) -> 
      D_ptrToMember(annotation_fun annot,
		    sourceLoc_fun sourceLoc,
		    pQName_fun pQName,
		    cVFlags_fun cVFlags,
		    iDeclarator_fun iDeclarator)

  | D_grouping(annot, sourceLoc, iDeclarator) -> 
      D_grouping(annotation_fun annot,
		 sourceLoc_fun sourceLoc,
		 iDeclarator_fun iDeclarator)

  | D_attribute(annot, sourceLoc, iDeclarator, attribute_list_list) ->
      D_attribute(annotation_fun annot,
		  sourceLoc_fun sourceLoc,
		  iDeclarator_fun iDeclarator,
		  List.map (List.map attribute_fun) attribute_list_list)



and exceptionSpec_fun(annot, aSTTypeId_list) =
  (annotation_fun annot,
   List.map aSTTypeId_fun aSTTypeId_list)


and operatorName_fun = function
  | ON_newDel(annot, bool_is_new, bool_is_array) -> 
      ON_newDel(annotation_fun annot,
		bool_fun bool_is_new,
		bool_fun bool_is_array)

  | ON_operator(annot, overloadableOp) -> 
      ON_operator(annotation_fun annot,
		  overloadableOp_fun overloadableOp)

  | ON_conversion(annot, aSTTypeId) -> 
      ON_conversion(annotation_fun annot,
		    aSTTypeId_fun aSTTypeId)


and statement_fun = function
  | S_skip(annot, sourceLoc) -> 
      S_skip(annotation_fun annot,
	     sourceLoc_fun sourceLoc)

  | S_label(annot, sourceLoc, stringRef, statement) -> 
      S_label(annotation_fun annot,
	      sourceLoc_fun sourceLoc,
	      string_fun stringRef,
	      statement_fun statement)

  | S_case(annot, sourceLoc, expression, statement) -> 
      S_case(annotation_fun annot,
	     sourceLoc_fun sourceLoc,
	     expression_fun expression,
	     statement_fun statement)

  | S_default(annot, sourceLoc, statement) -> 
      S_default(annotation_fun annot,
		sourceLoc_fun sourceLoc,
		statement_fun statement)

  | S_expr(annot, sourceLoc, fullExpression) -> 
      S_expr(annotation_fun annot,
	     sourceLoc_fun sourceLoc,
	     fullExpression_fun fullExpression)

  | S_compound(annot, sourceLoc, statement_list) -> 
      S_compound(annotation_fun annot,
		 sourceLoc_fun sourceLoc,
		 List.map statement_fun statement_list)

  | S_if(annot, sourceLoc, condition, statement_then, statement_else) -> 
      S_if(annotation_fun annot,
	   sourceLoc_fun sourceLoc,
	   condition_fun condition,
	   statement_fun statement_then,
	   statement_fun statement_else)

  | S_switch(annot, sourceLoc, condition, statement) -> 
      S_switch(annotation_fun annot,
	       sourceLoc_fun sourceLoc,
	       condition_fun condition,
	       statement_fun statement)

  | S_while(annot, sourceLoc, condition, statement) -> 
      S_while(annotation_fun annot,
	      sourceLoc_fun sourceLoc,
	      condition_fun condition,
	      statement_fun statement)

  | S_doWhile(annot, sourceLoc, statement, fullExpression) -> 
      S_doWhile(annotation_fun annot,
		sourceLoc_fun sourceLoc,
		statement_fun statement,
		fullExpression_fun fullExpression)

  | S_for(annot, sourceLoc, statement_init, condition, fullExpression, 
	  statement_body) -> 
      S_for(annotation_fun annot,
	    sourceLoc_fun sourceLoc,
	    statement_fun statement_init,
	    condition_fun condition,
	    fullExpression_fun fullExpression,
	    statement_fun statement_body)

  | S_break(annot, sourceLoc) -> 
      S_break(annotation_fun annot,
	      sourceLoc_fun sourceLoc)

  | S_continue(annot, sourceLoc) -> 
      S_continue(annotation_fun annot,
		 sourceLoc_fun sourceLoc)

  | S_return(annot, sourceLoc, fullExpression_opt, statement_opt) -> 
      S_return(annotation_fun annot,
	       sourceLoc_fun sourceLoc,
	       opt_map fullExpression_fun fullExpression_opt,
	       opt_map statement_fun statement_opt)

  | S_goto(annot, sourceLoc, stringRef) -> 
      S_goto(annotation_fun annot,
	     sourceLoc_fun sourceLoc,
	     string_fun stringRef)

  | S_decl(annot, sourceLoc, declaration) -> 
      S_decl(annotation_fun annot,
	     sourceLoc_fun sourceLoc,
	     declaration_fun declaration)

  | S_try(annot, sourceLoc, statement, handler_list) -> 
      S_try(annotation_fun annot,
	    sourceLoc_fun sourceLoc,
	    statement_fun statement,
	    List.map handler_fun handler_list)

  | S_asm(annot, sourceLoc, e_stringLit) -> 
      assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
      S_asm(annotation_fun annot,
	    sourceLoc_fun sourceLoc,
	    expression_fun e_stringLit)

  | S_namespaceDecl(annot, sourceLoc, namespaceDecl) -> 
      S_namespaceDecl(annotation_fun annot,
		      sourceLoc_fun sourceLoc,
		      namespaceDecl_fun namespaceDecl)

  | S_function(annot, sourceLoc, func) -> 
      S_function(annotation_fun annot,
		 sourceLoc_fun sourceLoc,
		 func_fun func)

  | S_rangeCase(annot, sourceLoc, expression_lo, expression_hi, statement) -> 
      S_rangeCase(annotation_fun annot,
		  sourceLoc_fun sourceLoc,
		  expression_fun expression_lo,
		  expression_fun expression_hi,
		  statement_fun statement)

  | S_computedGoto(annot, sourceLoc, expression) -> 
      S_computedGoto(annotation_fun annot,
		     sourceLoc_fun sourceLoc,
		     expression_fun expression)


and condition_fun = function
  | CN_expr(annot, fullExpression) -> 
      CN_expr(annotation_fun annot,
	      fullExpression_fun fullExpression)

  | CN_decl(annot, aSTTypeId) -> 
      CN_decl(annotation_fun annot,
	      aSTTypeId_fun aSTTypeId)


and handler_fun(annot, aSTTypeId, statement_body, 
		expression_opt, statement_gdtor) =
  (annotation_fun annot,
   aSTTypeId_fun aSTTypeId,
   statement_fun statement_body,
   opt_map expression_fun expression_opt,
   opt_map statement_fun statement_gdtor)


and expression_fun = function
  | E_boolLit(annot, bool) -> 
      E_boolLit(annotation_fun annot,
		bool_fun bool)

  | E_intLit(annot, stringRef) -> 
      E_intLit(annotation_fun annot,
	       string_fun stringRef)

  | E_floatLit(annot, stringRef) -> 
      E_floatLit(annotation_fun annot,
		 string_fun stringRef)

  | E_stringLit(annot, stringRef, e_stringLit_opt) -> 
      assert(match e_stringLit_opt with 
	       | Some(E_stringLit _) -> true 
	       | None -> true
	       | _ -> false);
      E_stringLit(annotation_fun annot,
		  string_fun stringRef,
		  opt_map expression_fun e_stringLit_opt)

  | E_charLit(annot, stringRef) -> 
      E_charLit(annotation_fun annot,
		string_fun stringRef)

  | E_this annot -> E_this(annotation_fun annot)

  | E_variable(annot, pQName) -> 
      E_variable(annotation_fun annot,
		 pQName_fun pQName)

  | E_funCall(annot, expression_func, argExpression_list, expression_retobj_opt) -> 
      E_funCall(annotation_fun annot,
		expression_fun expression_func,
		List.map argExpression_fun argExpression_list,
		opt_map expression_fun expression_retobj_opt)

  | E_constructor(annot, typeSpecifier, argExpression_list, bool, expression_opt) -> 
      E_constructor(annotation_fun annot,
		    typeSpecifier_fun typeSpecifier,
		    List.map argExpression_fun argExpression_list,
		    bool_fun bool,
		    opt_map expression_fun expression_opt)

  | E_fieldAcc(annot, expression, pQName) -> 
      E_fieldAcc(annotation_fun annot,
		 expression_fun expression,
		 pQName_fun pQName)

  | E_sizeof(annot, expression) -> 
      E_sizeof(annotation_fun annot,
	       expression_fun expression)

  | E_unary(annot, unaryOp, expression) -> 
      E_unary(annotation_fun annot,
	      unaryOp_fun unaryOp,
	      expression_fun expression)

  | E_effect(annot, effectOp, expression) -> 
      E_effect(annotation_fun annot,
	       effectOp_fun effectOp,
	       expression_fun expression)

  | E_binary(annot, expression_left, binaryOp, expression_right) -> 
      E_binary(annotation_fun annot,
	       expression_fun expression_left,
	       binaryOp_fun binaryOp,
	       expression_fun expression_right)

  | E_addrOf(annot, expression) -> 
      E_addrOf(annotation_fun annot,
	       expression_fun expression)

  | E_deref(annot, expression) -> 
      E_deref(annotation_fun annot,
	      expression_fun expression)

  | E_cast(annot, aSTTypeId, expression) -> 
      E_cast(annotation_fun annot,
	     aSTTypeId_fun aSTTypeId,
	     expression_fun expression)

  | E_cond(annot, expression_cond, expression_true, expression_false) -> 
      E_cond(annotation_fun annot,
	     expression_fun expression_cond,
	     expression_fun expression_true,
	     expression_fun expression_false)

  | E_sizeofType(annot, aSTTypeId) -> 
      E_sizeofType(annotation_fun annot,
		   aSTTypeId_fun aSTTypeId)

  | E_assign(annot, expression_target, binaryOp, expression_src) -> 
      E_assign(annotation_fun annot,
	       expression_fun expression_target,
	       binaryOp_fun binaryOp,
	       expression_fun expression_src)

  | E_new(annot, bool, argExpression_list, aSTTypeId, argExpressionListOpt_opt,
	  statement_opt) -> 
      E_new(annotation_fun annot,
	    bool_fun bool,
	    List.map argExpression_fun argExpression_list,
	    aSTTypeId_fun aSTTypeId,
	    opt_map argExpressionListOpt_fun argExpressionListOpt_opt,
	    opt_map statement_fun statement_opt)

  | E_delete(annot, bool_colon, bool_array, expression_opt, statement_opt) -> 
      E_delete(annotation_fun annot,
	       bool_fun bool_colon,
	       bool_fun bool_array,
	       opt_map expression_fun expression_opt,
	       opt_map statement_fun statement_opt)

  | E_throw(annot, expression_opt, statement_opt) -> 
      E_throw(annotation_fun annot,
	      opt_map expression_fun expression_opt,
	      opt_map statement_fun statement_opt)

  | E_keywordCast(annot, castKeyword, aSTTypeId, expression) -> 
      E_keywordCast(annotation_fun annot,
		    castKeyword_fun castKeyword,
		    aSTTypeId_fun aSTTypeId,
		    expression_fun expression)

  | E_typeidExpr(annot, expression) -> 
      E_typeidExpr(annotation_fun annot,
		   expression_fun expression)

  | E_typeidType(annot, aSTTypeId) -> 
      E_typeidType(annotation_fun annot,
		   aSTTypeId_fun aSTTypeId)

  | E_grouping(annot, expression) -> 
      E_grouping(annotation_fun annot,
		 expression_fun expression)

  | E_arrow(annot, expression, pQName) -> 
      E_arrow(annotation_fun annot,
	      expression_fun expression,
	      pQName_fun pQName)

  | E_statement(annot, s_compound) -> 
      assert(match s_compound with | S_compound _ -> true | _ -> false);
      E_statement(annotation_fun annot,
		  statement_fun s_compound)

  | E_compoundLit(annot, aSTTypeId, in_compound) -> 
      assert(match in_compound with | IN_compound _ -> true | _ -> false);
      E_compoundLit(annotation_fun annot,
		    aSTTypeId_fun aSTTypeId,
		    init_fun in_compound)

  | E___builtin_constant_p(annot, sourceLoc, expression) -> 
      E___builtin_constant_p(annotation_fun annot,
			     sourceLoc_fun sourceLoc,
			     expression_fun expression)

  | E___builtin_va_arg(annot, sourceLoc, expression, aSTTypeId) -> 
      E___builtin_va_arg(annotation_fun annot,
			 sourceLoc_fun sourceLoc,
			 expression_fun expression,
			 aSTTypeId_fun aSTTypeId)

  | E_alignofType(annot, aSTTypeId) -> 
      E_alignofType(annotation_fun annot,
		    aSTTypeId_fun aSTTypeId)

  | E_alignofExpr(annot, expression) -> 
      E_alignofExpr(annotation_fun annot,
		    expression_fun expression)

  | E_gnuCond(annot, expression_cond, expression_false) -> 
      E_gnuCond(annotation_fun annot,
		expression_fun expression_cond,
		expression_fun expression_false)

  | E_addrOfLabel(annot, stringRef) -> 
      E_addrOfLabel(annotation_fun annot,
		    string_fun stringRef)


and fullExpression_fun(annot, expression_opt) =
  (annotation_fun annot,
   opt_map expression_fun expression_opt)


and argExpression_fun(annot, expression) =
  (annotation_fun annot,
   expression_fun expression)


and argExpressionListOpt_fun(annot, argExpression_list) =
  (annotation_fun annot,
   List.map argExpression_fun argExpression_list)


and init_fun = function
  | IN_expr(annot, sourceLoc, expression) -> 
      IN_expr(annotation_fun annot,
	      sourceLoc_fun sourceLoc,
	      expression_fun expression)

  | IN_compound(annot, sourceLoc, init_list) -> 
      IN_compound(annotation_fun annot,
		  sourceLoc_fun sourceLoc,
		  List.map init_fun init_list)

  | IN_ctor(annot, sourceLoc, argExpression_list, bool) -> 
      IN_ctor(annotation_fun annot,
	      sourceLoc_fun sourceLoc,
	      List.map argExpression_fun argExpression_list,
	      bool_fun bool)

  | IN_designated(annot, sourceLoc, designator_list, init) -> 
      IN_designated(annotation_fun annot,
		    sourceLoc_fun sourceLoc,
		    List.map designator_fun designator_list,
		    init_fun init)


and templateDeclaration_fun = function
  | TD_func(annot, templateParameter_opt, func) -> 
      TD_func(annotation_fun annot,
	      opt_map templateParameter_fun templateParameter_opt,
	      func_fun func)

  | TD_decl(annot, templateParameter_opt, declaration) -> 
      TD_decl(annotation_fun annot,
	      opt_map templateParameter_fun templateParameter_opt,
	      declaration_fun declaration)

  | TD_tmember(annot, templateParameter_opt, templateDeclaration) -> 
      TD_tmember(annotation_fun annot,
		 opt_map templateParameter_fun templateParameter_opt,
		 templateDeclaration_fun templateDeclaration)


and templateParameter_fun = function
  | TP_type(annot, sourceLoc, stringRef, aSTTypeId_opt, templateParameter_opt) -> 
      TP_type(annotation_fun annot,
	      sourceLoc_fun sourceLoc,
	      string_fun stringRef,
	      opt_map aSTTypeId_fun aSTTypeId_opt,
	      opt_map templateParameter_fun templateParameter_opt)

  | TP_nontype(annot, sourceLoc, aSTTypeId, templateParameter_opt) -> 
      TP_nontype(annotation_fun annot,
		 sourceLoc_fun sourceLoc,
		 aSTTypeId_fun aSTTypeId,
		 opt_map templateParameter_fun templateParameter_opt)


and templateArgument_fun = function
  | TA_type(annot, aSTTypeId, templateArgument_opt) -> 
      TA_type(annotation_fun annot,
	      aSTTypeId_fun aSTTypeId,
	      opt_map templateArgument_fun templateArgument_opt)

  | TA_nontype(annot, expression, templateArgument_opt) -> 
      TA_nontype(annotation_fun annot,
		 expression_fun expression,
		 opt_map templateArgument_fun templateArgument_opt)

  | TA_templateUsed(annot, templateArgument_opt) -> 
      TA_templateUsed(annotation_fun annot,
		      opt_map templateArgument_fun templateArgument_opt)


and namespaceDecl_fun = function
  | ND_alias(annot, stringRef, pQName) -> 
      ND_alias(annotation_fun annot,
	       string_fun stringRef,
	       pQName_fun pQName)

  | ND_usingDecl(annot, pQName) -> 
      ND_usingDecl(annotation_fun annot,
		   pQName_fun pQName)

  | ND_usingDir(annot, pQName) -> 
      ND_usingDir(annotation_fun annot,
		  pQName_fun pQName)


and fullExpressionAnnot_fun(declaration_list) =
    List.map declaration_fun declaration_list


and aSTTypeof_fun = function
  | TS_typeof_expr(annot, fullExpression) -> 
      TS_typeof_expr(annotation_fun annot,
		     fullExpression_fun fullExpression)

  | TS_typeof_type(annot, aSTTypeId) -> 
      TS_typeof_type(annotation_fun annot,
		     aSTTypeId_fun aSTTypeId)


and designator_fun = function
  | FieldDesignator(annot, sourceLoc, stringRef) -> 
      FieldDesignator(annotation_fun annot,
		      sourceLoc_fun sourceLoc,
		      string_fun stringRef)

  | SubscriptDesignator(annot, sourceLoc, expression, expression_opt) -> 
      SubscriptDesignator(annotation_fun annot,
			  sourceLoc_fun sourceLoc,
			  expression_fun expression,
			  opt_map expression_fun expression_opt)


and attributeSpecifierList_fun = function
  | AttributeSpecifierList_cons(annot, attributeSpecifier, 
				attributeSpecifierList) -> 
      AttributeSpecifierList_cons(annotation_fun annot,
				  attributeSpecifier_fun attributeSpecifier,
				  attributeSpecifierList_fun 
				    attributeSpecifierList)


and attributeSpecifier_fun = function
  | AttributeSpecifier_cons(annot, attribute, attributeSpecifier) -> 
      AttributeSpecifier_cons(annotation_fun annot,
			      attribute_fun attribute,
			      attributeSpecifier_fun attributeSpecifier)


and attribute_fun = function
  | AT_empty(annot, sourceLoc) -> 
      AT_empty(annotation_fun annot,
	       sourceLoc_fun sourceLoc)

  | AT_word(annot, sourceLoc, stringRef) -> 
      AT_word(annotation_fun annot,
	      sourceLoc_fun sourceLoc,
	      string_fun stringRef)

  | AT_func(annot, sourceLoc, stringRef, argExpression_list) -> 
      AT_func(annotation_fun annot,
	      sourceLoc_fun sourceLoc,
	      string_fun stringRef,
	      List.map argExpression_fun argExpression_list)


(**************************************************************************
 *
 * end of astmap.ml 
 *
 **************************************************************************)


