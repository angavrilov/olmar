(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* blueprint for an iteration using the supertype ast array *)

open Superast
open Ast_annotation
open Cc_ml_types
open Ml_ctype
open Cc_ast_gen_type

(**************************************************************************
 *
 * contents of astmatch.ml
 *
 **************************************************************************)


let annotation_fun (a : annotated) = ()

let opt_iter f = function
  | None -> ()
  | Some x -> f x

let bool_fun (b : bool) = ()

let int_fun (i : int) = ()

let nativeint_fun (i : nativeint) = ()

let int32_fun (i : int32) = ()

let float_fun (x : float) = ()

let string_fun (s : string) = ()

let sourceLoc_fun((file : string), (line : int), (char : int)) = ()

let declFlags_fun(l : declFlag list) = ()

let simpleTypeId_fun(id : simpleTypeId) = ()

let typeIntr_fun(keyword : typeIntr) = ()

let accessKeyword_fun(keyword : accessKeyword) = ()

let cVFlags_fun(fl : cVFlag list) = ()

let overloadableOp_fun(op :overloadableOp) = ()

let unaryOp_fun(op : unaryOp) = ()

let effectOp_fun(op : effectOp) = ()

let binaryOp_fun(op : binaryOp) = ()

let castKeyword_fun(keyword : castKeyword) = ()

let function_flags_fun(flags : function_flags) = ()

let declaratorContext_fun(context : declaratorContext) = ()

let scopeKind_fun(sk : scopeKind) = ()


let array_size_fun = function
  | NO_SIZE -> ()
  | DYN_SIZE -> ()
  | FIXED_SIZE(int) -> int_fun int

let compoundType_Keyword_fun = function
  | K_STRUCT -> ()
  | K_CLASS -> ()
  | K_UNION -> ()

let templ_kind_fun(kind : templateThingKind) = ()



let ast_node_fun = function
  | NoAstNode -> 
      assert false

  | Variable v ->
      (* unused record copy to provoke compilation errors for new fields *)
      let _dummy = {			
	poly_var = v.poly_var; loc = v.loc; var_name = v.var_name;
	var_type = v.var_type; flags = v.flags; value = v.value;
	defaultParam = v.defaultParam; funcDefn = v.funcDefn;
	overload = v.overload; virtuallyOverride = v.virtuallyOverride;
	scope = v.scope; templ_info = v.templ_info
      }
      in
	annotation_fun v.poly_var;
	sourceLoc_fun v.loc;
	opt_iter string_fun v.var_name;
	declFlags_fun v.flags;
      
  | TemplateInfo ti -> 
      (* unused record copy to provoke compilation errors for new fields *)
      let _dummy = {
	poly_templ = ti.poly_templ; templ_kind = ti.templ_kind;
	template_params = ti.template_params;
	template_var = ti.template_var; inherited_params = ti.inherited_params; 
	instantiation_of = ti.instantiation_of; 
	instantiations = ti.instantiations; 
	specialization_of = ti.specialization_of; 
	specializations = ti.specializations; arguments = ti.arguments; 
	inst_loc = ti.inst_loc; 
	partial_instantiation_of = ti.partial_instantiation_of; 
	partial_instantiations = ti.partial_instantiations; 
	arguments_to_primary = ti.arguments_to_primary; 
	defn_scope = ti.defn_scope; 
	definition_template_info = ti.definition_template_info; 
	instantiate_body = ti.instantiate_body; 
	instantiation_disallowed = ti.instantiation_disallowed; 
	uninstantiated_default_args = ti.uninstantiated_default_args; 
	dependent_bases = ti.dependent_bases;
      }
      in
	annotation_fun ti.poly_templ;
	templ_kind_fun ti.templ_kind;
	sourceLoc_fun ti.inst_loc;
	bool_fun ti.instantiate_body;
	bool_fun ti.instantiation_disallowed;
	int_fun ti.uninstantiated_default_args;

  | InheritedTemplateParams itp ->
      let _dummy = {
	poly_inherited_templ = itp.poly_inherited_templ;
	inherited_template_params = itp.inherited_template_params;
	enclosing = itp.enclosing;
      }
      in
	assert(!(itp.enclosing) <> None);
	annotation_fun itp.poly_inherited_templ;

  | BaseClass baseClass ->
      (* unused record copy to provoke compilation errors for new fields *)
      let _dummy = {
	poly_base = baseClass.poly_base; compound = baseClass.compound;
	bc_access = baseClass.bc_access; is_virtual = baseClass.is_virtual
      }
      in
	annotation_fun baseClass.poly_base;
	accessKeyword_fun baseClass.bc_access;
	bool_fun baseClass.is_virtual

  | Compound_info i ->
      (* unused record copy to provoke compilation errors for new fields *)
      let _dummy = {
	compound_info_poly = i.compound_info_poly;
	compound_name = i.compound_name; typedef_var = i.typedef_var;
	ci_access = i.ci_access; compound_scope = i.compound_scope;
	is_forward_decl = i.is_forward_decl;
	is_transparent_union = i.is_transparent_union; keyword = i.keyword;
	data_members = i.data_members; bases = i.bases;
	conversion_operators = i.conversion_operators;
	friends = i.friends; inst_name = i.inst_name; syntax = i.syntax;
	self_type = i.self_type;
      }
      in
	assert(match !(i.syntax) with
		 | None
		 | Some(TS_classSpec _) -> true
		 | _ -> false);
	annotation_fun i.compound_info_poly;
	opt_iter string_fun i.compound_name;
	accessKeyword_fun i.ci_access;
	bool_fun i.is_forward_decl;
	bool_fun i.is_transparent_union;
	compoundType_Keyword_fun i.keyword;
	opt_iter string_fun i.inst_name;

  | EnumType_Value_type(annot, string, nativeint) ->
      annotation_fun annot;
      string_fun string;
      nativeint_fun nativeint

  | AtomicType(SimpleType(annot, simpleTypeId)) ->
      annotation_fun annot;
      simpleTypeId_fun simpleTypeId

  | AtomicType(CompoundType compound_info) ->
      (* does not occur: instead of (AtomicType(CompoundType...))
       * the ast_array contains a (Compound_info ...)
       *)
      assert false

  | AtomicType(PseudoInstantiation(annot, str, variable_opt, accessKeyword, 
				   compound_info, sTemplateArgument_list)) ->
      annotation_fun annot;
      string_fun str;
      accessKeyword_fun accessKeyword;

  | AtomicType(EnumType(annot, string, variable, accessKeyword, 
			enum_value_list, has_negatives)) ->
      annotation_fun annot;
      opt_iter string_fun string;
      accessKeyword_fun accessKeyword;
      bool_fun has_negatives

  | AtomicType(TypeVariable(annot, string, variable, accessKeyword)) ->
      annotation_fun annot;
      string_fun string;
      accessKeyword_fun accessKeyword

  | AtomicType(TemplateTypeVariable(annot, string, variable, accessKeyword, params)) ->
      annotation_fun annot;
      string_fun string;
      accessKeyword_fun accessKeyword

  | AtomicType(DependentQType(annot, string, variable, accessKeyword, 
			      atomic, pq_name)) ->
      annotation_fun annot;
      string_fun string;
      accessKeyword_fun accessKeyword;
      
  | CType(CVAtomicType(annot, cVFlags, atomicType)) ->
      annotation_fun annot;
      cVFlags_fun cVFlags;

  | CType(PointerType(annot, cVFlags, cType)) ->
      annotation_fun annot;
      cVFlags_fun cVFlags;

  | CType(ReferenceType(annot, cType)) ->
      annotation_fun annot;

  | CType(FunctionType(annot, function_flags, cType, variable_list, 
		       cType_list_opt)) ->
      annotation_fun annot;
      function_flags_fun function_flags;

  | CType(ArrayType(annot, cType, array_size)) ->
      annotation_fun annot;
      array_size_fun array_size

  | CType(PointerToMemberType(annot, atomicType (* = NamedAtomicType *), 
			      cVFlags, cType)) ->
      assert(match atomicType with 
	       | SimpleType _ -> false
	       | CompoundType _
	       | PseudoInstantiation _
	       | EnumType _
	       | TypeVariable _ 
               | TemplateTypeVariable _ 
               | DependentQType _ -> true);
      annotation_fun annot;
      cVFlags_fun cVFlags;

  | CType(DependentSizedArrayType(annot, cType, array_size)) ->
      annotation_fun annot

  | STemplateArgument(STA_NONE annot) -> 
      annotation_fun annot

  | STemplateArgument(STA_TYPE(annot, cType)) -> 
      annotation_fun annot;

  | STemplateArgument(STA_INT(annot, int)) -> 
      annotation_fun annot;
      int_fun int

  | STemplateArgument(STA_ENUMERATOR(annot, variable)) -> 
      annotation_fun annot;

  | STemplateArgument(STA_REFERENCE(annot, variable)) -> 
      annotation_fun annot;

  | STemplateArgument(STA_POINTER(annot, variable)) -> 
      annotation_fun annot;

  | STemplateArgument(STA_MEMBER(annot, variable)) -> 
      annotation_fun annot;

  | STemplateArgument(STA_DEPEXPR(annot, expression)) -> 
      annotation_fun annot;

  | STemplateArgument(STA_TEMPLATE(annot, atomicType)) -> 
      annotation_fun annot;

  | STemplateArgument(STA_ATOMIC(annot, atomicType)) -> 
      annotation_fun annot;

  | Scope s ->
      (* unused record copy to provoke compilation errors for new fields *)
      let _dummy = {
	poly_scope = s.poly_scope; variables = s.variables; 
	type_tags = s.type_tags; parent_scope = s.parent_scope;
	scope_kind = s.scope_kind; namespace_var = s.namespace_var;
	scope_template_params = s.scope_template_params; 
	parameterized_entity = s.parameterized_entity
      }
      in
	Hashtbl.iter 
	  (fun str _var -> string_fun str)
	  s.variables;
	Hashtbl.iter
	  (fun str _var -> string_fun str)
	  s.type_tags;
	scopeKind_fun s.scope_kind

  | TranslationUnit_type((annot, topForm_list, scope_opt) 
			   as x : annotated translationUnit_type) ->
      annotation_fun annot;

  | TopForm_type(TF_decl(annot, sourceLoc, declaration)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | TopForm_type(TF_func(annot, sourceLoc, func)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | TopForm_type(TF_template(annot, sourceLoc, templateDeclaration)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | TopForm_type(TF_explicitInst(annot, sourceLoc, declFlags, declaration)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      declFlags_fun declFlags;

  | TopForm_type(TF_linkage(annot, sourceLoc, stringRef, translationUnit)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      string_fun stringRef;

  | TopForm_type(TF_one_linkage(annot, sourceLoc, stringRef, topForm)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      string_fun stringRef;

  | TopForm_type(TF_asm(annot, sourceLoc, e_stringLit)) -> 
      assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | TopForm_type(TF_namespaceDefn(annot, sourceLoc, stringRef_opt, 
				  topForm_list)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      opt_iter string_fun stringRef_opt;

  | TopForm_type(TF_namespaceDecl(annot, sourceLoc, namespaceDecl)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;


  | Function_type(annot, declFlags, typeSpecifier, declarator, memberInit_list, 
		  s_compound_opt, handler_list, func, variable_opt_1, 
		  variable_opt_2, statement_opt, bool) ->
      assert(match s_compound_opt with
	       | None -> true
	       | Some s_compound ->
		   match s_compound with 
		     | S_compound _ -> true 
		     | _ -> false);
      assert(match func with 
	       | FunctionType _ -> true
	       | _ -> false);
      annotation_fun annot;
      declFlags_fun declFlags;
      bool_fun bool


  | MemberInit_type(annot, pQName, argExpression_list, 
		    variable_opt_1, compound_opt, variable_opt_2, 
		    full_expr_annot, statement_opt) ->
      assert(match compound_opt with
	       | None
	       | Some(CompoundType _) -> true
	       | _ -> false);
      annotation_fun annot;

  | Declaration_type(annot, declFlags, typeSpecifier, declarator_list) ->
      annotation_fun annot;
      declFlags_fun declFlags;

  | ASTTypeId_type(annot, typeSpecifier, declarator) ->
      annotation_fun annot;

  | PQName_type(PQ_qualifier(annot, sourceLoc, stringRef_opt, 
			     templateArgument_opt, pQName, 
			     variable_opt, s_template_arg_list)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      opt_iter string_fun stringRef_opt;

  | PQName_type(PQ_name(annot, sourceLoc, stringRef)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      string_fun stringRef

  | PQName_type(PQ_operator(annot, sourceLoc, operatorName, stringRef)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      string_fun stringRef

  | PQName_type(PQ_template(annot, sourceLoc, stringRef, templateArgument_opt, 
			    s_template_arg_list)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      string_fun stringRef;

  | PQName_type(PQ_variable(annot, sourceLoc, variable)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;


  | TypeSpecifier_type(TS_name(annot, sourceLoc, cVFlags, pQName, bool, 
			       var_opt_1, var_opt_2)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;
      bool_fun bool;

  | TypeSpecifier_type(TS_simple(annot, sourceLoc, cVFlags, simpleTypeId)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;
      simpleTypeId_fun simpleTypeId

  | TypeSpecifier_type(TS_elaborated(annot, sourceLoc, cVFlags, typeIntr, 
				     pQName, namedAtomicType_opt)) -> 
      assert(match namedAtomicType_opt with
	       | Some(SimpleType _) -> false
	       | _ -> true);
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;
      typeIntr_fun typeIntr;

  | TypeSpecifier_type(TS_classSpec(annot, sourceLoc, cVFlags, typeIntr, 
				    pQName_opt, baseClassSpec_list, memberList, 
				    compoundType)) -> 
      assert(match compoundType with
	       | CompoundType _ -> true
	       | _ -> false);
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;
      typeIntr_fun typeIntr;

  | TypeSpecifier_type(TS_enumSpec(annot, sourceLoc, cVFlags, stringRef_opt, 
				   enumerator_list, enumType)) -> 
      assert(match enumType with 
	       | EnumType _ -> true
	       | _ -> false);
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;
      opt_iter string_fun stringRef_opt;

  | TypeSpecifier_type(TS_type(annot, sourceLoc, cVFlags, cType)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;

  | TypeSpecifier_type(TS_typeof(annot, sourceLoc, cVFlags, aSTTypeof)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;

  | BaseClassSpec_type(annot, bool, accessKeyword, pQName, compoundType_opt) ->
      assert(match compoundType_opt with
	       | None
	       | Some(CompoundType _ ) -> true
	       | _ -> false);
      annotation_fun annot;
      bool_fun bool;
      accessKeyword_fun accessKeyword

  | Enumerator_type(annot, sourceLoc, stringRef, expression_opt, 
		    variable, int32) ->
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      string_fun stringRef;
      int32_fun int32

  | MemberList_type(annot, member_list) ->
      annotation_fun annot;


  | Member_type(MR_decl(annot, sourceLoc, declaration)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Member_type(MR_func(annot, sourceLoc, func)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Member_type(MR_access(annot, sourceLoc, accessKeyword)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      accessKeyword_fun accessKeyword

  | Member_type(MR_usingDecl(annot, sourceLoc, nd_usingDecl)) -> 
      assert(match nd_usingDecl with ND_usingDecl _ -> true | _ -> false);
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Member_type(MR_template(annot, sourceLoc, templateDeclaration)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Declarator_type(annot, iDeclarator, init_opt, variable_opt, ctype_opt, 
		    declaratorContext, statement_opt_ctor, 
		    statement_opt_dtor) ->
      annotation_fun annot;
      declaratorContext_fun declaratorContext;

  | IDeclarator_type(D_name(annot, sourceLoc, pQName_opt)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | IDeclarator_type(D_pointer(annot, sourceLoc, cVFlags, iDeclarator)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;

  | IDeclarator_type(D_reference(annot, sourceLoc, iDeclarator)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | IDeclarator_type(D_func(annot, sourceLoc, iDeclarator, aSTTypeId_list, 
			    cVFlags, exceptionSpec_opt, pq_name_list, bool)) -> 
      assert(List.for_all (function | PQ_name _ -> true | _ -> false) 
	       pq_name_list);
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;
      bool_fun bool

  | IDeclarator_type(D_array(annot, sourceLoc, iDeclarator, expression_opt, 
			     bool)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      bool_fun bool

  | IDeclarator_type(D_bitfield(annot, sourceLoc, pQName_opt, expression, 
				int)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      int_fun int

  | IDeclarator_type(D_ptrToMember(annot, sourceLoc, pQName, cVFlags, 
				   iDeclarator)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;

  | IDeclarator_type(D_grouping(annot, sourceLoc, iDeclarator)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | IDeclarator_type(D_attribute(annot, sourceLoc, iDeclarator, 
				 attribute_list_list)) ->
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | ExceptionSpec_type(annot, aSTTypeId_list) ->
      annotation_fun annot;


  | OperatorName_type(ON_newDel(annot, bool_is_new, bool_is_array)) -> 
      annotation_fun annot;
      bool_fun bool_is_new;
      bool_fun bool_is_array

  | OperatorName_type(ON_operator(annot, overloadableOp)) -> 
      annotation_fun annot;
      overloadableOp_fun overloadableOp

  | OperatorName_type(ON_conversion(annot, aSTTypeId)) -> 
      annotation_fun annot;


  | Statement_type(S_skip(annot, sourceLoc)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc

  | Statement_type(S_label(annot, sourceLoc, stringRef, statement)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      string_fun stringRef;

  | Statement_type(S_case(annot, sourceLoc, expression, statement, int32)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      int32_fun int32

  | Statement_type(S_default(annot, sourceLoc, statement)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Statement_type(S_expr(annot, sourceLoc, fullExpression)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Statement_type(S_compound(annot, sourceLoc, statement_list)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Statement_type(S_if(annot, sourceLoc, condition, statement_then, 
			statement_else)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Statement_type(S_switch(annot, sourceLoc, condition, statement)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Statement_type(S_while(annot, sourceLoc, condition, statement)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Statement_type(S_doWhile(annot, sourceLoc, statement, fullExpression)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Statement_type(S_for(annot, sourceLoc, statement_init, condition, 
			 fullExpression, statement_body)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Statement_type(S_break(annot, sourceLoc)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc

  | Statement_type(S_continue(annot, sourceLoc)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc

  | Statement_type(S_return(annot, sourceLoc, fullExpression_opt, 
			    statement_opt)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Statement_type(S_goto(annot, sourceLoc, stringRef)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      string_fun stringRef

  | Statement_type(S_decl(annot, sourceLoc, declaration)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Statement_type(S_try(annot, sourceLoc, statement, handler_list)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Statement_type(S_asm(annot, sourceLoc, e_stringLit)) -> 
      assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Statement_type(S_namespaceDecl(annot, sourceLoc, namespaceDecl)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Statement_type(S_function(annot, sourceLoc, func)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Statement_type(S_rangeCase(annot, sourceLoc, expression_lo, expression_hi, 
			       statement, label_lo, label_hi)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      int_fun label_lo;
      int_fun label_hi

  | Statement_type(S_computedGoto(annot, sourceLoc, expression)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Condition_type(CN_expr(annot, fullExpression)) -> 
      annotation_fun annot;

  | Condition_type(CN_decl(annot, aSTTypeId)) -> 
      annotation_fun annot


  | Handler_type(annot, aSTTypeId, statement_body, variable_opt, 
		 fullExpressionAnnot, expression_opt, statement_gdtor) ->
      annotation_fun annot;

  | Expression_type(E_boolLit(annot, type_opt, bool)) -> 
      annotation_fun annot;
      bool_fun bool

  | Expression_type(E_intLit(annot, type_opt, stringRef, ulong)) -> 
      annotation_fun annot;
      string_fun stringRef;
      int32_fun ulong

  | Expression_type(E_floatLit(annot, type_opt, stringRef, double)) -> 
      annotation_fun annot;
      string_fun stringRef;
      float_fun double

  | Expression_type(E_stringLit(annot, type_opt, stringRef, e_stringLit_opt, 
				stringRef_opt)) -> 
      assert(match e_stringLit_opt with 
	       | Some(E_stringLit _) -> true 
	       | None -> true
	       | _ -> false);
      annotation_fun annot;
      string_fun stringRef;
      opt_iter string_fun stringRef_opt

  | Expression_type(E_charLit(annot, type_opt, stringRef, int32)) -> 
      annotation_fun annot;
      string_fun stringRef;
      int32_fun int32

  | Expression_type(E_this(annot, type_opt, variable)) -> 
      annotation_fun annot;

  | Expression_type(E_variable(annot, type_opt, pQName, var_opt, 
			       nondep_var_opt)) -> 
      annotation_fun annot;

  | Expression_type(E_funCall(annot, type_opt, expression_func, 
			      argExpression_list, expression_retobj_opt)) -> 
      annotation_fun annot

  | Expression_type(E_constructor(annot, type_opt, typeSpecifier, 
				  argExpression_list, var_opt, bool, 
				  expression_opt)) -> 
      annotation_fun annot;
      bool_fun bool;

  | Expression_type(E_fieldAcc(annot, type_opt, expression, pQName, 
			       var_opt)) -> 
      annotation_fun annot;

  | Expression_type(E_sizeof(annot, type_opt, expression, int)) -> 
      annotation_fun annot;
      int_fun int

  | Expression_type(E_unary(annot, type_opt, unaryOp, expression)) -> 
      annotation_fun annot;
      unaryOp_fun unaryOp;

  | Expression_type(E_effect(annot, type_opt, effectOp, expression)) -> 
      annotation_fun annot;
      effectOp_fun effectOp;

  | Expression_type(E_binary(annot, type_opt, expression_left, binaryOp, 
			     expression_right)) -> 
      annotation_fun annot;
      binaryOp_fun binaryOp;

  | Expression_type(E_addrOf(annot, type_opt, expression)) -> 
      annotation_fun annot;

  | Expression_type(E_deref(annot, type_opt, expression)) -> 
      annotation_fun annot;

  | Expression_type(E_cast(annot, type_opt, aSTTypeId, expression, bool)) -> 
      annotation_fun annot;
      bool_fun bool

  | Expression_type(E_cond(annot, type_opt, expression_cond, expression_true, 
			   expression_false)) -> 
      annotation_fun annot;

  | Expression_type(E_sizeofType(annot, type_opt, aSTTypeId, int, bool)) -> 
      annotation_fun annot;
      int_fun int;
      bool_fun bool

  | Expression_type(E_assign(annot, type_opt, expression_target, binaryOp, 
			     expression_src)) -> 
      annotation_fun annot;
      binaryOp_fun binaryOp;

  | Expression_type(E_new(annot, type_opt, bool, argExpression_list, aSTTypeId, 
			  argExpressionListOpt_opt, array_size_opt, ctor_opt,
			  statement_opt, heep_var_opt)) -> 
      annotation_fun annot;
      bool_fun bool;

  | Expression_type(E_delete(annot, type_opt, bool_colon, bool_array, 
			     expression_opt, statement_opt)) -> 
      annotation_fun annot;
      bool_fun bool_colon;
      bool_fun bool_array;

  | Expression_type(E_throw(annot, type_opt, expression_opt, var_opt, 
			    statement_opt)) -> 
      annotation_fun annot;

  | Expression_type(E_keywordCast(annot, type_opt, castKeyword, aSTTypeId, 
				  expression)) -> 
      annotation_fun annot;
      castKeyword_fun castKeyword;

  | Expression_type(E_typeidExpr(annot, type_opt, expression)) -> 
      annotation_fun annot;

  | Expression_type(E_typeidType(annot, type_opt, aSTTypeId)) -> 
      annotation_fun annot;

  | Expression_type(E_grouping(annot, type_opt, expression)) -> 
      annotation_fun annot;

  | Expression_type(E_arrow(annot, type_opt, expression, pQName)) -> 
      annotation_fun annot;

  | Expression_type(E_statement(annot, type_opt, s_compound)) -> 
      assert(match s_compound with | S_compound _ -> true | _ -> false);
      annotation_fun annot;

  | Expression_type(E_compoundLit(annot, type_opt, aSTTypeId, in_compound)) -> 
      assert(match in_compound with | IN_compound _ -> true | _ -> false);
      annotation_fun annot;

  | Expression_type(E___builtin_constant_p(annot, type_opt, sourceLoc, 
					   expression)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Expression_type(E___builtin_va_arg(annot, type_opt, sourceLoc, expression, 
				       aSTTypeId)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Expression_type(E_alignofType(annot, type_opt, aSTTypeId, int)) -> 
      annotation_fun annot;
      int_fun int

  | Expression_type(E_alignofExpr(annot, type_opt, expression, int)) -> 
      annotation_fun annot;
      int_fun int

  | Expression_type(E_gnuCond(annot, type_opt, expression_cond, 
			      expression_false)) -> 
      annotation_fun annot;

  | Expression_type(E_addrOfLabel(annot, type_opt, stringRef)) -> 
      annotation_fun annot;
      string_fun stringRef


  | FullExpression_type(annot, expression_opt, fullExpressionAnnot) ->
      annotation_fun annot;

  | ArgExpression_type(annot, expression) -> 
      annotation_fun annot


  | ArgExpressionListOpt_type(annot, argExpression_list) ->
      annotation_fun annot;


  | Initializer_type(IN_expr(annot, sourceLoc, fullExpressionAnnot, 
			     expression_opt)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Initializer_type(IN_compound(annot, sourceLoc, fullExpressionAnnot, 
				 init_list)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | Initializer_type(IN_ctor(annot, sourceLoc, fullExpressionAnnot, 
			     argExpression_list, var_opt, bool)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      bool_fun bool

  | Initializer_type(IN_designated(annot, sourceLoc, fullExpressionAnnot, 
				   designator_list, init)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;

  | TemplateDeclaration_type(TD_func(annot, templateParameter_opt, func)) -> 
      annotation_fun annot;

  | TemplateDeclaration_type(TD_decl(annot, templateParameter_opt, 
				     declaration)) -> 
      annotation_fun annot;

  | TemplateDeclaration_type(TD_tmember(annot, templateParameter_opt, 
					templateDeclaration)) -> 
      annotation_fun annot;

  | TemplateParameter_type(TP_type(annot, sourceLoc, variable, stringRef, 
				   aSTTypeId_opt, templateParameter_opt)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      string_fun stringRef;

  | TemplateParameter_type(TP_nontype(annot, sourceLoc, variable, aSTTypeId, 
				      templateParameter_opt)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc

  | TemplateParameter_type(TP_template(annot, sourceLoc, variable, params, stringRef, 
                                   pqname, templateParameter_opt)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      string_fun stringRef;

  | TemplateArgument_type(TA_type(annot, aSTTypeId, templateArgument_opt)) -> 
      annotation_fun annot;

  | TemplateArgument_type(TA_nontype(annot, expression, 
				     templateArgument_opt)) ->
      annotation_fun annot;

  | TemplateArgument_type(TA_templateUsed(annot, templateArgument_opt)) -> 
      annotation_fun annot;


  | NamespaceDecl_type(ND_alias(annot, stringRef, pQName)) -> 
      annotation_fun annot;
      string_fun stringRef

  | NamespaceDecl_type(ND_usingDecl(annot, pQName)) -> 
      annotation_fun annot

  | NamespaceDecl_type(ND_usingDir(annot, pQName)) -> 
      annotation_fun annot


  | FullExpressionAnnot_type(annot, declaration_list) ->
      annotation_fun annot


  | ASTTypeof_type(TS_typeof_expr(annot, ctype, fullExpression)) -> 
      annotation_fun annot

  | ASTTypeof_type(TS_typeof_type(annot, ctype, aSTTypeId)) -> 
      annotation_fun annot


  | Designator_type(FieldDesignator(annot, sourceLoc, stringRef)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      string_fun stringRef

  | Designator_type(SubscriptDesignator(annot, sourceLoc, expression, 
					expression_opt, idx_start, idx_end)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      int_fun idx_start;
      int_fun idx_end


  | Attribute_type(AT_empty(annot, sourceLoc)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc

  | Attribute_type(AT_word(annot, sourceLoc, stringRef)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      string_fun stringRef

  | Attribute_type(AT_func(annot, sourceLoc, stringRef, argExpression_list)) -> 
      annotation_fun annot;
      sourceLoc_fun sourceLoc;
      string_fun stringRef


(**************************************************************************
 *
 * end of astmatch.ml 
 *
 **************************************************************************)


