(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* 
 * This program prints useless messages like
 * 
 *     ../elsa/astgen.oast contains 264861 ast nodes with in total:
 * 	    140171 source locations
 * 	    16350 booleans 
 * 	    280342 integers
 * 	    62556 native integers
 * 	    140171 strings
 *     maximal node id: 264861
 *     all node ids from 1 to 264861 present
 * 
 * The main purpose here is to have a simple example for doing something 
 * with a C++ abstract syntax tree.
 * 
 * This is the ast-array variant: It iterates over the array of all ast-nodes.
 *)

(* 
 * open Cc_ast_gen_type
 * open Ml_ctype
 * open Ast_annotation
 * open Ast_util
 *)


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

let opt_iter f = function
  | None -> ()
  | Some x -> f x

let count_bool = ref 0
let count_int = ref 0
let count_nativeint = ref 0
let count_string = ref 0
let count_sourceLoc = ref 0

let bool_fun _ = incr count_bool

let int_fun _ = incr count_int

let nativeint_fun _ = incr count_nativeint

let string_fun _ = 
  (* Printf.eprintf "STRING\n%!"; *)
  incr count_string

let sourceLoc_fun _ = 
  incr count_sourceLoc;
  (* Printf.eprintf "STRING\n%!"; *)
  incr count_string;
  incr count_int;
  incr count_int



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
	scope = v.scope; templ_info = v.templ_info;
      }
      in
	sourceLoc_fun v.loc;
	opt_iter string_fun v.var_name;
      
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

  | BaseClass baseClass ->
      (* unused record copy to provoke compilation errors for new fields *)
      let _dummy = {
	poly_base = baseClass.poly_base; compound = baseClass.compound;
	bc_access = baseClass.bc_access; is_virtual = baseClass.is_virtual
      }
      in
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
	opt_iter string_fun i.compound_name;
	bool_fun i.is_forward_decl;
	bool_fun i.is_transparent_union;
	opt_iter string_fun i.inst_name;


  | EnumType_Value_type(_annot, string, nativeint) ->
      string_fun string;
      nativeint_fun nativeint

  | AtomicType(SimpleType(_annot, _simpleTypeId)) ->
      ()

  | AtomicType(CompoundType _compound_info) ->
      (* does not occur: instead of (AtomicType(CompoundType...))
       * the ast_array contains a (Compound_info ...)
       *)
      assert false

  | AtomicType(PseudoInstantiation(_annot, str, _variable_opt, _accessKeyword, 
				   _compound_info, _sTemplateArgument_list)) ->
      string_fun str;

  | AtomicType(EnumType(_annot, string, _variable, _accessKeyword, 
			_enum_value_list, has_negatives)) ->
      opt_iter string_fun string;
      (* List.iter enum_value_fun enum_value_list; *)
      bool_fun has_negatives

  | AtomicType(TypeVariable(_annot, string, _variable, _accessKeyword)) ->
      string_fun string;

  | AtomicType(DependentQType(_annot, string, _variable, _accessKeyword, 
			      _atomic, _pq_name)) ->
      string_fun string;
      
  | CType(CVAtomicType(_annot, _cVFlags, _atomicType)) ->
      ()

  | CType(PointerType(_annot, _cVFlags, _cType)) ->
      ()

  | CType(ReferenceType(_annot, _cType)) ->
      ()

  | CType(FunctionType(_annot, _function_flags, _cType, _variable_list, 
		       _cType_list_opt)) ->
      ()

  | CType(ArrayType(_annot, _cType, _array_size)) ->
      ()

  | CType(PointerToMemberType(_annot, atomicType (* = NamedAtomicType *), 
			      _cVFlags, _cType)) ->
      assert(match atomicType with 
	       | SimpleType _ -> false
	       | CompoundType _
	       | PseudoInstantiation _
	       | EnumType _
	       | TypeVariable _ 
	       | DependentQType _ -> true);

  | CType(DependentSizedArrayType(_annot, _cType, _array_size)) ->
      ()

  | STemplateArgument(STA_NONE _annot) -> 
      ()

  | STemplateArgument(STA_TYPE(_annot, _cType)) -> 
      ()

  | STemplateArgument(STA_INT(_annot, int)) -> 
      int_fun int

  | STemplateArgument(STA_ENUMERATOR(_annot, _variable)) -> 
      ()

  | STemplateArgument(STA_REFERENCE(_annot, _variable)) -> 
      ()

  | STemplateArgument(STA_POINTER(_annot, _variable)) -> 
      ()

  | STemplateArgument(STA_MEMBER(_annot, _variable)) -> 
      ()

  | STemplateArgument(STA_DEPEXPR(_annot, _expression)) -> 
      ()

  | STemplateArgument(STA_TEMPLATE _annot) -> 
      ()

  | STemplateArgument(STA_ATOMIC(_annot, _atomicType)) -> 
      ()

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
	  s.type_tags

  | TranslationUnit_type((_annot, _topForm_list, _scope_opt) 
			   as _x : annotated translationUnit_type) ->
      ()

  | TopForm_type(TF_decl(_annot, sourceLoc, _declaration)) -> 
      sourceLoc_fun sourceLoc;

  | TopForm_type(TF_func(_annot, sourceLoc, _func)) -> 
      sourceLoc_fun sourceLoc;

  | TopForm_type(TF_template(_annot, sourceLoc, _templateDeclaration)) -> 
      sourceLoc_fun sourceLoc;

  | TopForm_type(TF_explicitInst(_annot, sourceLoc, _declFlags, _declaration)) -> 
      sourceLoc_fun sourceLoc;

  | TopForm_type(TF_linkage(_annot, sourceLoc, stringRef, _translationUnit)) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef;

  | TopForm_type(TF_one_linkage(_annot, sourceLoc, stringRef, _topForm)) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef;

  | TopForm_type(TF_asm(_annot, sourceLoc, e_stringLit)) -> 
      assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
      sourceLoc_fun sourceLoc;

  | TopForm_type(TF_namespaceDefn(_annot, sourceLoc, stringRef_opt, 
				  _topForm_list)) -> 
      sourceLoc_fun sourceLoc;
      opt_iter string_fun stringRef_opt;

  | TopForm_type(TF_namespaceDecl(_annot, sourceLoc, _namespaceDecl)) -> 
      sourceLoc_fun sourceLoc;


  | Function_type(_annot, _declFlags, _typeSpecifier, _declarator, _memberInit_list, 
		  s_compound_opt, _handler_list, func, _variable_opt_1, 
		  _variable_opt_2, _statement_opt, bool) ->
      assert(match s_compound_opt with
	       | None -> true
	       | Some s_compound ->
		   match s_compound with 
		     | S_compound _ -> true 
		     | _ -> false);
      assert(match func with 
	       | FunctionType _ -> true
	       | _ -> false);
      bool_fun bool


  | MemberInit_type(_annot, _pQName, _argExpression_list, 
		    _variable_opt_1, compound_opt, _variable_opt_2, 
		    _full_expr_annot, _statement_opt) ->
      assert(match compound_opt with
	       | None
	       | Some(CompoundType _) -> true
	       | _ -> false);

  | Declaration_type(_annot, _declFlags, _typeSpecifier, _declarator_list) ->
      ()

  | ASTTypeId_type(_annot, _typeSpecifier, _declarator) ->
      ()

  | PQName_type(PQ_qualifier(_annot, sourceLoc, stringRef_opt, 
			     _templateArgument_opt, _pQName, 
			     _variable_opt, _s_template_arg_list)) -> 
      sourceLoc_fun sourceLoc;
      opt_iter string_fun stringRef_opt;

  | PQName_type(PQ_name(_annot, sourceLoc, stringRef)) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef

  | PQName_type(PQ_operator(_annot, sourceLoc, _operatorName, stringRef)) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef

  | PQName_type(PQ_template(_annot, sourceLoc, stringRef, _templateArgument_opt, 
			    _s_template_arg_list)) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef;

  | PQName_type(PQ_variable(_annot, sourceLoc, _variable)) -> 
      sourceLoc_fun sourceLoc;


  | TypeSpecifier_type(TS_name(_annot, sourceLoc, _cVFlags, _pQName, bool, 
			       _var_opt_1, _var_opt_2)) -> 
      sourceLoc_fun sourceLoc;
      bool_fun bool;

  | TypeSpecifier_type(TS_simple(_annot, sourceLoc, _cVFlags, _simpleTypeId)) -> 
      sourceLoc_fun sourceLoc;

  | TypeSpecifier_type(TS_elaborated(_annot, sourceLoc, _cVFlags, _typeIntr, 
				     _pQName, namedAtomicType_opt)) -> 
      assert(match namedAtomicType_opt with
	       | Some(SimpleType _) -> false
	       | _ -> true);
      sourceLoc_fun sourceLoc;

  | TypeSpecifier_type(TS_classSpec(_annot, sourceLoc, _cVFlags, _typeIntr, 
				    _pQName_opt, _baseClassSpec_list, _memberList, 
				    compoundType)) -> 
      assert(match compoundType with
	       | CompoundType _ -> true
	       | _ -> false);
      sourceLoc_fun sourceLoc;

  | TypeSpecifier_type(TS_enumSpec(_annot, sourceLoc, _cVFlags, stringRef_opt, 
				   _enumerator_list, enumType)) -> 
      assert(match enumType with 
	       | EnumType _ -> true
	       | _ -> false);
      sourceLoc_fun sourceLoc;
      opt_iter string_fun stringRef_opt;

  | TypeSpecifier_type(TS_type(_annot, sourceLoc, _cVFlags, _cType)) -> 
      sourceLoc_fun sourceLoc;

  | TypeSpecifier_type(TS_typeof(_annot, sourceLoc, _cVFlags, _aSTTypeof)) -> 
      sourceLoc_fun sourceLoc;

  | BaseClassSpec_type(_annot, bool, _accessKeyword, _pQName, compoundType_opt) ->
      assert(match compoundType_opt with
	       | None
	       | Some(CompoundType _ ) -> true
	       | _ -> false);
      bool_fun bool;

  | Enumerator_type(_annot, sourceLoc, stringRef, _expression_opt, 
		    _variable, _int32) ->
      sourceLoc_fun sourceLoc;
      string_fun stringRef;

  | MemberList_type(_annot, _member_list) ->
      ()


  | Member_type(MR_decl(_annot, sourceLoc, _declaration)) -> 
      sourceLoc_fun sourceLoc;

  | Member_type(MR_func(_annot, sourceLoc, _func)) -> 
      sourceLoc_fun sourceLoc;

  | Member_type(MR_access(_annot, sourceLoc, _accessKeyword)) -> 
      sourceLoc_fun sourceLoc;

  | Member_type(MR_usingDecl(_annot, sourceLoc, nd_usingDecl)) -> 
      assert(match nd_usingDecl with ND_usingDecl _ -> true | _ -> false);
      sourceLoc_fun sourceLoc;

  | Member_type(MR_template(_annot, sourceLoc, _templateDeclaration)) -> 
      sourceLoc_fun sourceLoc;

  | Declarator_type(_annot, _iDeclarator, _init_opt, _variable_opt, _ctype_opt, 
		    _declaratorContext, _statement_opt_ctor, 
		    _statement_opt_dtor) ->
      ()


  | IDeclarator_type(D_name(_annot, sourceLoc, _pQName_opt)) -> 
      sourceLoc_fun sourceLoc;

  | IDeclarator_type(D_pointer(_annot, sourceLoc, _cVFlags, _iDeclarator)) -> 
      sourceLoc_fun sourceLoc;

  | IDeclarator_type(D_reference(_annot, sourceLoc, _iDeclarator)) -> 
      sourceLoc_fun sourceLoc;

  | IDeclarator_type(D_func(_annot, sourceLoc, _iDeclarator, _aSTTypeId_list, 
			    _cVFlags, _exceptionSpec_opt, pq_name_list, bool)) -> 
      assert(List.for_all (function | PQ_name _ -> true | _ -> false) 
	       pq_name_list);
      sourceLoc_fun sourceLoc;
      bool_fun bool

  | IDeclarator_type(D_array(_annot, sourceLoc, _iDeclarator, _expression_opt, 
			     bool)) -> 
      sourceLoc_fun sourceLoc;
      bool_fun bool

  | IDeclarator_type(D_bitfield(_annot, sourceLoc, _pQName_opt, _expression, 
				int)) -> 
      sourceLoc_fun sourceLoc;
      int_fun int

  | IDeclarator_type(D_ptrToMember(_annot, sourceLoc, _pQName, _cVFlags, 
				   _iDeclarator)) -> 
      sourceLoc_fun sourceLoc;

  | IDeclarator_type(D_grouping(_annot, sourceLoc, _iDeclarator)) -> 
      sourceLoc_fun sourceLoc;

  | IDeclarator_type(D_attribute(_annot, sourceLoc, _iDeclarator, 
				 _attribute_list_list)) ->
      sourceLoc_fun sourceLoc;

  | ExceptionSpec_type(_annot, _aSTTypeId_list) ->
      ()


  | OperatorName_type(ON_newDel(_annot, bool_is_new, bool_is_array)) -> 
      bool_fun bool_is_new;
      bool_fun bool_is_array

  | OperatorName_type(ON_operator(_annot, _overloadableOp)) -> 
      ()

  | OperatorName_type(ON_conversion(_annot, _aSTTypeId)) -> 
      ()


  | Statement_type(S_skip(_annot, sourceLoc)) -> 
      sourceLoc_fun sourceLoc

  | Statement_type(S_label(_annot, sourceLoc, stringRef, _statement)) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef;

  | Statement_type(S_case(_annot, sourceLoc, _expression, _statement, _int32)) -> 
      sourceLoc_fun sourceLoc;

  | Statement_type(S_default(_annot, sourceLoc, _statement)) -> 
      sourceLoc_fun sourceLoc;

  | Statement_type(S_expr(_annot, sourceLoc, _fullExpression)) -> 
      sourceLoc_fun sourceLoc;

  | Statement_type(S_compound(_annot, sourceLoc, _statement_list)) -> 
      sourceLoc_fun sourceLoc;

  | Statement_type(S_if(_annot, sourceLoc, _condition, _statement_then, 
			_statement_else)) -> 
      sourceLoc_fun sourceLoc;

  | Statement_type(S_switch(_annot, sourceLoc, _condition, _statement)) -> 
      sourceLoc_fun sourceLoc;

  | Statement_type(S_while(_annot, sourceLoc, _condition, _statement)) -> 
      sourceLoc_fun sourceLoc;

  | Statement_type(S_doWhile(_annot, sourceLoc, _statement, _fullExpression)) -> 
      sourceLoc_fun sourceLoc;

  | Statement_type(S_for(_annot, sourceLoc, _statement_init, _condition, 
			 _fullExpression, _statement_body)) -> 
      sourceLoc_fun sourceLoc;

  | Statement_type(S_break(_annot, sourceLoc)) -> 
      sourceLoc_fun sourceLoc

  | Statement_type(S_continue(_annot, sourceLoc)) -> 
      sourceLoc_fun sourceLoc

  | Statement_type(S_return(_annot, sourceLoc, _fullExpression_opt, 
			    _statement_opt)) -> 
      sourceLoc_fun sourceLoc;

  | Statement_type(S_goto(_annot, sourceLoc, stringRef)) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef

  | Statement_type(S_decl(_annot, sourceLoc, _declaration)) -> 
      sourceLoc_fun sourceLoc;

  | Statement_type(S_try(_annot, sourceLoc, _statement, _handler_list)) -> 
      sourceLoc_fun sourceLoc;

  | Statement_type(S_asm(_annot, sourceLoc, e_stringLit)) -> 
      assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
      sourceLoc_fun sourceLoc;

  | Statement_type(S_namespaceDecl(_annot, sourceLoc, _namespaceDecl)) -> 
      sourceLoc_fun sourceLoc;

  | Statement_type(S_function(_annot, sourceLoc, _func)) -> 
      sourceLoc_fun sourceLoc;

  | Statement_type(S_rangeCase(_annot, sourceLoc, _expression_lo, _expression_hi, 
			       _statement, label_lo, label_hi)) -> 
      sourceLoc_fun sourceLoc;
      int_fun label_lo;
      int_fun label_hi

  | Statement_type(S_computedGoto(_annot, sourceLoc, _expression)) -> 
      sourceLoc_fun sourceLoc;

  | Condition_type(CN_expr(_annot, _fullExpression)) -> 
      ()

  | Condition_type(CN_decl(_annot, _aSTTypeId)) -> 
      ()


  | Handler_type(_annot, _aSTTypeId, _statement_body, _variable_opt, 
		 _fullExpressionAnnot, _expression_opt, _statement_gdtor) ->
      ()

  | Expression_type(E_boolLit(_annot, _type_opt, bool)) -> 
      bool_fun bool

  | Expression_type(E_intLit(_annot, _type_opt, stringRef, _ulong)) -> 
      string_fun stringRef;

  | Expression_type(E_floatLit(_annot, _type_opt, stringRef, _double)) -> 
      string_fun stringRef;

  | Expression_type(E_stringLit(_annot, _type_opt, stringRef, e_stringLit_opt, 
				stringRef_opt)) -> 
      assert(match e_stringLit_opt with 
	       | Some(E_stringLit _) -> true 
	       | None -> true
	       | _ -> false);
      string_fun stringRef;
      opt_iter string_fun stringRef_opt

  | Expression_type(E_charLit(_annot, _type_opt, stringRef, _int32)) -> 
      string_fun stringRef;

  | Expression_type(E_this(_annot, _type_opt, _variable)) -> 
      ()

  | Expression_type(E_variable(_annot, _type_opt, _pQName, _var_opt, 
			       _nondep_var_opt)) -> 
      ()

  | Expression_type(E_funCall(_annot, _type_opt, _expression_func, 
			      _argExpression_list, _expression_retobj_opt)) -> 
      ()

  | Expression_type(E_constructor(_annot, _type_opt, _typeSpecifier, 
				  _argExpression_list, _var_opt, bool, 
				  _expression_opt)) -> 
      bool_fun bool;

  | Expression_type(E_fieldAcc(_annot, _type_opt, _expression, _pQName, 
			       _var_opt)) -> 
      ()

  | Expression_type(E_sizeof(_annot, _type_opt, _expression, int)) -> 
      int_fun int

  | Expression_type(E_unary(_annot, _type_opt, _unaryOp, _expression)) -> 
      ()

  | Expression_type(E_effect(_annot, _type_opt, _effectOp, _expression)) -> 
      ()

  | Expression_type(E_binary(_annot, _type_opt, _expression_left, _binaryOp, 
			     _expression_right)) -> 
      ()

  | Expression_type(E_addrOf(_annot, _type_opt, _expression)) -> 
      ()

  | Expression_type(E_deref(_annot, _type_opt, _expression)) -> 
      ()

  | Expression_type(E_cast(_annot, _type_opt, _aSTTypeId, _expression, bool)) -> 
      bool_fun bool

  | Expression_type(E_cond(_annot, _type_opt, _expression_cond, _expression_true, 
			   _expression_false)) -> 
      ()

  | Expression_type(E_sizeofType(_annot, _type_opt, _aSTTypeId, int, bool)) -> 
      int_fun int;
      bool_fun bool

  | Expression_type(E_assign(_annot, _type_opt, _expression_target, _binaryOp, 
			     _expression_src)) -> 
      ()

  | Expression_type(E_new(_annot, _type_opt, bool, _argExpression_list, _aSTTypeId, 
			  _argExpressionListOpt_opt, _array_size_opt, _ctor_opt,
			  _statement_opt, _heep_var_opt)) -> 
      bool_fun bool;

  | Expression_type(E_delete(_annot, _type_opt, bool_colon, bool_array, 
			     _expression_opt, _statement_opt)) -> 
      bool_fun bool_colon;
      bool_fun bool_array;

  | Expression_type(E_throw(_annot, _type_opt, _expression_opt, _var_opt, 
			    _statement_opt)) -> 
      ()

  | Expression_type(E_keywordCast(_annot, _type_opt, _castKeyword, _aSTTypeId, 
				  _expression)) -> 
      ()

  | Expression_type(E_typeidExpr(_annot, _type_opt, _expression)) -> 
      ()

  | Expression_type(E_typeidType(_annot, _type_opt, _aSTTypeId)) -> 
      ()

  | Expression_type(E_grouping(_annot, _type_opt, _expression)) -> 
      ()

  | Expression_type(E_arrow(_annot, _type_opt, _expression, _pQName)) -> 
      ()

  | Expression_type(E_statement(_annot, _type_opt, s_compound)) -> 
      assert(match s_compound with | S_compound _ -> true | _ -> false);

  | Expression_type(E_compoundLit(_annot, _type_opt, _aSTTypeId, in_compound)) -> 
      assert(match in_compound with | IN_compound _ -> true | _ -> false);

  | Expression_type(E___builtin_constant_p(_annot, _type_opt, sourceLoc, 
					   _expression)) -> 
      sourceLoc_fun sourceLoc;

  | Expression_type(E___builtin_va_arg(_annot, _type_opt, sourceLoc, _expression, 
				       _aSTTypeId)) -> 
      sourceLoc_fun sourceLoc;

  | Expression_type(E_alignofType(_annot, _type_opt, _aSTTypeId, int)) -> 
      int_fun int

  | Expression_type(E_alignofExpr(_annot, _type_opt, _expression, int)) -> 
      int_fun int

  | Expression_type(E_gnuCond(_annot, _type_opt, _expression_cond, 
			      _expression_false)) -> 
      ()

  | Expression_type(E_addrOfLabel(_annot, _type_opt, stringRef)) -> 
      string_fun stringRef


  | FullExpression_type(_annot, _expression_opt, _fullExpressionAnnot) ->
      ()

  | ArgExpression_type(_annot, _expression) -> 
      ()


  | ArgExpressionListOpt_type(_annot, _argExpression_list) ->
      ()


  | Initializer_type(IN_expr(_annot, sourceLoc, _fullExpressionAnnot, 
			     _expression_opt)) -> 
      sourceLoc_fun sourceLoc;

  | Initializer_type(IN_compound(_annot, sourceLoc, _fullExpressionAnnot, 
				 _init_list)) -> 
      sourceLoc_fun sourceLoc;

  | Initializer_type(IN_ctor(_annot, sourceLoc, _fullExpressionAnnot, 
			     _argExpression_list, _var_opt, bool)) -> 
      sourceLoc_fun sourceLoc;
      bool_fun bool

  | Initializer_type(IN_designated(_annot, sourceLoc, _fullExpressionAnnot, 
				   _designator_list, _init)) -> 
      sourceLoc_fun sourceLoc;

  | TemplateDeclaration_type(TD_func(_annot, _templateParameter_opt, _func)) -> 
      ()

  | TemplateDeclaration_type(TD_decl(_annot, _templateParameter_opt, 
				     _declaration)) -> 
      ()

  | TemplateDeclaration_type(TD_tmember(_annot, _templateParameter_opt, 
					_templateDeclaration)) -> 
      ()

  | TemplateParameter_type(TP_type(_annot, sourceLoc, _variable, stringRef, 
				   _aSTTypeId_opt, _templateParameter_opt)) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef;

  | TemplateParameter_type(TP_nontype(_annot, sourceLoc, _variable, _aSTTypeId, 
				      _templateParameter_opt)) -> 
      sourceLoc_fun sourceLoc


  | TemplateArgument_type(TA_type(_annot, _aSTTypeId, _templateArgument_opt)) -> 
      ()

  | TemplateArgument_type(TA_nontype(_annot, _expression, 
				     _templateArgument_opt)) ->
      ()

  | TemplateArgument_type(TA_templateUsed(_annot, _templateArgument_opt)) -> 
      ()


  | NamespaceDecl_type(ND_alias(_annot, stringRef, _pQName)) -> 
      string_fun stringRef

  | NamespaceDecl_type(ND_usingDecl(_annot, _pQName)) -> 
      ()

  | NamespaceDecl_type(ND_usingDir(_annot, _pQName)) -> 
      ()


  | FullExpressionAnnot_type(_annot, _declaration_list) ->
      ()


  | ASTTypeof_type(TS_typeof_expr(_annot, _ctype, _fullExpression)) -> 
      ()

  | ASTTypeof_type(TS_typeof_type(_annot, _ctype, _aSTTypeId)) -> 
      ()


  | Designator_type(FieldDesignator(_annot, sourceLoc, stringRef)) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef

  | Designator_type(SubscriptDesignator(_annot, sourceLoc, _expression, 
					_expression_opt, idx_start, idx_end)) -> 
      sourceLoc_fun sourceLoc;
      int_fun idx_start;
      int_fun idx_end


  | Attribute_type(AT_empty(_annot, sourceLoc)) -> 
      sourceLoc_fun sourceLoc

  | Attribute_type(AT_word(_annot, sourceLoc, stringRef)) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef

  | Attribute_type(AT_func(_annot, sourceLoc, stringRef, _argExpression_list)) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef


(**************************************************************************
 *
 * end of astmatch.ml 
 *
 **************************************************************************)




let file = ref ""

let file_set = ref false


let print_stats o_max_node =
  let missing_ids = ref []
  in
    Printf.printf "%s contains %d ast nodes with in total:\n" 
      !file o_max_node;
    Printf.printf 
      "\t%d source locations\n\
       \t%d booleans \n\
       \t%d integers\n\
       \t%d native integers\n\
       \t%d strings\n"
      !count_sourceLoc
      !count_bool
      !count_int
      !count_nativeint
      !count_string;
    Printf.printf "maximal node id: %d (oast header %d)\n" 
      o_max_node o_max_node;
    (* 
     * for i = 1 to o_max_node do
     *   if not (DS.mem i visited_nodes) then
     * 	missing_ids := i :: !missing_ids
     * done;
     *)
    missing_ids := List.rev !missing_ids;
    if !missing_ids = [] then
      Printf.printf "all node ids from 1 to %d present\n" o_max_node
    else begin
	Printf.printf "missing node ids: %d" (List.hd !missing_ids);
	List.iter
	  (fun i -> Printf.printf ", %d" i)
	  (List.tl !missing_ids);
	print_endline "";
    end;
    if o_max_node <> o_max_node or o_max_node <> o_max_node then
      print_endline "node counts differ!"




let arguments = Arg.align
  [
  ]

let usage_msg = 
  "usage: count-ast [options...] <file>\n\
   recognized options are:"

let usage () =
  Arg.usage arguments usage_msg;  
  exit(1)
  
let anonfun fn = 
  if !file_set 
  then
    begin
      Printf.eprintf "don't know what to do with %s\n" fn;
      usage()
    end
  else
    begin
      file := fn;
      file_set := true
    end

let main () =
  Arg.parse arguments anonfun usage_msg;
  if not !file_set then
    usage();				(* does not return *)
  let ast_array = Superast.load_marshaled_ast_array !file
  in
    for i = 1 to Array.length ast_array -1 do
      (* Printf.eprintf "visit node %d\n%!" i; *)
      ast_node_fun ast_array.(i)
    done;
    print_stats (Array.length ast_array -1)
;;


Printexc.catch main ()


