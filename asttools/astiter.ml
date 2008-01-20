(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* blueprint for an iteration function over the complete ast *)

open Cc_ml_types
open Cc_ast_gen_type
open Ml_ctype
open Ast_annotation
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
 * contents of astiter.ml
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


(***************** variable ***************************)

let rec variable_fun(v : annotated variable) =
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
      visit annot;

      annotation_fun v.poly_var;
      sourceLoc_fun v.loc;
      opt_iter string_fun v.var_name;

      (* POSSIBLY CIRCULAR *)
      opt_iter cType_fun !(v.var_type);
      declFlags_fun v.flags;
      (* POSSIBLY CIRCULAR *)
      opt_iter expression_fun !(v.value);
      opt_iter cType_fun v.defaultParam;

      (* POSSIBLY CIRCULAR *)
      opt_iter func_fun !(v.funcDefn);
      (* POSSIBLY CIRCULAR *)
      List.iter variable_fun !(v.overload);
      List.iter variable_fun v.virtuallyOverride;
      opt_iter scope_fun v.scope;
      opt_iter templ_info_fun v.templ_info;
    end

(**************** templateInfo ************************)

and templ_info_fun ti =
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
      visit annot;

      annotation_fun ti.poly_templ;
      templ_kind_fun ti.templ_kind;
      List.iter variable_fun ti.template_params;

      (* POSSIBLY CIRCULAR *)
      opt_iter variable_fun !(ti.template_var);
      List.iter inherited_templ_params_fun ti.inherited_params;

      (* POSSIBLY CIRCULAR *)
      opt_iter variable_fun !(ti.instantiation_of);
      List.iter variable_fun ti.instantiations;

      (* POSSIBLY CIRCULAR *)
      opt_iter variable_fun !(ti.specialization_of);
      List.iter variable_fun ti.specializations;
      List.iter sTemplateArgument_fun ti.arguments;
      sourceLoc_fun ti.inst_loc;

      (* POSSIBLY CIRCULAR *)
      opt_iter variable_fun !(ti.partial_instantiation_of);
      List.iter variable_fun ti.partial_instantiations;
      List.iter sTemplateArgument_fun ti.arguments_to_primary;
      opt_iter scope_fun ti.defn_scope;
      opt_iter templ_info_fun ti.definition_template_info;
      bool_fun ti.instantiate_body;
      bool_fun ti.instantiation_disallowed;
      int_fun ti.uninstantiated_default_args;
      List.iter cType_fun ti.dependent_bases;
    end

(************* inheritedTemplateParams ****************)

and inherited_templ_params_fun itp =
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

      visit annot;

      annotation_fun itp.poly_inherited_templ;
      List.iter variable_fun itp.inherited_template_params;

      (* POSSIBLY CIRCULAR *)
      opt_iter compound_info_fun !(itp.enclosing);
    end

(***************** cType ******************************)

and baseClass_fun baseClass =
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
	visit annot;
      annotation_fun baseClass.poly_base;
      compound_info_fun baseClass.compound;
      accessKeyword_fun baseClass.bc_access;
      bool_fun baseClass.is_virtual
    end


and compound_info_fun i = 
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
      visit annot;
      assert(match !(i.syntax) with
	       | None
	       | Some(TS_classSpec _) -> true
	       | _ -> false);
      annotation_fun i.compound_info_poly;
      opt_iter string_fun i.compound_name;
      variable_fun i.typedef_var;
      accessKeyword_fun i.ci_access;
      scope_fun i.compound_scope;
      bool_fun i.is_forward_decl;
      bool_fun i.is_transparent_union;
      compoundType_Keyword_fun i.keyword;
      List.iter variable_fun i.data_members;
      List.iter baseClass_fun i.bases;
      List.iter variable_fun i.conversion_operators;
      List.iter variable_fun i.friends;
      opt_iter typeSpecifier_fun !(i.syntax);

      (* POSSIBLY CIRCULAR *)
      opt_iter string_fun i.inst_name;

      (* POSSIBLY CIRCULAR *)
      opt_iter cType_fun !(i.self_type)
    end

and enum_value_fun ((annot, string, nativeint) as x) =
  if visited annot then ()
  else begin
    visit annot;
    annotation_fun annot;
    string_fun string;
    nativeint_fun nativeint
  end

and atomicType_fun x = 
  let annot = atomicType_annotation x
  in
    if visited annot then ()
    else match x with
	(*
	 * put calls to visit here before in each case, except for CompoundType
	 *)

      | SimpleType(annot, simpleTypeId) ->
	  visit annot;
	  annotation_fun annot;
	  simpleTypeId_fun simpleTypeId

      | CompoundType(compound_info) ->
	  compound_info_fun compound_info

      | PseudoInstantiation(annot, str, variable_opt, accessKeyword, 
			    compound_info, sTemplateArgument_list) ->
	  visit annot;
	  annotation_fun annot;
	  string_fun str;
	  opt_iter variable_fun variable_opt;
	  accessKeyword_fun accessKeyword;
	  atomicType_fun compound_info;
	  List.iter sTemplateArgument_fun sTemplateArgument_list

      | EnumType(annot, string, variable, accessKeyword, 
		 enum_value_list, has_negatives) ->
	  visit annot;
	  annotation_fun annot;
	  opt_iter string_fun string;
	  opt_iter variable_fun variable;
	  accessKeyword_fun accessKeyword;
	  List.iter enum_value_fun enum_value_list;
	  bool_fun has_negatives

      | TypeVariable(annot, string, variable, accessKeyword) ->
	  visit annot;
	  annotation_fun annot;
	  string_fun string;
	  variable_fun variable;
	  accessKeyword_fun accessKeyword

      | TemplateTypeVariable(annot, string, variable, accessKeyword, params) ->
          visit annot;
          annotation_fun annot;
          string_fun string;
          variable_fun variable;
          accessKeyword_fun accessKeyword;
          List.iter variable_fun params

      | DependentQType(annot, string, variable, 
		      accessKeyword, atomic, pq_name) ->
	  visit annot;
	  annotation_fun annot;
	  string_fun string;
	  variable_fun variable;
	  accessKeyword_fun accessKeyword;
	  atomicType_fun atomic;
	  pQName_fun pq_name
	    


and cType_fun x = 
  let annot = cType_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
      | CVAtomicType(annot, cVFlags, atomicType) ->
	  annotation_fun annot;
	  cVFlags_fun cVFlags;
	  atomicType_fun atomicType

      | PointerType(annot, cVFlags, cType) ->
	  annotation_fun annot;
	  cVFlags_fun cVFlags;
	  cType_fun cType

      | ReferenceType(annot, cType) ->
	  annotation_fun annot;
	  cType_fun cType

      | FunctionType(annot, function_flags, cType, 
		     variable_list, cType_list_opt) ->
	  annotation_fun annot;
	  function_flags_fun function_flags;
	  cType_fun cType;
	  List.iter variable_fun variable_list;
	  opt_iter (List.iter cType_fun) cType_list_opt

      | ArrayType(annot, cType, array_size) ->
	  annotation_fun annot;
	  cType_fun cType;
	  array_size_fun array_size

      | PointerToMemberType(annot, atomicType (* = NamedAtomicType *), 
			    cVFlags, cType) ->
	  assert(match atomicType with 
		   | SimpleType _ -> false
		   | CompoundType _
		   | PseudoInstantiation _
		   | EnumType _
		   | TypeVariable _ 
                   | TemplateTypeVariable _ 
                   | DependentQType _ -> true);
	  annotation_fun annot;
	  atomicType_fun atomicType;
	  cVFlags_fun cVFlags;
	  cType_fun cType

      | DependentSizedArrayType(annot, cType, array_size) ->
          annotation_fun annot;
          cType_fun cType;
          expression_fun array_size


and sTemplateArgument_fun ta = 
  let annot = sTemplateArgument_annotation ta
  in
    if visited annot then ()
    else 
      let _ = visit annot 
      in match ta with
	| STA_NONE annot -> 
	    annotation_fun annot

	| STA_TYPE(annot, cType) -> 
	    annotation_fun annot;
	    cType_fun cType

	| STA_INT(annot, int) -> 
	    annotation_fun annot;
	    int_fun int

	| STA_ENUMERATOR(annot, variable) -> 
	    annotation_fun annot;
	    variable_fun variable

	| STA_REFERENCE(annot, variable) -> 
	    annotation_fun annot;
	    variable_fun variable

	| STA_POINTER(annot, variable) -> 
	    annotation_fun annot;
	    variable_fun variable

	| STA_MEMBER(annot, variable) -> 
	    annotation_fun annot;
	    variable_fun variable

	| STA_DEPEXPR(annot, expression) -> 
	    annotation_fun annot;
	    expression_fun expression

	| STA_TEMPLATE(annot, atomicType) -> 
            annotation_fun annot;
            atomicType_fun atomicType

	| STA_ATOMIC(annot, atomicType) -> 
	    annotation_fun annot;
	    atomicType_fun atomicType


and scope_fun s = 
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
      visit annot;
      Hashtbl.iter 
	(fun str var -> string_fun str; variable_fun var)
	s.variables;
      Hashtbl.iter
	(fun str var -> string_fun str; variable_fun var)
	s.type_tags;
      opt_iter scope_fun s.parent_scope;
      scopeKind_fun s.scope_kind;
      opt_iter variable_fun !(s.namespace_var);
      List.iter variable_fun s.scope_template_params;
      opt_iter variable_fun s.parameterized_entity;
    end


(***************** generated ast nodes ****************)

and translationUnit_fun 
    ((annot, topForm_list, scope_opt) as x : annotated translationUnit_type) =
  if visited annot then ()
  else begin
    visit annot;
    annotation_fun annot;
    List.iter topForm_fun topForm_list;
    opt_iter scope_fun scope_opt      
  end


and topForm_fun x = 
  let annot = topForm_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| TF_decl(annot, sourceLoc, declaration) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    declaration_fun declaration

	| TF_func(annot, sourceLoc, func) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    func_fun func

	| TF_template(annot, sourceLoc, templateDeclaration) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    templateDeclaration_fun templateDeclaration

	| TF_explicitInst(annot, sourceLoc, declFlags, declaration) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    declFlags_fun declFlags;
	    declaration_fun declaration

	| TF_linkage(annot, sourceLoc, stringRef, translationUnit) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef;
	    translationUnit_fun translationUnit

	| TF_one_linkage(annot, sourceLoc, stringRef, topForm) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef;
	    topForm_fun topForm

	| TF_asm(annot, sourceLoc, e_stringLit) -> 
	    assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    expression_fun e_stringLit

	| TF_namespaceDefn(annot, sourceLoc, stringRef_opt, topForm_list) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    opt_iter string_fun stringRef_opt;
	    List.iter topForm_fun topForm_list

	| TF_namespaceDecl(annot, sourceLoc, namespaceDecl) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    namespaceDecl_fun namespaceDecl



and func_fun((annot, declFlags, typeSpecifier, declarator, memberInit_list, 
	     s_compound_opt, handler_list, func, variable_opt_1, 
	     variable_opt_2, statement_opt, bool) as x) =

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
    visit annot;
    annotation_fun annot;
    declFlags_fun declFlags;
    typeSpecifier_fun typeSpecifier;
    declarator_fun declarator;
    List.iter memberInit_fun memberInit_list;
    opt_iter statement_fun s_compound_opt;
    List.iter handler_fun handler_list;
    cType_fun func;
    opt_iter variable_fun variable_opt_1;
    opt_iter variable_fun variable_opt_2;
    opt_iter statement_fun statement_opt;
    bool_fun bool
  end


and memberInit_fun((annot, pQName, argExpression_list, 
		   variable_opt_1, compound_opt, variable_opt_2, 
		   full_expr_annot, statement_opt) as x) =

  if visited annot then ()
  else begin
      assert(match compound_opt with
	| None
	| Some(CompoundType _) -> true
	| _ -> false);
    visit annot;
    annotation_fun annot;
    pQName_fun pQName;
    List.iter argExpression_fun argExpression_list;
    opt_iter variable_fun variable_opt_1;
    opt_iter atomicType_fun compound_opt;
    opt_iter variable_fun variable_opt_2;
    fullExpressionAnnot_fun full_expr_annot;
    opt_iter statement_fun statement_opt
  end


and declaration_fun((annot, declFlags, typeSpecifier, declarator_list) as x) =
  if visited annot then ()
  else begin
    visit annot;
    annotation_fun annot;
    declFlags_fun declFlags;
    typeSpecifier_fun typeSpecifier;
    List.iter declarator_fun declarator_list
  end


and aSTTypeId_fun((annot, typeSpecifier, declarator) as x) =
  if visited annot then ()
  else begin
    visit annot;
    annotation_fun annot;
    typeSpecifier_fun typeSpecifier;
    declarator_fun declarator
  end


and pQName_fun x = 
  let annot = pQName_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| PQ_qualifier(annot, sourceLoc, stringRef_opt, 
		       templateArgument_opt, pQName, 
		      variable_opt, s_template_arg_list) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    opt_iter string_fun stringRef_opt;
	    opt_iter templateArgument_fun templateArgument_opt;
	    pQName_fun pQName;
	    opt_iter variable_fun variable_opt;
	    List.iter sTemplateArgument_fun s_template_arg_list

	| PQ_name(annot, sourceLoc, stringRef) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef

	| PQ_operator(annot, sourceLoc, operatorName, stringRef) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    operatorName_fun operatorName;
	    string_fun stringRef

	| PQ_template(annot, sourceLoc, stringRef, templateArgument_opt, 
		     s_template_arg_list) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef;
	    opt_iter templateArgument_fun templateArgument_opt;
	    List.iter sTemplateArgument_fun s_template_arg_list

	| PQ_variable(annot, sourceLoc, variable) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    variable_fun variable



and typeSpecifier_fun x = 
  let annot = typeSpecifier_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| TS_name(annot, sourceLoc, cVFlags, pQName, bool, 
		 var_opt_1, var_opt_2) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    cVFlags_fun cVFlags;
	    pQName_fun pQName;
	    bool_fun bool;
	    opt_iter variable_fun var_opt_1;
	    opt_iter variable_fun var_opt_2

	| TS_simple(annot, sourceLoc, cVFlags, simpleTypeId) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    cVFlags_fun cVFlags;
	    simpleTypeId_fun simpleTypeId

	| TS_elaborated(annot, sourceLoc, cVFlags, typeIntr, 
		       pQName, namedAtomicType_opt) -> 
	    assert(match namedAtomicType_opt with
	      | Some(SimpleType _) -> false
	      | _ -> true);
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    cVFlags_fun cVFlags;
	    typeIntr_fun typeIntr;
	    pQName_fun pQName;
	    opt_iter atomicType_fun namedAtomicType_opt

	| TS_classSpec(annot, sourceLoc, cVFlags, typeIntr, pQName_opt, 
		       baseClassSpec_list, memberList, compoundType) -> 
	    assert(match compoundType with
	      | CompoundType _ -> true
	      | _ -> false);
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    cVFlags_fun cVFlags;
	    typeIntr_fun typeIntr;
	    opt_iter pQName_fun pQName_opt;
	    List.iter baseClassSpec_fun baseClassSpec_list;
	    memberList_fun memberList;
	    atomicType_fun compoundType

	| TS_enumSpec(annot, sourceLoc, cVFlags, 
		      stringRef_opt, enumerator_list, enumType) -> 
	    assert(match enumType with 
	      | EnumType _ -> true
	      | _ -> false);
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    cVFlags_fun cVFlags;
	    opt_iter string_fun stringRef_opt;
	    List.iter enumerator_fun enumerator_list;
	    atomicType_fun enumType

	| TS_type(annot, sourceLoc, cVFlags, cType) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    cVFlags_fun cVFlags;
	    cType_fun cType

	| TS_typeof(annot, sourceLoc, cVFlags, aSTTypeof) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    cVFlags_fun cVFlags;
	    aSTTypeof_fun aSTTypeof


and baseClassSpec_fun
    ((annot, bool, accessKeyword, pQName, compoundType_opt) as x) =
  if visited annot then ()
  else begin
    assert(match compoundType_opt with
	     | None
	     | Some(CompoundType _ ) -> true
	     | _ -> false);
    visit annot;
    annotation_fun annot;
    bool_fun bool;
    accessKeyword_fun accessKeyword;
    pQName_fun pQName;
    opt_iter atomicType_fun compoundType_opt
  end


and enumerator_fun((annot, sourceLoc, stringRef, 
		   expression_opt, variable, int32) as x) =
  if visited annot then ()
  else begin
    visit annot;
    annotation_fun annot;
    sourceLoc_fun sourceLoc;
    string_fun stringRef;
    opt_iter expression_fun expression_opt;
    variable_fun variable;
    int32_fun int32
  end


and memberList_fun((annot, member_list) as x) =
  if visited annot then ()
  else begin
    visit annot;
    annotation_fun annot;
    List.iter member_fun member_list
  end


and member_fun x = 
  let annot = member_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| MR_decl(annot, sourceLoc, declaration) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    declaration_fun declaration

	| MR_func(annot, sourceLoc, func) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    func_fun func

	| MR_access(annot, sourceLoc, accessKeyword) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    accessKeyword_fun accessKeyword

	| MR_usingDecl(annot, sourceLoc, nd_usingDecl) -> 
	    assert(match nd_usingDecl with ND_usingDecl _ -> true | _ -> false);
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    namespaceDecl_fun nd_usingDecl

	| MR_template(annot, sourceLoc, templateDeclaration) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    templateDeclaration_fun templateDeclaration


and declarator_fun((annot, iDeclarator, init_opt, 
		   variable_opt, ctype_opt, declaratorContext,
		   statement_opt_ctor, statement_opt_dtor) as x) =
  if visited annot then ()
  else begin
    visit annot;
    annotation_fun annot;
    iDeclarator_fun iDeclarator;
    opt_iter init_fun init_opt;
    opt_iter variable_fun variable_opt;
    opt_iter cType_fun ctype_opt;
    declaratorContext_fun declaratorContext;
    opt_iter statement_fun statement_opt_ctor;
    opt_iter statement_fun statement_opt_dtor
  end


and iDeclarator_fun x = 
  let annot = iDeclarator_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| D_name(annot, sourceLoc, pQName_opt) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    opt_iter pQName_fun pQName_opt

	| D_pointer(annot, sourceLoc, cVFlags, iDeclarator) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    cVFlags_fun cVFlags;
	    iDeclarator_fun iDeclarator

	| D_reference(annot, sourceLoc, iDeclarator) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    iDeclarator_fun iDeclarator

	| D_func(annot, sourceLoc, iDeclarator, aSTTypeId_list, cVFlags, 
		 exceptionSpec_opt, pq_name_list, bool) -> 
	    assert(List.for_all (function | PQ_name _ -> true | _ -> false) 
		     pq_name_list);
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    iDeclarator_fun iDeclarator;
	    List.iter aSTTypeId_fun aSTTypeId_list;
	    cVFlags_fun cVFlags;
	    opt_iter exceptionSpec_fun exceptionSpec_opt;
	    List.iter pQName_fun pq_name_list;
	    bool_fun bool

	| D_array(annot, sourceLoc, iDeclarator, expression_opt, bool) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    iDeclarator_fun iDeclarator;
	    opt_iter expression_fun expression_opt;
	    bool_fun bool

	| D_bitfield(annot, sourceLoc, pQName_opt, expression, int) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    opt_iter pQName_fun pQName_opt;
	    expression_fun expression;
	    int_fun int

	| D_ptrToMember(annot, sourceLoc, pQName, cVFlags, iDeclarator) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    pQName_fun pQName;
	    cVFlags_fun cVFlags;
	    iDeclarator_fun iDeclarator

	| D_grouping(annot, sourceLoc, iDeclarator) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    iDeclarator_fun iDeclarator

	| D_attribute(annot, sourceLoc, iDeclarator, attribute_list_list) ->
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    iDeclarator_fun iDeclarator;
	    List.iter (List.iter attribute_fun) attribute_list_list



and exceptionSpec_fun((annot, aSTTypeId_list) as x) =
  if visited annot then ()
  else begin
    visit annot;
    annotation_fun annot;
    List.iter aSTTypeId_fun aSTTypeId_list
  end


and operatorName_fun x = 
  let annot = operatorName_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| ON_newDel(annot, bool_is_new, bool_is_array) -> 
	    annotation_fun annot;
	    bool_fun bool_is_new;
	    bool_fun bool_is_array

	| ON_operator(annot, overloadableOp) -> 
	    annotation_fun annot;
	    overloadableOp_fun overloadableOp

	| ON_conversion(annot, aSTTypeId) -> 
	    annotation_fun annot;
	    aSTTypeId_fun aSTTypeId


and statement_fun x = 
  let annot = statement_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| S_skip(annot, sourceLoc) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc

	| S_label(annot, sourceLoc, stringRef, statement) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef;
	    statement_fun statement

	| S_case(annot, sourceLoc, expression, statement, int32) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    expression_fun expression;
	    statement_fun statement;
	    int32_fun int32

	| S_default(annot, sourceLoc, statement) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    statement_fun statement

	| S_expr(annot, sourceLoc, fullExpression) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    fullExpression_fun fullExpression

	| S_compound(annot, sourceLoc, statement_list) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    List.iter statement_fun statement_list

	| S_if(annot, sourceLoc, condition, statement_then, statement_else) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    condition_fun condition;
	    statement_fun statement_then;
	    statement_fun statement_else

	| S_switch(annot, sourceLoc, condition, statement) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    condition_fun condition;
	    statement_fun statement

	| S_while(annot, sourceLoc, condition, statement) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    condition_fun condition;
	    statement_fun statement

	| S_doWhile(annot, sourceLoc, statement, fullExpression) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    statement_fun statement;
	    fullExpression_fun fullExpression

	| S_for(annot, sourceLoc, statement_init, condition, fullExpression, 
		statement_body) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    statement_fun statement_init;
	    condition_fun condition;
	    fullExpression_fun fullExpression;
	    statement_fun statement_body

	| S_break(annot, sourceLoc) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc

	| S_continue(annot, sourceLoc) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc

	| S_return(annot, sourceLoc, fullExpression_opt, statement_opt) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    opt_iter fullExpression_fun fullExpression_opt;
	    opt_iter statement_fun statement_opt

	| S_goto(annot, sourceLoc, stringRef) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef

	| S_decl(annot, sourceLoc, declaration) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    declaration_fun declaration

	| S_try(annot, sourceLoc, statement, handler_list) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    statement_fun statement;
	    List.iter handler_fun handler_list

	| S_asm(annot, sourceLoc, e_stringLit) -> 
	    assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    expression_fun e_stringLit

	| S_namespaceDecl(annot, sourceLoc, namespaceDecl) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    namespaceDecl_fun namespaceDecl

	| S_function(annot, sourceLoc, func) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    func_fun func

	| S_rangeCase(annot, sourceLoc, 
		      expression_lo, expression_hi, statement, 
		     label_lo, label_hi) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    expression_fun expression_lo;
	    expression_fun expression_hi;
	    statement_fun statement;
	    int_fun label_lo;
	    int_fun label_hi

	| S_computedGoto(annot, sourceLoc, expression) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    expression_fun expression


and condition_fun x = 
  let annot = condition_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| CN_expr(annot, fullExpression) -> 
	    annotation_fun annot;
	    fullExpression_fun fullExpression

	| CN_decl(annot, aSTTypeId) -> 
	    annotation_fun annot;
	    aSTTypeId_fun aSTTypeId


and handler_fun((annot, aSTTypeId, statement_body, variable_opt, 
		fullExpressionAnnot, expression_opt, statement_gdtor) as x) =
  if visited annot then ()
  else begin
    visit annot;
    annotation_fun annot;
    aSTTypeId_fun aSTTypeId;
    statement_fun statement_body;
    opt_iter variable_fun variable_opt;
    fullExpressionAnnot_fun fullExpressionAnnot;
    opt_iter expression_fun expression_opt;
    opt_iter statement_fun statement_gdtor
  end


and expression_fun x = 
  let annot = expression_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| E_boolLit(annot, type_opt, bool) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    bool_fun bool

	| E_intLit(annot, type_opt, stringRef, ulong) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    string_fun stringRef;
	    int32_fun ulong

	| E_floatLit(annot, type_opt, stringRef, double) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    string_fun stringRef;
	    float_fun double

	| E_stringLit(annot, type_opt, stringRef, 
		      e_stringLit_opt, stringRef_opt) -> 
	    assert(match e_stringLit_opt with 
		     | Some(E_stringLit _) -> true 
		     | None -> true
		     | _ -> false);
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    string_fun stringRef;
	    opt_iter expression_fun e_stringLit_opt;
	    opt_iter string_fun stringRef_opt

	| E_charLit(annot, type_opt, stringRef, int32) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    string_fun stringRef;
	    int32_fun int32

	| E_this(annot, type_opt, variable) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    variable_fun variable

	| E_variable(annot, type_opt, pQName, var_opt, nondep_var_opt) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    pQName_fun pQName;
	    opt_iter variable_fun var_opt;
	    opt_iter variable_fun nondep_var_opt

	| E_funCall(annot, type_opt, expression_func, 
		    argExpression_list, expression_retobj_opt) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    expression_fun expression_func;
	    List.iter argExpression_fun argExpression_list;
	    opt_iter expression_fun expression_retobj_opt

	| E_constructor(annot, type_opt, typeSpecifier, argExpression_list, 
			var_opt, bool, expression_opt) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    typeSpecifier_fun typeSpecifier;
	    List.iter argExpression_fun argExpression_list;
	    opt_iter variable_fun var_opt;
	    bool_fun bool;
	    opt_iter expression_fun expression_opt

	| E_fieldAcc(annot, type_opt, expression, pQName, var_opt) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    expression_fun expression;
	    pQName_fun pQName;
	    opt_iter variable_fun var_opt

	| E_sizeof(annot, type_opt, expression, int) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    expression_fun expression;
	    int_fun int

	| E_unary(annot, type_opt, unaryOp, expression) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    unaryOp_fun unaryOp;
	    expression_fun expression

	| E_effect(annot, type_opt, effectOp, expression) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    effectOp_fun effectOp;
	    expression_fun expression

	| E_binary(annot, type_opt, expression_left, binaryOp, expression_right) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    expression_fun expression_left;
	    binaryOp_fun binaryOp;
	    expression_fun expression_right

	| E_addrOf(annot, type_opt, expression) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    expression_fun expression

	| E_deref(annot, type_opt, expression) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    expression_fun expression

	| E_cast(annot, type_opt, aSTTypeId, expression, bool) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    aSTTypeId_fun aSTTypeId;
	    expression_fun expression;
	    bool_fun bool

	| E_cond(annot, type_opt, expression_cond, expression_true, expression_false) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    expression_fun expression_cond;
	    expression_fun expression_true;
	    expression_fun expression_false

	| E_sizeofType(annot, type_opt, aSTTypeId, int, bool) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    aSTTypeId_fun aSTTypeId;
	    int_fun int;
	    bool_fun bool

	| E_assign(annot, type_opt, expression_target, binaryOp, expression_src) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    expression_fun expression_target;
	    binaryOp_fun binaryOp;
	    expression_fun expression_src

	| E_new(annot, type_opt, bool, argExpression_list, aSTTypeId, 
		argExpressionListOpt_opt, array_size_opt, ctor_opt,
	        statement_opt, heep_var_opt) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    bool_fun bool;
	    List.iter argExpression_fun argExpression_list;
	    aSTTypeId_fun aSTTypeId;
	    opt_iter argExpressionListOpt_fun argExpressionListOpt_opt;
	    opt_iter expression_fun array_size_opt;
	    opt_iter variable_fun ctor_opt;
	    opt_iter statement_fun statement_opt;
	    opt_iter variable_fun heep_var_opt

	| E_delete(annot, type_opt, bool_colon, bool_array, 
		   expression_opt, statement_opt) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    bool_fun bool_colon;
	    bool_fun bool_array;
	    opt_iter expression_fun expression_opt;
	    opt_iter statement_fun statement_opt

	| E_throw(annot, type_opt, expression_opt, var_opt, statement_opt) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    opt_iter expression_fun expression_opt;
	    opt_iter variable_fun var_opt;
	    opt_iter statement_fun statement_opt

	| E_keywordCast(annot, type_opt, castKeyword, aSTTypeId, expression) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    castKeyword_fun castKeyword;
	    aSTTypeId_fun aSTTypeId;
	    expression_fun expression

	| E_typeidExpr(annot, type_opt, expression) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    expression_fun expression

	| E_typeidType(annot, type_opt, aSTTypeId) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    aSTTypeId_fun aSTTypeId

	| E_grouping(annot, type_opt, expression) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    expression_fun expression

	| E_arrow(annot, type_opt, expression, pQName) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    expression_fun expression;
	    pQName_fun pQName

	| E_statement(annot, type_opt, s_compound) -> 
	    assert(match s_compound with | S_compound _ -> true | _ -> false);
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    statement_fun s_compound

	| E_compoundLit(annot, type_opt, aSTTypeId, in_compound) -> 
	    assert(match in_compound with | IN_compound _ -> true | _ -> false);
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    aSTTypeId_fun aSTTypeId;
	    init_fun in_compound

	| E___builtin_constant_p(annot, type_opt, sourceLoc, expression) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    sourceLoc_fun sourceLoc;
	    expression_fun expression

	| E___builtin_va_arg(annot, type_opt, sourceLoc, expression, aSTTypeId) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    sourceLoc_fun sourceLoc;
	    expression_fun expression;
	    aSTTypeId_fun aSTTypeId

	| E_alignofType(annot, type_opt, aSTTypeId, int) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    aSTTypeId_fun aSTTypeId;
	    int_fun int

	| E_alignofExpr(annot, type_opt, expression, int) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    expression_fun expression;
	    int_fun int

	| E_gnuCond(annot, type_opt, expression_cond, expression_false) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    expression_fun expression_cond;
	    expression_fun expression_false

	| E_addrOfLabel(annot, type_opt, stringRef) -> 
	    annotation_fun annot;
	    opt_iter cType_fun type_opt;
	    string_fun stringRef


and fullExpression_fun((annot, expression_opt, fullExpressionAnnot) as x) =
  if visited annot then ()
  else begin
    visit annot;
    annotation_fun annot;
    opt_iter expression_fun expression_opt;
    fullExpressionAnnot_fun fullExpressionAnnot
  end


and argExpression_fun((annot, expression) as x) =
  if visited annot then ()
  else begin
    visit annot;
    annotation_fun annot;
    expression_fun expression
  end


and argExpressionListOpt_fun((annot, argExpression_list) as x) =
  if visited annot then ()
  else begin
    visit annot;
    annotation_fun annot;
    List.iter argExpression_fun argExpression_list
  end


and init_fun x = 
  let annot = init_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| IN_expr(annot, sourceLoc, fullExpressionAnnot, expression_opt) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    fullExpressionAnnot_fun fullExpressionAnnot;
	    opt_iter expression_fun expression_opt

	| IN_compound(annot, sourceLoc, fullExpressionAnnot, init_list) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    fullExpressionAnnot_fun fullExpressionAnnot;
	    List.iter init_fun init_list

	| IN_ctor(annot, sourceLoc, fullExpressionAnnot, 
		 argExpression_list, var_opt, bool) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    fullExpressionAnnot_fun fullExpressionAnnot;
	    List.iter argExpression_fun argExpression_list;
	    opt_iter variable_fun var_opt;
	    bool_fun bool

	| IN_designated(annot, sourceLoc, fullExpressionAnnot, 
		       designator_list, init) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    fullExpressionAnnot_fun fullExpressionAnnot;
	    List.iter designator_fun designator_list;
	    init_fun init


and templateDeclaration_fun x = 
  let annot = templateDeclaration_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| TD_func(annot, templateParameter_opt, func) -> 
	    annotation_fun annot;
	    opt_iter templateParameter_fun templateParameter_opt;
	    func_fun func

	| TD_decl(annot, templateParameter_opt, declaration) -> 
	    annotation_fun annot;
	    opt_iter templateParameter_fun templateParameter_opt;
	    declaration_fun declaration

	| TD_tmember(annot, templateParameter_opt, templateDeclaration) -> 
	    annotation_fun annot;
	    opt_iter templateParameter_fun templateParameter_opt;
	    templateDeclaration_fun templateDeclaration


and templateParameter_fun x = 
  let annot = templateParameter_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| TP_type(annot, sourceLoc, variable, stringRef, 
		  aSTTypeId_opt, templateParameter_opt) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    variable_fun variable;
	    string_fun stringRef;
	    opt_iter aSTTypeId_fun aSTTypeId_opt;
	    opt_iter templateParameter_fun templateParameter_opt

	| TP_nontype(annot, sourceLoc, variable,
		    aSTTypeId, templateParameter_opt) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    variable_fun variable;
	    aSTTypeId_fun aSTTypeId;
	    opt_iter templateParameter_fun templateParameter_opt

        | TP_template(annot, sourceLoc, variable, params, stringRef,
                  pqname, templateParameter_opt) -> 
            annotation_fun annot;
            sourceLoc_fun sourceLoc;
            variable_fun variable;
            List.iter templateParameter_fun params;
            string_fun stringRef;
            opt_iter pQName_fun pqname;
            opt_iter templateParameter_fun templateParameter_opt

and templateArgument_fun x = 
  let annot = templateArgument_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| TA_type(annot, aSTTypeId, templateArgument_opt) -> 
	    annotation_fun annot;
	    aSTTypeId_fun aSTTypeId;
	    opt_iter templateArgument_fun templateArgument_opt

	| TA_nontype(annot, expression, templateArgument_opt) -> 
	    annotation_fun annot;
	    expression_fun expression;
	    opt_iter templateArgument_fun templateArgument_opt

	| TA_templateUsed(annot, templateArgument_opt) -> 
	    annotation_fun annot;
	    opt_iter templateArgument_fun templateArgument_opt


and namespaceDecl_fun x = 
  let annot = namespaceDecl_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| ND_alias(annot, stringRef, pQName) -> 
	    annotation_fun annot;
	    string_fun stringRef;
	    pQName_fun pQName

	| ND_usingDecl(annot, pQName) -> 
	    annotation_fun annot;
	    pQName_fun pQName

	| ND_usingDir(annot, pQName) -> 
	    annotation_fun annot;
	    pQName_fun pQName


and fullExpressionAnnot_fun((annot, declaration_list) as x) =
  if visited annot then ()
  else begin
    visit annot;
    List.iter declaration_fun declaration_list
  end


and aSTTypeof_fun x = 
  let annot = aSTTypeof_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| TS_typeof_expr(annot, ctype, fullExpression) -> 
	    annotation_fun annot;
	    cType_fun ctype;
	    fullExpression_fun fullExpression

	| TS_typeof_type(annot, ctype, aSTTypeId) -> 
	    annotation_fun annot;
	    cType_fun ctype;
	    aSTTypeId_fun aSTTypeId


and designator_fun x = 
  let annot = designator_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| FieldDesignator(annot, sourceLoc, stringRef) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef

	| SubscriptDesignator(annot, sourceLoc, expression, expression_opt, 
			     idx_start, idx_end) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    expression_fun expression;
	    opt_iter expression_fun expression_opt;
	    int_fun idx_start;
	    int_fun idx_end


and attribute_fun x = 
  let annot = attribute_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| AT_empty(annot, sourceLoc) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc

	| AT_word(annot, sourceLoc, stringRef) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef

	| AT_func(annot, sourceLoc, stringRef, argExpression_list) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef;
	    List.iter argExpression_fun argExpression_list


(**************************************************************************
 *
 * end of astiter.ml 
 *
 **************************************************************************)


