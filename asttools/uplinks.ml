open Ast_annotation
open Cc_ast_gen_type
open Ast_annotation
open Superast
open Ast_util
open Ml_ctype

let add_links up down myindex annots =
  let childs = List.map id_annotation annots 
  in
    assert(down.(myindex) = []);
    down.(myindex) <- childs;
    List.iter
      (fun child -> up.(child) <- myindex :: up.(child))
      childs

let opt_link f opt_annot annots =
  match opt_annot with
    | None -> annots
    | Some x -> (f x) :: annots

let ast_node_fun up down myindex = function
  | NoAstNode -> 
      assert false

  (* 1 *)
  | Variable v ->
      let _dummy = {
	poly_var = v.poly_var; loc = v.loc; var_name = v.var_name;
	var_type = v.var_type; flags = v.flags; value = v.value;
	defaultParam = v.defaultParam; funcDefn = v.funcDefn;
	overload = v.overload; virtuallyOverride = v.virtuallyOverride;
	scope = v.scope
      }
      in
	add_links up down myindex
	  ((opt_link cType_annotation !(v.var_type)
	      (opt_link expression_annotation !(v.value)
		 (opt_link cType_annotation v.defaultParam
		    (opt_link func_annotation !(v.funcDefn)
		       (List.map variable_annotation !(v.overload))))))
	   @ (List.map variable_annotation v.virtuallyOverride)
	   @ (opt_link scope_annotation v.scope []))

  (* 2 *)
  | BaseClass baseClass ->
      let _dummy = {
	poly_base = baseClass.poly_base; compound = baseClass.compound;
	bc_access = baseClass.bc_access; is_virtual = baseClass.is_virtual
      }
      in
	add_links up down myindex
	  [compound_info_annotation baseClass.compound]

  (* 3 *)
  | Compound_info i ->
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
	add_links up down myindex
	  ([variable_annotation i.typedef_var;
	    scope_annotation i.compound_scope]
	   @ (List.map variable_annotation i.data_members)
	   @ (List.map baseClass_annotation i.bases)
	   @ (List.map variable_annotation i.conversion_operators)
	   @ (List.map variable_annotation i.friends)
	   @ (opt_link typeSpecifier_annotation !(i.syntax)
		(opt_link cType_annotation !(i.self_type) [])))

  (* 4 *)
  | AtomicType(SimpleType(annot, simpleTypeId)) ->
      ()

  (* 5 *)
  | AtomicType(CompoundType compound_info) ->
      (* does not occor: instead of (AtomicType(CompoundType...))
       * the ast_array contains a (Compound_info ...)
       *)
      assert false

  (* 6 *)
  | AtomicType(PseudoInstantiation(annot, str, variable_opt, accessKeyword, 
				   compound_info, sTemplateArgument_list)) ->
      add_links up down myindex
	(opt_link variable_annotation variable_opt
	   ([compound_info_annotation compound_info]
	    @ (List.map sTemplateArgument_annotation sTemplateArgument_list)))

  (* 7 *)
  | AtomicType(EnumType(annot, string, variable, accessKeyword, 
			string_nativeint_list, has_negatives)) ->
      add_links up down myindex
	(opt_link variable_annotation variable [])

  (* 8 *)
  | AtomicType(TypeVariable(annot, string, variable, accessKeyword)) ->
      add_links up down myindex
	[variable_annotation variable]

  (* 9 *)
  | AtomicType(DependentQType(annot, string, variable, accessKeyword, 
			      atomic, pq_name)) ->
      add_links up down myindex
	[variable_annotation variable;
	 atomicType_annotation atomic;
	 pQName_annotation pq_name]
      
  (* 10 *)
  | CType(CVAtomicType(annot, cVFlags, atomicType)) ->
      add_links up down myindex
	[atomicType_annotation atomicType]

  (* 11 *)
  | CType(PointerType(annot, cVFlags, cType)) ->
      add_links up down myindex
	[cType_annotation cType]

  (* 12 *)
  | CType(ReferenceType(annot, cType)) ->
      add_links up down myindex
	[cType_annotation cType]

  (* 13 *)
  | CType(FunctionType(annot, function_flags, cType, variable_list, 
		       cType_list_opt)) ->
      add_links up down myindex
	([cType_annotation cType] 
	 @ (List.map variable_annotation variable_list) 
	 @ (match cType_list_opt with
	      | None -> []
	      | Some l -> List.map cType_annotation l))

  (* 14 *)
  | CType(ArrayType(annot, cType, array_size)) ->
      add_links up down myindex
	[cType_annotation cType]

  (* 15 *)
  | CType(PointerToMemberType(annot, atomicType (* = NamedAtomicType *), 
			      cVFlags, cType)) ->
      assert(match atomicType with 
	       | SimpleType _ -> false
	       | CompoundType _
	       | PseudoInstantiation _
	       | EnumType _
	       | TypeVariable _ 
	       | DependentQType _ -> true);
      add_links up down myindex
	[atomicType_annotation atomicType;
	 cType_annotation cType]

  (* 16 *)
  | STemplateArgument(STA_NONE annot) ->
      ()

  (* 17 *)
  | STemplateArgument(STA_TYPE(annot, cType)) ->
      add_links up down myindex
	[cType_annotation cType]

  (* 18 *)
  | STemplateArgument(STA_INT(annot, int)) ->
      ()

  (* 19 *)
  | STemplateArgument(STA_ENUMERATOR(annot, variable)) ->
      add_links up down myindex
	[variable_annotation variable]

  (* 20 *)
  | STemplateArgument(STA_REFERENCE(annot, variable)) ->
      add_links up down myindex
	[variable_annotation variable]

  (* 21 *)
  | STemplateArgument(STA_POINTER(annot, variable)) ->
      add_links up down myindex
	[variable_annotation variable]

  (* 22 *)
  | STemplateArgument(STA_MEMBER(annot, variable)) ->
      add_links up down myindex
	[variable_annotation variable]

  (* 23 *)
  | STemplateArgument(STA_DEPEXPR(annot, expression)) ->
      add_links up down myindex
	[expression_annotation expression]

  (* 24 *)
  | STemplateArgument(STA_TEMPLATE annot) ->
      add_links up down myindex
	[]

  (* 25 *)
  | STemplateArgument(STA_ATOMIC(annot, atomicType)) ->
      add_links up down myindex
	[atomicType_annotation atomicType]

  (* 26 *)
  | Scope s ->
      let _dummy = {
	poly_scope = s.poly_scope; variables = s.variables; 
	type_tags = s.type_tags; parent_scope = s.parent_scope;
	scope_kind = s.scope_kind; namespace_var = s.namespace_var;
	template_params = s.template_params; 
	parameterized_entity = s.parameterized_entity
      }
      in
	add_links up down myindex
	  ((Hashtbl.fold 
	      (fun _string var annots -> (variable_annotation var) :: annots)
	      s.variables 
	      [])
	   @ (Hashtbl.fold 
		(fun _string var annots -> (variable_annotation var) :: annots)
		s.type_tags
		[])
	   @ (opt_link scope_annotation s.parent_scope
		(opt_link variable_annotation !(s.namespace_var) []))
	   @ (List.map variable_annotation s.template_params)
	   @ (opt_link variable_annotation s.parameterized_entity []))


  (* 27 *)
  | TranslationUnit_type((annot, topForm_list, scope_opt) 
			   as _x : annotated translationUnit_type) ->
      add_links up down myindex
	((List.map topForm_annotation topForm_list)
	 @ opt_link scope_annotation scope_opt [])

  (* 28 *)
  | TopForm_type(TF_decl(annot, sourceLoc, declaration)) ->
      add_links up down myindex
	[declaration_annotation declaration]

  (* 29 *)
  | TopForm_type(TF_func(annot, sourceLoc, func)) ->
      add_links up down myindex
	[func_annotation func]

  (* 30 *)
  | TopForm_type(TF_template(annot, sourceLoc, templateDeclaration)) ->
      add_links up down myindex
	[templateDeclaration_annotation templateDeclaration]

  (* 31 *)
  | TopForm_type(TF_explicitInst(annot, sourceLoc, declFlags, declaration)) ->
      add_links up down myindex
	[declaration_annotation declaration]

  (* 32 *)
  | TopForm_type(TF_linkage(annot, sourceLoc, stringRef, translationUnit)) ->
      add_links up down myindex
	[translationUnit_annotation translationUnit]

  (* 33 *)
  | TopForm_type(TF_one_linkage(annot, sourceLoc, stringRef, topForm)) ->
      add_links up down myindex
	[topForm_annotation topForm]

  (* 34 *)
  | TopForm_type(TF_asm(annot, sourceLoc, e_stringLit)) ->
      assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
      add_links up down myindex
	[expression_annotation e_stringLit]

  (* 35 *)
  | TopForm_type(TF_namespaceDefn(annot, sourceLoc, stringRef_opt, 
				  topForm_list)) ->
      add_links up down myindex
	(List.map topForm_annotation topForm_list)

  (* 36 *)
  | TopForm_type(TF_namespaceDecl(annot, sourceLoc, namespaceDecl)) ->
      add_links up down myindex
	[namespaceDecl_annotation namespaceDecl]

  (* 37 *)
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
      add_links up down myindex
	([typeSpecifier_annotation typeSpecifier;
	  declarator_annotation declarator;
	 ]
	 @ (List.map memberInit_annotation memberInit_list)
	 @ (opt_link statement_annotation s_compound_opt [])
	 @ (List.map handler_annotation handler_list)
	 @ [cType_annotation func]
	 @ (opt_link variable_annotation variable_opt_1
	      (opt_link variable_annotation variable_opt_2
		 (opt_link statement_annotation statement_opt []))))

  (* 38 *)
  | MemberInit_type(annot, pQName, argExpression_list, 
		    variable_opt_1, compound_opt, variable_opt_2, 
		    full_expr_annot, statement_opt) ->
      assert(match compound_opt with
	       | None
	       | Some(CompoundType _) -> true
	       | _ -> false);
      add_links up down myindex
	([pQName_annotation pQName]
	 @ (List.map argExpression_annotation argExpression_list)
	 @ (opt_link variable_annotation variable_opt_1
	      (opt_link atomicType_annotation compound_opt
		 (opt_link variable_annotation variable_opt_2
		    ((fullExpressionAnnot_annotation full_expr_annot) 
		     :: (opt_link statement_annotation statement_opt []))))))

  (* 39 *)
  | Declaration_type(annot, declFlags, typeSpecifier, declarator_list) ->
      add_links up down myindex
	((typeSpecifier_annotation typeSpecifier) 
	 :: (List.map declarator_annotation declarator_list))

  (* 40 *)
  | ASTTypeId_type(annot, typeSpecifier, declarator) ->
      add_links up down myindex
	[typeSpecifier_annotation typeSpecifier;
	 declarator_annotation declarator]

  (* 41 *)
  | PQName_type(PQ_qualifier(annot, sourceLoc, stringRef_opt, 
			     templateArgument_opt, pQName, 
			     variable_opt, s_template_arg_list)) ->
      add_links up down myindex
	(opt_link templateArgument_annotation templateArgument_opt
	   ((pQName_annotation pQName) ::
	       (opt_link variable_annotation variable_opt []))
	 @ (List.map sTemplateArgument_annotation s_template_arg_list))

  (* 42 *)
  | PQName_type(PQ_name(annot, sourceLoc, stringRef)) ->
      ()

  (* 43 *)
  | PQName_type(PQ_operator(annot, sourceLoc, operatorName, stringRef)) ->
      add_links up down myindex
	[operatorName_annotation operatorName]

  (* 44 *)
  | PQName_type(PQ_template(annot, sourceLoc, stringRef, templateArgument_opt, 
			    s_template_arg_list)) ->
      add_links up down myindex
	(opt_link templateArgument_annotation templateArgument_opt
	   (List.map sTemplateArgument_annotation s_template_arg_list))

  (* 45 *)
  | PQName_type(PQ_variable(annot, sourceLoc, variable)) ->
      add_links up down myindex
	[variable_annotation variable]

  (* 46 *)
  | TypeSpecifier_type(TS_name(annot, sourceLoc, cVFlags, pQName, bool, 
			       var_opt_1, var_opt_2)) ->
      add_links up down myindex
	((pQName_annotation pQName) ::
	   (opt_link variable_annotation var_opt_1
	      (opt_link variable_annotation var_opt_2 [])))

  (* 47 *)
  | TypeSpecifier_type(TS_simple(annot, sourceLoc, cVFlags, simpleTypeId)) ->
      ()

  (* 48 *)
  | TypeSpecifier_type(TS_elaborated(annot, sourceLoc, cVFlags, typeIntr, 
				     pQName, namedAtomicType_opt)) ->
      assert(match namedAtomicType_opt with
	       | Some(SimpleType _) -> false
	       | _ -> true);
      add_links up down myindex
	((pQName_annotation pQName) ::
	   (opt_link atomicType_annotation namedAtomicType_opt []))

  (* 49 *)
  | TypeSpecifier_type(TS_classSpec(annot, sourceLoc, cVFlags, typeIntr, 
				    pQName_opt, baseClassSpec_list, memberList, 
				    compoundType)) ->
      assert(match compoundType with
	       | CompoundType _ -> true
	       | _ -> false);
      add_links up down myindex
	((opt_link pQName_annotation pQName_opt
	    (List.map baseClassSpec_annotation baseClassSpec_list))
	 @ [memberList_annotation memberList;
	    atomicType_annotation compoundType])

  (* 50 *)
  | TypeSpecifier_type(TS_enumSpec(annot, sourceLoc, cVFlags, stringRef_opt, 
				   enumerator_list, enumType)) ->
      assert(match enumType with 
	       | EnumType _ -> true
	       | _ -> false);
      add_links up down myindex
	((List.map enumerator_annotation enumerator_list)
	 @ [atomicType_annotation enumType])

  (* 51 *)
  | TypeSpecifier_type(TS_type(annot, sourceLoc, cVFlags, cType)) ->
      add_links up down myindex
	[cType_annotation cType]

  (* 52 *)
  | TypeSpecifier_type(TS_typeof(annot, sourceLoc, cVFlags, aSTTypeof)) ->
      add_links up down myindex
	[aSTTypeof_annotation aSTTypeof]

  (* 53 *)
  | BaseClassSpec_type(annot, bool, accessKeyword, pQName, compoundType_opt) ->
      assert(match compoundType_opt with
	       | None
	       | Some(CompoundType _ ) -> true
	       | _ -> false);
      add_links up down myindex
	((pQName_annotation pQName) ::
	   (opt_link atomicType_annotation compoundType_opt []))

  (* 54 *)
  | Enumerator_type(annot, sourceLoc, stringRef, expression_opt, 
		    variable, int32) ->
      add_links up down myindex
	(opt_link expression_annotation expression_opt
	   [variable_annotation variable])

  (* 55 *)
  | MemberList_type(annot, member_list) ->
      add_links up down myindex
	(List.map member_annotation member_list)

  (* 56 *)
  | Member_type(MR_decl(annot, sourceLoc, declaration)) ->
      add_links up down myindex
	[declaration_annotation declaration]

  (* 57 *)
  | Member_type(MR_func(annot, sourceLoc, func)) ->
      add_links up down myindex
	[func_annotation func]

  (* 58 *)
  | Member_type(MR_access(annot, sourceLoc, accessKeyword)) ->
      ()

  (* 59 *)
  | Member_type(MR_usingDecl(annot, sourceLoc, nd_usingDecl)) ->
      assert(match nd_usingDecl with ND_usingDecl _ -> true | _ -> false);
      add_links up down myindex
	[namespaceDecl_annotation nd_usingDecl]

  (* 60 *)
  | Member_type(MR_template(annot, sourceLoc, templateDeclaration)) ->
      add_links up down myindex
	[templateDeclaration_annotation templateDeclaration]

  (* 61 *)
  | ExceptionSpec_type(annot, aSTTypeId_list) ->
      add_links up down myindex
	(List.map aSTTypeId_annotation aSTTypeId_list)

  (* 62 *)
  | OperatorName_type(ON_newDel(annot, bool_is_new, bool_is_array)) ->
      ()

  (* 63 *)
  | OperatorName_type(ON_operator(annot, overloadableOp)) ->
      ()

  (* 64 *)
  | OperatorName_type(ON_conversion(annot, aSTTypeId)) ->
      add_links up down myindex
	[aSTTypeId_annotation aSTTypeId]

  (* 65 *)
  | Statement_type(S_skip(annot, sourceLoc)) ->
      ()

  (* 66 *)
  | Statement_type(S_label(annot, sourceLoc, stringRef, statement)) ->
      add_links up down myindex
	[statement_annotation statement]

  (* 67 *)
  | Statement_type(S_case(annot, sourceLoc, expression, statement, int32)) ->
      add_links up down myindex
	[expression_annotation expression;
	 statement_annotation statement]

  (* 68 *)
  | Statement_type(S_default(annot, sourceLoc, statement)) ->
      add_links up down myindex
	[statement_annotation statement]

  (* 69 *)
  | Statement_type(S_expr(annot, sourceLoc, fullExpression)) ->
      add_links up down myindex
	[fullExpression_annotation fullExpression]

  (* 70 *)
  | Statement_type(S_compound(annot, sourceLoc, statement_list)) ->
      add_links up down myindex
	(List.map statement_annotation statement_list)

  (* 71 *)
  | Statement_type(S_if(annot, sourceLoc, condition, statement_then, 
			statement_else)) ->
      add_links up down myindex
	[condition_annotation condition;
	 statement_annotation statement_then;
	 statement_annotation statement_else]

  (* 72 *)
  | Statement_type(S_switch(annot, sourceLoc, condition, statement)) ->
      add_links up down myindex
	[condition_annotation condition;
	 statement_annotation statement]

  (* 73 *)
  | Statement_type(S_while(annot, sourceLoc, condition, statement)) ->
      add_links up down myindex
	[condition_annotation condition;
	 statement_annotation statement]

  (* 74 *)
  | Statement_type(S_doWhile(annot, sourceLoc, statement, fullExpression)) ->
      add_links up down myindex
	[statement_annotation statement;
	 fullExpression_annotation fullExpression]

  (* 75 *)
  | Statement_type(S_for(annot, sourceLoc, statement_init, condition, 
			 fullExpression, statement_body)) ->
      add_links up down myindex
	[statement_annotation statement_init;
	 condition_annotation condition;
	 fullExpression_annotation fullExpression;
	 statement_annotation statement_body]

  (* 76 *)
  | Statement_type(S_break(annot, sourceLoc)) ->
      ()

  (* 77 *)
  | Statement_type(S_continue(annot, sourceLoc)) ->
      ()

  (* 78 *)
  | Statement_type(S_return(annot, sourceLoc, fullExpression_opt, 
			    statement_opt)) ->
      add_links up down myindex
	(opt_link fullExpression_annotation fullExpression_opt
	   (opt_link statement_annotation statement_opt []))

  (* 79 *)
  | Statement_type(S_goto(annot, sourceLoc, stringRef)) ->
      ()

  (* 80 *)
  | Statement_type(S_decl(annot, sourceLoc, declaration)) ->
      add_links up down myindex
	[declaration_annotation declaration]

  (* 81 *)
  | Statement_type(S_try(annot, sourceLoc, statement, handler_list)) ->
      add_links up down myindex
	((statement_annotation statement) ::
	   (List.map handler_annotation handler_list))

  (* 82 *)
  | Statement_type(S_asm(annot, sourceLoc, e_stringLit)) ->
      assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
      add_links up down myindex
	[expression_annotation e_stringLit]

  (* 83 *)
  | Statement_type(S_namespaceDecl(annot, sourceLoc, namespaceDecl)) ->
      add_links up down myindex
	[namespaceDecl_annotation namespaceDecl]

  (* 84 *)
  | Statement_type(S_function(annot, sourceLoc, func)) ->
      add_links up down myindex
	[func_annotation func]

  (* 85 *)
  | Statement_type(S_rangeCase(annot, sourceLoc, expression_lo, expression_hi, 
			       statement, label_lo, label_hi)) ->
      add_links up down myindex
	[expression_annotation expression_lo;
	 expression_annotation expression_hi;
	 statement_annotation statement]

  (* 86 *)
  | Statement_type(S_computedGoto(annot, sourceLoc, expression)) ->
      add_links up down myindex
	[expression_annotation expression]

  (* 87 *)
  | Condition_type(CN_expr(annot, fullExpression)) ->
      add_links up down myindex
	[fullExpression_annotation fullExpression]

  (* 88 *)
  | Condition_type(CN_decl(annot, aSTTypeId)) ->
      add_links up down myindex
	[aSTTypeId_annotation aSTTypeId]

  (* 89 *)
  | Handler_type(annot, aSTTypeId, statement_body, variable_opt, 
		 fullExpressionAnnot, expression_opt, statement_gdtor) ->
      add_links up down myindex
	((aSTTypeId_annotation aSTTypeId)
	 :: (statement_annotation statement_body) 
	 :: (opt_link variable_annotation variable_opt
	       ((fullExpressionAnnot_annotation fullExpressionAnnot)
		:: (opt_link expression_annotation expression_opt
		      (opt_link statement_annotation statement_gdtor [])))))

  (* 90 *)
  | Expression_type(E_boolLit(annot, type_opt, bool)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt [])

  (* 91 *)
  | Expression_type(E_intLit(annot, type_opt, stringRef, ulong)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt [])

  (* 92 *)
  | Expression_type(E_floatLit(annot, type_opt, stringRef, double)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt [])

  (* 93 *)
  | Expression_type(E_stringLit(annot, type_opt, stringRef, e_stringLit_opt, 
				stringRef_opt)) ->
      assert(match e_stringLit_opt with 
	       | Some(E_stringLit _) -> true 
	       | None -> true
	       | _ -> false);
      add_links up down myindex
	(opt_link cType_annotation type_opt
	   (opt_link expression_annotation e_stringLit_opt []))

  (* 94 *)
  | Expression_type(E_charLit(annot, type_opt, stringRef, int32)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt [])

  (* 95 *)
  | Expression_type(E_this(annot, type_opt, variable)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [variable_annotation variable])

  (* 96 *)
  | Expression_type(E_variable(annot, type_opt, pQName, var_opt, 
			       nondep_var_opt)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   ((pQName_annotation pQName)
	    :: (opt_link variable_annotation var_opt
		  (opt_link variable_annotation nondep_var_opt []))))

  (* 97 *)
  | Expression_type(E_funCall(annot, type_opt, expression_func, 
			      argExpression_list, expression_retobj_opt)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   ((expression_annotation expression_func)
	    :: (List.map argExpression_annotation argExpression_list)
	    @ (opt_link expression_annotation expression_retobj_opt [])))

  (* 98 *)
  | Expression_type(E_constructor(annot, type_opt, typeSpecifier, 
				  argExpression_list, var_opt, bool, 
				  expression_opt)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   ((typeSpecifier_annotation typeSpecifier)
	    :: (List.map argExpression_annotation argExpression_list)
	    @ (opt_link variable_annotation var_opt
		 (opt_link expression_annotation expression_opt []))))

  (* 99 *)
  | Expression_type(E_fieldAcc(annot, type_opt, expression, pQName, 
			       var_opt)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   ((expression_annotation expression)
	    :: (pQName_annotation pQName)
	    :: (opt_link variable_annotation var_opt [])))

  (* 100 *)
  | Expression_type(E_sizeof(annot, type_opt, expression, int)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [expression_annotation expression])

  (* 101 *)
  | Expression_type(E_unary(annot, type_opt, unaryOp, expression)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [expression_annotation expression])

  (* 102 *)
  | Expression_type(E_effect(annot, type_opt, effectOp, expression)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [expression_annotation expression])

  (* 103 *)
  | Expression_type(E_binary(annot, type_opt, expression_left, binaryOp, 
			     expression_right)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [expression_annotation expression_left;
	    expression_annotation expression_right])

  (* 104 *)
  | Expression_type(E_addrOf(annot, type_opt, expression)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [expression_annotation expression])

  (* 105 *)
  | Expression_type(E_deref(annot, type_opt, expression)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [expression_annotation expression])

  (* 106 *)
  | Expression_type(E_cast(annot, type_opt, aSTTypeId, expression, bool)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [aSTTypeId_annotation aSTTypeId;
	    expression_annotation expression])

  (* 107 *)
  | Expression_type(E_cond(annot, type_opt, expression_cond, expression_true, 
			   expression_false)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [expression_annotation expression_cond;
	    expression_annotation expression_true;
	    expression_annotation expression_false])

  (* 108 *)
  | Expression_type(E_sizeofType(annot, type_opt, aSTTypeId, int, bool)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [aSTTypeId_annotation aSTTypeId])

  (* 109 *)
  | Expression_type(E_assign(annot, type_opt, expression_target, binaryOp, 
			     expression_src)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [expression_annotation expression_target;
	    expression_annotation expression_src])

  (* 110 *)
  | Expression_type(E_new(annot, type_opt, bool, argExpression_list, aSTTypeId, 
			  argExpressionListOpt_opt, array_size_opt, ctor_opt,
			  statement_opt, heep_var_opt)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   ((List.map argExpression_annotation argExpression_list)
	    @ (aSTTypeId_annotation aSTTypeId)
	    :: (opt_link argExpressionListOpt_annotation 
		  argExpressionListOpt_opt
		  (opt_link expression_annotation array_size_opt
		     (opt_link variable_annotation ctor_opt
			(opt_link statement_annotation statement_opt
			   (opt_link variable_annotation heep_var_opt [])))))))

  (* 111 *)
  | Expression_type(E_delete(annot, type_opt, bool_colon, bool_array, 
			     expression_opt, statement_opt)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   (opt_link expression_annotation expression_opt
	      (opt_link statement_annotation statement_opt [])))

  (* 112 *)
  | Expression_type(E_throw(annot, type_opt, expression_opt, var_opt, 
			    statement_opt)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   (opt_link expression_annotation expression_opt
	      (opt_link variable_annotation var_opt
		 (opt_link statement_annotation statement_opt []))))

  (* 113 *)
  | Expression_type(E_keywordCast(annot, type_opt, castKeyword, aSTTypeId, 
				  expression)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [aSTTypeId_annotation aSTTypeId;
	    expression_annotation expression])

  (* 114 *)
  | Expression_type(E_typeidExpr(annot, type_opt, expression)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [expression_annotation expression])

  (* 115 *)
  | Expression_type(E_typeidType(annot, type_opt, aSTTypeId)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [aSTTypeId_annotation aSTTypeId])

  (* 116 *)
  | Expression_type(E_grouping(annot, type_opt, expression)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [expression_annotation expression])

  (* 117 *)
  | Expression_type(E_arrow(annot, type_opt, expression, pQName)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [expression_annotation expression;
	    pQName_annotation pQName])

  (* 118 *)
  | Expression_type(E_statement(annot, type_opt, s_compound)) ->
      assert(match s_compound with | S_compound _ -> true | _ -> false);
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [statement_annotation s_compound])

  (* 119 *)
  | Expression_type(E_compoundLit(annot, type_opt, aSTTypeId, in_compound)) ->
      assert(match in_compound with | IN_compound _ -> true | _ -> false);
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [aSTTypeId_annotation aSTTypeId;
	    init_annotation in_compound])

  (* 120 *)
  | Expression_type(E___builtin_constant_p(annot, type_opt, sourceLoc, 
					   expression)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [expression_annotation expression])

  (* 121 *)
  | Expression_type(E___builtin_va_arg(annot, type_opt, sourceLoc, expression, 
				       aSTTypeId)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [expression_annotation expression;
	    aSTTypeId_annotation aSTTypeId])

  (* 122 *)
  | Expression_type(E_alignofType(annot, type_opt, aSTTypeId, int)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [aSTTypeId_annotation aSTTypeId])

  (* 123 *)
  | Expression_type(E_alignofExpr(annot, type_opt, expression, int)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [expression_annotation expression])

  (* 124 *)
  | Expression_type(E_gnuCond(annot, type_opt, expression_cond, 
			      expression_false)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt 
	   [expression_annotation expression_cond;
	    expression_annotation expression_false])

  (* 125 *)
  | Expression_type(E_addrOfLabel(annot, type_opt, stringRef)) ->
      add_links up down myindex
	(opt_link cType_annotation type_opt [])

  (* 126 *)
  | FullExpression_type(annot, expression_opt, fullExpressionAnnot) ->
      add_links up down myindex
	(opt_link expression_annotation expression_opt
	   [fullExpressionAnnot_annotation fullExpressionAnnot])

  (* 127 *)
  | ArgExpression_type(annot, expression) ->
      add_links up down myindex
	[expression_annotation expression]

  (* 128 *)
  | ArgExpressionListOpt_type(annot, argExpression_list) ->
      add_links up down myindex
	(List.map argExpression_annotation argExpression_list)

  (* 129 *)
  | Initializer_type(IN_expr(annot, sourceLoc, fullExpressionAnnot, 
			     expression_opt)) ->
      add_links up down myindex
	((fullExpressionAnnot_annotation fullExpressionAnnot)
	 :: (opt_link expression_annotation expression_opt []))

  (* 130 *)
  | Initializer_type(IN_compound(annot, sourceLoc, fullExpressionAnnot, 
				 init_list)) ->
      add_links up down myindex
	((fullExpressionAnnot_annotation fullExpressionAnnot)
	 :: (List.map init_annotation init_list))

  (* 131 *)
  | Initializer_type(IN_ctor(annot, sourceLoc, fullExpressionAnnot, 
			     argExpression_list, var_opt, bool)) ->
      add_links up down myindex
	((fullExpressionAnnot_annotation fullExpressionAnnot)
	 :: (List.map argExpression_annotation argExpression_list)
	 @ (opt_link variable_annotation var_opt []))

  (* 132 *)
  | Initializer_type(IN_designated(annot, sourceLoc, fullExpressionAnnot, 
				   designator_list, init)) ->
      add_links up down myindex
	((fullExpressionAnnot_annotation fullExpressionAnnot)
	 :: (List.map designator_annotation designator_list)
	 @ [init_annotation init])

  (* 133 *)
  | TemplateDeclaration_type(TD_func(annot, templateParameter_opt, func)) ->
      add_links up down myindex
	(opt_link templateParameter_annotation templateParameter_opt
	   [func_annotation func])

  (* 134 *)
  | TemplateDeclaration_type(TD_decl(annot, templateParameter_opt, 
				     declaration)) ->
      add_links up down myindex
	(opt_link templateParameter_annotation templateParameter_opt
	   [declaration_annotation declaration])

  (* 135 *)
  | TemplateDeclaration_type(TD_tmember(annot, templateParameter_opt, 
					templateDeclaration)) ->
      add_links up down myindex
	(opt_link templateParameter_annotation templateParameter_opt
	   [templateDeclaration_annotation templateDeclaration])

  (* 136 *)
  | TemplateParameter_type(TP_type(annot, sourceLoc, variable, stringRef, 
				   aSTTypeId_opt, templateParameter_opt)) ->
      add_links up down myindex
	((variable_annotation variable)
	 :: (opt_link aSTTypeId_annotation aSTTypeId_opt
	       (opt_link templateParameter_annotation 
		  templateParameter_opt [])))

  (* 137 *)
  | TemplateParameter_type(TP_nontype(annot, sourceLoc, variable, aSTTypeId, 
				      templateParameter_opt)) ->
      add_links up down myindex
	((variable_annotation variable)
	 :: (aSTTypeId_annotation aSTTypeId)
	 :: (opt_link templateParameter_annotation templateParameter_opt []))

  (* 138 *)
  | TemplateArgument_type(TA_type(annot, aSTTypeId, templateArgument_opt)) ->
      add_links up down myindex
	((aSTTypeId_annotation aSTTypeId)
	 :: (opt_link templateArgument_annotation templateArgument_opt []))

  (* 139 *)
  | TemplateArgument_type(TA_nontype(annot, expression, 
				     templateArgument_opt)) ->
      add_links up down myindex
	((expression_annotation expression)
	 :: (opt_link templateArgument_annotation templateArgument_opt []))

  (* 140 *)
  | TemplateArgument_type(TA_templateUsed(annot, templateArgument_opt)) ->
      add_links up down myindex
	(opt_link templateArgument_annotation templateArgument_opt [])

  (* 141 *)
  | NamespaceDecl_type(ND_alias(annot, stringRef, pQName)) ->
      add_links up down myindex
	[pQName_annotation pQName]

  (* 142 *)
  | NamespaceDecl_type(ND_usingDecl(annot, pQName)) ->
      add_links up down myindex
	[pQName_annotation pQName]

  (* 143 *)
  | NamespaceDecl_type(ND_usingDir(annot, pQName)) ->
      add_links up down myindex
	[pQName_annotation pQName]

  (* 144 *)
  | Declarator_type(annot, iDeclarator, init_opt, variable_opt, ctype_opt, 
		    declaratorContext, statement_opt_ctor, 
		    statement_opt_dtor) ->
      add_links up down myindex
	((iDeclarator_annotation iDeclarator)
	 :: (opt_link init_annotation init_opt
	       (opt_link variable_annotation variable_opt
		  (opt_link cType_annotation ctype_opt
		     (opt_link statement_annotation statement_opt_ctor
			(opt_link statement_annotation statement_opt_dtor [])
		     )))))

  (* 145 *)
  | IDeclarator_type(D_name(annot, sourceLoc, pQName_opt)) ->
      add_links up down myindex
	(opt_link pQName_annotation pQName_opt [])

  (* 146 *)
  | IDeclarator_type(D_pointer(annot, sourceLoc, cVFlags, iDeclarator)) ->
      add_links up down myindex
	[iDeclarator_annotation iDeclarator]

  (* 147 *)
  | IDeclarator_type(D_reference(annot, sourceLoc, iDeclarator)) ->
      add_links up down myindex
	[]

  (* 148 *)
  | IDeclarator_type(D_func(annot, sourceLoc, iDeclarator, aSTTypeId_list, 
			    cVFlags, exceptionSpec_opt, pq_name_list, bool)) ->
      assert(List.for_all (function | PQ_name _ -> true | _ -> false) 
	       pq_name_list);
      add_links up down myindex
	((iDeclarator_annotation iDeclarator)
	 :: (List.map aSTTypeId_annotation aSTTypeId_list)
	 @ (opt_link exceptionSpec_annotation exceptionSpec_opt
	      (List.map pQName_annotation pq_name_list)))

  (* 149 *)
  | IDeclarator_type(D_array(annot, sourceLoc, iDeclarator, expression_opt, 
			     bool)) ->
      add_links up down myindex
	((iDeclarator_annotation iDeclarator)
	 :: (opt_link expression_annotation expression_opt []))

  (* 150 *)
  | IDeclarator_type(D_bitfield(annot, sourceLoc, pQName_opt, expression, 
				int)) ->
      add_links up down myindex
	(opt_link pQName_annotation pQName_opt
	   [expression_annotation expression])

  (* 151 *)
  | IDeclarator_type(D_ptrToMember(annot, sourceLoc, pQName, cVFlags, 
				   iDeclarator)) ->
      add_links up down myindex
	[pQName_annotation pQName;
	 iDeclarator_annotation iDeclarator]

  (* 152 *)
  | IDeclarator_type(D_grouping(annot, sourceLoc, iDeclarator)) ->
      add_links up down myindex
	[iDeclarator_annotation iDeclarator]

  (* 153 *)
  | IDeclarator_type(D_attribute(annot, sourceLoc, iDeclarator, 
				 attribute_list_list)) ->
      add_links up down myindex
	((iDeclarator_annotation iDeclarator)
	 :: (List.flatten
	       (List.map (List.map attribute_annotation) attribute_list_list)))

  (* 154 *)
  | FullExpressionAnnot_type(annot, declaration_list) ->
      add_links up down myindex
	(List.map declaration_annotation declaration_list)

  (* 155 *)
  | ASTTypeof_type(TS_typeof_expr(annot, ctype, fullExpression)) ->
      add_links up down myindex
	[cType_annotation ctype;
	 fullExpression_annotation fullExpression]

  (* 156 *)
  | ASTTypeof_type(TS_typeof_type(annot, ctype, aSTTypeId)) ->
      add_links up down myindex
	[cType_annotation ctype;
	 aSTTypeId_annotation aSTTypeId]

  (* 157 *)
  | Designator_type(FieldDesignator(annot, sourceLoc, stringRef)) ->
      ()

  (* 158 *)
  | Designator_type(SubscriptDesignator(annot, sourceLoc, expression, 
					expression_opt, idx_start, idx_end)) ->
      add_links up down myindex
	((expression_annotation expression)
	 :: (opt_link expression_annotation expression_opt []))

  (* 159 *)
  | Attribute_type(AT_empty(annot, sourceLoc)) ->
      ()

  (* 160 *)
  | Attribute_type(AT_word(annot, sourceLoc, stringRef)) ->
      ()

  (* 161 *)
  | Attribute_type(AT_func(annot, sourceLoc, stringRef, argExpression_list)) ->
      add_links up down myindex
	(List.map argExpression_annotation argExpression_list)


(**************************************************************************
 *
 * end of astmatch.ml 
 *
 **************************************************************************)


let create ast_array =
  let up = Array.create (Array.length ast_array) [] in
  let down = Array.create (Array.length ast_array) [] 
  in
    Superast.iteri (ast_node_fun up down) ast_array;
    (up, down)
