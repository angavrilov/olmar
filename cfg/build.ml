(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* XXX
 * FullExpressionAnnot contains declarations which might contain 
 * constructors: Therefore check if all FullExpressionAnnot's are examined! 
 * XXX check generally, if all expressions/statements are reached
 *)


(* 
 * open Cc_ml_types
 * open Ml_ctype
 * open Cc_ast_gen_type
 * open Ast_annotation
 * open Ast_accessors
 * open Ast_util
 *)
open Cc_ml_types
open Ml_ctype
open Cc_ast_gen_type
open Ast_annotation
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


let redefinition_error entry other_loc name =
  Printf.printf 
    ("%s: %s\n"
     ^^ "    nonidentical function redefinition in %s\n"
     ^^ "    callee %s missing in one of the definitions\n"
     ^^ "%s: original definition in %s\n")
    (error_location entry.loc)
    (string_of_function_id entry.fun_id)
    (!current_oast)
    (string_of_function_id name)
    (error_location other_loc)
    entry.oast


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
      Redef(entry, call_hash, check_hash, sourceLoc, ref false)
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
    | Redef(entry, call_hash, check_hash, other_loc, error) ->
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
		  redefinition_error entry other_loc f;
		  error := true
		end
	  end
  

exception Finished

let finish_fun_call = function
  | New(entry, call_hash) ->
      entry.callees <- Hashtbl.fold (fun f _ l -> f::l) call_hash []
  | Redef(entry, _call_hash, check_hash, other_loc, error) ->
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
	    redefinition_error entry other_loc !missing_fun
	
	


(***********************************************************************
 *
 * utility functions
 *
 ***********************************************************************)


(* append the list of nested scope names to accumulator name *)
let rec get_scope_name sourceLoc name_list scope =
  match scope.scope_kind with
    | SK_GLOBAL -> name_list
    | SK_NAMESPACE ->
	(match !(scope.namespace_var) with
	   | None ->
	       fatal sourceLoc scope.poly_scope
		 (Printf.sprintf "scope %d without var node" 
		    (id_annotation scope.poly_scope));
	   | Some variable ->
	       match variable.var_name with
		 | None -> 
		     fatal sourceLoc variable.poly_var
		       "scope var without name";
		 | Some name ->
		     match scope.parent_scope with
		       | None -> 
			   fatal sourceLoc scope.poly_scope
			     "inner scope without parent";
		       | Some parent ->
			   get_scope_name sourceLoc (name :: name_list) parent
	)
    | SK_CLASS ->
	(match !(scope.scope_compound) with
	   | None -> 
	       fatal sourceLoc scope.poly_scope
		 "class scope without compound";
	   | Some compound ->
	       match compound.compound_name with
		 | None ->
		     fatal sourceLoc compound.compound_info_poly
		       "class scope compound without name";
		 | Some name ->
		     match scope.parent_scope with
		       | None -> 
			   fatal sourceLoc scope.poly_scope
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
	fatal sourceLoc scope.poly_scope"strange scope"


(* convert an atomic argument type into a string (no C++ syntax) *)
let string_of_atomic_type sourceLoc = function
  | SimpleType(_annot, simpleTypeId) ->
      string_of_simpleTypeId simpleTypeId

  | CompoundType(compound) ->
      (match compound.compound_name with
	| None -> 
	    fatal sourceLoc compound.compound_info_poly
	      "atomic compound argument without name";
	| Some x -> x
      )

  | PseudoInstantiation(annot, _str, _variable_opt, _accessKeyword, 
			_compound_info, _sTemplateArgument_list) ->
      found sourceLoc annot Unimplemented
	"atomic pseudo instantiation argument";
      "unknown atomic type I"

  | EnumType(annot, string_opt, _variable, _accessKeyword, 
	     _enum_value_list, _has_negatives) ->
      (match string_opt with
	 | None -> 
	     fatal sourceLoc annot"enum argument without name";
	 | Some name -> name
      )

  | TypeVariable(annot, _string, _variable, _accessKeyword) ->
      found sourceLoc annot Unimplemented "type variable argument";
      "unknown atomic type II"

  | DependentQType(annot, _string, _variable, 
		   _accessKeyword, _atomic, _pq_name) ->
      found sourceLoc annot Unimplemented "dependent Q type argument";
      "unknown atomic type III"


(* convert an argument type into a string, (no C++ syntax) *)
let rec string_of_ctype sourceLoc = function
  | CVAtomicType(annot, cVFlags, atomicType) ->
      if List.mem CV_VOLATILE cVFlags then 
	found sourceLoc annot Unimplemented "volatile function argument";
      let const = List.mem CV_CONST cVFlags
      in
	(if const then "const(" else "")
	^ string_of_atomic_type sourceLoc atomicType
	^ (if const then ")" else "")

  | PointerType(annot, cVFlags, cType) ->
      if List.mem CV_VOLATILE cVFlags then 
	found sourceLoc annot Unimplemented "volatile function argument";
      let const = List.mem CV_CONST cVFlags
      in
	(if const then "const " else "")
	^ "pointer("
	^ (string_of_ctype sourceLoc cType)
	^ ")"

  | ReferenceType(_annot, cType) ->
      "reference("
      ^ (string_of_ctype sourceLoc cType)
      ^ ")"

  | FunctionType(annot, function_flags, cType, 
		 variable_list, _cType_list_opt) ->
      if function_flags <> [] then 
	found sourceLoc annot Unimplemented "function type argument with flags";
      "function("
      ^ (String.concat ", " 
	   (List.map (get_param_type sourceLoc) variable_list))
      ^ " -> "
      ^ (string_of_ctype sourceLoc cType)
      ^ ")"

  | ArrayType(annot, _cType, _array_size) ->
      found sourceLoc annot Unimplemented "array type argument";
      "unknown ctype I"

  | DependentSizeArrayType(annot, _cType, _size_expr) ->
      found sourceLoc annot Unimplemented "dependent size array argument";
      "unknown ctype II"

  | PointerToMemberType(annot, _atomicType (* = NamedAtomicType *), 
			_cVFlags, _cType) ->
      found sourceLoc annot Unimplemented "member pointer argument";
      "unknown ctype III"


(* get a string representation of one parameter type *)
and get_param_type sourceLoc variable =
  match !(variable.var_type) with
    | None -> assert false
    | Some ctype -> string_of_ctype sourceLoc ctype
  
(* get a string representation of the parameter types of a function
 * to disambiguate overloaded functions
 *)
let get_param_types sourceLoc variable =
  match !(variable.var_type) with
    | None -> assert false
    | Some ctype -> 
	match ctype with 
	  | FunctionType(annot, function_flags, _cType,
			 variable_list, _cType_list_opt) ->
	      let parameters =
		if List.mem FF_METHOD function_flags
		then
		  match variable_list with
		    | [] -> 
			fatal sourceLoc annot
			  "method without implicit argument";
		    | receiver :: args ->
			match receiver.var_name with
			  | Some "__receiver" -> args
			  | _ -> 
			      fatal sourceLoc annot
				"method without __receiver arg";
		else
		  variable_list
	      in
		List.map (get_param_type sourceLoc) parameters

	  | CVAtomicType _
	  | PointerType _
	  | ReferenceType _
	  | ArrayType _
	  | DependentSizeArrayType _
	  | PointerToMemberType _ ->
	      found sourceLoc (cType_annotation ctype) Unimplemented
		"function without function type";
	      []		


(* extracts the function id from the variable declaration node *)
let get_function_id_from_variable_node sourceLoc variable =
  match variable.var_name with
    | None -> 
	fatal sourceLoc variable.poly_var
	  "function var node without name";
    | Some var_name -> 
	match variable.scope with
	  | None -> 
	      fatal sourceLoc variable.poly_var
		"function decl with empty scope"
	  | Some scope ->
	      { name = get_scope_name sourceLoc [var_name] scope;
		param_types = get_param_types sourceLoc variable;
	      }
		


(* extracts the function id from the declarator exessible in a 
 * function definition 
 *)
let get_function_definition_id sourceLoc declarator =
  let (annot, _iDeclarator, _init_opt, 
       variable_opt, _ctype_opt, _declaratorContext,
       _statement_opt_ctor, _statement_opt_dtor) = declarator
  in
    match variable_opt with
      | None -> 
	  fatal sourceLoc annot"function def with empty declarator";
      | Some variable ->
	  get_function_id_from_variable_node sourceLoc variable

(* extracts the function id from 
 * - a simple function call 
 * - a method call
 *)
let get_function_id sourceLoc function_expression =
  match function_expression with
    | E_variable(annot, _type_opt, _pQName, var_opt, _nondep_var_opt) ->
	(* Printf.printf "E_variable at %s\n" (error_location sourceLoc); *)
	(match var_opt with
	   | None ->
	       fatal sourceLoc annot"function without decl var node";
	   | Some variable ->
	       get_function_id_from_variable_node sourceLoc variable
	)
    | E_fieldAcc(annot, _type_opt, _expression, _pQName, var_opt) -> 
	(match var_opt with
	   | None ->
	       fatal sourceLoc annot"field access without var node";
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


let rec compilationUnit_fun 
    ((_annot, name, tu) : annotated compilationUnit_type) =
  let comp_unit_id = {
    name = ["CU " ^ name];
    param_types = []
  } in
  let comp_unit_loc = (name, 1, 0) in
  let comp_unit = function_def comp_unit_id comp_unit_loc name 0
  in
    translationUnit_fun comp_unit tu;
    finish_fun_call comp_unit


and translationUnit_fun comp_unit
    ((_annot, topForm_list, _scope_opt)  : annotated translationUnit_type) =
  List.iter (topForm_fun comp_unit) topForm_list


and topForm_fun comp_unit = function
  | TF_linkage(_annot, _sourceLoc, _stringRef, translationUnit) -> 
      translationUnit_fun comp_unit translationUnit

  | TF_one_linkage(_annot, _sourceLoc, _stringRef, topForm) -> 
      topForm_fun comp_unit topForm

  | TF_func(_annot, sourceLoc, func) -> 
      function_fun sourceLoc func

  | TF_namespaceDefn(_annot, _sourceLoc, _stringRef_opt, topForm_list) -> 
      List.iter (topForm_fun comp_unit) topForm_list

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
    | TS_classSpec(_annot, _sourceLoc, _cVFlags, _typeIntr, _pQName_opt, 
		   _baseClassSpec_list, memberList, _compoundType) -> 
	let (_, mem_list) = memberList
	in
	  List.iter member_fun mem_list

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



and function_fun sourceLoc 
    (annot, _declFlags, _typeSpecifier, declarator, memberInit_list, 
     s_compound_opt, _handler_list, _func, _variable_opt_1, 
     _variable_opt_2, _statement_opt, _bool) =
  (* XXX handler_list?? And the FullExpressionAnnot therein?? *)
  (* XXX dtorStatement ?? *)
  (* XXX declarator might contain initializers or other expressions? *)
  let entry = 
    function_def (get_function_definition_id sourceLoc declarator) 
      sourceLoc !current_oast (id_annotation annot);
  in
    List.iter 
      (memberInit_fun entry sourceLoc)
      memberInit_list;
    (match s_compound_opt with
       | None -> ()
       | Some compound -> 
	   statement_fun entry compound
    );
    finish_fun_call entry


and memberInit_fun current_func sourceLoc
    (annot, _pQName, argExpression_list, _data_member_var_opt, 
     class_member_compound_opt, _ctor_var_opt, full_expr_annot, 
     statement_opt) 
    =
  (match full_expr_annot with
     | (_, []) -> ()
     | _ -> 
	 found sourceLoc annot Unimplemented
	   "declaration in FullExpressionAnnot";
  );
  (* go through the arguments *)
  List.iter (argExpression_fun current_func sourceLoc) argExpression_list;
  if (class_member_compound_opt <> None) && (statement_opt = None) then
    found sourceLoc annot Unimplemented "base init without ctor statement";
  opt_iter (statement_fun current_func) statement_opt      


and statement_fun current_func = function
  | S_skip(_annot, _sourceLoc) -> ()
	    
  | S_label(_annot, _sourceLoc, _stringRef, statement) -> 
      statement_fun current_func statement

  | S_case(_annot, _sourceLoc, _expression, statement, _int32) -> 
      statement_fun current_func statement

  | S_default(_annot, _sourceLoc, statement) -> 
      statement_fun current_func statement
	
  | S_expr(_annot, sourceLoc, fullExpression) -> 
      full_expression_fun current_func sourceLoc fullExpression

  | S_compound(_annot, _sourceLoc, statement_list) -> 
      List.iter (statement_fun current_func) statement_list

  | S_if(_annot, sourceLoc, condition, statement_then, statement_else) -> 
      condition_fun current_func sourceLoc condition;
      statement_fun current_func statement_then;
      statement_fun current_func statement_else

  | S_switch(_annot, sourceLoc, condition, statement) -> 
      condition_fun current_func sourceLoc condition;
      statement_fun current_func statement

  | S_while(_annot, sourceLoc, condition, statement) -> 
      condition_fun current_func sourceLoc condition;
      statement_fun current_func statement

  | S_doWhile(_annot, sourceLoc, statement, fullExpression) -> 
      statement_fun current_func statement;
      full_expression_fun current_func sourceLoc fullExpression

  | S_for(_annot, sourceLoc, statement_init, condition, fullExpression, 
	  statement_body) -> 
      statement_fun current_func statement_init;
      condition_fun current_func sourceLoc condition;
      full_expression_fun current_func sourceLoc fullExpression;
      statement_fun current_func statement_body

  | S_break(_annot, _sourceLoc) -> ()

  | S_continue(_annot, _sourceLoc) -> ()

  | S_return(_annot, sourceLoc, fullExpression_opt, statement_opt) -> 
      opt_iter (full_expression_fun current_func sourceLoc) fullExpression_opt;
      opt_iter (statement_fun current_func) statement_opt

  | S_goto(_annot, _sourceLoc, _stringRef) -> ()

  | S_decl(_annot, sourceLoc, declaration) -> 
      inner_declaration_fun current_func sourceLoc declaration

  | S_try(_annot, _sourceLoc, statement, _handler_list) -> 
      statement_fun current_func statement
	(* XXX handlers? And the FullExpressionAnnot therein?? *)

  | S_asm(_annot, _sourceLoc, _e_stringLit) -> ()

  | S_namespaceDecl(_annot, _sourceLoc, _namespaceDecl) -> ()

  | S_function(_annot, sourceLoc, func) -> 
      function_fun sourceLoc func

  | S_rangeCase(_annot, _sourceLoc, 
		_expression_lo, _expression_hi, statement, 
		_label_lo, _label_hi) -> 
      (* XXX expressions?? *)
      statement_fun current_func statement

  | S_computedGoto(_annot, sourceLoc, expression) -> 
      expression_fun current_func sourceLoc expression


(* Distinguish between inner (in function bodies) and outer (top level)
 * declarations. Outer declarations might carry a class that we have
 * to go through. Their list of declared variables can be empty.
 * In inner declarations we don't accept class definitions and there must be 
 * at least one variable declared and
 *)
and outer_declaration_fun current_func sourceLoc
    (_annot, _declFlags, typeSpecifier, declarator_list) =
  typeSpecifier_fun typeSpecifier;
  List.iter (declarator_fun current_func sourceLoc) declarator_list


and inner_declaration_fun current_func sourceLoc
    (annot, _declFlags, typeSpecifier, declarator_list) =
  if declarator_list = [] then
    found sourceLoc annot Unimplemented "empty declaration list";
  (match typeSpecifier with
     | TS_classSpec _ ->
	 found 
	   (typeSpecifier_loc typeSpecifier)
	   (typeSpecifier_annotation typeSpecifier)
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
	   (typeSpecifier_loc typeSpecifier) 
	   (typeSpecifier_annotation typeSpecifier)
	   Unimplemented
	   "unknown type specifier in inner decl";
  );
  List.iter (declarator_fun current_func sourceLoc) declarator_list


and declarator_fun current_func sourceLoc
    (annot, _iDeclarator, init_opt, _variable_opt, _ctype_opt, 
     _declaratorContext, statement_opt_ctor, statement_opt_dtor) =
  (* It seems that IN_ctor arguments are removed, give a message
   * if we find some.
   *)
  (match init_opt with
     | Some(IN_ctor(_,_,_,_ :: _,_,_)) ->
	 found sourceLoc annot Message "IN_ctor with arguments"
     | _ -> ());
  (* Tell me if there is a ctor without init! *)
  (match (init_opt, statement_opt_ctor) with
     | (None, Some _) -> 
	 fatal sourceLoc annot "declarator with ctor and without init"
     | _ -> ());
  (* Came to the conclusion that ctor statement is synthesised from
   * init. Therefore, whenever there is a ctor there is also an init.
   * Inits of the form IN_ctor lack their arguments, therefore for such
   * inits, it is necessary to traverse the ctor. Things like int i(3) 
   * are apparently rewritten into IN_expr. Therefore we have the hypothesis 
   * that you have IN_ctor precisely when you have a ctor.
   * 
   * We assume now that the material of IN_ctor is captured in the 
   * ctor. For other initializers all material is in the initializer 
   * and there is no ctor at all (eg Z * z = new ...).
   *)
  (match (init_opt, statement_opt_ctor) with
     | (Some(IN_ctor _), None) -> 
	 fatal sourceLoc annot "IN_ctor without ctor statement"
     | _ -> ());
  
  (* do some processing now *)
  if (match init_opt with
	| Some(IN_ctor _) -> true
	| _ -> false)
  then
    opt_iter (statement_fun current_func) statement_opt_ctor
  else
    opt_iter (initializer_fun current_func) init_opt;
  opt_iter (statement_fun current_func) statement_opt_dtor


and initializer_fun current_func = function
    (* XXX do the FullExpressionAnnot in all cases!! *)
  | IN_expr(_annot, sourceLoc, fullExpressionAnnot, expression_opt) -> 
      (* work around elsa bug: expression_opt should never be None *)
      opt_iter (expression_fun current_func sourceLoc) expression_opt;
      fullExpressionAnnot_fun current_func sourceLoc fullExpressionAnnot

  | IN_compound(_annot, sourceLoc, fullExpressionAnnot, init_list) -> 
      
      if snd fullExpressionAnnot <> [] then
	found sourceLoc (fst fullExpressionAnnot) Unimplemented
	  "IN_compound with nonempty fullExpressionAnnot";
      List.iter (initializer_fun current_func) init_list

  | IN_ctor(annot, sourceLoc, fullExpressionAnnot, 
	    argExpression_list, _var_opt, _bool) -> 
      if snd fullExpressionAnnot <> [] then
	found sourceLoc (fst fullExpressionAnnot) Unimplemented
	  "IN_ctor with nonempty fullExpressionAnnot";
      if argExpression_list <> [] then
	 found sourceLoc annot Message "IN_ctor with arguments";
      (* we carefully checked above to avoid this case... *)
      assert false;

  | IN_designated(annot, sourceLoc, fullExpressionAnnot, 
		  _designator_list, _init) -> 
      if snd fullExpressionAnnot <> [] then
	found sourceLoc (fst fullExpressionAnnot) Unimplemented
	  "IN_designated with nonempty fullExpressionAnnot";
      (* if this assertion is triggered investigate what to traverse *)
      found sourceLoc annot Unimplemented "designated initializer";



and full_expression_fun current_func sourceLoc
    (_annot, expression_opt, fullExpressionAnnot) =
  opt_iter (expression_fun current_func sourceLoc) expression_opt;
  fullExpressionAnnot_fun current_func sourceLoc fullExpressionAnnot


and fullExpressionAnnot_fun current_func sourceLoc (_annot, decl_list) =
    List.iter (inner_declaration_fun current_func sourceLoc) decl_list


and condition_fun current_func sourceLoc = function
  | CN_expr(_annot, fullExpression) -> 
      full_expression_fun current_func sourceLoc fullExpression

  | CN_decl(_annot, _aSTTypeId) -> 
      (* XXX initializer? *)
      (* XXX declarator/expressions in aSTTypeId?? *)
      ()


and expression_fun current_func sourceLoc = function
  | E_boolLit(_annot, _type_opt, _bool) -> ()

  | E_intLit(_annot, _type_opt, _stringRef, _ulong) -> ()

  | E_floatLit(_annot, _type_opt, _stringRef, _double) -> ()

  | E_stringLit(_annot, _type_opt, _stringRef, 
		      _e_stringLit_opt, _stringRef_opt) -> ()

  | E_charLit(_annot, _type_opt, _stringRef, _int32) -> ()

  | E_this(_annot, _type_opt, _variable) -> ()

  | E_variable(_annot, _type_opt, _pQName, _var_opt, _nondep_var_opt) -> ()

  | E_funCall(annot, _type_opt, expression_func, 
	      argExpression_list, expression_retobj_opt) -> 
      (match expression_retobj_opt with
	 | None -> ()
	 | Some _ -> 
	     (* if this assertion is hit: investigate if we have to traverse
	      * the expression_retobj
	      *)
	     found sourceLoc annot Unimplemented "retObj in E_funCall";
      );
      expression_fun current_func sourceLoc expression_func;
      List.iter (argExpression_fun current_func sourceLoc) argExpression_list;
      add_callee current_func (get_function_id sourceLoc expression_func)

  | E_constructor(annot, _type_opt, _typeSpecifier, argExpression_list, 
		  var_opt, _bool, expression_retobj_opt) -> 
      List.iter (argExpression_fun current_func sourceLoc) argExpression_list;
      (match expression_retobj_opt with
	 | None -> ()
	 | Some(E_variable _) -> ()
	 | Some _ -> 
	     (* if this assertion is hit: investigate if we have to traverse
	      * the expression_retobj
	      *)
	     found sourceLoc annot Unimplemented "retObj in E_constructor";
      );
      (match var_opt with
	 | None ->
	     fatal sourceLoc annot"E_constructor without constructor";
	 | Some var ->
	     add_callee current_func 
	       (get_function_id_from_variable_node sourceLoc var)
      )

  | E_fieldAcc(_annot, _type_opt, expression, _pQName, _var_opt) -> 
      expression_fun current_func sourceLoc expression
	(* XXX what about those ~ destructor fields?? *)

  | E_sizeof(_annot, _type_opt, _expression, _int) -> ()
      (* XXX is expression evaluated at runtime?? *)

  | E_unary(_annot, _type_opt, _unaryOp, expression) -> 
      expression_fun current_func sourceLoc expression

  | E_effect(_annot, _type_opt, _effectOp, expression) -> 
      expression_fun current_func sourceLoc expression

  | E_binary(_annot, _type_opt, expression_left, _binaryOp, expression_right) -> 
      expression_fun current_func sourceLoc expression_left;
      expression_fun current_func sourceLoc expression_right

  | E_addrOf(_annot, _type_opt, expression) -> 
      expression_fun current_func sourceLoc expression

  | E_deref(_annot, _type_opt, expression) -> 
      expression_fun current_func sourceLoc expression

  | E_cast(_annot, _type_opt, _aSTTypeId, expression, _bool) -> 
      expression_fun current_func sourceLoc expression

  | E_cond(_annot, _type_opt, expression_cond, 
	   expression_true, expression_false) -> 
      expression_fun current_func sourceLoc expression_cond;
      expression_fun current_func sourceLoc expression_true;
      expression_fun current_func sourceLoc expression_false

  | E_sizeofType(_annot, _type_opt, _aSTTypeId, _int, _bool) -> ()

  | E_assign(_annot, _type_opt, expression_target, 
	     _binaryOp, expression_src) -> 
      expression_fun current_func sourceLoc expression_target;
      expression_fun current_func sourceLoc expression_src

  | E_new(annot, _type_opt, _bool, argExpression_list, _aSTTypeId, 
	  argExpressionListOpt_opt, array_size_opt, _ctor_opt,
	  statement_opt, _heep_var) -> 
      List.iter (argExpression_fun current_func sourceLoc) argExpression_list;
      opt_iter (argExpressionListOpt_fun current_func sourceLoc) 
	argExpressionListOpt_opt;
      opt_iter (expression_fun current_func sourceLoc) array_size_opt;
      if statement_opt = None then
	found sourceLoc annot Unimplemented "E_new without ctor statement";
      opt_iter (statement_fun current_func) statement_opt

  | E_delete(_annot, _type_opt, _bool_colon, _bool_array, 
	     expression_opt, statement_opt) -> 
      (* XXX dtorStatement *)
      opt_iter (expression_fun current_func sourceLoc) expression_opt;
      opt_iter (statement_fun current_func) statement_opt

  | E_throw(_annot, _type_opt, _expression_opt, _var_opt, _statement_opt) -> 
      (* XXX *)
      (* XXX globalCtorStatement *)
      ()

  | E_keywordCast(_annot, _type_opt, _castKeyword, _aSTTypeId, expression) -> 
      expression_fun current_func sourceLoc expression

  | E_typeidExpr(_annot, _type_opt, _expression) -> 
      (* XXX what's that? *)
      ()

  | E_typeidType(_annot, _type_opt, _aSTTypeId) -> 
      (* XXX what's that? *)
      ()

  | E_grouping(_annot, _type_opt, _expression) -> 
      (* only used before typechecking *)
      assert false

  | E_arrow(_annot, _type_opt, _expression, _pQName) -> 
      (* only used before typechecking *)
      assert false

  | E_statement(_annot, _type_opt, s_compound) -> 
      statement_fun current_func s_compound

  | E_compoundLit(_annot, _type_opt, _aSTTypeId, _in_compound) -> 
      (* XXX *)
      (* XXX in_compound contains some expression or whatever leads to a 
       * FullExpressionAnnot 
       *)
      ()

  | E___builtin_constant_p(_annot, _type_opt, _sourceLoc, _expression) -> 
      (* XXX ?? *)
      ()

  | E___builtin_va_arg(_annot, _type_opt, _sourceLoc, _expression, _aSTTypeId) -> 
      (* XXX ?? *)
      ()

  | E_alignofType(_annot, _type_opt, _aSTTypeId, _int) -> 
      (* XXX evaluated? *)
      ()

  | E_alignofExpr(_annot, _type_opt, _expression, _int) -> 
      (* XXX what'd that? *)
      ()

  | E_gnuCond(_annot, _type_opt, expression_cond, expression_false) -> 
      expression_fun current_func sourceLoc expression_cond;
      expression_fun current_func sourceLoc expression_false

  | E_addrOfLabel(_annot, _type_opt, _stringRef) -> ()

  | E_stdConv(_annot, _type_opt, expression, scs, _implicitConversion_Kind) ->
      assert(check_standardConversion scs);
      expression_fun current_func sourceLoc expression



and argExpression_fun current_func sourceLoc (_annot, expression) =
  expression_fun current_func sourceLoc expression


and argExpressionListOpt_fun current_func sourceLoc 
    (_annot, argExpressionList) =
  List.iter (argExpression_fun current_func sourceLoc) argExpressionList

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
  let (_, ast) = Oast_header.unmarshal_oast file
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
