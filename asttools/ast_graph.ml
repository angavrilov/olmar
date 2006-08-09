
open Cc_ml_types
open Cc_ast_gen_type
open Ml_ctype
open Ast_annotation

(* node type done: 7, 8, 9, 11, 15 *)

let oc = ref stdout;;

let print_caddr = ref false

(**************************************************************************
 *
 * contents of astiter.ml with sourceLoc_fun removed
 *
 **************************************************************************)


(* 
 * 
 * let opt_iter f = function
 *   | None -> ()
 *   | Some x -> f x
 *)

let bool_label field_name (b : bool) = 
  Printf.fprintf !oc "\\n%s = %B" field_name b

(* let int_fun (i : int) = ()
 * 
 * let string_fun (s : string) = ()
 * 
 * let sourceLoc_fun((file : string), (line : int), (char : int)) = ()
 *)

let declFlags_label field_name (l : declFlags) = 
  Printf.fprintf !oc "\\n%s = %s" field_name (string_of_declFlags l)

(* let variable_fun(v : annotated variable) = ()
 * 
 * let cType_fun(c : annotated cType) = ()
 * 
 * let simpleTypeId_fun(id : simpleTypeId) = ()
 * 
 * let typeIntr_fun(keywort : typeIntr) = ()
 * 
 * let accessKeyword_fun(keyword : accessKeyword) = ()
 * 
 * let cVFlags_fun(fl : cVFlag list) = ()
 * 
 * let overloadableOp_fun(op :overloadableOp) = ()
 * 
 * let unaryOp_fun(op : unaryOp) = ()
 * 
 * let effectOp_fun(op : effectOp) = ()
 * 
 * let binaryOp_fun(op : binaryOp) = ()
 * 
 * let castKeyword_fun(keyword : castKeyword) = ()
 * 
 * let function_flags_fun(flags : function_flags) = ()
 * 
 * 
 * let array_size_fun = function
 *   | NO_SIZE -> ()
 *   | DYN_SIZE -> ()
 *   | FIXED_SIZE(int) -> int_fun int
 * 
 * let compoundType_Keyword_fun = function
 *   | K_STRUCT -> ()
 *   | K_CLASS -> ()
 *   | K_UNION -> ()
 * 
 * 
 * (\***************** variable ***************************\)
 * 
 * 1 let rec variable_fun(v : annotated variable) =
 *   sourceLoc_fun v.loc;
 *   opt_iter string_fun v.var_name;
 * 
 *   (\* POSSIBLY CIRCULAR *\)
 *   opt_iter cType_fun !(v.var_type);
 *   declFlags_fun v.flags;
 *   opt_iter expression_fun v.value;
 *   opt_iter cType_fun v.defaultParam;
 * 
 *   (\* POSSIBLY CIRCULAR *\)
 *   opt_iter func_fun !(v.funcDefn)
 * 
 * 
 * (\***************** cType ******************************\)
 * 
 * 2 and baseClass_fun baseClass =
 *   compound_info_fun baseClass.compound;
 *   accessKeyword_fun baseClass.bc_access;
 *   bool_fun baseClass.is_virtual
 * 
 * 
 * 3 and compound_info_fun info = 
 *   string_fun info.compound_name;
 *   variable_fun info.typedef_var;
 *   accessKeyword_fun info.ci_access;
 *   bool_fun info.is_forward_decl;
 *   compoundType_Keyword_fun info.keyword;
 *   List.iter variable_fun info.data_members;
 *   List.iter baseClass_fun info.bases;
 *   List.iter variable_fun info.conversion_operators;
 *   List.iter variable_fun info.friends;
 *   string_fun info.inst_name;
 * 
 *   (\* POSSIBLY CIRCULAR *\)
 *   opt_iter cType_fun !(info.self_type)
 * 
 * 
 * 4 and atomicType_fun = function
 *   | SimpleType(annot, simpleTypeId) ->
 *       simpleTypeId_fun simpleTypeId
 * 
 *   | CompoundType(compound_info) ->
 *       compound_info_fun compound_info
 * 
 *   | PseudoInstantiation(annot, str, variable_opt, accessKeyword, 
 * 			compound_info, sTemplateArgument_list) ->
 *       string_fun str;
 *       opt_iter variable_fun variable_opt;
 *       accessKeyword_fun accessKeyword;
 *       compound_info_fun compound_info;
 *       List.iter sTemplateArgument_fun sTemplateArgument_list
 * 
 *   | EnumType(annot, string, variable, accessKeyword, string_int_list) ->
 *       string_fun string;
 *       variable_fun variable;
 *       accessKeyword_fun accessKeyword;
 *       List.iter (fun (string, int) -> 
 * 		  (string_fun string; int_fun int))
 * 	string_int_list
 * 
 *   | TypeVariable(annot, string, variable, accessKeyword) ->
 *       string_fun string;
 *       variable_fun variable;
 *       accessKeyword_fun accessKeyword
 * 
 * 
 * 5 and cType_fun = function
 *   | CVAtomicType(annot, cVFlags, atomicType) ->
 *       cVFlags_fun cVFlags;
 *       atomicType_fun atomicType
 * 
 *   | PointerType(annot, cVFlags, cType) ->
 *       cVFlags_fun cVFlags;
 *       cType_fun cType
 * 
 *   | ReferenceType(annot, cType) ->
 *       cType_fun cType
 * 
 *   | FunctionType(annot, function_flags, cType, variable_list, cType_list_opt) ->
 *       function_flags_fun function_flags;
 *       cType_fun cType;
 *       List.iter variable_fun variable_list;
 *       opt_iter (List.iter cType_fun) cType_list_opt
 * 
 *   | ArrayType(annot, cType, array_size) ->
 *       cType_fun cType;
 *       array_size_fun array_size
 * 
 *   | PointerToMemberType(annot, atomicType (\* = NamedAtomicType *\), 
 * 			cVFlags, cType) ->
 *       assert(match atomicType with 
 * 	       | SimpleType _ -> false
 * 	       | CompoundType _
 * 	       | PseudoInstantiation _
 * 	       | EnumType _
 * 	       | TypeVariable _ -> true);
 *       atomicType_fun atomicType;
 *       cVFlags_fun cVFlags;
 *       cType_fun cType
 * 
 * 6 and sTemplateArgument_fun = function
 *   | STA_NONE -> ()
 *   | STA_TYPE(cType) -> cType_fun cType
 *   | STA_INT(int) -> int_fun int
 *   | STA_ENUMERATOR(variable) -> variable_fun variable
 *   | STA_REFERENCE(variable) -> variable_fun variable
 *   | STA_POINTER(variable) -> variable_fun variable
 *   | STA_MEMBER(variable) -> variable_fun variable
 *   | STA_DEPEXPR(expression) -> expression_fun expression
 *   | STA_TEMPLATE -> ()
 *   | STA_ATOMIC(atomicType) -> atomicType_fun atomicType
 *)


let count_rev base l =
  let counter = ref (List.length l)
  in
    List.rev_map 
      (fun x -> decr counter; (x, Printf.sprintf "%s[%d]" base !counter)) 
      l

let opt_string = function
  | None -> "(nil)"
  | Some s -> s

let start_label name color id =
  Printf.fprintf !oc "    \"%d\" [color=\"%s\", label=\"%s %d" 
    id color name id

let finish_label caddr =
  if !print_caddr then
    Printf.fprintf !oc "\\nAddr: 0x%lx"
      (Int32.shift_left (Int32.of_int caddr) 1);
  output_string !oc "\"];\n"


let short_label name color annot =
  start_label name color (id_annotation annot);
  finish_label (caddr_annotation annot)

let loc_start name color id ((file : string), (line : int), (char : int)) =
  start_label name color id;
  Printf.fprintf !oc "\\n%s line %d char %d" file line char

let loc_label name color annot loc =
  loc_start name color (id_annotation annot) loc;
  finish_label (caddr_annotation annot)

let child_edge id (cid,label) =
  Printf.fprintf !oc "    \"%d\" -> \"%d\" [label=\"%s\"];\n" id cid label

let child_edges id childs = 
  List.iter (child_edge id) childs

  
let loc_label_one_child name color annot loc child =
  loc_label name color annot loc;
  child_edge (id_annotation annot) child


let opt_child child_fun field_name opt child_list = 
  match opt with
    | None -> child_list
    | Some c -> (child_fun c, field_name) :: child_list


(***************** colors *****************************)

let color_translationUnit = "red"
let color_TF = "orange"
let color_Declaration = "cyan"
let color_Declarator = "SkyBlue"
let color_Function = "magenta"

(***************** generated ast nodes ****************)

let rec translationUnit_fun 
               ((annot, topForm_list) : annotated translationUnit_type) =
  let id = id_annotation annot in
  let childs = count_rev "topForms" (List.rev_map topForm_fun topForm_list)
  in
    short_label "TranslationUnit" color_translationUnit annot;
    child_edges id childs;
    id


and topForm_fun = function
  | TF_decl(annot, loc, declaration) -> 
      loc_label_one_child "TF_decl" color_TF annot loc 
	(declaration_fun declaration, "decl");
      (id_annotation annot)

  | TF_func(annot, loc, func) -> 
      loc_label_one_child "TF_func" color_TF annot loc (func_fun func, "f");
      (id_annotation annot)

  | TF_template(annot, loc, templateDeclaration) -> 
      loc_label_one_child "TF_template" color_TF annot loc 
	(templateDeclaration_fun templateDeclaration, "td");
      (id_annotation annot)

  | TF_explicitInst(annot, loc, declFlags, declaration) -> 
      let id = id_annotation annot in
      let child = declaration_fun declaration
      in
	loc_start "TF_explicitInst" color_TF id loc;
	declFlags_label "instFlags" declFlags;
	finish_label (caddr_annotation annot);
	child_edge id (child, "d");
	id

  | TF_linkage(annot, loc, linkage, translationUnit) -> 
      let id = id_annotation annot in
      let child = translationUnit_fun translationUnit
      in
	loc_start "TF_linkage" color_TF id loc;
	Printf.fprintf !oc "\\nlinkage: %s" linkage;
	finish_label (caddr_annotation annot);
	child_edge id (child, "forms");
	id

  | TF_one_linkage(annot, loc, linkage, topForm) -> 
      let id = id_annotation annot in
      let child = topForm_fun topForm
      in
	loc_start "TF_one_linkage" color_TF id loc;
	Printf.fprintf !oc "\\nlinkage: %s" linkage;
	finish_label (caddr_annotation annot);
	child_edge id (child, "form");
	id

  | TF_asm(annot, loc, e_stringLit) -> 
      assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
      loc_label_one_child "TF_asm" color_TF annot loc 
	(expression_fun e_stringLit, "text");
      (id_annotation annot)

  | TF_namespaceDefn(annot, loc, name_opt, topForm_list) -> 
      let id = id_annotation annot in
      let childs = count_rev "forms" (List.rev_map topForm_fun topForm_list)
      in
	loc_start "TF_namespaceDefn" color_TF id loc;
	Printf.fprintf !oc "\\nname: %s" (opt_string name_opt);
	finish_label (caddr_annotation annot);
	child_edges id childs;
	id

  | TF_namespaceDecl(annot, loc, namespaceDecl) -> 
      loc_label_one_child "TF_namespaceDecl" color_TF annot loc 
	(namespaceDecl_fun namespaceDecl, "decl");
      (id_annotation annot)


and templateDeclaration_fun _ = 0
and expression_fun _ = 0
and namespaceDecl_fun _ = 0



and func_fun(annot, declFlags, typeSpecifier, declarator, memberInit_list, 
	 s_compound_opt, handler_list, statement_opt, bool) =
  let _ = assert(match s_compound_opt with
		   | None -> true
		   | Some s_compound ->
		       match s_compound with 
			 | S_compound _ -> true 
			 | _ -> false)
  in
  let id = id_annotation annot in
  let childs = 
    (typeSpecifier_fun typeSpecifier, "retspec") ::
      (declarator_fun declarator, "nameAndParams") ::
      (count_rev "inits" (List.rev_map memberInit_fun memberInit_list)) @
      (opt_child statement_fun "body" s_compound_opt
	 ((count_rev "handlers" (List.rev_map handler_fun handler_list)) @ 
	    (opt_child statement_fun "dtor" statement_opt [])))
  in
    start_label "Function" color_Function id;
    declFlags_label "dflags" declFlags;
    bool_label "implicit def" bool;
    finish_label (caddr_annotation annot);
    child_edges id childs;
    id


and memberInit_fun _ = 0
and handler_fun _ = 0

(* 
 * 
 * 10 and memberInit_fun(annot, pQName, argExpression_list, statement_opt) =
 *   pQName_fun pQName;
 *   List.iter argExpression_fun argExpression_list;
 *   opt_iter statement_fun statement_opt
 * 
 * 
 *)

and declaration_fun(annot, declFlags, typeSpecifier, declarator_list) =
  let id = id_annotation annot in
  let childs = 
    (typeSpecifier_fun typeSpecifier, "spec") ::
      count_rev "decllist" (List.rev_map declarator_fun declarator_list)
  in
    start_label "Declaration" color_Declaration id;
    declFlags_label "dflags" declFlags;
    finish_label (caddr_annotation annot);
    child_edges id childs;
    id

and typeSpecifier_fun _ = 0

(* 12 and aSTTypeId_fun(annot, typeSpecifier, declarator) =
 *   typeSpecifier_fun typeSpecifier;
 *   declarator_fun declarator
 * 
 * 
 * 
 * 13 and pQName_fun = function
 *   | PQ_qualifier(annot, sourceLoc, stringRef_opt, 
 * 		 templateArgument_opt, pQName) -> 
 *       sourceLoc_fun sourceLoc;
 *       opt_iter string_fun stringRef_opt;
 *       opt_iter templateArgument_fun templateArgument_opt;
 *       pQName_fun pQName
 * 
 *   | PQ_name(annot, sourceLoc, stringRef) -> 
 *       sourceLoc_fun sourceLoc;
 *       string_fun stringRef
 * 
 *   | PQ_operator(annot, sourceLoc, operatorName, stringRef) -> 
 *       sourceLoc_fun sourceLoc;
 *       operatorName_fun operatorName;
 *       string_fun stringRef
 * 
 *   | PQ_template(annot, sourceLoc, stringRef, templateArgument_opt) -> 
 *       sourceLoc_fun sourceLoc;
 *       string_fun stringRef;
 *       opt_iter templateArgument_fun templateArgument_opt
 * 
 *   | PQ_variable(annot, sourceLoc, variable) -> 
 *       sourceLoc_fun sourceLoc;
 *       variable_fun variable
 * 
 * 
 * 
 * 14 and typeSpecifier_fun = function
 *   | TS_name(annot, sourceLoc, cVFlags, pQName, bool) -> 
 *       sourceLoc_fun sourceLoc;
 *       cVFlags_fun cVFlags;
 *       pQName_fun pQName;
 *       bool_fun bool
 * 
 *   | TS_simple(annot, sourceLoc, cVFlags, simpleTypeId) -> 
 *       sourceLoc_fun sourceLoc;
 *       cVFlags_fun cVFlags;
 *       simpleTypeId_fun simpleTypeId
 * 
 *   | TS_elaborated(annot, sourceLoc, cVFlags, typeIntr, pQName) -> 
 *       sourceLoc_fun sourceLoc;
 *       cVFlags_fun cVFlags;
 *       typeIntr_fun typeIntr;
 *       pQName_fun pQName
 * 
 *   | TS_classSpec(annot, sourceLoc, cVFlags, typeIntr, pQName_opt, 
 * 		 baseClassSpec_list, memberList) -> 
 *       sourceLoc_fun sourceLoc;
 *       cVFlags_fun cVFlags;
 *       typeIntr_fun typeIntr;
 *       opt_iter pQName_fun pQName_opt;
 *       List.iter baseClassSpec_fun baseClassSpec_list;
 *       memberList_fun memberList      
 * 
 *   | TS_enumSpec(annot, sourceLoc, cVFlags, stringRef_opt, enumerator_list) -> 
 *       sourceLoc_fun sourceLoc;
 *       cVFlags_fun cVFlags;
 *       opt_iter string_fun stringRef_opt;
 *       List.iter enumerator_fun enumerator_list
 * 
 *   | TS_type(annot, sourceLoc, cVFlags, cType) -> 
 *       sourceLoc_fun sourceLoc;
 *       cVFlags_fun cVFlags;
 *       cType_fun cType
 * 
 *   | TS_typeof(annot, sourceLoc, cVFlags, aSTTypeof) -> 
 *       sourceLoc_fun sourceLoc;
 *       cVFlags_fun cVFlags;
 *       aSTTypeof_fun aSTTypeof
 * 
 * 
 * 37 and baseClassSpec_fun(annot, bool, accessKeyword, pQName) =
 *   bool_fun bool;
 *   accessKeyword_fun accessKeyword;
 *   pQName_fun pQName
 * 
 * 
 * 38 and enumerator_fun(annot, sourceLoc, stringRef, expression_opt) =
 *   sourceLoc_fun sourceLoc;
 *   string_fun stringRef;
 *   opt_iter expression_fun expression_opt
 * 
 * 
 * 39 and memberList_fun(annot, member_list) =
 *   List.iter member_fun member_list
 * 
 * 
 * 40 and member_fun = function
 *   | MR_decl(annot, sourceLoc, declaration) -> 
 *       sourceLoc_fun sourceLoc;
 *       declaration_fun declaration
 * 
 *   | MR_func(annot, sourceLoc, func) -> 
 *       sourceLoc_fun sourceLoc;
 *       func_fun func
 * 
 *   | MR_access(annot, sourceLoc, accessKeyword) -> 
 *       sourceLoc_fun sourceLoc;
 *       accessKeyword_fun accessKeyword
 * 
 *   | MR_usingDecl(annot, sourceLoc, nd_usingDecl) -> 
 *       assert(match nd_usingDecl with ND_usingDecl _ -> true | _ -> false);
 *       sourceLoc_fun sourceLoc;
 *       namespaceDecl_fun nd_usingDecl
 * 
 *   | MR_template(annot, sourceLoc, templateDeclaration) -> 
 *       sourceLoc_fun sourceLoc;
 *       templateDeclaration_fun templateDeclaration
 * 
 *)

and declarator_fun(annot, iDeclarator, init_opt, 
		   statement_opt_ctor, statement_opt_dtor) =
  let id = id_annotation annot in
  let childs = 
      (iDeclarator_fun iDeclarator, "decl") :: 
	(opt_child init_fun "init" init_opt
	   (opt_child statement_fun "ctor" statement_opt_ctor
	      (opt_child statement_fun "dtor" statement_opt_dtor [])))
  in
    short_label "Declarator" color_Declarator annot;
    child_edges id childs;
    id
    
and statement_fun _ = 0
and init_fun _ = 0
and iDeclarator_fun _ = 0

(* 
 * 16 and iDeclarator_fun = function
 *   | D_name(annot, sourceLoc, pQName_opt) -> 
 *       sourceLoc_fun sourceLoc;
 *       opt_iter pQName_fun pQName_opt
 * 
 *   | D_pointer(annot, sourceLoc, cVFlags, iDeclarator) -> 
 *       sourceLoc_fun sourceLoc;
 *       cVFlags_fun cVFlags;
 *       iDeclarator_fun iDeclarator
 * 
 *   | D_reference(annot, sourceLoc, iDeclarator) -> 
 *       sourceLoc_fun sourceLoc;
 *       iDeclarator_fun iDeclarator
 * 
 *   | D_func(annot, sourceLoc, iDeclarator, aSTTypeId_list, cVFlags, 
 * 	   exceptionSpec_opt, pq_name_list) -> 
 *       assert(List.for_all (function | PQ_name _ -> true | _ -> false) 
 * 	       pq_name_list);
 *       sourceLoc_fun sourceLoc;
 *       iDeclarator_fun iDeclarator;
 *       List.iter aSTTypeId_fun aSTTypeId_list;
 *       cVFlags_fun cVFlags;
 *       opt_iter exceptionSpec_fun exceptionSpec_opt;
 *       List.iter pQName_fun pq_name_list
 * 
 *   | D_array(annot, sourceLoc, iDeclarator, expression_opt) -> 
 *       sourceLoc_fun sourceLoc;
 *       iDeclarator_fun iDeclarator;
 *       opt_iter expression_fun expression_opt
 * 
 *   | D_bitfield(annot, sourceLoc, pQName_opt, expression) -> 
 *       sourceLoc_fun sourceLoc;
 *       opt_iter pQName_fun pQName_opt;
 *       expression_fun expression
 * 
 *   | D_ptrToMember(annot, sourceLoc, pQName, cVFlags, iDeclarator) -> 
 *       sourceLoc_fun sourceLoc;
 *       pQName_fun pQName;
 *       cVFlags_fun cVFlags;
 *       iDeclarator_fun iDeclarator
 * 
 *   | D_grouping(annot, sourceLoc, iDeclarator) -> 
 *       sourceLoc_fun sourceLoc;
 *       iDeclarator_fun iDeclarator
 * 
 * 
 * 17 and exceptionSpec_fun(annot, aSTTypeId_list) =
 *   List.iter aSTTypeId_fun aSTTypeId_list
 * 
 * 
 * 18 and operatorName_fun = function
 *   | ON_newDel(annot, bool_is_new, bool_is_array) -> 
 *       bool_fun bool_is_new;
 *       bool_fun bool_is_array
 * 
 *   | ON_operator(annot, overloadableOp) -> 
 *       overloadableOp_fun overloadableOp
 * 
 *   | ON_conversion(annot, aSTTypeId) -> 
 *       aSTTypeId_fun aSTTypeId
 * 
 * 
 * 19 and statement_fun = function
 *   | S_skip(annot, sourceLoc) -> 
 *       sourceLoc_fun sourceLoc
 * 
 *   | S_label(annot, sourceLoc, stringRef, statement) -> 
 *       sourceLoc_fun sourceLoc;
 *       string_fun stringRef;
 *       statement_fun statement
 * 
 *   | S_case(annot, sourceLoc, expression, statement) -> 
 *       sourceLoc_fun sourceLoc;
 *       expression_fun expression;
 *       statement_fun statement
 * 
 *   | S_default(annot, sourceLoc, statement) -> 
 *       sourceLoc_fun sourceLoc;
 *       statement_fun statement
 * 
 *   | S_expr(annot, sourceLoc, fullExpression) -> 
 *       sourceLoc_fun sourceLoc;
 *       fullExpression_fun fullExpression
 * 
 *   | S_compound(annot, sourceLoc, statement_list) -> 
 *       sourceLoc_fun sourceLoc;
 *       List.iter statement_fun statement_list
 * 
 *   | S_if(annot, sourceLoc, condition, statement_then, statement_else) -> 
 *       sourceLoc_fun sourceLoc;
 *       condition_fun condition;
 *       statement_fun statement_then;
 *       statement_fun statement_else
 * 
 *   | S_switch(annot, sourceLoc, condition, statement) -> 
 *       sourceLoc_fun sourceLoc;
 *       condition_fun condition;
 *       statement_fun statement
 * 
 *   | S_while(annot, sourceLoc, condition, statement) -> 
 *       sourceLoc_fun sourceLoc;
 *       condition_fun condition;
 *       statement_fun statement
 * 
 *   | S_doWhile(annot, sourceLoc, statement, fullExpression) -> 
 *       sourceLoc_fun sourceLoc;
 *       statement_fun statement;
 *       fullExpression_fun fullExpression
 * 
 *   | S_for(annot, sourceLoc, statement_init, condition, fullExpression, 
 * 	  statement_body) -> 
 *       sourceLoc_fun sourceLoc;
 *       statement_fun statement_init;
 *       condition_fun condition;
 *       fullExpression_fun fullExpression;
 *       statement_fun statement_body
 * 
 *   | S_break(annot, sourceLoc) -> 
 *       sourceLoc_fun sourceLoc
 * 
 *   | S_continue(annot, sourceLoc) -> 
 *       sourceLoc_fun sourceLoc
 * 
 *   | S_return(annot, sourceLoc, fullExpression_opt, statement_opt) -> 
 *       sourceLoc_fun sourceLoc;
 *       opt_iter fullExpression_fun fullExpression_opt;
 *       opt_iter statement_fun statement_opt
 * 
 *   | S_goto(annot, sourceLoc, stringRef) -> 
 *       sourceLoc_fun sourceLoc;
 *       string_fun stringRef
 * 
 *   | S_decl(annot, sourceLoc, declaration) -> 
 *       sourceLoc_fun sourceLoc;
 *       declaration_fun declaration
 * 
 *   | S_try(annot, sourceLoc, statement, handler_list) -> 
 *       sourceLoc_fun sourceLoc;
 *       statement_fun statement;
 *       List.iter handler_fun handler_list
 * 
 *   | S_asm(annot, sourceLoc, e_stringLit) -> 
 *       assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
 *       sourceLoc_fun sourceLoc;
 *       expression_fun e_stringLit
 * 
 *   | S_namespaceDecl(annot, sourceLoc, namespaceDecl) -> 
 *       sourceLoc_fun sourceLoc;
 *       namespaceDecl_fun namespaceDecl
 * 
 *   | S_function(annot, sourceLoc, func) -> 
 *       sourceLoc_fun sourceLoc;
 *       func_fun func
 * 
 *   | S_rangeCase(annot, sourceLoc, expression_lo, expression_hi, statement) -> 
 *       sourceLoc_fun sourceLoc;
 *       expression_fun expression_lo;
 *       expression_fun expression_hi;
 *       statement_fun statement
 * 
 *   | S_computedGoto(annot, sourceLoc, expression) -> 
 *       sourceLoc_fun sourceLoc;
 *       expression_fun expression
 * 
 * 
 * 20 and condition_fun = function
 *   | CN_expr(annot, fullExpression) -> 
 *       fullExpression_fun fullExpression
 * 
 *   | CN_decl(annot, aSTTypeId) -> 
 *       aSTTypeId_fun aSTTypeId
 * 
 * 
 * 21 and handler_fun(annot, aSTTypeId, statement_body, 
 * 		expression_opt, statement_gdtor) =
 *   aSTTypeId_fun aSTTypeId;
 *   statement_fun statement_body;
 *   opt_iter expression_fun expression_opt;
 *   opt_iter statement_fun statement_gdtor
 * 
 * 
 * 22 and expression_fun = function
 *   | E_boolLit(annot, bool) -> 
 *       bool_fun bool
 * 
 *   | E_intLit(annot, stringRef) -> 
 *       string_fun stringRef
 * 
 *   | E_floatLit(annot, stringRef) -> 
 *       string_fun stringRef
 * 
 *   | E_stringLit(annot, stringRef, e_stringLit_opt) -> 
 *       assert(match e_stringLit_opt with 
 * 	       | Some(E_stringLit _) -> true 
 * 	       | None -> true
 * 	       | _ -> false);
 *       string_fun stringRef;
 *       opt_iter expression_fun e_stringLit_opt
 * 
 *   | E_charLit(annot, stringRef) -> 
 *       string_fun stringRef
 * 
 *   | E_this annot -> 
 * 
 *   | E_variable(annot, pQName) -> 
 *       pQName_fun pQName
 * 
 *   | E_funCall(annot, expression_func, argExpression_list, expression_retobj_opt) -> 
 *       expression_fun expression_func;
 *       List.iter argExpression_fun argExpression_list;
 *       opt_iter expression_fun expression_retobj_opt
 * 
 *   | E_constructor(annot, typeSpecifier, argExpression_list, bool, expression_opt) -> 
 *       typeSpecifier_fun typeSpecifier;
 *       List.iter argExpression_fun argExpression_list;
 *       bool_fun bool;
 *       opt_iter expression_fun expression_opt
 * 
 *   | E_fieldAcc(annot, expression, pQName) -> 
 *       expression_fun expression;
 *       pQName_fun pQName
 * 
 *   | E_sizeof(annot, expression) -> 
 *       expression_fun expression
 * 
 *   | E_unary(annot, unaryOp, expression) -> 
 *       unaryOp_fun unaryOp;
 *       expression_fun expression
 * 
 *   | E_effect(annot, effectOp, expression) -> 
 *       effectOp_fun effectOp;
 *       expression_fun expression
 * 
 *   | E_binary(annot, expression_left, binaryOp, expression_right) -> 
 *       expression_fun expression_left;
 *       binaryOp_fun binaryOp;
 *       expression_fun expression_right
 * 
 *   | E_addrOf(annot, expression) -> 
 *       expression_fun expression
 * 
 *   | E_deref(annot, expression) -> 
 *       expression_fun expression
 * 
 *   | E_cast(annot, aSTTypeId, expression) -> 
 *       aSTTypeId_fun aSTTypeId;
 *       expression_fun expression
 * 
 *   | E_cond(annot, expression_cond, expression_true, expression_false) -> 
 *       expression_fun expression_cond;
 *       expression_fun expression_true;
 *       expression_fun expression_false
 * 
 *   | E_sizeofType(annot, aSTTypeId) -> 
 *       aSTTypeId_fun aSTTypeId
 * 
 *   | E_assign(annot, expression_target, binaryOp, expression_src) -> 
 *       expression_fun expression_target;
 *       binaryOp_fun binaryOp;
 *       expression_fun expression_src
 * 
 *   | E_new(annot, bool, argExpression_list, aSTTypeId, argExpressionListOpt_opt,
 * 	  statement_opt) -> 
 *       bool_fun bool;
 *       List.iter argExpression_fun argExpression_list;
 *       aSTTypeId_fun aSTTypeId;
 *       opt_iter argExpressionListOpt_fun argExpressionListOpt_opt;
 *       opt_iter statement_fun statement_opt
 * 
 *   | E_delete(annot, bool_colon, bool_array, expression_opt, statement_opt) -> 
 *       bool_fun bool_colon;
 *       bool_fun bool_array;
 *       opt_iter expression_fun expression_opt;
 *       opt_iter statement_fun statement_opt
 * 
 *   | E_throw(annot, expression_opt, statement_opt) -> 
 *       opt_iter expression_fun expression_opt;
 *       opt_iter statement_fun statement_opt
 * 
 *   | E_keywordCast(annot, castKeyword, aSTTypeId, expression) -> 
 *       castKeyword_fun castKeyword;
 *       aSTTypeId_fun aSTTypeId;
 *       expression_fun expression
 * 
 *   | E_typeidExpr(annot, expression) -> 
 *       expression_fun expression
 * 
 *   | E_typeidType(annot, aSTTypeId) -> 
 *       aSTTypeId_fun aSTTypeId
 * 
 *   | E_grouping(annot, expression) -> 
 *       expression_fun expression
 * 
 *   | E_arrow(annot, expression, pQName) -> 
 *       expression_fun expression;
 *       pQName_fun pQName
 * 
 *   | E_statement(annot, s_compound) -> 
 *       assert(match s_compound with | S_compound _ -> true | _ -> false);
 *       statement_fun s_compound
 * 
 *   | E_compoundLit(annot, aSTTypeId, in_compound) -> 
 *       assert(match in_compound with | IN_compound _ -> true | _ -> false);
 *       aSTTypeId_fun aSTTypeId;
 *       init_fun in_compound
 * 
 *   | E___builtin_constant_p(annot, sourceLoc, expression) -> 
 *       sourceLoc_fun sourceLoc;
 *       expression_fun expression
 * 
 *   | E___builtin_va_arg(annot, sourceLoc, expression, aSTTypeId) -> 
 *       sourceLoc_fun sourceLoc;
 *       expression_fun expression;
 *       aSTTypeId_fun aSTTypeId
 * 
 *   | E_alignofType(annot, aSTTypeId) -> 
 *       aSTTypeId_fun aSTTypeId
 * 
 *   | E_alignofExpr(annot, expression) -> 
 *       expression_fun expression
 * 
 *   | E_gnuCond(annot, expression_cond, expression_false) -> 
 *       expression_fun expression_cond;
 *       expression_fun expression_false
 * 
 *   | E_addrOfLabel(annot, stringRef) -> 
 *       string_fun stringRef
 * 
 * 
 * 23 and fullExpression_fun(annot, expression_opt) =
 *   opt_iter expression_fun expression_opt
 * 
 * 
 * 24 and argExpression_fun(annot, expression) =
 *   expression_fun expression
 * 
 * 
 * 25 and argExpressionListOpt_fun(annot, argExpression_list) =
 *   List.iter argExpression_fun argExpression_list
 * 
 * 
 * 26 and init_fun = function
 *   | IN_expr(annot, sourceLoc, expression) -> 
 *       sourceLoc_fun sourceLoc;
 *       expression_fun expression
 * 
 *   | IN_compound(annot, sourceLoc, init_list) -> 
 *       sourceLoc_fun sourceLoc;
 *       List.iter init_fun init_list
 * 
 *   | IN_ctor(annot, sourceLoc, argExpression_list, bool) -> 
 *       sourceLoc_fun sourceLoc;
 *       List.iter argExpression_fun argExpression_list;
 *       bool_fun bool
 * 
 *   | IN_designated(annot, sourceLoc, designator_list, init) -> 
 *       sourceLoc_fun sourceLoc;
 *       List.iter designator_fun designator_list;
 *       init_fun init
 * 
 * 
 * 27 and templateDeclaration_fun = function
 *   | TD_func(annot, templateParameter_opt, func) -> 
 *       opt_iter templateParameter_fun templateParameter_opt;
 *       func_fun func
 * 
 *   | TD_decl(annot, templateParameter_opt, declaration) -> 
 *       opt_iter templateParameter_fun templateParameter_opt;
 *       declaration_fun declaration
 * 
 *   | TD_tmember(annot, templateParameter_opt, templateDeclaration) -> 
 *       opt_iter templateParameter_fun templateParameter_opt;
 *       templateDeclaration_fun templateDeclaration
 * 
 * 
 * 28 and templateParameter_fun = function
 *   | TP_type(annot, sourceLoc, stringRef, aSTTypeId_opt, templateParameter_opt) -> 
 *       sourceLoc_fun sourceLoc;
 *       string_fun stringRef;
 *       opt_iter aSTTypeId_fun aSTTypeId_opt;
 *       opt_iter templateParameter_fun templateParameter_opt
 * 
 *   | TP_nontype(annot, sourceLoc, aSTTypeId, templateParameter_opt) -> 
 *       sourceLoc_fun sourceLoc;
 *       aSTTypeId_fun aSTTypeId;
 *       opt_iter templateParameter_fun templateParameter_opt
 * 
 * 
 * 29 and templateArgument_fun = function
 *   | TA_type(annot, aSTTypeId, templateArgument_opt) -> 
 *       aSTTypeId_fun aSTTypeId;
 *       opt_iter templateArgument_fun templateArgument_opt
 * 
 *   | TA_nontype(annot, expression, templateArgument_opt) -> 
 *       expression_fun expression;
 *       opt_iter templateArgument_fun templateArgument_opt
 * 
 *   | TA_templateUsed(annot, templateArgument_opt) -> 
 *       opt_iter templateArgument_fun templateArgument_opt
 * 
 * 
 * 30 and namespaceDecl_fun = function
 *   | ND_alias(annot, stringRef, pQName) -> 
 *       string_fun stringRef;
 *       pQName_fun pQName
 * 
 *   | ND_usingDecl(annot, pQName) -> 
 *       pQName_fun pQName
 * 
 *   | ND_usingDir(annot, pQName) -> 
 *       pQName_fun pQName
 * 
 * 
 * 31 and fullExpressionAnnot_fun(declaration_list) =
 *     List.iter declaration_fun declaration_list
 * 
 * 
 * 32 and aSTTypeof_fun = function
 *   | TS_typeof_expr(annot, fullExpression) -> 
 *       fullExpression_fun fullExpression
 * 
 *   | TS_typeof_type(annot, aSTTypeId) -> 
 *       aSTTypeId_fun aSTTypeId
 * 
 * 
 * 33 and designator_fun = function
 *   | FieldDesignator(annot, sourceLoc, stringRef) -> 
 *       sourceLoc_fun sourceLoc;
 *       string_fun stringRef
 * 
 *   | SubscriptDesignator(annot, sourceLoc, expression, expression_opt) -> 
 *       sourceLoc_fun sourceLoc;
 *       expression_fun expression;
 *       opt_iter expression_fun expression_opt
 * 
 * 
 * 34 and attributeSpecifierList_fun = function
 *   | AttributeSpecifierList_cons(annot, attributeSpecifier, 
 * 				attributeSpecifierList) -> 
 *       attributeSpecifier_fun attributeSpecifier;
 *       attributeSpecifierList_fun 
 * 	attributeSpecifierList
 * 
 * 
 * 35 and attributeSpecifier_fun = function
 *   | AttributeSpecifier_cons(annot, attribute, attributeSpecifier) -> 
 *       attribute_fun attribute;
 *       attributeSpecifier_fun attributeSpecifier
 * 
 * 
 * 36 and attribute_fun = function
 *   | AT_empty(annot, sourceLoc) -> 
 *       sourceLoc_fun sourceLoc
 * 
 *   | AT_word(annot, sourceLoc, stringRef) -> 
 *       sourceLoc_fun sourceLoc;
 *       string_fun stringRef
 * 
 *   | AT_func(annot, sourceLoc, stringRef, argExpression_list) -> 
 *       sourceLoc_fun sourceLoc;
 *       string_fun stringRef;
 *       List.iter argExpression_fun argExpression_list
 *)


(**************************************************************************
 *
 * end of astiter.ml 
 *
 **************************************************************************)


let out_file = ref ""


let arguments = Arg.align
  [
    ("-o", Arg.Set_string out_file,
     "file set output file name");
  ]

let usage_msg = 
  "usage: ast_graph [options...] <file>\n\
   recognized options are:"

let usage () =
  prerr_endline usage_msg;
  exit(1)
  
let file = ref ""

let file_set = ref false

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

let start_file infile =
  output_string !oc "digraph ";
  Printf.fprintf !oc "\"%s\"" infile;
  output_string !oc " {\n";
  output_string !oc 
    "    color=white    node [ color = grey95, style = filled ]\n"

let finish_file () =
  output_string !oc "}\n"

let main () =
  Arg.parse arguments anonfun usage_msg;
  if not !file_set then
    usage();				(* does not return *)
  let ic = open_in !file in
  let ofile = 
    if !out_file <> "" 
    then !out_file
    else "nodes.dot"
  in
  let _ = oc := open_out (ofile) in
  let ast = (Marshal.from_channel ic : annotated translationUnit_type)
  in
    start_file !file;
    ignore(translationUnit_fun ast);
    finish_file ()
      
;;


Printexc.catch main ()




(*** Local Variables: ***)
(*** compile-command: "ocamlc.opt -I ../elsa ast_annotation.cmo cc_ml_types.cmo ast_graph.ml" ***)
(*** End: ***)
