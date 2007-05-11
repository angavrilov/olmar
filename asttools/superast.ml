open Cc_ast_gen_type
open Ast_annotation
open Ast_util

type 'a super_ast =
  | TranslationUnit_type of 'a translationUnit_type
  | TopForm_type of 'a topForm_type
  | Function_type of 'a function_type
  | MemberInit_type of 'a memberInit_type
  | Declaration_type of 'a declaration_type
  | ASTTypeId_type of 'a aSTTypeId_type
  | PQName_type of 'a pQName_type
  | TypeSpecifier_type of 'a typeSpecifier_type
  | BaseClassSpec_type of 'a baseClassSpec_type
  | Enumerator_type of 'a enumerator_type
  | MemberList_type of 'a memberList_type
  | Member_type of 'a member_type
  | ExceptionSpec_type of 'a exceptionSpec_type
  | OperatorName_type of 'a operatorName_type
  | Statement_type of 'a statement_type
  | Condition_type of 'a condition_type
  | Handler_type of 'a handler_type
  | Expression_type of 'a expression_type
  | FullExpression_type of 'a fullExpression_type
  | ArgExpression_type of 'a argExpression_type
  | ArgExpressionListOpt_type of 'a argExpressionListOpt_type
  | Initializer_type of 'a initializer_type
  | TemplateDeclaration_type of 'a templateDeclaration_type
  | TemplateParameter_type of 'a templateParameter_type
  | TemplateArgument_type of 'a templateArgument_type
  | NamespaceDecl_type of 'a namespaceDecl_type
  | Declarator_type of 'a declarator_type
  | IDeclarator_type of 'a iDeclarator_type
  | FullExpressionAnnot_type of 'a fullExpressionAnnot_type
  | ASTTypeof_type of 'a aSTTypeof_type
  | Designator_type of 'a designator_type
  (* we use attribute_type list list instead of AttributeSpecifierList_type,
   * see gnu_attribute_hack.ast
   * 
   * | AttributeSpecifierList_type of 'a attributeSpecifierList_type
   * | AttributeSpecifier_type of 'a attributeSpecifier_type
   *)
  | Attribute_type of 'a attribute_type
  | Variable of 'a variable
  | TemplateInfo of 'a templateInfo
  | InheritedTemplateParams of 'a inheritedTemplateParams
  | BaseClass of 'a baseClass
  | Compound_info of 'a compound_info
  | EnumType_Value_type of 'a enumType_Value_type
  | AtomicType of 'a atomicType
  | CType of 'a cType
  | STemplateArgument of 'a sTemplateArgument
  | Scope of 'a scope 

  | NoAstNode



module Into_array = struct
  (**************************************************************************
   **************************************************************************
   **************************************************************************
   **************************************************************************
   *
   * contents of astiternodes.ml
   *
   * adoptions:
   * - unused variables are prefixed with _
   *
   **************************************************************************
   **************************************************************************
   **************************************************************************
   **************************************************************************
   **************************************************************************)

  open Cc_ml_types
  open Ml_ctype
  open Ast_util


  module DS = Dense_set

  let visited_nodes = DS.make ()

  let visited (annot : annotated) =
    DS.mem (id_annotation annot) visited_nodes

  let visit (annot : annotated) =
    (* Printf.eprintf "visit %d\n%!" (id_annotation annot); *)
    DS.add (id_annotation annot) visited_nodes


  (**************************************************************************
   *
   * contents of astiternodes.ml
   *
   **************************************************************************)

  let opt_iter f = function
    | None -> ()
    | Some x -> f x


  (***************** variable ***************************)

  let rec variable_fun ast_array (v : annotated variable) =
    (* unused record copy to provoke compilation errors for new fields *)
    let _dummy = {			
      poly_var = v.poly_var; loc = v.loc; var_name = v.var_name;
      var_type = v.var_type; flags = v.flags; value = v.value;
      defaultParam = v.defaultParam; funcDefn = v.funcDefn;
      overload = v.overload; virtuallyOverride = v.virtuallyOverride;
      scope = v.scope; templ_info = v.templ_info;
    }
    in
    let annot = variable_annotation v
    in
      if visited annot then ()
      else begin
	ast_array.(id_annotation annot) <- Variable v;
	visit annot;


	(* POSSIBLY CIRCULAR *)
	opt_iter (cType_fun ast_array) !(v.var_type);
	(* POSSIBLY CIRCULAR *)
	opt_iter (expression_fun ast_array) !(v.value);
	opt_iter (cType_fun ast_array) v.defaultParam;

	(* POSSIBLY CIRCULAR *)
	opt_iter (func_fun ast_array) !(v.funcDefn);
	(* POSSIBLY CIRCULAR *)
	List.iter (variable_fun ast_array) !(v.overload);
	List.iter (variable_fun ast_array) v.virtuallyOverride;
	opt_iter (scope_fun ast_array) v.scope;
	opt_iter (templ_info_fun ast_array) v.templ_info;
      end

  (**************** templateInfo ************************)

  and templ_info_fun ast_array ti =
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
    let annot = templ_info_annotation ti
    in
      if visited annot then ()
      else begin
	ast_array.(id_annotation annot) <- TemplateInfo ti;
	visit annot;

	List.iter (variable_fun ast_array) ti.template_params;

	(* POSSIBLY CIRCULAR *)
	opt_iter (variable_fun ast_array) !(ti.template_var);
	List.iter (inherited_templ_params_fun ast_array) ti.inherited_params;

	(* POSSIBLY CIRCULAR *)
	opt_iter (variable_fun ast_array) !(ti.instantiation_of);
	List.iter (variable_fun ast_array) ti.instantiations;

	(* POSSIBLY CIRCULAR *)
	opt_iter (variable_fun ast_array) !(ti.specialization_of);
	List.iter (variable_fun ast_array) ti.specializations;
	List.iter (sTemplateArgument_fun ast_array) ti.arguments;

	(* POSSIBLY CIRCULAR *)
	opt_iter (variable_fun ast_array) !(ti.partial_instantiation_of);
	List.iter (variable_fun ast_array) ti.partial_instantiations;
	List.iter (sTemplateArgument_fun ast_array) ti.arguments_to_primary;
	opt_iter (scope_fun ast_array) ti.defn_scope;
	opt_iter (templ_info_fun ast_array) ti.definition_template_info;
	List.iter (cType_fun ast_array) ti.dependent_bases;
      end

  (************* inheritedTemplateParams ****************)

  and inherited_templ_params_fun ast_array itp =
    (* unused record copy to provoke compilation errors for new fields *)
    let _dummy = {
      poly_inherited_templ = itp.poly_inherited_templ;
      inherited_template_params = itp.inherited_template_params;
      enclosing = itp.enclosing;
    }
    in
    let annot = inherited_templ_params_annotation itp
    in
      if visited annot then ()
      else begin
	assert(!(itp.enclosing) <> None);
	ast_array.(id_annotation annot) <- InheritedTemplateParams itp;

	visit annot;

	List.iter (variable_fun ast_array) itp.inherited_template_params;

	(* POSSIBLY CIRCULAR *)
	opt_iter (compound_info_fun ast_array) !(itp.enclosing);
      end

  (***************** cType ******************************)

  and baseClass_fun ast_array baseClass =
    (* unused record copy to provoke compilation errors for new fields *)
    let _dummy = {
      poly_base = baseClass.poly_base; compound = baseClass.compound;
      bc_access = baseClass.bc_access; is_virtual = baseClass.is_virtual
    }
    in
    let annot = baseClass_annotation baseClass
    in
      if visited annot then ()
      else begin
	ast_array.(id_annotation annot) <- BaseClass baseClass;
	visit annot;
	compound_info_fun ast_array baseClass.compound;
      end


  and compound_info_fun ast_array i = 
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
    let annot = compound_info_annotation i
    in
      if visited annot then ()
      else begin
	ast_array.(id_annotation annot) <- Compound_info i;
	visit annot;
	assert(match !(i.syntax) with
		 | None
		 | Some(TS_classSpec _) -> true
		 | _ -> false);
	variable_fun ast_array i.typedef_var;
	scope_fun ast_array i.compound_scope;
	List.iter (variable_fun ast_array) i.data_members;
	List.iter (baseClass_fun ast_array) i.bases;
	List.iter (variable_fun ast_array) i.conversion_operators;
	List.iter (variable_fun ast_array) i.friends;

	(* POSSIBLY CIRCULAR *)
	opt_iter (typeSpecifier_fun ast_array) !(i.syntax);

	(* POSSIBLY CIRCULAR *)
	opt_iter (cType_fun ast_array) !(i.self_type)
      end


  and enum_value_fun ast_array ((annot, _string, _nativeint) as x) =
    if visited annot then ()
    else begin
      ast_array.(id_annotation annot) <- EnumType_Value_type x;
      visit annot;
    end
	

  and atomicType_fun ast_array x = 
    let annot = atomicType_annotation x
    in
      if visited annot then ()
      else 
	match x with
	  (*
	   * put calls to visit here before in each case, except for CompoundType
	   *)

	| SimpleType(annot, _simpleTypeId) ->
	    ast_array.(id_annotation annot) <- AtomicType x;
	    visit annot

	| CompoundType(compound_info) ->
	    (* assign a Compound_info into ast_array *)
	    compound_info_fun ast_array compound_info

	| PseudoInstantiation(annot, _str, variable_opt, _accessKeyword, 
			      compound_info, sTemplateArgument_list) ->
	    ast_array.(id_annotation annot) <- AtomicType x;
	    visit annot;
	    opt_iter (variable_fun ast_array) variable_opt;
	    compound_info_fun ast_array compound_info;
	    List.iter (sTemplateArgument_fun ast_array) sTemplateArgument_list

	| EnumType(annot, _string, variable, _accessKeyword, 
		   enum_value_list, _has_negatives) ->
	    ast_array.(id_annotation annot) <- AtomicType x;
	    visit annot;
	    opt_iter (variable_fun ast_array) variable;
	    List.iter (enum_value_fun ast_array) enum_value_list

	| TypeVariable(annot, _string, variable, _accessKeyword) ->
	    ast_array.(id_annotation annot) <- AtomicType x;
	    visit annot;
	    variable_fun ast_array variable;

	| DependentQType(annot, _string, variable, 
			_accessKeyword, atomic, pq_name) ->
	    ast_array.(id_annotation annot) <- AtomicType x;
	    visit annot;
	    variable_fun ast_array variable;
	    atomicType_fun ast_array atomic;
	    pQName_fun ast_array pq_name



  and cType_fun ast_array x = 
    let annot = cType_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- CType x in
	let _ = visit annot 
	in match x with
	| CVAtomicType(_annot, _cVFlags, atomicType) ->
	    atomicType_fun ast_array atomicType

	| PointerType(_annot, _cVFlags, cType) ->
	    cType_fun ast_array cType

	| ReferenceType(_annot, cType) ->
	    cType_fun ast_array cType

	| FunctionType(_annot, _function_flags, cType, 
		       variable_list, cType_list_opt) ->
	    cType_fun ast_array cType;
	    List.iter (variable_fun ast_array) variable_list;
	    opt_iter (List.iter (cType_fun ast_array)) cType_list_opt

	| ArrayType(_annot, cType, _array_size) ->
	    cType_fun ast_array cType;

	| PointerToMemberType(_annot, atomicType (* = NamedAtomicType *), 
			      _cVFlags, cType) ->
	    assert(match atomicType with 
		     | SimpleType _ -> false
		     | CompoundType _
		     | PseudoInstantiation _
		     | EnumType _
		     | TypeVariable _ 
		     | DependentQType _ -> true);
	    atomicType_fun ast_array atomicType;
	    cType_fun ast_array cType


  and sTemplateArgument_fun ast_array ta = 
    let annot = sTemplateArgument_annotation ta
    in
      if visited annot then ()
      else 
	let _ = ast_array.(id_annotation annot) <- STemplateArgument ta in
	let _ = visit annot 
	in match ta with
	  | STA_NONE _annot -> 
	      ()

	  | STA_TYPE(_annot, cType) -> 
	      cType_fun ast_array cType

	  | STA_INT(_annot, _int) -> 
	      ()

	  | STA_ENUMERATOR(_annot, variable) -> 
	      variable_fun ast_array variable

	  | STA_REFERENCE(_annot, variable) -> 
	      variable_fun ast_array variable

	  | STA_POINTER(_annot, variable) -> 
	      variable_fun ast_array variable

	  | STA_MEMBER(_annot, variable) -> 
	      variable_fun ast_array variable

	  | STA_DEPEXPR(_annot, expression) -> 
	      expression_fun ast_array expression

	  | STA_TEMPLATE _annot -> 
	      ()

	  | STA_ATOMIC(_annot, atomicType) -> 
	      atomicType_fun ast_array atomicType


  and scope_fun ast_array s = 
    (* unused record copy to provoke compilation errors for new fields *)
    let _dummy = {
      poly_scope = s.poly_scope; variables = s.variables; 
      type_tags = s.type_tags; parent_scope = s.parent_scope;
      scope_kind = s.scope_kind; namespace_var = s.namespace_var;
      scope_template_params = s.scope_template_params; 
      parameterized_entity = s.parameterized_entity
    }
    in
    let annot = scope_annotation s
    in
      if visited annot then ()
      else begin
	ast_array.(id_annotation annot) <- Scope s;
	visit annot;
	Hashtbl.iter 
	  (fun _str var -> variable_fun ast_array var)
	  s.variables;
	Hashtbl.iter
	  (fun _str var -> variable_fun ast_array var)
	  s.type_tags;
	opt_iter (scope_fun ast_array) s.parent_scope;
	opt_iter (variable_fun ast_array) !(s.namespace_var);
	List.iter (variable_fun ast_array) s.scope_template_params;
	opt_iter (variable_fun ast_array) s.parameterized_entity;
      end



  (***************** generated ast nodes ****************)

  and translationUnit_fun ast_array
      ((annot, topForm_list, scope_opt) as x : annotated translationUnit_type) =
    if visited annot then ()
    else begin
      ast_array.(id_annotation annot) <- TranslationUnit_type x;
      visit annot;
      List.iter (topForm_fun ast_array) topForm_list;
      opt_iter (scope_fun ast_array) scope_opt
    end


  and topForm_fun ast_array x = 
    let annot = topForm_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- TopForm_type x in
	let _ = visit annot 
	in match x with
	  | TF_decl(_annot, _sourceLoc, declaration) -> 
	      declaration_fun ast_array declaration

	  | TF_func(_annot, _sourceLoc, func) -> 
	      func_fun ast_array func

	  | TF_template(_annot, _sourceLoc, templateDeclaration) -> 
	      templateDeclaration_fun ast_array templateDeclaration

	  | TF_explicitInst(_annot, _sourceLoc, _declFlags, declaration) -> 
	      declaration_fun ast_array declaration

	  | TF_linkage(_annot, _sourceLoc, _stringRef, translationUnit) -> 
	      translationUnit_fun ast_array translationUnit

	  | TF_one_linkage(_annot, _sourceLoc, _stringRef, topForm) -> 
	      topForm_fun ast_array topForm

	  | TF_asm(_annot, _sourceLoc, e_stringLit) -> 
	      assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
	      expression_fun ast_array e_stringLit

	  | TF_namespaceDefn(_annot, _sourceLoc, _stringRef_opt, topForm_list) -> 
	      List.iter (topForm_fun ast_array) topForm_list

	  | TF_namespaceDecl(_annot, _sourceLoc, namespaceDecl) -> 
	      namespaceDecl_fun ast_array namespaceDecl



  and func_fun ast_array
      ((annot, _declFlags, typeSpecifier, declarator, memberInit_list, 
	s_compound_opt, handler_list, func, variable_opt_1, 
	variable_opt_2, statement_opt, _bool) as x) =

    if visited annot then ()
    else begin
      assert(match s_compound_opt with
	       | None -> true
	       | Some s_compound ->
		   match s_compound with 
		     | S_compound _ -> true 
		     | _ -> false);
      assert(match func with 
	| FunctionType _ -> true
	| _ -> false);
      ast_array.(id_annotation annot) <- Function_type x;
      visit annot;
      typeSpecifier_fun ast_array typeSpecifier;
      declarator_fun ast_array declarator;
      List.iter (memberInit_fun ast_array) memberInit_list;
      opt_iter (statement_fun ast_array) s_compound_opt;
      List.iter (handler_fun ast_array) handler_list;
      cType_fun ast_array func;
      opt_iter (variable_fun ast_array) variable_opt_1;
      opt_iter (variable_fun ast_array) variable_opt_2;
      opt_iter (statement_fun ast_array) statement_opt;
    end


  and memberInit_fun ast_array((annot, pQName, argExpression_list, 
		     variable_opt_1, compound_opt, variable_opt_2, 
		     full_expr_annot, statement_opt) as x) =

    if visited annot then ()
    else begin
	assert(match compound_opt with
	  | None
	  | Some(CompoundType _) -> true
	  | _ -> false);
      ast_array.(id_annotation annot) <- MemberInit_type x;
      visit annot;
      pQName_fun ast_array pQName;
      List.iter (argExpression_fun ast_array) argExpression_list;
      opt_iter (variable_fun ast_array) variable_opt_1;
      opt_iter (atomicType_fun ast_array) compound_opt;
      opt_iter (variable_fun ast_array) variable_opt_2;
      fullExpressionAnnot_fun ast_array full_expr_annot;
      opt_iter (statement_fun ast_array) statement_opt
    end


  and declaration_fun ast_array((annot, _declFlags, typeSpecifier, declarator_list) as x) =
    if visited annot then ()
    else begin
      ast_array.(id_annotation annot) <- Declaration_type x;
      visit annot;
      typeSpecifier_fun ast_array typeSpecifier;
      List.iter (declarator_fun ast_array) declarator_list
    end


  and aSTTypeId_fun ast_array((annot, typeSpecifier, declarator) as x) =
    if visited annot then ()
    else begin
      ast_array.(id_annotation annot) <- ASTTypeId_type x;
      visit annot;
      typeSpecifier_fun ast_array typeSpecifier;
      declarator_fun ast_array declarator
    end


  and pQName_fun ast_array x = 
    let annot = pQName_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- PQName_type x in
	let _ = visit annot 
	in match x with
	  | PQ_qualifier(_annot, _sourceLoc, _stringRef_opt, 
			 templateArgument_opt, pQName, 
			variable_opt, s_template_arg_list) -> 
	      opt_iter (templateArgument_fun ast_array) templateArgument_opt;
	      pQName_fun ast_array pQName;
	      opt_iter (variable_fun ast_array) variable_opt;
	      List.iter (sTemplateArgument_fun ast_array) s_template_arg_list

	  | PQ_name(_annot, _sourceLoc, _stringRef) -> 
	      ()

	  | PQ_operator(_annot, _sourceLoc, operatorName, _stringRef) -> 
	      operatorName_fun ast_array operatorName;

	  | PQ_template(_annot, _sourceLoc, _stringRef, templateArgument_opt, 
		       s_template_arg_list) -> 
	      opt_iter (templateArgument_fun ast_array) templateArgument_opt;
	      List.iter (sTemplateArgument_fun ast_array) s_template_arg_list

	  | PQ_variable(_annot, _sourceLoc, variable) -> 
	      variable_fun ast_array variable



  and typeSpecifier_fun ast_array x = 
    let annot = typeSpecifier_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- TypeSpecifier_type x in
	let _ = visit annot 
	in match x with
	  | TS_name(_annot, _sourceLoc, _cVFlags, pQName, _bool, 
		   var_opt_1, var_opt_2) -> 
	      pQName_fun ast_array pQName;
	      opt_iter (variable_fun ast_array) var_opt_1;
	      opt_iter (variable_fun ast_array) var_opt_2

	  | TS_simple(_annot, _sourceLoc, _cVFlags, _simpleTypeId) -> 
	      ()

	  | TS_elaborated(_annot, _sourceLoc, _cVFlags, _typeIntr, 
			 pQName, namedAtomicType_opt) -> 
	      assert(match namedAtomicType_opt with
		| Some(SimpleType _) -> false
		| _ -> true);
	      pQName_fun ast_array pQName;
	      opt_iter (atomicType_fun ast_array) namedAtomicType_opt

	  | TS_classSpec(_annot, _sourceLoc, _cVFlags, _typeIntr, pQName_opt, 
			 baseClassSpec_list, memberList, compoundType) -> 
	      assert(match compoundType with
		| CompoundType _ -> true
		| _ -> false);
	      opt_iter (pQName_fun ast_array) pQName_opt;
	      List.iter (baseClassSpec_fun ast_array) baseClassSpec_list;
	      memberList_fun ast_array memberList;
	      atomicType_fun ast_array compoundType

	  | TS_enumSpec(_annot, _sourceLoc, _cVFlags, 
			_stringRef_opt, enumerator_list, enumType) -> 
	      assert(match enumType with 
		| EnumType _ -> true
		| _ -> false);
	      List.iter (enumerator_fun ast_array) enumerator_list;
	      atomicType_fun ast_array enumType

	  | TS_type(_annot, _sourceLoc, _cVFlags, cType) -> 
	      cType_fun ast_array cType

	  | TS_typeof(_annot, _sourceLoc, _cVFlags, aSTTypeof) -> 
	      aSTTypeof_fun ast_array aSTTypeof


  and baseClassSpec_fun ast_array
      ((annot, _bool, _accessKeyword, pQName, compoundType_opt) as x) =
    if visited annot then ()
    else begin
      assert(match compoundType_opt with
	       | None
	       | Some(CompoundType _ ) -> true
	       | _ -> false);
      ast_array.(id_annotation annot) <- BaseClassSpec_type x;
      visit annot;
      pQName_fun ast_array pQName;
      opt_iter (atomicType_fun ast_array) compoundType_opt
    end


  and enumerator_fun ast_array((annot, _sourceLoc, _stringRef, 
		     expression_opt, variable, _int32) as x) =
    if visited annot then ()
    else begin
      ast_array.(id_annotation annot) <- Enumerator_type x;
      visit annot;
      opt_iter (expression_fun ast_array) expression_opt;
      variable_fun ast_array variable;
    end


  and memberList_fun ast_array((annot, member_list) as x) =
    if visited annot then ()
    else begin
      ast_array.(id_annotation annot) <- MemberList_type x;
      visit annot;
      List.iter (member_fun ast_array) member_list
    end


  and member_fun ast_array x = 
    let annot = member_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- Member_type x in
	let _ = visit annot 
	in match x with
	  | MR_decl(_annot, _sourceLoc, declaration) -> 
	      declaration_fun ast_array declaration

	  | MR_func(_annot, _sourceLoc, func) -> 
	      func_fun ast_array func

	  | MR_access(_annot, _sourceLoc, _accessKeyword) -> 
	      ()

	  | MR_usingDecl(_annot, _sourceLoc, nd_usingDecl) -> 
	      assert(match nd_usingDecl with ND_usingDecl _ -> true | _ -> false);
	      namespaceDecl_fun ast_array nd_usingDecl

	  | MR_template(_annot, _sourceLoc, templateDeclaration) -> 
	      templateDeclaration_fun ast_array templateDeclaration


  and declarator_fun ast_array((annot, iDeclarator, init_opt, 
		     variable_opt, ctype_opt, _declaratorContext,
		     statement_opt_ctor, statement_opt_dtor) as x) =
    if visited annot then ()
    else begin
      ast_array.(id_annotation annot) <- Declarator_type x;
      visit annot;
      iDeclarator_fun ast_array iDeclarator;
      opt_iter (init_fun ast_array) init_opt;
      opt_iter (variable_fun ast_array) variable_opt;
      opt_iter (cType_fun ast_array) ctype_opt;
      opt_iter (statement_fun ast_array) statement_opt_ctor;
      opt_iter (statement_fun ast_array) statement_opt_dtor
    end


  and iDeclarator_fun ast_array x = 
    let annot = iDeclarator_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- IDeclarator_type x in
	let _ = visit annot 
	in match x with
	  | D_name(_annot, _sourceLoc, pQName_opt) -> 
	      opt_iter (pQName_fun ast_array) pQName_opt

	  | D_pointer(_annot, _sourceLoc, _cVFlags, iDeclarator) -> 
	      iDeclarator_fun ast_array iDeclarator

	  | D_reference(_annot, _sourceLoc, iDeclarator) -> 
	      iDeclarator_fun ast_array iDeclarator

	  | D_func(_annot, _sourceLoc, iDeclarator, aSTTypeId_list, _cVFlags, 
		   exceptionSpec_opt, pq_name_list, _bool) -> 
	      assert(List.for_all (function | PQ_name _ -> true | _ -> false) 
		       pq_name_list);
	      iDeclarator_fun ast_array iDeclarator;
	      List.iter (aSTTypeId_fun ast_array) aSTTypeId_list;
	      opt_iter (exceptionSpec_fun ast_array) exceptionSpec_opt;
	      List.iter (pQName_fun ast_array) pq_name_list;

	  | D_array(_annot, _sourceLoc, iDeclarator, expression_opt, _bool) -> 
	      iDeclarator_fun ast_array iDeclarator;
	      opt_iter (expression_fun ast_array) expression_opt;

	  | D_bitfield(_annot, _sourceLoc, pQName_opt, expression, _int) -> 
	      opt_iter (pQName_fun ast_array) pQName_opt;
	      expression_fun ast_array expression;

	  | D_ptrToMember(_annot, _sourceLoc, pQName, _cVFlags, iDeclarator) -> 
	      pQName_fun ast_array pQName;
	      iDeclarator_fun ast_array iDeclarator

	  | D_grouping(_annot, _sourceLoc, iDeclarator) -> 
	      iDeclarator_fun ast_array iDeclarator

	  | D_attribute(_annot, _sourceLoc, iDeclarator, attribute_list_list) ->
	      iDeclarator_fun ast_array iDeclarator;
	      List.iter
		(List.iter (attribute_fun ast_array)) 
		attribute_list_list



  and exceptionSpec_fun ast_array((annot, aSTTypeId_list) as x) =
    if visited annot then ()
    else begin
      ast_array.(id_annotation annot) <- ExceptionSpec_type x;
      visit annot;
      List.iter (aSTTypeId_fun ast_array) aSTTypeId_list
    end


  and operatorName_fun ast_array x = 
    let annot = operatorName_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- OperatorName_type x in
	let _ = visit annot 
	in match x with
	  | ON_newDel(_annot, _bool_is_new, _bool_is_array) -> 
	      ()

	  | ON_operator(_annot, _overloadableOp) -> 
	      ()

	  | ON_conversion(_annot, aSTTypeId) -> 
	      aSTTypeId_fun ast_array aSTTypeId


  and statement_fun ast_array x = 
    let annot = statement_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- Statement_type x in
	let _ = visit annot 
	in match x with
	  | S_skip(_annot, _sourceLoc) -> 
	      ()

	  | S_label(_annot, _sourceLoc, _stringRef, statement) -> 
	      statement_fun ast_array statement

	  | S_case(_annot, _sourceLoc, expression, statement, _int) -> 
	      expression_fun ast_array expression;
	      statement_fun ast_array statement;

	  | S_default(_annot, _sourceLoc, statement) -> 
	      statement_fun ast_array statement

	  | S_expr(_annot, _sourceLoc, fullExpression) -> 
	      fullExpression_fun ast_array fullExpression

	  | S_compound(_annot, _sourceLoc, statement_list) -> 
	      List.iter (statement_fun ast_array) statement_list

	  | S_if(_annot, _sourceLoc, condition, statement_then, statement_else) -> 
	      condition_fun ast_array condition;
	      statement_fun ast_array statement_then;
	      statement_fun ast_array statement_else

	  | S_switch(_annot, _sourceLoc, condition, statement) -> 
	      condition_fun ast_array condition;
	      statement_fun ast_array statement

	  | S_while(_annot, _sourceLoc, condition, statement) -> 
	      condition_fun ast_array condition;
	      statement_fun ast_array statement

	  | S_doWhile(_annot, _sourceLoc, statement, fullExpression) -> 
	      statement_fun ast_array statement;
	      fullExpression_fun ast_array fullExpression

	  | S_for(_annot, _sourceLoc, statement_init, condition, fullExpression, 
		  statement_body) -> 
	      statement_fun ast_array statement_init;
	      condition_fun ast_array condition;
	      fullExpression_fun ast_array fullExpression;
	      statement_fun ast_array statement_body

	  | S_break(_annot, _sourceLoc) -> 
	      ()

	  | S_continue(_annot, _sourceLoc) -> 
	      ()

	  | S_return(_annot, _sourceLoc, fullExpression_opt, statement_opt) -> 
	      opt_iter (fullExpression_fun ast_array) fullExpression_opt;
	      opt_iter (statement_fun ast_array) statement_opt

	  | S_goto(_annot, _sourceLoc, _stringRef) -> 
	      ()

	  | S_decl(_annot, _sourceLoc, declaration) -> 
	      declaration_fun ast_array declaration

	  | S_try(_annot, _sourceLoc, statement, handler_list) -> 
	      statement_fun ast_array statement;
	      List.iter (handler_fun ast_array) handler_list

	  | S_asm(_annot, _sourceLoc, e_stringLit) -> 
	      assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
	      expression_fun ast_array e_stringLit

	  | S_namespaceDecl(_annot, _sourceLoc, namespaceDecl) -> 
	      namespaceDecl_fun ast_array namespaceDecl

	  | S_function(_annot, _sourceLoc, func) -> 
	      func_fun ast_array func

	  | S_rangeCase(_annot, _sourceLoc, 
			expression_lo, expression_hi, statement, 
		       _label_lo, _label_hi) -> 
	      expression_fun ast_array expression_lo;
	      expression_fun ast_array expression_hi;
	      statement_fun ast_array statement;

	  | S_computedGoto(_annot, _sourceLoc, expression) -> 
	      expression_fun ast_array expression


  and condition_fun ast_array x = 
    let annot = condition_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- Condition_type x in
	let _ = visit annot 
	in match x with
	  | CN_expr(_annot, fullExpression) -> 
	      fullExpression_fun ast_array fullExpression

	  | CN_decl(_annot, aSTTypeId) -> 
	      aSTTypeId_fun ast_array aSTTypeId


  and handler_fun ast_array((annot, aSTTypeId, statement_body, variable_opt, 
		  fullExpressionAnnot, expression_opt, statement_gdtor) as x) =
    if visited annot then ()
    else begin
      ast_array.(id_annotation annot) <- Handler_type x;
      visit annot;
      aSTTypeId_fun ast_array aSTTypeId;
      statement_fun ast_array statement_body;
      opt_iter (variable_fun ast_array) variable_opt;
      fullExpressionAnnot_fun ast_array fullExpressionAnnot;
      opt_iter (expression_fun ast_array) expression_opt;
      opt_iter (statement_fun ast_array) statement_gdtor
    end


  and expression_fun ast_array x = 
    let annot = expression_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- Expression_type x in
	let _ = visit annot 
	in match x with
	  | E_boolLit(_annot, type_opt, _bool) -> 
	      opt_iter (cType_fun ast_array) type_opt;

	  | E_intLit(_annot, type_opt, _stringRef, _ulong) -> 
	      opt_iter (cType_fun ast_array) type_opt;

	  | E_floatLit(_annot, type_opt, _stringRef, _double) -> 
	      opt_iter (cType_fun ast_array) type_opt;

	  | E_stringLit(_annot, type_opt, _stringRef, 
			e_stringLit_opt, _stringRef_opt) -> 
	      assert(match e_stringLit_opt with 
		       | Some(E_stringLit _) -> true 
		       | None -> true
		       | _ -> false);
	      opt_iter (cType_fun ast_array) type_opt;
	      opt_iter (expression_fun ast_array) e_stringLit_opt

	  | E_charLit(_annot, type_opt, _stringRef, _int32) -> 
	      opt_iter (cType_fun ast_array) type_opt;

	  | E_this(_annot, type_opt, variable) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      variable_fun ast_array variable

	  | E_variable(_annot, type_opt, pQName, var_opt, nondep_var_opt) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      pQName_fun ast_array pQName;
	      opt_iter (variable_fun ast_array) var_opt;
	      opt_iter (variable_fun ast_array) nondep_var_opt

	  | E_funCall(_annot, type_opt, expression_func, 
		      argExpression_list, expression_retobj_opt) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression_func;
	      List.iter (argExpression_fun ast_array) argExpression_list;
	      opt_iter (expression_fun ast_array) expression_retobj_opt

	  | E_constructor(_annot, type_opt, typeSpecifier, argExpression_list, 
			  var_opt, _bool, expression_opt) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      typeSpecifier_fun ast_array typeSpecifier;
	      List.iter (argExpression_fun ast_array) argExpression_list;
	      opt_iter (variable_fun ast_array) var_opt;
	      opt_iter (expression_fun ast_array) expression_opt

	  | E_fieldAcc(_annot, type_opt, expression, pQName, var_opt) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression;
	      pQName_fun ast_array pQName;
	      opt_iter (variable_fun ast_array) var_opt

	  | E_sizeof(_annot, type_opt, expression, _int) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression;

	  | E_unary(_annot, type_opt, _unaryOp, expression) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression

	  | E_effect(_annot, type_opt, _effectOp, expression) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression

	  | E_binary(_annot, type_opt, expression_left, _binaryOp, expression_right) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression_left;
	      expression_fun ast_array expression_right

	  | E_addrOf(_annot, type_opt, expression) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression

	  | E_deref(_annot, type_opt, expression) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression

	  | E_cast(_annot, type_opt, aSTTypeId, expression, _bool) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      aSTTypeId_fun ast_array aSTTypeId;
	      expression_fun ast_array expression;

	  | E_cond(_annot, type_opt, expression_cond, expression_true, expression_false) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression_cond;
	      expression_fun ast_array expression_true;
	      expression_fun ast_array expression_false

	  | E_sizeofType(_annot, type_opt, aSTTypeId, _int, _bool) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      aSTTypeId_fun ast_array aSTTypeId;

	  | E_assign(_annot, type_opt, expression_target, _binaryOp, expression_src) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression_target;
	      expression_fun ast_array expression_src

	  | E_new(_annot, type_opt, _bool, argExpression_list, aSTTypeId, 
		  argExpressionListOpt_opt, array_size_opt, ctor_opt,
		  statement_opt, heep_var) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      List.iter (argExpression_fun ast_array) argExpression_list;
	      aSTTypeId_fun ast_array aSTTypeId;
	      opt_iter (argExpressionListOpt_fun ast_array) argExpressionListOpt_opt;
	      opt_iter (expression_fun ast_array) array_size_opt;
	      opt_iter (variable_fun ast_array) ctor_opt;
	      opt_iter (statement_fun ast_array) statement_opt;
	      opt_iter (variable_fun ast_array) heep_var

	  | E_delete(_annot, type_opt, _bool_colon, _bool_array, 
		     expression_opt, statement_opt) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      opt_iter (expression_fun ast_array) expression_opt;
	      opt_iter (statement_fun ast_array) statement_opt

	  | E_throw(_annot, type_opt, expression_opt, var_opt, statement_opt) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      opt_iter (expression_fun ast_array) expression_opt;
	      opt_iter (variable_fun ast_array) var_opt;
	      opt_iter (statement_fun ast_array) statement_opt

	  | E_keywordCast(_annot, type_opt, _castKeyword, aSTTypeId, expression) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      aSTTypeId_fun ast_array aSTTypeId;
	      expression_fun ast_array expression

	  | E_typeidExpr(_annot, type_opt, expression) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression

	  | E_typeidType(_annot, type_opt, aSTTypeId) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      aSTTypeId_fun ast_array aSTTypeId

	  | E_grouping(_annot, type_opt, expression) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression

	  | E_arrow(_annot, type_opt, expression, pQName) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression;
	      pQName_fun ast_array pQName

	  | E_statement(_annot, type_opt, s_compound) -> 
	      assert(match s_compound with | S_compound _ -> true | _ -> false);
	      opt_iter (cType_fun ast_array) type_opt;
	      statement_fun ast_array s_compound

	  | E_compoundLit(_annot, type_opt, aSTTypeId, in_compound) -> 
	      assert(match in_compound with | IN_compound _ -> true | _ -> false);
	      opt_iter (cType_fun ast_array) type_opt;
	      aSTTypeId_fun ast_array aSTTypeId;
	      init_fun ast_array in_compound

	  | E___builtin_constant_p(_annot, type_opt, _sourceLoc, expression) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression

	  | E___builtin_va_arg(_annot, type_opt, _sourceLoc, expression, aSTTypeId) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression;
	      aSTTypeId_fun ast_array aSTTypeId

	  | E_alignofType(_annot, type_opt, aSTTypeId, _int) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      aSTTypeId_fun ast_array aSTTypeId;

	  | E_alignofExpr(_annot, type_opt, expression, _int) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression;

	  | E_gnuCond(_annot, type_opt, expression_cond, expression_false) -> 
	      opt_iter (cType_fun ast_array) type_opt;
	      expression_fun ast_array expression_cond;
	      expression_fun ast_array expression_false

	  | E_addrOfLabel(_annot, type_opt, _stringRef) -> 
	      opt_iter (cType_fun ast_array) type_opt;


  and fullExpression_fun ast_array
      ((annot, expression_opt, fullExpressionAnnot) as x) =
    if visited annot then ()
    else begin
      ast_array.(id_annotation annot) <- FullExpression_type x;
      visit annot;
      opt_iter (expression_fun ast_array) expression_opt;
      fullExpressionAnnot_fun ast_array fullExpressionAnnot
    end


  and argExpression_fun ast_array((annot, expression) as x) =
    if visited annot then ()
    else begin
      ast_array.(id_annotation annot) <- ArgExpression_type x;
      visit annot;
      expression_fun ast_array expression
    end


  and argExpressionListOpt_fun ast_array((annot, argExpression_list) as x) =
    if visited annot then ()
    else begin
      ast_array.(id_annotation annot) <- ArgExpressionListOpt_type x;
      visit annot;
      List.iter (argExpression_fun ast_array) argExpression_list
    end


  and init_fun ast_array x = 
    let annot = init_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- Initializer_type x in
	let _ = visit annot 
	in match x with
	  | IN_expr(_annot, _sourceLoc, fullExpressionAnnot, expression_opt) -> 
	      fullExpressionAnnot_fun ast_array fullExpressionAnnot;
	      opt_iter (expression_fun ast_array) expression_opt

	  | IN_compound(_annot, _sourceLoc, fullExpressionAnnot, init_list) -> 
	      fullExpressionAnnot_fun ast_array fullExpressionAnnot;
	      List.iter (init_fun ast_array) init_list

	  | IN_ctor(_annot, _sourceLoc, fullExpressionAnnot, 
		   argExpression_list, var_opt, _bool) -> 
	      fullExpressionAnnot_fun ast_array fullExpressionAnnot;
	      List.iter (argExpression_fun ast_array) argExpression_list;
	      opt_iter (variable_fun ast_array) var_opt;

	  | IN_designated(_annot, _sourceLoc, fullExpressionAnnot, 
			 designator_list, init) -> 
	      fullExpressionAnnot_fun ast_array fullExpressionAnnot;
	      List.iter (designator_fun ast_array) designator_list;
	      init_fun ast_array init


  and templateDeclaration_fun ast_array x = 
    let annot = templateDeclaration_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- TemplateDeclaration_type x in
	let _ = visit annot 
	in match x with
	  | TD_func(_annot, templateParameter_opt, func) -> 
	      opt_iter (templateParameter_fun ast_array) templateParameter_opt;
	      func_fun ast_array func

	  | TD_decl(_annot, templateParameter_opt, declaration) -> 
	      opt_iter (templateParameter_fun ast_array) templateParameter_opt;
	      declaration_fun ast_array declaration

	  | TD_tmember(_annot, templateParameter_opt, templateDeclaration) -> 
	      opt_iter (templateParameter_fun ast_array) templateParameter_opt;
	      templateDeclaration_fun ast_array templateDeclaration


  and templateParameter_fun ast_array x = 
    let annot = templateParameter_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- TemplateParameter_type x in
	let _ = visit annot 
	in match x with
	  | TP_type(_annot, _sourceLoc, variable, _stringRef, 
		    aSTTypeId_opt, templateParameter_opt) -> 
	      variable_fun ast_array variable;
	      opt_iter (aSTTypeId_fun ast_array) aSTTypeId_opt;
	      opt_iter (templateParameter_fun ast_array) templateParameter_opt

	  | TP_nontype(_annot, _sourceLoc, variable,
		      aSTTypeId, templateParameter_opt) -> 
	      variable_fun ast_array variable;
	      aSTTypeId_fun ast_array aSTTypeId;
	      opt_iter (templateParameter_fun ast_array) templateParameter_opt


  and templateArgument_fun ast_array x = 
    let annot = templateArgument_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- TemplateArgument_type x in
	let _ = visit annot 
	in match x with
	  | TA_type(_annot, aSTTypeId, templateArgument_opt) -> 
	      aSTTypeId_fun ast_array aSTTypeId;
	      opt_iter (templateArgument_fun ast_array) templateArgument_opt

	  | TA_nontype(_annot, expression, templateArgument_opt) -> 
	      expression_fun ast_array expression;
	      opt_iter (templateArgument_fun ast_array) templateArgument_opt

	  | TA_templateUsed(_annot, templateArgument_opt) -> 
	      opt_iter (templateArgument_fun ast_array) templateArgument_opt


  and namespaceDecl_fun ast_array x = 
    let annot = namespaceDecl_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- NamespaceDecl_type x in
	let _ = visit annot 
	in match x with
	  | ND_alias(_annot, _stringRef, pQName) -> 
	      pQName_fun ast_array pQName

	  | ND_usingDecl(_annot, pQName) -> 
	      pQName_fun ast_array pQName

	  | ND_usingDir(_annot, pQName) -> 
	      pQName_fun ast_array pQName


  and fullExpressionAnnot_fun ast_array((annot, declaration_list) as x) =
    if visited annot then ()
    else begin
      ast_array.(id_annotation annot) <- FullExpressionAnnot_type x;
      visit annot;
      List.iter (declaration_fun ast_array) declaration_list
    end


  and aSTTypeof_fun ast_array x = 
    let annot = aSTTypeof_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- ASTTypeof_type x in
	let _ = visit annot 
	in match x with
	  | TS_typeof_expr(_annot, ctype, fullExpression) -> 
	      cType_fun ast_array ctype;
	      fullExpression_fun ast_array fullExpression

	  | TS_typeof_type(_annot, ctype, aSTTypeId) -> 
	      cType_fun ast_array ctype;
	      aSTTypeId_fun ast_array aSTTypeId


  and designator_fun ast_array x = 
    let annot = designator_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- Designator_type x in
	let _ = visit annot 
	in match x with
	  | FieldDesignator(_annot, _sourceLoc, _stringRef) -> 
	      ()

	  | SubscriptDesignator(_annot, _sourceLoc, expression, expression_opt, 
			       _idx_start, _idx_end) -> 
	      expression_fun ast_array expression;
	      opt_iter (expression_fun ast_array) expression_opt;


  and attribute_fun ast_array x = 
    let annot = attribute_annotation x
    in
      if visited annot then ()
      else
	let _ = ast_array.(id_annotation annot) <- Attribute_type x in
	let _ = visit annot 
	in match x with
	  | AT_empty(_annot, _sourceLoc) -> 
	      ()

	  | AT_word(_annot, _sourceLoc, _stringRef) -> 
	      ()

	  | AT_func(_annot, _sourceLoc, _stringRef, argExpression_list) -> 
	      List.iter (argExpression_fun ast_array) argExpression_list


  (**************************************************************************
   **************************************************************************
   **************************************************************************
   **************************************************************************
   **************************************************************************
   *
   * end of astiternodes.ml 
   *
   **************************************************************************
   **************************************************************************
   **************************************************************************
   **************************************************************************
   **************************************************************************)

end    (* of module Into_array *)


let into_array max_node ast =
  let ast_array = Array.create (max_node +1) NoAstNode 
  in
    Into_array.translationUnit_fun ast_array ast;
    assert(let res = ref true
	   in
	     for i = 1 to max_node do
	       if ast_array.(i) = NoAstNode then begin
		 Printf.eprintf "Superast.into_array: node id %d missing\n" i;
		 res := false
	       end
	     done;
	     !res);
    ast_array
    

let load_marshaled_ast_array file =
  let (max_node, ast) = Oast_header.unmarshal_oast file
  in
    into_array max_node ast

let iter f ast_array =
  for i = 1 to (Array.length ast_array -1) do
    f ast_array.(i)
  done

let iteri f ast_array =
  for i = 1 to (Array.length ast_array -1) do
    f i ast_array.(i)
  done

let fold f ast_array x =
  let r = ref x
  in
    for i = 1 to (Array.length ast_array -1) do
      r := f !r ast_array.(i)
    done;
    !r

let node_loc = function
  | TopForm_type tf -> Some(topForm_loc tf)
  | PQName_type pq -> Some(pQName_loc pq)
  | TypeSpecifier_type x -> Some(typeSpecifier_loc x)
  | Enumerator_type x -> Some(enumerator_loc x)
  | Member_type x -> Some(member_loc x)
  | Statement_type x -> Some(statement_loc x)
  | Expression_type(E___builtin_constant_p(_,_,loc,_)) -> Some loc
  | Expression_type(E___builtin_va_arg(_,_,loc,_,_)) -> Some loc
  | Initializer_type x -> Some(init_loc x)
  | TemplateParameter_type x -> Some(templateParameter_loc x)
  | IDeclarator_type x -> Some(iDeclarator_loc x)
  | Designator_type x -> Some(designator_loc x)
  | Attribute_type x -> Some(attribute_loc x)
  | Variable v -> Some(v.loc)

  | TranslationUnit_type _
  | Function_type _
  | MemberInit_type _
  | Declaration_type _
  | ASTTypeId_type _
  | BaseClassSpec_type _
  | MemberList_type _
  | ExceptionSpec_type _
  | OperatorName_type _
  | Condition_type _
  | Handler_type _
  | Expression_type _
  | FullExpression_type _
  | ArgExpression_type _
  | ArgExpressionListOpt_type _
  | TemplateDeclaration_type _
  | TemplateArgument_type _
  | NamespaceDecl_type _
  | Declarator_type _
  | FullExpressionAnnot_type _
  | ASTTypeof_type _
  | TemplateInfo _
  | InheritedTemplateParams _
  | BaseClass _
  | Compound_info _
  | EnumType_Value_type _
  | AtomicType _
  | CType _
  | STemplateArgument _
  | Scope _
    -> None

  | NoAstNode -> assert(false)


let node_annotation node = 
  match node with
    | TranslationUnit_type x -> translationUnit_annotation x
    | TopForm_type x -> topForm_annotation x
    | Function_type x -> func_annotation x
    | MemberInit_type x -> memberInit_annotation x
    | Declaration_type x -> declaration_annotation x
    | ASTTypeId_type x -> aSTTypeId_annotation x
    | PQName_type x -> pQName_annotation x
    | TypeSpecifier_type x -> typeSpecifier_annotation x
    | BaseClassSpec_type x -> baseClassSpec_annotation x
    | Enumerator_type x -> enumerator_annotation x
    | MemberList_type x -> memberList_annotation x
    | Member_type x -> member_annotation x
    | ExceptionSpec_type x -> exceptionSpec_annotation x
    | OperatorName_type x -> operatorName_annotation x
    | Statement_type x -> statement_annotation x
    | Condition_type x -> condition_annotation x
    | Handler_type x -> handler_annotation x
    | Expression_type x -> expression_annotation x
    | FullExpression_type x -> fullExpression_annotation x
    | ArgExpression_type x -> argExpression_annotation x
    | ArgExpressionListOpt_type x -> argExpressionListOpt_annotation x
    | Initializer_type x -> init_annotation x
    | TemplateDeclaration_type x -> templateDeclaration_annotation x
    | TemplateParameter_type x -> templateParameter_annotation x
    | TemplateArgument_type x -> templateArgument_annotation x
    | NamespaceDecl_type x -> namespaceDecl_annotation x
    | Declarator_type x -> declarator_annotation x
    | IDeclarator_type x -> iDeclarator_annotation x
    | FullExpressionAnnot_type x -> fullExpressionAnnot_annotation x
    | ASTTypeof_type x -> aSTTypeof_annotation x
    | Designator_type x -> designator_annotation x
    | Attribute_type x -> attribute_annotation x
    | Variable x -> variable_annotation x
    | TemplateInfo x -> templ_info_annotation x
    | InheritedTemplateParams x -> inherited_templ_params_annotation x
    | BaseClass x -> baseClass_annotation x
    | Compound_info x -> compound_info_annotation x
    | EnumType_Value_type x -> enum_value_annotation x
    | AtomicType x -> atomicType_annotation x
    | CType x -> cType_annotation x
    | STemplateArgument x -> sTemplateArgument_annotation x
    | Scope x -> scope_annotation x

    | NoAstNode -> assert false

let node_id node =
  id_annotation(node_annotation node)
