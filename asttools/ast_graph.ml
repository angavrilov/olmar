(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* generate a dot graph from the ast
 *
 * to make use of it you need the graphviz package (http://www.graphviz.org/)
 * 
 * to visualize the graph 
 * - zgrviewer on the dot file (http://zvtm.sourceforge.net/zgrviewer.html)
 * - run dot -Tps nodes.dot > nodes.ps
 *        if ghostview cannot allocate a bitmap big enough to hold 
 *        the graph you can experiment with the -size option or change
 *        the dot file to get multiple pages
 * - run dot -Tfig nodes.dot > nodes.fig  and use xfig
 *        works surprisingly good, even for the biggest graphs
 * - consult the dot documentation for other options (jpeg for instance)
 *)          

open Cc_ml_types
open Cc_ast_gen_type
open Ml_ctype
open Ast_annotation
open Ast_util




(* node type done: 1 - 30, 32, 33, 36 - 40
   not done: 31, 34, 35,
 *)

let oc = ref stdout;;

let print_caddr = ref false

module DS = Dense_set

let visited_nodes = DS.make ()

let visited (annot : annotated) =
  DS.mem (id_annotation annot) visited_nodes

let visit (annot : annotated) =
  (* Printf.eprintf "visit %d\n%!" (id_annotation annot); *)
  DS.add (id_annotation annot) visited_nodes


let not_implemented () =
  if 1 = 0 then 0 else assert false

let dot_escape s =
  let b = Buffer.create (max 31 (String.length s))
  in
    for i = 0 to String.length s -1 do
      match s.[i] with
	| '\\' -> Buffer.add_string b "\\\\"
	| '"' -> Buffer.add_string b "\\\""
	| '\n' -> Buffer.add_string b "\\n"
	| c -> Buffer.add_char b c
    done;
    Buffer.contents b

let any_label (field_name, value) =
  Printf.fprintf !oc "\\n%s: %s" field_name (dot_escape value)

let string_opt = function
  | None -> "(nil)"
  | Some s -> s

let count_rev base l =
  let counter = ref (List.length l)
  in
    List.rev_map 
      (fun x -> decr counter; (x, Printf.sprintf "%s[%d]" base !counter)) 
      l

let nodes_counter = ref 0

let start_label name color id =
  incr nodes_counter;
  Printf.fprintf !oc "    \"%d\" [color=\"%s\", label=\"%s %d" 
    id color name id

let finish_label caddr =
  output_string !oc "\"];\n"


let loc_label (file, line, char) =
  ("loc", Printf.sprintf "%s:%d:%d" file line char)

let edge_counter = ref 0

let child_edge id (cid,label) =
  incr edge_counter;
  Printf.fprintf !oc "    \"%d\" -> \"%d\" [label=\"%s\"];\n" id cid label

let child_edges id childs = 
  List.iter (child_edge id) childs

  
let opt_child child_fun field_name opt = 
  match opt with
    | None -> []
    | Some c -> [(child_fun c, field_name)]

let caddr_label caddr =
  ("caddr", Printf.sprintf "0x%lx" (Int32.shift_left (Int32.of_int caddr) 1))

let ast_node color annot name (labels :(string*string) list) childs =
  let id = id_annotation annot
  in
    start_label name color id;
    List.iter any_label 
      (if !print_caddr then
	 labels @ [(caddr_label (caddr_annotation annot))]
       else
	 labels);
    finish_label ();
    child_edges id childs;
    (* the return value must be the same as retval annot: *)
    (* assert(id = retval annot); *)
    id


let retval annot = id_annotation annot


let ast_loc_node color annot loc name labels childs =
  ast_node color annot name ((loc_label loc) :: labels) childs


(***************** colors *****************************)

let color_TranslationUnit = "red"
let color_TF = "firebrick2"
let color_Declaration = "cyan"
let color_Declarator = "SkyBlue"
let color_Function = "magenta"
let color_MemberInit = "HotPink"
let color_IDeclarator = "SteelBlue"
let color_Member = "SlateBlue1"
let color_MemberList = "SlateBlue4"
let color_PQName = "SkyBlue"
let color_Variable = "purple"
let color_TypeSpecifier = "OliveDrab"
let color_ASTTypeof = "SeaGreen"
let color_Statement = "yellow"
let color_handler = "khaki1"
let color_FullExpression = "coral"
let color_Expression = "orange"
let color_ArgExpression = "DarkOrange"
let color_ArgExpressionListOpt = "tomato"
let color_Condition = "OrangeRed"
let color_OperatorName = "chocolate1"
let color_Initializer = "gold"
let color_ASTTypeID = "MediumAquamarine"
let color_Enumerator = "PaleTurquoise"
let color_CType = "green"
let color_ATomicType = "LimeGreen"
let color_Compound_info = "LawnGreen"
let color_Enum_Value = "OliveDrab1"
let color_BaseClassSpec = "PaleGreen"
let color_BaseClass = "SpringGreen"
let color_exceptionSpec = "salmon"
let color_TemplateDeclaration = "brown3"
let color_TemplateParameter = "tan3"
let color_TemplateArgument = "peru"
let color_STemplateArgument = "maroon"
let color_NamespaceDecl = "grey50"
let color_Designator = "LemonChiffon"
let color_Attribute = "orchid1"

(**************************************************************************
 *
 * contents of astiternodes.ml
 *
 **************************************************************************)

let opt_iter f = function
  | None -> ()
  | Some x -> f x

(***************** variable ***************************)

let rec variable_fun(v : annotated variable) =
  let annot = variable_annotation v
  in
    if visited annot then ()
    else begin
      visit annot;


      (* POSSIBLY CIRCULAR *)
      opt_iter cType_fun !(v.var_type);
      (* POSSIBLY CIRCULAR *)
      opt_iter expression_fun !(v.value);
      opt_iter cType_fun v.defaultParam;

      (* POSSIBLY CIRCULAR *)
      opt_iter func_fun !(v.funcDefn)
    end

(***************** cType ******************************)

and baseClass_fun baseClass =
  let annot = baseClass_annotation baseClass
  in
    if visited annot then ()
    else begin
	visit annot;
      compound_info_fun baseClass.compound;
    end


and compound_info_fun info = 
  let annot = compound_info_annotation info
  in
    if visited annot then ()
    else begin
	visit annot;
      variable_fun info.typedef_var;
      List.iter variable_fun info.data_members;
      List.iter baseClass_fun info.bases;
      List.iter variable_fun info.conversion_operators;
      List.iter variable_fun info.friends;

      (* POSSIBLY CIRCULAR *)
      opt_iter cType_fun !(info.self_type)
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

      | CompoundType(compound_info) ->
	  compound_info_fun compound_info

      | PseudoInstantiation(annot, str, variable_opt, accessKeyword, 
			    compound_info, sTemplateArgument_list) ->
	  visit annot;
	  opt_iter variable_fun variable_opt;
	  compound_info_fun compound_info;
	  List.iter sTemplateArgument_fun sTemplateArgument_list

      | EnumType(annot, string, variable, accessKeyword, 
		 string_nativeint_list) ->
	  visit annot;
	  opt_iter variable_fun variable;

      | TypeVariable(annot, string, variable, accessKeyword) ->
	  visit annot;
	  variable_fun variable;

      | DependentQType(annot, string, variable, 
		      accessKeyword, atomic, pq_name) ->
	  visit annot;
	  variable_fun variable;
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
	  atomicType_fun atomicType

      | PointerType(annot, cVFlags, cType) ->
	  cType_fun cType

      | ReferenceType(annot, cType) ->
	  cType_fun cType

      | FunctionType(annot, function_flags, cType, 
		     variable_list, cType_list_opt) ->
	  cType_fun cType;
	  List.iter variable_fun variable_list;
	  opt_iter (List.iter cType_fun) cType_list_opt

      | ArrayType(annot, cType, array_size) ->
	  cType_fun cType;

      | PointerToMemberType(annot, atomicType (* = NamedAtomicType *), 
			    cVFlags, cType) ->
	  assert(match atomicType with 
		   | SimpleType _ -> false
		   | CompoundType _
		   | PseudoInstantiation _
		   | EnumType _
		   | TypeVariable _ 
		   | DependentQType _ -> true);
	  atomicType_fun atomicType;
	  cType_fun cType


and sTemplateArgument_fun ta = 
  let annot = sTemplateArgument_annotation ta
  in
    if visited annot then ()
    else 
      let _ = visit annot 
      in match ta with
	| STA_NONE annot -> 
	    ()

	| STA_TYPE(annot, cType) -> 
	    cType_fun cType

	| STA_INT(annot, int) -> 
	    ()

	| STA_ENUMERATOR(annot, variable) -> 
	    variable_fun variable

	| STA_REFERENCE(annot, variable) -> 
	    variable_fun variable

	| STA_POINTER(annot, variable) -> 
	    variable_fun variable

	| STA_MEMBER(annot, variable) -> 
	    variable_fun variable

	| STA_DEPEXPR(annot, expression) -> 
	    expression_fun expression

	| STA_TEMPLATE annot -> 
	    ()

	| STA_ATOMIC(annot, atomicType) -> 
	    atomicType_fun atomicType



(***************** generated ast nodes ****************)

and translationUnit_fun 
    ((annot, topForm_list)  : annotated translationUnit_type) =
  if visited annot then ()
  else begin
    visit annot;
    List.iter topForm_fun topForm_list
  end


and topForm_fun x = 
  let annot = topForm_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| TF_decl(annot, sourceLoc, declaration) -> 
	    declaration_fun declaration

	| TF_func(annot, sourceLoc, func) -> 
	    func_fun func

	| TF_template(annot, sourceLoc, templateDeclaration) -> 
	    templateDeclaration_fun templateDeclaration

	| TF_explicitInst(annot, sourceLoc, declFlags, declaration) -> 
	    declaration_fun declaration

	| TF_linkage(annot, sourceLoc, stringRef, translationUnit) -> 
	    translationUnit_fun translationUnit

	| TF_one_linkage(annot, sourceLoc, stringRef, topForm) -> 
	    topForm_fun topForm

	| TF_asm(annot, sourceLoc, e_stringLit) -> 
	    assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
	    expression_fun e_stringLit

	| TF_namespaceDefn(annot, sourceLoc, stringRef_opt, topForm_list) -> 
	    List.iter topForm_fun topForm_list

	| TF_namespaceDecl(annot, sourceLoc, namespaceDecl) -> 
	    namespaceDecl_fun namespaceDecl



and func_fun((annot, declFlags, typeSpecifier, declarator, memberInit_list, 
	     s_compound_opt, handler_list, func, variable_opt_1, 
	     variable_opt_2, statement_opt, bool) ) =

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
    cType_fun func;
    visit annot;
    typeSpecifier_fun typeSpecifier;
    declarator_fun declarator;
    List.iter memberInit_fun memberInit_list;
    opt_iter statement_fun s_compound_opt;
    List.iter handler_fun handler_list;
    cType_fun func;
    opt_iter variable_fun variable_opt_1;
    opt_iter variable_fun variable_opt_2;
    opt_iter statement_fun statement_opt;
  end


and memberInit_fun((annot, pQName, argExpression_list, 
		   variable_opt_1, compound_opt, variable_opt_2, 
		   full_expr_annot, statement_opt) ) =

  if visited annot then ()
  else begin
      assert(match compound_opt with
	| None
	| Some(CompoundType _) -> true
	| _ -> false);
    visit annot;
    pQName_fun pQName;
    List.iter argExpression_fun argExpression_list;
    opt_iter variable_fun variable_opt_1;
    opt_iter atomicType_fun compound_opt;
    opt_iter variable_fun variable_opt_2;
    fullExpressionAnnot_fun full_expr_annot;
    opt_iter statement_fun statement_opt
  end


and declaration_fun((annot, declFlags, typeSpecifier, declarator_list) ) =
  if visited annot then ()
  else begin
    visit annot;
    typeSpecifier_fun typeSpecifier;
    List.iter declarator_fun declarator_list
  end


and aSTTypeId_fun((annot, typeSpecifier, declarator) ) =
  if visited annot then ()
  else begin
    visit annot;
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
	    opt_iter templateArgument_fun templateArgument_opt;
	    pQName_fun pQName;
	    opt_iter variable_fun variable_opt;
	    List.iter sTemplateArgument_fun s_template_arg_list

	| PQ_name(annot, sourceLoc, stringRef) -> 
	    ()

	| PQ_operator(annot, sourceLoc, operatorName, stringRef) -> 
	    operatorName_fun operatorName;

	| PQ_template(annot, sourceLoc, stringRef, templateArgument_opt, 
		     s_template_arg_list) -> 
	    opt_iter templateArgument_fun templateArgument_opt;
	    List.iter sTemplateArgument_fun s_template_arg_list

	| PQ_variable(annot, sourceLoc, variable) -> 
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
	    pQName_fun pQName;
	    opt_iter variable_fun var_opt_1;
	    opt_iter variable_fun var_opt_2

	| TS_simple(annot, sourceLoc, cVFlags, simpleTypeId) -> 
	    ()

	| TS_elaborated(annot, sourceLoc, cVFlags, typeIntr, 
		       pQName, namedAtomicType_opt) -> 
	    assert(match namedAtomicType_opt with
	      | Some(SimpleType _) -> false
	      | _ -> true);
	    pQName_fun pQName;
	    opt_iter atomicType_fun namedAtomicType_opt

	| TS_classSpec(annot, sourceLoc, cVFlags, typeIntr, pQName_opt, 
		       baseClassSpec_list, memberList, compoundType) -> 
	    assert(match compoundType with
	      | CompoundType _ -> true
	      | _ -> false);
	    opt_iter pQName_fun pQName_opt;
	    List.iter baseClassSpec_fun baseClassSpec_list;
	    memberList_fun memberList;
	    atomicType_fun compoundType

	| TS_enumSpec(annot, sourceLoc, cVFlags, 
		      stringRef_opt, enumerator_list, enumType) -> 
	    assert(match enumType with 
	      | EnumType _ -> true
	      | _ -> false);
	    List.iter enumerator_fun enumerator_list;
	    atomicType_fun enumType

	| TS_type(annot, sourceLoc, cVFlags, cType) -> 
	    cType_fun cType

	| TS_typeof(annot, sourceLoc, cVFlags, aSTTypeof) -> 
	    aSTTypeof_fun aSTTypeof


and baseClassSpec_fun
    ((annot, bool, accessKeyword, pQName, compoundType_opt) ) =
  if visited annot then ()
  else begin
      assert(match compoundType_opt with
	| Some(CompoundType _ ) -> true
	| _ -> false);
    visit annot;
    pQName_fun pQName;
    opt_iter atomicType_fun compoundType_opt
  end


and enumerator_fun((annot, sourceLoc, stringRef, 
		   expression_opt, variable, int32) ) =
  if visited annot then ()
  else begin
    visit annot;
    opt_iter expression_fun expression_opt;
    variable_fun variable;
  end


and memberList_fun((annot, member_list) ) =
  if visited annot then ()
  else begin
    visit annot;
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
	    declaration_fun declaration

	| MR_func(annot, sourceLoc, func) -> 
	    func_fun func

	| MR_access(annot, sourceLoc, accessKeyword) -> 
	    ()

	| MR_usingDecl(annot, sourceLoc, nd_usingDecl) -> 
	    assert(match nd_usingDecl with ND_usingDecl _ -> true | _ -> false);
	    namespaceDecl_fun nd_usingDecl

	| MR_template(annot, sourceLoc, templateDeclaration) -> 
	    templateDeclaration_fun templateDeclaration


and declarator_fun((annot, iDeclarator, init_opt, 
		   variable_opt, ctype_opt, declaratorContext,
		   statement_opt_ctor, statement_opt_dtor) ) =
  if visited annot then ()
  else begin
    visit annot;
    iDeclarator_fun iDeclarator;
    opt_iter init_fun init_opt;
    opt_iter variable_fun variable_opt;
    opt_iter cType_fun ctype_opt;
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
	    opt_iter pQName_fun pQName_opt

	| D_pointer(annot, sourceLoc, cVFlags, iDeclarator) -> 
	    iDeclarator_fun iDeclarator

	| D_reference(annot, sourceLoc, iDeclarator) -> 
	    iDeclarator_fun iDeclarator

	| D_func(annot, sourceLoc, iDeclarator, aSTTypeId_list, cVFlags, 
		 exceptionSpec_opt, pq_name_list, bool) -> 
	    assert(List.for_all (function | PQ_name _ -> true | _ -> false) 
		     pq_name_list);
	    iDeclarator_fun iDeclarator;
	    List.iter aSTTypeId_fun aSTTypeId_list;
	    opt_iter exceptionSpec_fun exceptionSpec_opt;
	    List.iter pQName_fun pq_name_list;

	| D_array(annot, sourceLoc, iDeclarator, expression_opt, bool) -> 
	    iDeclarator_fun iDeclarator;
	    opt_iter expression_fun expression_opt;

	| D_bitfield(annot, sourceLoc, pQName_opt, expression, int) -> 
	    opt_iter pQName_fun pQName_opt;
	    expression_fun expression;

	| D_ptrToMember(annot, sourceLoc, pQName, cVFlags, iDeclarator) -> 
	    pQName_fun pQName;
	    iDeclarator_fun iDeclarator

	| D_grouping(annot, sourceLoc, iDeclarator) -> 
	    iDeclarator_fun iDeclarator

	| D_attribute(annot, sourceLoc, iDeclarator, attribute_list_list) ->
	    iDeclarator_fun iDeclarator;
	    List.iter (List.iter attribute_fun) attribute_list_list



and exceptionSpec_fun((annot, aSTTypeId_list) ) =
  if visited annot then ()
  else begin
    visit annot;
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
	    ()

	| ON_operator(annot, overloadableOp) -> 
	    ()

	| ON_conversion(annot, aSTTypeId) -> 
	    aSTTypeId_fun aSTTypeId


and statement_fun x = 
  let annot = statement_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| S_skip(annot, sourceLoc) -> 
	    ()

	| S_label(annot, sourceLoc, stringRef, statement) -> 
	    statement_fun statement

	| S_case(annot, sourceLoc, expression, statement, int) -> 
	    expression_fun expression;
	    statement_fun statement;

	| S_default(annot, sourceLoc, statement) -> 
	    statement_fun statement

	| S_expr(annot, sourceLoc, fullExpression) -> 
	    fullExpression_fun fullExpression

	| S_compound(annot, sourceLoc, statement_list) -> 
	    List.iter statement_fun statement_list

	| S_if(annot, sourceLoc, condition, statement_then, statement_else) -> 
	    condition_fun condition;
	    statement_fun statement_then;
	    statement_fun statement_else

	| S_switch(annot, sourceLoc, condition, statement) -> 
	    condition_fun condition;
	    statement_fun statement

	| S_while(annot, sourceLoc, condition, statement) -> 
	    condition_fun condition;
	    statement_fun statement

	| S_doWhile(annot, sourceLoc, statement, fullExpression) -> 
	    statement_fun statement;
	    fullExpression_fun fullExpression

	| S_for(annot, sourceLoc, statement_init, condition, fullExpression, 
		statement_body) -> 
	    statement_fun statement_init;
	    condition_fun condition;
	    fullExpression_fun fullExpression;
	    statement_fun statement_body

	| S_break(annot, sourceLoc) -> 
	    ()

	| S_continue(annot, sourceLoc) -> 
	    ()

	| S_return(annot, sourceLoc, fullExpression_opt, statement_opt) -> 
	    opt_iter fullExpression_fun fullExpression_opt;
	    opt_iter statement_fun statement_opt

	| S_goto(annot, sourceLoc, stringRef) -> 
	    ()

	| S_decl(annot, sourceLoc, declaration) -> 
	    declaration_fun declaration

	| S_try(annot, sourceLoc, statement, handler_list) -> 
	    statement_fun statement;
	    List.iter handler_fun handler_list

	| S_asm(annot, sourceLoc, e_stringLit) -> 
	    assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
	    expression_fun e_stringLit

	| S_namespaceDecl(annot, sourceLoc, namespaceDecl) -> 
	    namespaceDecl_fun namespaceDecl

	| S_function(annot, sourceLoc, func) -> 
	    func_fun func

	| S_rangeCase(annot, sourceLoc, 
		      expression_lo, expression_hi, statement, 
		     label_lo, label_hi) -> 
	    expression_fun expression_lo;
	    expression_fun expression_hi;
	    statement_fun statement;

	| S_computedGoto(annot, sourceLoc, expression) -> 
	    expression_fun expression


and condition_fun x = 
  let annot = condition_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| CN_expr(annot, fullExpression) -> 
	    fullExpression_fun fullExpression

	| CN_decl(annot, aSTTypeId) -> 
	    aSTTypeId_fun aSTTypeId


and handler_fun((annot, aSTTypeId, statement_body, variable_opt, 
		fullExpressionAnnot, expression_opt, statement_gdtor) ) =
  if visited annot then ()
  else begin
    visit annot;
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
	    opt_iter cType_fun type_opt;

	| E_intLit(annot, type_opt, stringRef, ulong) -> 
	    opt_iter cType_fun type_opt;

	| E_floatLit(annot, type_opt, stringRef, double) -> 
	    opt_iter cType_fun type_opt;

	| E_stringLit(annot, type_opt, stringRef, e_stringLit_opt) -> 
	    assert(match e_stringLit_opt with 
		     | Some(E_stringLit _) -> true 
		     | None -> true
		     | _ -> false);
	    opt_iter cType_fun type_opt;
	    opt_iter expression_fun e_stringLit_opt

	| E_charLit(annot, type_opt, stringRef, int32) -> 
	    opt_iter cType_fun type_opt;

	| E_this(annot, type_opt, variable) -> 
	    opt_iter cType_fun type_opt;
	    variable_fun variable

	| E_variable(annot, type_opt, pQName, var_opt, nondep_var_opt) -> 
	    opt_iter cType_fun type_opt;
	    pQName_fun pQName;
	    opt_iter variable_fun var_opt;
	    opt_iter variable_fun nondep_var_opt

	| E_funCall(annot, type_opt, expression_func, 
		    argExpression_list, expression_retobj_opt) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression_func;
	    List.iter argExpression_fun argExpression_list;
	    opt_iter expression_fun expression_retobj_opt

	| E_constructor(annot, type_opt, typeSpecifier, argExpression_list, 
			var_opt, bool, expression_opt) -> 
	    opt_iter cType_fun type_opt;
	    typeSpecifier_fun typeSpecifier;
	    List.iter argExpression_fun argExpression_list;
	    opt_iter variable_fun var_opt;
	    opt_iter expression_fun expression_opt

	| E_fieldAcc(annot, type_opt, expression, pQName, var_opt) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression;
	    pQName_fun pQName;
	    opt_iter variable_fun var_opt

	| E_sizeof(annot, type_opt, expression, int) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression;

	| E_unary(annot, type_opt, unaryOp, expression) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression

	| E_effect(annot, type_opt, effectOp, expression) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression

	| E_binary(annot, type_opt, expression_left, binaryOp, expression_right) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression_left;
	    expression_fun expression_right

	| E_addrOf(annot, type_opt, expression) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression

	| E_deref(annot, type_opt, expression) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression

	| E_cast(annot, type_opt, aSTTypeId, expression, bool) -> 
	    opt_iter cType_fun type_opt;
	    aSTTypeId_fun aSTTypeId;
	    expression_fun expression;

	| E_cond(annot, type_opt, expression_cond, expression_true, expression_false) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression_cond;
	    expression_fun expression_true;
	    expression_fun expression_false

	| E_sizeofType(annot, type_opt, aSTTypeId, int, bool) -> 
	    opt_iter cType_fun type_opt;
	    aSTTypeId_fun aSTTypeId;

	| E_assign(annot, type_opt, expression_target, binaryOp, expression_src) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression_target;
	    expression_fun expression_src

	| E_new(annot, type_opt, bool, argExpression_list, aSTTypeId, 
		argExpressionListOpt_opt, array_size_opt, ctor_opt,
	        statement_opt, heep_var) -> 
	    opt_iter cType_fun type_opt;
	    List.iter argExpression_fun argExpression_list;
	    aSTTypeId_fun aSTTypeId;
	    opt_iter argExpressionListOpt_fun argExpressionListOpt_opt;
	    opt_iter expression_fun array_size_opt;
	    opt_iter variable_fun ctor_opt;
	    opt_iter statement_fun statement_opt;
	    opt_iter variable_fun heep_var

	| E_delete(annot, type_opt, bool_colon, bool_array, 
		   expression_opt, statement_opt) -> 
	    opt_iter cType_fun type_opt;
	    opt_iter expression_fun expression_opt;
	    opt_iter statement_fun statement_opt

	| E_throw(annot, type_opt, expression_opt, var_opt, statement_opt) -> 
	    opt_iter cType_fun type_opt;
	    opt_iter expression_fun expression_opt;
	    opt_iter variable_fun var_opt;
	    opt_iter statement_fun statement_opt

	| E_keywordCast(annot, type_opt, castKeyword, aSTTypeId, expression) -> 
	    opt_iter cType_fun type_opt;
	    aSTTypeId_fun aSTTypeId;
	    expression_fun expression

	| E_typeidExpr(annot, type_opt, expression) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression

	| E_typeidType(annot, type_opt, aSTTypeId) -> 
	    opt_iter cType_fun type_opt;
	    aSTTypeId_fun aSTTypeId

	| E_grouping(annot, type_opt, expression) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression

	| E_arrow(annot, type_opt, expression, pQName) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression;
	    pQName_fun pQName

	| E_statement(annot, type_opt, s_compound) -> 
	    assert(match s_compound with | S_compound _ -> true | _ -> false);
	    opt_iter cType_fun type_opt;
	    statement_fun s_compound

	| E_compoundLit(annot, type_opt, aSTTypeId, in_compound) -> 
	    assert(match in_compound with | IN_compound _ -> true | _ -> false);
	    opt_iter cType_fun type_opt;
	    aSTTypeId_fun aSTTypeId;
	    init_fun in_compound

	| E___builtin_constant_p(annot, type_opt, sourceLoc, expression) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression

	| E___builtin_va_arg(annot, type_opt, sourceLoc, expression, aSTTypeId) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression;
	    aSTTypeId_fun aSTTypeId

	| E_alignofType(annot, type_opt, aSTTypeId, int) -> 
	    opt_iter cType_fun type_opt;
	    aSTTypeId_fun aSTTypeId;

	| E_alignofExpr(annot, type_opt, expression, int) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression;

	| E_gnuCond(annot, type_opt, expression_cond, expression_false) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression_cond;
	    expression_fun expression_false

	| E_addrOfLabel(annot, type_opt, stringRef) -> 
	    opt_iter cType_fun type_opt;


and fullExpression_fun((annot, expression_opt, fullExpressionAnnot) ) =
  if visited annot then ()
  else begin
    visit annot;
    opt_iter expression_fun expression_opt;
    fullExpressionAnnot_fun fullExpressionAnnot
  end


and argExpression_fun((annot, expression) ) =
  if visited annot then ()
  else begin
    visit annot;
    expression_fun expression
  end


and argExpressionListOpt_fun((annot, argExpression_list) ) =
  if visited annot then ()
  else begin
    visit annot;
    List.iter argExpression_fun argExpression_list
  end


and init_fun x = 
  let annot = init_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| IN_expr(annot, sourceLoc, fullExpressionAnnot, expression) -> 
	    fullExpressionAnnot_fun fullExpressionAnnot;
	    expression_fun expression

	| IN_compound(annot, sourceLoc, fullExpressionAnnot, init_list) -> 
	    fullExpressionAnnot_fun fullExpressionAnnot;
	    List.iter init_fun init_list

	| IN_ctor(annot, sourceLoc, fullExpressionAnnot, 
		 argExpression_list, var_opt, bool) -> 
	    fullExpressionAnnot_fun fullExpressionAnnot;
	    List.iter argExpression_fun argExpression_list;
	    opt_iter variable_fun var_opt;

	| IN_designated(annot, sourceLoc, fullExpressionAnnot, 
		       designator_list, init) -> 
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
	    opt_iter templateParameter_fun templateParameter_opt;
	    func_fun func

	| TD_decl(annot, templateParameter_opt, declaration) -> 
	    opt_iter templateParameter_fun templateParameter_opt;
	    declaration_fun declaration

	| TD_tmember(annot, templateParameter_opt, templateDeclaration) -> 
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
	    variable_fun variable;
	    opt_iter aSTTypeId_fun aSTTypeId_opt;
	    opt_iter templateParameter_fun templateParameter_opt

	| TP_nontype(annot, sourceLoc, variable,
		    aSTTypeId, templateParameter_opt) -> 
	    variable_fun variable;
	    aSTTypeId_fun aSTTypeId;
	    opt_iter templateParameter_fun templateParameter_opt


and templateArgument_fun x = 
  let annot = templateArgument_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| TA_type(annot, aSTTypeId, templateArgument_opt) -> 
	    aSTTypeId_fun aSTTypeId;
	    opt_iter templateArgument_fun templateArgument_opt

	| TA_nontype(annot, expression, templateArgument_opt) -> 
	    expression_fun expression;
	    opt_iter templateArgument_fun templateArgument_opt

	| TA_templateUsed(annot, templateArgument_opt) -> 
	    opt_iter templateArgument_fun templateArgument_opt


and namespaceDecl_fun x = 
  let annot = namespaceDecl_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| ND_alias(annot, stringRef, pQName) -> 
	    pQName_fun pQName

	| ND_usingDecl(annot, pQName) -> 
	    pQName_fun pQName

	| ND_usingDir(annot, pQName) -> 
	    pQName_fun pQName


and fullExpressionAnnot_fun((annot, declaration_list) ) =
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
	    cType_fun ctype;
	    fullExpression_fun fullExpression

	| TS_typeof_type(annot, ctype, aSTTypeId) -> 
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
	    ()

	| SubscriptDesignator(annot, sourceLoc, expression, expression_opt, 
			     idx_start, idx_end) -> 
	    expression_fun expression;
	    opt_iter expression_fun expression_opt;


and attribute_fun x = 
  let annot = attribute_annotation x
  in
    if visited annot then ()
    else
      let _ = visit annot 
      in match x with
	| AT_empty(annot, sourceLoc) -> 
	    ()

	| AT_word(annot, sourceLoc, stringRef) -> 
	    ()

	| AT_func(annot, sourceLoc, stringRef, argExpression_list) -> 
	    List.iter argExpression_fun argExpression_list


(**************************************************************************
 *
 * end of astiternodes.ml 
 *
 **************************************************************************)


let out_file = ref ""

let size_flag = ref false

let dot_page_attribute = ref ""

let arguments = Arg.align
  [
    ("-o", Arg.Set_string out_file,
     "file set output file name [default nodes.dot]");
    ("-size", Arg.Set size_flag,
     " limit size of output");
    ("-page", Arg.Set_string dot_page_attribute,
     "pattr add pattr as page attribute in the output");
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
  if !size_flag then
    output_string !oc "    size=\"90,90\";\n";
  if !dot_page_attribute <> "" then
    Printf.fprintf !oc "    page=\"%s\";\n" !dot_page_attribute;
  output_string !oc 
    "    color=white;\n";
  output_string !oc
    "    ordering=out;\n";
  output_string !oc
    "    node [ style = filled ];\n";
  output_string !oc
    "    edge [ arrowtail=odot ];\n"


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
    finish_file ();
    Printf.printf "graph with %d nodes and %d edges generated\n%!"
      !nodes_counter !edge_counter
      
;;


Printexc.catch main ()


