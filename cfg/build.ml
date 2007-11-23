(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* XXX
 * FullExpressionAnnot contains declarations which might contain 
 * constructors: Therefore check if all FullExpressionAnnot's are examined! 
 * XXX check generally, if all expressions/statements are reached
 *)


open Ast_annotation
open Elsa_ml_flag_types
open Elsa_reflect_type
open Ast_accessors
open Cfg_type
open Cfg_util


(***********************************************************************
 *
 * general state
 *
 ***********************************************************************)

let current_oast = ref ""


(***********************************************************************
 *
 * error messages
 *
 ***********************************************************************)

type error_level =
  | Fatal
  | Unimplemented
  | Warning
  | Message

let report_level_number = 4

let error_level_index = function
  | Fatal -> 0
  | Unimplemented -> 1
  | Warning -> 2
  | Message -> 3

type error_report_level =
  | Report_all
  | Ignore_unimplemented
  | Report_nothing

let error_report_array = Array.create report_level_number true
let error_exit_array = Array.create report_level_number true

let rec set_error_array array value = function
  | [] -> ()
  | err::rest -> 
      array.(error_level_index err) <- value;
      set_error_array array value rest

let init_error_report_array report_levels =
  (* do default *)
  set_error_array error_report_array true 
    [Fatal; Unimplemented; Warning; Message];
  set_error_array error_exit_array true [Fatal; Unimplemented; ];
  set_error_array error_exit_array false [Warning; Message];
  
  if List.mem Report_all report_levels then begin
    set_error_array error_report_array true 
      [Fatal; Unimplemented; Warning; Message];
  end;
  if List.mem Report_nothing report_levels then begin
    set_error_array error_report_array false
      [Fatal; Unimplemented; Warning; Message];
  end;
  if List.mem Ignore_unimplemented report_levels then begin
    set_error_array error_exit_array false [Unimplemented]
  end


let found sourceLoc annot level mesg =
  if error_report_array.(error_level_index level) then
    Printf.eprintf "%s: (node %d)\nFound %s\n" 
      (error_location sourceLoc) 
      (id_annotation annot)
      mesg;
  if error_exit_array.(error_level_index level) then
    exit 1

let fatal sourceLoc annot mesg = 
  found sourceLoc annot Fatal mesg;
  (* still here ?? *)
  assert false

let string_of_function_id (name : function_id_type) = 
  let buf = Buffer.create 50 in
  let rec doit = function
    | [] -> assert false
    | name :: [] -> 
	Buffer.add_string buf name;
	Buffer.contents buf
    | scope :: name -> 
	Buffer.add_string buf scope;
	Buffer.add_string buf "::";
	doit name
  in
    doit name.name


let redefinition_error entry other_loc other_node name =
  Printf.printf 
    ("%s: %s\n"
     ^^ "    nonidentical function redefinition in %s (id %d)\n"
     ^^ "    callee %s missing in one of the definitions\n"
     ^^ "%s: original definition in %s (id %d)\n")
    (error_location entry.loc)
    (string_of_function_id entry.fun_id)
    (!current_oast)
    other_node
    (string_of_function_id name)
    (error_location other_loc)
    entry.oast
    entry.node_id


(***********************************************************************
 *
 * data base
 *
 ***********************************************************************)

let new_func_def_hash () = Hashtbl.create 2287

let function_def_hash : cfg_type ref = ref (new_func_def_hash ())


let function_def fun_id sourceLoc oast node =
  if Hashtbl.mem !function_def_hash fun_id 
  then 
    let entry = Hashtbl.find !function_def_hash fun_id in
    let check_hash = Hashtbl.create (2 * List.length entry.callees) in
    let call_hash = Hashtbl.create 73
    in
      List.iter 
	(fun f -> Hashtbl.add check_hash f ())
	entry.callees;
      Redef(entry, call_hash, check_hash, sourceLoc, node, ref false)
  else
    let entry = {
      fun_id = fun_id;
      loc = sourceLoc;
      oast = oast;
      node_id = node;
      callees = []
    } in
    let call_hash = Hashtbl.create 73      
    in
      Hashtbl.add !function_def_hash fun_id entry;
      New(entry, call_hash)

let add_callee current_fun f =
  match current_fun with
    | New(_fun_def, call_hash) ->
	if not (Hashtbl.mem call_hash f)
	then
	  Hashtbl.add call_hash f ()
    | Redef(entry, call_hash, check_hash, other_loc, other_node, error) ->
	if not (Hashtbl.mem call_hash f)
	then
	  begin
	    Hashtbl.add call_hash f ();
	    if Hashtbl.mem check_hash f
	    then
	      Hashtbl.remove check_hash f
	    else
	      if not !error 
	      then
		begin
		  redefinition_error entry other_loc other_node f;
		  error := true
		end
	  end
  

exception Finished

let finish_fun_call = function
  | New(entry, call_hash) ->
      entry.callees <- Hashtbl.fold (fun f _ l -> f::l) call_hash []
  | Redef(entry, _call_hash, check_hash, other_loc, other_node, error) ->
      if not !error then
	if Hashtbl.length check_hash = 0 
	then
	  (* 
           * Printf.printf 
	   *   ("%s: %s \n"
	   *    ^^ "    (seemingly) identical function redefinition\n"
	   *    ^^ "%s: original\n")
	   *   (error_location entry.loc)
	   *   (string_of_function_id entry.fun_id)
	   *   (error_location other_loc);
           *)
	  ()
	else
	  let missing_fun = ref invalid_function_id
	  in
	    (try 
	       Hashtbl.iter 
		 (fun f _ -> missing_fun := f; raise Finished)
		 check_hash;
	       (* should have left via exception *)
	       assert false
	     with
	       | Finished -> ();
	    );
	    redefinition_error entry other_loc other_node !missing_fun
	
	


(***********************************************************************
 *
 * utility functions
 *
 ***********************************************************************)


(* append the list of nested scope names to accumulator name *)
let rec get_scope_name sourceLoc name_list scope =
  match scope.scopeKind with
    | SK_GLOBAL -> name_list
    | SK_NAMESPACE ->
	(match (scope.namespaceVar) with
	   | None ->
	       fatal sourceLoc scope.scope_annotation
		 (Printf.sprintf "scope %d without var node" 
		    (id_annotation scope.scope_annotation));
	   | Some variable ->
	       match variable.variable_name with
		 | None -> 
		     fatal sourceLoc variable.variable_annotation
		       "scope var without name";
		 | Some name ->
		     match scope.parentScope with
		       | None -> 
			   fatal sourceLoc scope.scope_annotation
			     "inner scope without parent";
		       | Some parent ->
			   get_scope_name sourceLoc (name :: name_list) parent
	)
    | SK_CLASS ->
	(match (scope.scope_compound) with
	   | None -> 
	       fatal sourceLoc scope.scope_annotation
		 "class scope without compound";
	   | Some compound ->
	       match compound.compound_name with
		 | None ->
		     fatal sourceLoc compound.aTY_Compound_annotation
		       "class scope compound without name";
		 | Some name ->
		     match scope.parentScope with
		       | None -> 
			   fatal sourceLoc scope.scope_annotation
			     "class scope without parent";
		       | Some parent ->
			   get_scope_name sourceLoc (name :: name_list) parent
	)
    | SK_TEMPLATE_ARGS
    | SK_TEMPLATE_PARAMS
    | SK_FUNCTION
    | SK_PARAMETER
    | SK_UNKNOWN
      ->
	fatal sourceLoc scope.scope_annotation "strange scope"


(* convert an atomic argument type into a string (no C++ syntax) *)
let string_of_atomic_type sourceLoc = function
  | ATY_Simple(_annot, simpleTypeId) ->
      string_of_simpleTypeId simpleTypeId

  | ATY_Compound(compound) ->
      (match compound.compound_name with
	| None -> 
	    fatal sourceLoc compound.aTY_Compound_annotation
	      "atomic compound argument without name";
	| Some x -> x
      )

  | ATY_PseudoInstantiation pseudo_inst ->
      found sourceLoc pseudo_inst.aTY_PseudoInstantiation_annotation
	Unimplemented
	"atomic pseudo instantiation argument";
      "unknown atomic type I"

  | ATY_Enum enum ->
      (match enum.enum_type_name with
	 | None -> 
	     fatal sourceLoc enum.aTY_Enum_annotation
	       "enum argument without name";
	 | Some name -> name
      )

  | ATY_TypeVariable tv ->
      found sourceLoc tv.aTY_TypeVariable_annotation
	Unimplemented "type variable argument";
      "unknown atomic type II"

  | ATY_DependentQ depq ->
      found sourceLoc depq.aTY_DependentQ_annotation
	Unimplemented "dependent Q type argument";
      "unknown atomic type III"


(* convert an argument type into a string, (no C++ syntax) *)
let rec string_of_ctype sourceLoc = function
  | TY_CVAtomic(annot, atomicType, cVFlags) ->
      if List.mem CV_VOLATILE cVFlags then 
	found sourceLoc annot Unimplemented "volatile function argument";
      let const = List.mem CV_CONST cVFlags
      in
	(if const then "const(" else "")
	^ string_of_atomic_type sourceLoc atomicType
	^ (if const then ")" else "")

  | TY_Pointer(annot, cVFlags, cType) ->
      if List.mem CV_VOLATILE cVFlags then 
	found sourceLoc annot Unimplemented "volatile function argument";
      let const = List.mem CV_CONST cVFlags
      in
	(if const then "const " else "")
	^ "pointer("
	^ (string_of_ctype sourceLoc cType)
	^ ")"

  | TY_Reference(_annot, cType) ->
      "reference("
      ^ (string_of_ctype sourceLoc cType)
      ^ ")"

  | TY_Function f ->
      if f.function_type_flags <> [] then 
	found sourceLoc f.tY_Function_annotation
	  Unimplemented "function type argument with flags";
      "function("
      ^ (String.concat ", " 
	   (List.map (get_param_type sourceLoc) f.function_type_params))
      ^ " -> "
      ^ (string_of_ctype sourceLoc f.retType)
      ^ ")"

  | TY_Array(annot, _cType, _array_size) ->
      found sourceLoc annot Unimplemented "array type argument";
      "unknown ctype I"

  | TY_DependentSizedArray(annot, _cType, _size_expr) ->
      found sourceLoc annot Unimplemented "dependent size array argument";
      "unknown ctype II"

  | TY_PointerToMember p ->
      found sourceLoc p.tY_PointerToMember_annotation
	Unimplemented "member pointer argument";
      "unknown ctype III"


(* get a string representation of one parameter type *)
and get_param_type sourceLoc variable =
  match !(variable.variable_type) with
    | None -> assert false
    | Some ctype -> string_of_ctype sourceLoc ctype
  
(* get a string representation of the parameter types of a function
 * to disambiguate overloaded functions
 *)
let get_param_types sourceLoc variable =
  match !(variable.variable_type) with
    | None -> assert false
    | Some ctype -> 
	match ctype with 
	  | TY_Function ft ->
	      let parameters =
		if List.mem FF_METHOD ft.function_type_flags
		then
		  match ft.function_type_params with
		    | [] -> 
			fatal sourceLoc ft.tY_Function_annotation
			  "method without implicit argument";
		    | receiver :: args ->
			match receiver.variable_name with
			  | Some "__receiver" -> args
			  | _ -> 
			      fatal sourceLoc ft.tY_Function_annotation
				"method without __receiver arg";
		else
		  ft.function_type_params
	      in
		List.map (get_param_type sourceLoc) parameters

	  | TY_CVAtomic _
	  | TY_Pointer _
	  | TY_Reference _
	  | TY_Array _
	  | TY_DependentSizedArray _
	  | TY_PointerToMember _ ->
	      found sourceLoc (cType_annotation ctype) Unimplemented
		"function without function type";
	      []		


(* extracts the function id from the variable declaration node *)
let get_function_id_from_variable_node sourceLoc variable =
  match variable.variable_name with
    | None -> 
	fatal sourceLoc variable.variable_annotation
	  "function var node without name";
    | Some variable_name -> 
	match variable.scope with
	  | None -> 
	      fatal sourceLoc variable.variable_annotation
		"function decl with empty scope"
	  | Some scope ->
	      { name = get_scope_name sourceLoc [variable_name] scope;
		param_types = get_param_types sourceLoc variable;
	      }
		


(* extracts the function id from the declarator exessible in a 
 * function definition 
 *)
let get_function_definition_id sourceLoc declarator =
  match declarator.declarator_var with
    | None -> 
	fatal sourceLoc declarator.declarator_annotation 
	  "function def with empty declarator";
    | Some variable ->
	get_function_id_from_variable_node sourceLoc variable

(* extracts the function id from 
 * - a simple function call 
 * - a method call
 *)
let get_function_id sourceLoc function_expression =
  match function_expression with
    | E_variable v ->
	(* Printf.printf "E_variable at %s\n" (error_location sourceLoc); *)
	(match v.expr_var_var with
	   | None ->
	       fatal sourceLoc v.e_variable_annotation
		 "function without decl var node";
	   | Some variable ->
	       get_function_id_from_variable_node sourceLoc variable
	)
    | E_fieldAcc f -> 
	(match f.field with
	   | None ->
	       fatal sourceLoc f.e_fieldAcc_annotation
		 "field access without var node";
	   | Some variable ->
	       get_function_id_from_variable_node sourceLoc variable
	)

    | E_deref(_annot_1, _type_opt_1, _) ->
	function_pointer_fun_id
    | _ ->
	fatal sourceLoc (expression_annotation function_expression)
	  "nontrivial function call"


(***********************************************************************
 *
 * AST walk
 *
 ***********************************************************************)

let opt_iter f = function
  | None -> ()
  | Some x -> f x


let rec compilationUnit_fun (cu : annotated compilationUnit_type) =
  let comp_unit_id = {
    name = ["CU " ^ cu.unit_name];
    param_types = []
  } in
  let comp_unit_loc = (cu.unit_name, 1, 0) in
  let comp_unit = function_def comp_unit_id comp_unit_loc cu.unit_name 0
  in
    translationUnit_fun comp_unit cu.unit;
    finish_fun_call comp_unit


and translationUnit_fun comp_unit
    (tu : annotated translationUnit_type) =
  List.iter (topForm_fun comp_unit) tu.topForms


and topForm_fun comp_unit = function
  | TF_linkage l -> 
      translationUnit_fun comp_unit l.linkage_forms

  | TF_one_linkage l -> 
      topForm_fun comp_unit l.form

  | TF_func(_annot, sourceLoc, func) -> 
      function_fun sourceLoc func

  | TF_namespaceDefn ns -> 
      List.iter (topForm_fun comp_unit) ns.name_space_defn_forms

  | TF_decl(_annot, sourceLoc, declaration) -> 
      outer_declaration_fun comp_unit sourceLoc declaration

      (* ignore the rest *)
  | TF_template _
  | TF_explicitInst _
  | TF_asm _
  | TF_namespaceDecl _
    -> ()

(* typeSpecifier's in Declarations *)
and typeSpecifier_fun ts = 
  match ts with
    | TS_classSpec cs ->
	  List.iter member_fun cs.members.member_list

    | TS_name _
    | TS_simple _
    | TS_elaborated _
    | TS_enumSpec _
      -> ()

    | TS_type _
    | TS_typeof _
	(* XXX TS_typeof contains an expression!! But this is not evaled? or? *)
      -> 
	found (typeSpecifier_loc ts) (typeSpecifier_annotation ts) 
	  Unimplemented "unknown type specifier in top decl";

(* members out of TS_classSpec *)
and member_fun = function
  | MR_func(_annot, sourceLoc, func) -> 
      function_fun sourceLoc func

  | MR_template _ ->
      (* XXX ? *)
      ()

  | MR_decl _
  | MR_access _
  | MR_usingDecl _
      -> ()



and function_fun sourceLoc fu =
  (* XXX handler_list?? And the FullExpressionAnnot therein?? *)
  (* XXX dtorStatement -- contains superclass/member destructors in 
     destructors *)
  (* XXX declarator might contain initializers or other expressions? *)
  let entry = 
    function_def (get_function_definition_id sourceLoc fu.nameAndParams) 
      sourceLoc !current_oast (id_annotation fu.function_annotation);
  in
    List.iter 
      (memberInit_fun entry sourceLoc)
      fu.function_inits;
    opt_iter (statement_fun entry) fu.function_body;
    opt_iter (statement_fun entry) fu.function_dtor_statement;
    finish_fun_call entry


and memberInit_fun current_func sourceLoc mi =
  (match mi.member_init_annot.declarations with
     | [] -> ()
     | _ -> 
	 found sourceLoc mi.memberInit_annotation Unimplemented
	   "declaration in FullExpressionAnnot";
  );
  (* go through the arguments *)
  List.iter (argExpression_fun current_func sourceLoc) mi.member_init_args;
  if (mi.member_init_base <> None) && 
    (mi.member_init_ctor_statement = None) 
  then
    found sourceLoc mi.memberInit_annotation 
      Unimplemented "base init without ctor statement";
  opt_iter (statement_fun current_func) mi.member_init_ctor_statement


and statement_fun current_func = function
  | S_skip(_annot, _sourceLoc) -> ()
	    
  | S_label s -> 
      statement_fun current_func s.label_stmt

  | S_case c ->
      statement_fun current_func c.case_stmt

  | S_default(_annot, _sourceLoc, statement) -> 
      statement_fun current_func statement
	
  | S_expr(_annot, sourceLoc, fullExpression) -> 
      full_expression_fun current_func sourceLoc fullExpression

  | S_compound(_annot, _sourceLoc, statement_list) -> 
      List.iter (statement_fun current_func) statement_list

  | S_if ifs ->
      condition_fun current_func ifs.if_loc ifs.if_cond;
      statement_fun current_func ifs.thenBranch;
      statement_fun current_func ifs.elseBranch

  | S_switch sw -> 
      condition_fun current_func sw.switch_loc sw.switch_cond;
      statement_fun current_func sw.branches

  | S_while sw -> 
      condition_fun current_func sw.while_loc sw.while_cond;
      statement_fun current_func sw.while_body

  | S_doWhile dw -> 
      statement_fun current_func dw.do_while_body;
      full_expression_fun current_func dw.do_while_loc dw.do_while_expr

  | S_for sf -> 
      statement_fun current_func sf.for_init;
      condition_fun current_func sf.for_loc sf.for_cond;
      full_expression_fun current_func sf.for_loc sf.after;
      statement_fun current_func sf.for_body

  | S_break(_annot, _sourceLoc) -> ()

  | S_continue(_annot, _sourceLoc) -> ()

  | S_return sr -> 
      opt_iter (full_expression_fun current_func sr.return_loc) sr.return_expr;
      opt_iter (statement_fun current_func) sr.return_ctor_statement

  | S_goto(_annot, _sourceLoc, _stringRef) -> ()

  | S_decl(_annot, sourceLoc, declaration) -> 
      inner_declaration_fun current_func sourceLoc declaration

  | S_try st -> 
      statement_fun current_func st.try_body
	(* XXX handlers? And the FullExpressionAnnot therein?? *)

  | S_asm(_annot, _sourceLoc, _e_stringLit) -> ()

  | S_namespaceDecl(_annot, _sourceLoc, _namespaceDecl) -> ()

  | S_function(_annot, sourceLoc, func) -> 
      function_fun sourceLoc func

  | S_rangeCase src -> 
      (* XXX expressions?? *)
      statement_fun current_func src.range_case_stmt

  | S_computedGoto(_annot, sourceLoc, expression) -> 
      expression_fun current_func sourceLoc expression


(* Distinguish between inner (in function bodies) and outer (top level)
 * declarations. Outer declarations might carry a class that we have
 * to go through. Their list of declared variables can be empty.
 * In inner declarations we don't accept class definitions and there must be 
 * at least one variable declared and
 *)
and outer_declaration_fun current_func sourceLoc dt =
  typeSpecifier_fun dt.declaration_spec;
  List.iter (declarator_fun current_func sourceLoc) dt.decllist


and inner_declaration_fun current_func sourceLoc dt =
  if dt.decllist = [] then
    found sourceLoc dt.declaration_annotation
      Unimplemented "empty declaration list";
  (match dt.declaration_spec with
     | TS_classSpec _ ->
	 found 
	   (typeSpecifier_loc dt.declaration_spec)
	   (typeSpecifier_annotation dt.declaration_spec)
	   Unimplemented
	   "inner class spec";

     | TS_name _
     | TS_simple _
     | TS_elaborated _
     | TS_enumSpec _ 
     | TS_type _ 
       -> ()
     | TS_typeof _
       -> 
	 found
	   (typeSpecifier_loc dt.declaration_spec) 
	   (typeSpecifier_annotation dt.declaration_spec)
	   Unimplemented
	   "unknown type specifier in inner decl";
  );
  List.iter (declarator_fun current_func sourceLoc) dt.decllist


and declarator_fun current_func sourceLoc declarator =
  (* It seems that IN_ctor arguments are removed, give a message
   * if we find some.
   *)
  (match declarator.declarator_init with
     | Some(IN_ctor { init_ctor_args = _ :: _ }) ->
	 found sourceLoc declarator.declarator_annotation
	   Message "IN_ctor with arguments"
     | _ -> ());
  (* Tell me if there is a ctor without init! *)
  (match (declarator.declarator_init, declarator.declarator_ctor_statement) with
     | (None, Some _) -> 
	 fatal sourceLoc declarator.declarator_annotation
	   "declarator with ctor and without init"
     | _ -> ());
  (* Came to the conclusion that ctor statement is synthesised from
   * init. Therefore, whenever there is a ctor there is also an init.
   * Inits of the form IN_ctor lack their arguments, therefore for such
   * inits, it is necessary to traverse the ctor. Things like int i(3) 
   * are apparently rewritten into IN_expr. Therefore we have the hypothesis 
   * that you have IN_ctor precisely when you have a direct ctor call.
   * 
   * However, temporaries and their descructor calls can only be found
   * in the FullExpressionAnnot of the IN_ctor.
   * 
   * We assume now that the material of IN_ctor is captured in the 
   * ctor. For other initializers all material is in the initializer 
   * and there is no ctor at all (eg Z * z = new ...).
   *)
  (match (declarator.declarator_init, declarator.declarator_ctor_statement) with
     | (Some(IN_ctor _), None) -> 
	 fatal sourceLoc declarator.declarator_annotation
	   "IN_ctor without ctor statement"
     | _ -> ());
  
  (* do some processing now *)
  if (match declarator.declarator_init with
	| Some(IN_ctor _) -> true
	| _ -> false)
  then 
    begin
      opt_iter (statement_fun current_func) 
	declarator.declarator_ctor_statement;
      match declarator.declarator_init with
	| Some(IN_ctor ct) -> 
	    fullExpressionAnnot_fun current_func ct.init_ctor_loc 
	      ct.init_ctor_annot
	| _ -> assert false
    end
  else
    opt_iter (initializer_fun current_func) declarator.declarator_init;
  opt_iter (statement_fun current_func) declarator.declarator_dtor_statement


and initializer_fun current_func = function
    (* XXX do the FullExpressionAnnot in all cases!! *)
  | IN_expr iex -> 
      (* work around elsa bug: expression_opt should never be None *)
      opt_iter (expression_fun current_func iex.init_expr_loc) iex.e;
      fullExpressionAnnot_fun current_func iex.init_expr_loc iex.init_expr_annot

  | IN_compound ic -> 
      if ic.init_compound_annot.declarations <> [] then
	found ic.init_compound_loc
	  ic.init_compound_annot.fullExpressionAnnot_annotation
	  Unimplemented
	  "IN_compound with nonempty init_compound_annot";
      List.iter (initializer_fun current_func) ic.init_compound_inits

  | IN_ctor ic -> 
      if ic.init_ctor_annot.declarations <> [] then
	found ic.init_ctor_loc ic.init_ctor_annot.fullExpressionAnnot_annotation
	  Unimplemented
	  "IN_ctor with nonempty init_ctor_annot";
      if ic.init_ctor_args <> [] then
	 found ic.init_ctor_loc ic.iN_ctor_annotation
	   Message "IN_ctor with arguments";
      (* we carefully checked above to avoid this case... *)
      assert false;

  | IN_designated id -> 
      if id.init_designated_annot.declarations <> [] then
	found id.init_designated_loc
	  id.init_designated_annot.fullExpressionAnnot_annotation
	  Unimplemented
	  "IN_designated with nonempty init_designated_annot";
      (* if this assertion is triggered investigate what to traverse *)
      found id.init_designated_loc id.iN_designated_annotation
	Unimplemented "designated initializer";



and full_expression_fun current_func sourceLoc fet =
  opt_iter (expression_fun current_func sourceLoc) fet.full_expr_expr;
  fullExpressionAnnot_fun current_func sourceLoc fet.full_expr_annot


and fullExpressionAnnot_fun current_func sourceLoc fea =
    List.iter (inner_declaration_fun current_func sourceLoc) 
      fea.declarations


and condition_fun current_func sourceLoc = function
  | CN_expr(_annot, fullExpression) -> 
      full_expression_fun current_func sourceLoc fullExpression

  | CN_decl(_annot, _aSTTypeId) -> 
      (* XXX initializer? *)
      (* XXX declarator/expressions in aSTTypeId?? *)
      ()


and expression_fun current_func sourceLoc = function
  | E_boolLit(_annot, _type_opt, _bool) -> ()

  | E_intLit _ -> ()

  | E_floatLit _ -> ()

  | E_stringLit _ -> ()

  | E_charLit _ -> ()

  | E_this(_annot, _type_opt, _variable) -> ()

  | E_variable _ -> ()

  | E_funCall fc -> 
      (match fc.fun_call_ret_obj with
	 | None 
	 | Some(E_variable _)
	     -> ()
	 | Some _ -> 
	     (* if this assertion is hit: investigate if we have to traverse
	      * the expression_retobj
	      *)
	     found sourceLoc fc.e_funCall_annotation
	       Unimplemented "retObj in E_funCall";
      );
      expression_fun current_func sourceLoc fc.func;
      List.iter (argExpression_fun current_func sourceLoc) fc.fun_call_args;
      add_callee current_func (get_function_id sourceLoc fc.func)

  | E_constructor ec (* (annot, _type_opt, _typeSpecifier, argExpression_list, 
		   * var_opt, _bool, expression_retobj_opt) *) -> 
      List.iter (argExpression_fun current_func sourceLoc) ec.constructor_args;
      (match ec.constructor_ret_obj with
	 | None -> ()
	 | Some(E_variable _) -> ()
	 | Some _ -> 
	     (* if this assertion is hit: investigate if we have to traverse
	      * the expression_retobj
	      *)
	     found sourceLoc ec.e_constructor_annotation
	       Unimplemented "retObj in E_constructor";
      );
      (match ec.constructor_ctor_var with
	 | None ->
	     fatal sourceLoc ec.e_constructor_annotation
	       "E_constructor without constructor";
	 | Some var ->
	     add_callee current_func 
	       (get_function_id_from_variable_node sourceLoc var)
      )

  | E_fieldAcc e -> 
      expression_fun current_func sourceLoc e.field_access_obj
	(* XXX what about those ~ destructor fields?? *)

  | E_sizeof _ -> ()
      (* XXX is expression evaluated at runtime?? *)

  | E_unary e -> 
      expression_fun current_func sourceLoc e.unary_expr_expr

  | E_effect e -> 
      expression_fun current_func sourceLoc e.effect_expr

  | E_binary e -> 
      expression_fun current_func sourceLoc e.e1;
      expression_fun current_func sourceLoc e.e2

  | E_addrOf(_annot, _type_opt, expression) -> 
      expression_fun current_func sourceLoc expression

  | E_deref(_annot, _type_opt, expression) -> 
      expression_fun current_func sourceLoc expression

  | E_cast e -> 
      expression_fun current_func sourceLoc e.cast_expr

  | E_cond e -> 
      expression_fun current_func sourceLoc e.cond_cond;
      expression_fun current_func sourceLoc e.th;
      expression_fun current_func sourceLoc e.cond_else

  | E_sizeofType _ -> ()

  | E_assign e -> 
      expression_fun current_func sourceLoc e.target;
      expression_fun current_func sourceLoc e.src

  | E_new e -> 
      List.iter (argExpression_fun current_func sourceLoc) e.placementArgs;
      opt_iter (argExpressionListOpt_fun current_func sourceLoc) e.ctorArgs;
      opt_iter (expression_fun current_func sourceLoc) e.arraySize;
      if e.new_ctor_statement = None then
	found sourceLoc e.e_new_annotation
	  Unimplemented "E_new without ctor statement";
      opt_iter (statement_fun current_func) e.new_ctor_statement

  | E_delete e -> 
      (* XXX dtorStatement *)
      opt_iter (expression_fun current_func sourceLoc) e.delete_expr;
      opt_iter (statement_fun current_func) e.delete_dtor_statement

  | E_throw _ ->
      (* XXX *)
      (* XXX globalCtorStatement *)
      ()

  | E_keywordCast e -> 
      expression_fun current_func sourceLoc e.keyword_cast_expr

  | E_typeidExpr(_annot, _type_opt, _expression) -> 
      (* XXX what's that? *)
      ()

  | E_typeidType(_annot, _type_opt, _aSTTypeId) -> 
      (* XXX what's that? *)
      ()

  | E_grouping(_annot, _type_opt, _expression) -> 
      (* only used before typechecking *)
      assert false

  | E_arrow _ -> 
      (* only used before typechecking *)
      assert false

  | E_statement(_annot, _type_opt, s_compound) -> 
      statement_fun current_func s_compound

  | E_compoundLit _ -> 
      (* XXX *)
      (* XXX in_compound contains some expression or whatever leads to a 
       * FullExpressionAnnot 
       *)
      ()

  | E___builtin_constant_p _ -> 
      (* XXX ?? *)
      ()

  | E___builtin_va_arg _ -> 
      (* XXX ?? *)
      ()

  | E_alignofType _ -> 
      (* XXX evaluated? *)
      ()

  | E_alignofExpr _ -> 
      (* XXX what'd that? *)
      ()

  | E_gnuCond e -> 
      expression_fun current_func sourceLoc e.gnu_cond_cond;
      expression_fun current_func sourceLoc e.gnu_cond_else

  | E_addrOfLabel(_annot, _type_opt, _stringRef) -> ()

  | E_stdConv e ->
      assert(check_standardConversion e.stdConv);
      expression_fun current_func sourceLoc e.std_conv_expr



and argExpression_fun current_func sourceLoc ae =
  expression_fun current_func sourceLoc ae.arg_expr_expr


and argExpressionListOpt_fun current_func sourceLoc ae =
  List.iter (argExpression_fun current_func sourceLoc) ae.arg_expression_list

and handler_fun _ =
  (* XXX
   * What to do here? 
   * In any case: don't forget the FullExpressionAnnot!!
   * globalDtorStatement
   * expression in localArg?
   *)
  ()


(***********************************************************************
 *
 * whole file processing
 *
 ***********************************************************************)

let print_gc_stat () =
  Gc.full_major();
  Printf.printf "live words: %d\n" 
    (Gc.stat()).Gc.live_words


let do_file file =
  let _ = current_oast := file in
  let (_, ast) = Elsa_oast_header.unmarshal_oast file
  in
    compilationUnit_fun ast


let do_file_gc file =
  Printf.printf "File %s\n" file;
  do_file file;
  print_gc_stat()


let do_file_list files error_report_levels = 
  init_error_report_array error_report_levels;
  List.iter do_file files;
  let res = !function_def_hash
  in
    function_def_hash := new_func_def_hash();
    res
