(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* employ the SafeUnmarshal module (http://www.pps.jussieu.fr/~henry/marshal/)
 * to check the consistency of C++ generated ast
 *
 * To use it you have to patch the SafeUnmarshal sources slightly. See
 *
 * http://caml.inria.fr/pub/ml-archives/caml-list/2006/08/033e1bf470934a2ea02d9c99bd9bd9b6.en.html
 *
 * and 
 * - add check to otherlibs/safe_unmarshaling/safeUnmarshal.{ml,mli} 
 *   as described in point 4
 * - fix check_block in otherlibs/safe_unmarshaling/check.ml as
 *   described in point 5
 *
 * If you use normal ocaml in parallel with the SafeUnmarshal patched one
 * (like I do) you might find it handy to use ``make-check-oast'' and 
 * ``make tyclean'' or ``make clean-ml-obj'' in subdir elsa
 *)


open Cc_ml_types
open Cc_ast_gen_type
open Ml_ctype
open Ast_annotation
open Ast_util

let indent_depth = ref 0

let indent() = indent_depth := !indent_depth + 2

let unindent() = 
  indent_depth := !indent_depth -2;
  assert(!indent_depth >= 0)

let indent_string () = String.make !indent_depth ' '

let check_fails node_string (node : 'a) (ty : 'a tyrepr) type_name =
  if SafeUnmarshal.check node ty then begin
      Printf.printf "%s%s (%s) is ok\n%!"
	(indent_string())
	node_string
	type_name;
      false
    end
  else begin
      Printf.printf "%sFailure in %s (%s):\n%!"
	(indent_string())
	node_string
	type_name;
      true
    end


let node_check_fails (annot : annotated) (node : 'a) 
                                          (ty : 'a tyrepr) type_name =
  check_fails (Printf.sprintf "Node %d" (id_annotation annot))
    node ty type_name



module DS = Dense_set

let visited_nodes = DS.make ()

let visited (annot : annotated) =
  if DS.mem (id_annotation annot) visited_nodes
  then begin
      Printf.printf "%sNode %d already visited\n%!" 
	(indent_string()) (id_annotation annot);
      true
    end
  else
    false

let visit (annot : annotated) =
  (* Printf.eprintf "visit %d\n%!" (id_annotation annot); *)
  DS.add (id_annotation annot) visited_nodes


(**************************************************************************
 *
 * contents of astiter.ml
 *
 **************************************************************************)


let annotation_fun (x : annotated) = 
  ignore(check_fails "Annot" x [^ annotated ^] "annotated")

let indirect_iter f (x : 'a) (full_type : 'a tyrepr) (top_type : 'a tyrepr) 
    inner_type_name top_type_name = 
  if SafeUnmarshal.check x full_type then begin
    Printf.printf "%s%s %s is ok\n%!" 
      (indent_string())
      inner_type_name top_type_name
  end
  else begin
    if SafeUnmarshal.check x top_type then begin
      Printf.printf "%sFailure inside %s %s:\n%!" 
	(indent_string())
	inner_type_name top_type_name;
      indent();
      f x;
      unindent();
    end
    else begin
      Printf.printf 
	"%sFailure in %s top structure (don't descend into %s)\n%!" 
	(indent_string())
	top_type_name inner_type_name
    end
  end

let opt_iter f (x : 'a option) (ty : 'a option tyrepr) type_name =
  indirect_iter 
    (function
      | Some x -> f x
      | None -> Printf.printf "%soption is None!!\n%!" (indent_string())
    )
    x ty [^ 'a option ^] type_name "option"


let ref_opt_iter f (x : 'a option ref) (full_type : 'a option ref tyrepr) 
    (opt_type : 'a option tyrepr) inner_type_name =
  indirect_iter
    (fun x -> opt_iter f !x opt_type inner_type_name)
    x
    full_type
    [^ 'a ref ^]
    (inner_type_name ^ " option")
    "ref"

let list_iter f (x : 'a list) (ty : 'a list tyrepr) type_name =
  indirect_iter (List.iter f) x ty [^ 'a list ^] type_name "list"
    

let bool_fun (b : bool) =
  ignore(check_fails "bool" b [^ bool ^] "bool")

let int_fun (i : int) = 
  ignore(check_fails "int" i [^ int ^] "int")

let nativeint_fun (i : nativeint) = 
  ignore(check_fails "nativeint" i [^ nativeint ^] "nativeint")

let int32_fun (i : int32) = 
  ignore(check_fails "int32" i [^ int32 ^] "int32")

let float_fun (x : float) = 
  ignore(check_fails "float" x [^ float ^] "float")

let string_fun (s : string) = 
  ignore(check_fails "string" s [^ string ^] "string")

let sourceLoc_fun (x :sourceLoc) = 
  ignore(check_fails "sourceLoc" x [^ sourceLoc ^] "sourceLoc")

let declFlags_fun(l : declFlag list) = 
  ignore(check_fails "declFlags" l [^ declFlags ^] "declFlags")

let simpleTypeId_fun(id : simpleTypeId) = 
  ignore(check_fails "simpleTypeId" id [^ simpleTypeId ^] "simpleTypeId")

let typeIntr_fun(keyword : typeIntr) = 
  ignore(check_fails "typeIntr" keyword [^ typeIntr ^] "typeIntr")

let accessKeyword_fun(keyword : accessKeyword) = 
  ignore(
    check_fails "accessKeyword" keyword [^ accessKeyword ^] "accessKeyword")

let cVFlags_fun(fl : cVFlag list) = 
  ignore(check_fails "cVFlags" fl [^ cVFlags ^] "cVFlags")

let overloadableOp_fun(op :overloadableOp) = 
  ignore(check_fails "overloadableOp" op [^ overloadableOp ^] "overloadableOp")

let unaryOp_fun(op : unaryOp) = 
  ignore(check_fails "unaryOp" op [^ unaryOp ^] "unaryOp")

let effectOp_fun(op : effectOp) = 
  ignore(check_fails "effectOp" op [^ effectOp ^] "effectOp")

let binaryOp_fun(op : binaryOp) = 
  ignore(check_fails "binaryOp" op [^ binaryOp ^] "binaryOp")

let castKeyword_fun(keyword : castKeyword) = 
  ignore(check_fails "castKeyword" keyword [^ castKeyword ^] "castKeyword")

let function_flags_fun(flags : function_flags) = 
  ignore(
    check_fails "function_flags" flags [^ function_flags ^] "function_flags")

let declaratorContext_fun(context : declaratorContext) = 
  ignore(
    check_fails "declaratorContext" context 
      [^ declaratorContext ^] "declaratorContext")


let array_size_fun x =
  if check_fails "array_size" x [^ array_size ^] "array_size"
  then
    match x with
      | NO_SIZE -> ()
      | DYN_SIZE -> ()
      | FIXED_SIZE(int) -> int_fun int


let compoundType_Keyword_fun x =
  if check_fails "compoundType_Keyword" x 
    [^ compoundType_Keyword ^] "compoundType_Keyword"
  then match x with
    | K_STRUCT -> ()
    | K_CLASS -> ()
    | K_UNION -> ()


(***************** variable ***************************)

let rec variable_fun(v : annotated variable) =
  let annot = variable_annotation v
  in
    if not (visited annot) &&
      node_check_fails annot v [^ annotated variable ^] "variable"
    then begin
      visit annot; indent();

      annotation_fun v.poly_var;
      sourceLoc_fun v.loc;
      opt_iter string_fun v.var_name [^ string option ^] "string";

      (* POSSIBLY CIRCULAR *)
      ref_opt_iter cType_fun v.var_type 
	[^ annotated cType option ref ^] [^ annotated cType option ^] "cType";
      declFlags_fun v.flags;
      (* POSSIBLY CIRCULAR *)
      ref_opt_iter expression_fun v.value
	[^ annotated expression_type option ref ^] 
	[^ annotated expression_type option ^] "expression_type";
      opt_iter cType_fun v.defaultParam [^ annotated cType option ^] "ctype";

      (* POSSIBLY CIRCULAR *)
      opt_iter func_fun !(v.funcDefn) [^ annotated function_type option ^] 
	"function_type";

      unindent()
    end

(***************** cType ******************************)

and baseClass_fun baseClass =
  let annot = baseClass_annotation baseClass
  in
    if not (visited annot) &&
      node_check_fails annot baseClass [^ annotated baseClass ^] "baseClass"
    then begin
      visit annot; indent();
      annotation_fun baseClass.poly_base;
      compound_info_fun baseClass.compound;
      accessKeyword_fun baseClass.bc_access;
      bool_fun baseClass.is_virtual;
      unindent()
    end


and compound_info_fun info = 
  let annot = compound_info_annotation info
  in
    if not (visited annot) &&
      node_check_fails annot info [^ annotated compound_info ^] "compound_info"
    then begin
      visit annot; indent();
      annotation_fun info.compound_info_poly;
      opt_iter string_fun info.compound_name [^ string option ^] "string";
      variable_fun info.typedef_var;
      accessKeyword_fun info.ci_access;
      bool_fun info.is_forward_decl;
      compoundType_Keyword_fun info.keyword;
      list_iter variable_fun info.data_members 
	[^ annotated variable list ^] "variable";
      list_iter baseClass_fun info.bases 
	[^ annotated baseClass list ^] "baseClass";
      list_iter variable_fun info.conversion_operators
	[^ annotated variable list ^] "variable";

      list_iter variable_fun info.friends
       [^ annotated variable list ^] "variable";
      opt_iter string_fun info.inst_name [^ string option ^] "string";

      (* POSSIBLY CIRCULAR *)
      ref_opt_iter cType_fun info.self_type
	[^ annotated cType option ref ^] [^ annotated cType option ^] "cType";

      unindent()
    end


and atomicType_fun x = 
  let annot = atomicType_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated atomicType ^] "atomicType_type"
    then 
      indent();
      (match x with
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
	  opt_iter variable_fun variable_opt [^ annotated variable option ^] 
	    "variable";
	  accessKeyword_fun accessKeyword;
	  compound_info_fun compound_info;
	  list_iter sTemplateArgument_fun sTemplateArgument_list
	    [^ annotated sTemplateArgument list ^] "sTemplateArgument";

      | EnumType(annot, string, variable, accessKeyword, 
		 string_nativeint_list) ->
	  visit annot;
	  annotation_fun annot;
	  opt_iter string_fun string [^ string option ^] "string";
	  opt_iter variable_fun variable [^ annotated variable option ^] 
	    "variable";
	  accessKeyword_fun accessKeyword;
	  list_iter 
	    (fun x -> indirect_iter
	      (fun (string, nativeint) -> 
		(string_fun string; nativeint_fun nativeint))
	      x [^ string * nativeint ^] [^ 'a * 'b ^] 
	      "(string, nativeint)" "pair")
	    string_nativeint_list
	    [^ (string * nativeint) list ^] "(string * nativeint) list"

      | TypeVariable(annot, string, variable, accessKeyword) ->
	  visit annot;
	  annotation_fun annot;
	  string_fun string;
	  variable_fun variable;
	  accessKeyword_fun accessKeyword

      | DependentQType(annot, string, variable, 
		      accessKeyword, atomic, pq_name) ->
	  visit annot;
	  annotation_fun annot;
	  string_fun string;
	  variable_fun variable;
	  accessKeyword_fun accessKeyword;
	  atomicType_fun atomic;
	  pQName_fun pq_name
      );
      unindent()


and cType_fun x = 
  let annot = cType_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated cType ^] "cType"
    then
      let _ = visit annot; indent()
      in 
	(match x with
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
	       list_iter variable_fun variable_list
		 [^ annotated variable list ^] "variable";
	       opt_iter 
		 (fun l -> 
		    list_iter cType_fun l [^ annotated cType list ^] "cType")
		 cType_list_opt [^ annotated cType list option ^] "cType list";

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
			| DependentQType _ -> true);
	       annotation_fun annot;
	       atomicType_fun atomicType;
	       cVFlags_fun cVFlags;
	       cType_fun cType
	);
	unindent()


and sTemplateArgument_fun ta = 
  let annot = sTemplateArgument_annotation ta
  in
    if not (visited annot) &&
      node_check_fails annot ta [^ annotated sTemplateArgument ^] "sTemplateArgument"
    then 
      let _ = visit annot; indent()
      in 
	(match ta with
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

	   | STA_TEMPLATE annot -> 
	       annotation_fun annot

	   | STA_ATOMIC(annot, atomicType) -> 
	       annotation_fun annot;
	       atomicType_fun atomicType
	);
	unindent()


(***************** generated ast nodes ****************)

and translationUnit_fun 
    ((annot, topForm_list) as x : annotated translationUnit_type) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated translationUnit_type ^] 
    "translationUnit_type"
  then begin
    visit annot; indent();
    annotation_fun annot;
    list_iter topForm_fun topForm_list
      [^ annotated topForm_type list ^] "topForm_type";

    unindent()
  end


and topForm_fun x = 
  let annot = topForm_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated topForm_type ^] "topForm_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
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
	       assert(match e_stringLit with 
			| E_stringLit _ -> true 
			| _ -> false);
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       expression_fun e_stringLit

	   | TF_namespaceDefn(annot, sourceLoc, stringRef_opt, topForm_list) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       opt_iter string_fun stringRef_opt [^ string option ^] "string";
	       list_iter topForm_fun topForm_list
		 [^ annotated topForm_type list ^] "topForm_type";

	   | TF_namespaceDecl(annot, sourceLoc, namespaceDecl) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       namespaceDecl_fun namespaceDecl
	);
	unindent()


and func_fun((annot, declFlags, typeSpecifier, declarator, memberInit_list, 
	     s_compound_opt, handler_list, func, variable_opt_1, 
	     variable_opt_2, statement_opt, bool) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated function_type ^] "function_type"
  then begin
    visit annot; indent();
    annotation_fun annot;
    declFlags_fun declFlags;
    typeSpecifier_fun typeSpecifier;
    declarator_fun declarator;
    list_iter memberInit_fun memberInit_list
      [^ annotated memberInit_type list ^] "memberInit_type";
    opt_iter statement_fun s_compound_opt 
      [^ annotated statement_type option ^] "statement_type";
    assert(match s_compound_opt with
	     | None -> true
	     | Some s_compound ->
		 match s_compound with 
		   | S_compound _ -> true 
		   | _ -> false);
    list_iter handler_fun handler_list
      [^ annotated handler_type list ^] "handler_type";
    cType_fun func;
    assert(match func with 
      | FunctionType _ -> true
      | _ -> false);
    opt_iter variable_fun variable_opt_1 [^ annotated variable option ^] 
      "variable";
    opt_iter variable_fun variable_opt_2 [^ annotated variable  option ^] 
      "variable";
    opt_iter statement_fun statement_opt 
      [^ annotated statement_type option ^] "statement_type";
    bool_fun bool;

    unindent();
  end


and memberInit_fun((annot, pQName, argExpression_list, 
		   variable_opt_1, compound_opt, variable_opt_2, 
		   full_expr_annot, statement_opt) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated memberInit_type ^] "memberInit_type"
  then begin
      assert(match compound_opt with
	| None
	| Some(CompoundType _) -> true
	| _ -> false);
    visit annot; indent();
    annotation_fun annot;
    pQName_fun pQName;
    list_iter argExpression_fun argExpression_list
      [^ annotated argExpression_type list ^] "argExpression_type";
    opt_iter variable_fun variable_opt_1 [^ annotated variable option ^] 
      "variable";
    opt_iter atomicType_fun compound_opt [^ annotated atomicType option ^] 
      "atomicType";
    opt_iter variable_fun variable_opt_2 [^ annotated variable option ^] "variable";
    fullExpressionAnnot_fun full_expr_annot;
    opt_iter statement_fun statement_opt 
      [^ annotated statement_type option ^] "statement_type";

    unindent()
  end


and declaration_fun((annot, declFlags, typeSpecifier, declarator_list) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated declaration_type ^] "declaration_type"
  then begin
    visit annot; indent();
    annotation_fun annot;
    declFlags_fun declFlags;
    typeSpecifier_fun typeSpecifier;
    list_iter declarator_fun declarator_list
      [^ annotated declarator_type list ^] "declarator_type";

    unindent()
  end


and aSTTypeId_fun((annot, typeSpecifier, declarator) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated aSTTypeId_type ^] "aSTTypeId_type"
  then begin
    visit annot; indent();
    annotation_fun annot;
    typeSpecifier_fun typeSpecifier;
    declarator_fun declarator;

    unindent()
  end


and pQName_fun x = 
  let annot = pQName_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated pQName_type ^] "pQName_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
	   | PQ_qualifier(annot, sourceLoc, stringRef_opt, 
			  templateArgument_opt, pQName, 
			  variable_opt, s_template_arg_list) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       opt_iter string_fun stringRef_opt [^ string option ^] "string";
	       opt_iter templateArgument_fun templateArgument_opt
		 [^ annotated templateArgument_type option ^]
		 "templateArgument_type";
	       pQName_fun pQName;
	       opt_iter variable_fun variable_opt 
		 [^ annotated variable option ^] "variable";
	       list_iter sTemplateArgument_fun s_template_arg_list
		 [^ annotated sTemplateArgument list ^] "sTemplateArgument";

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
	       opt_iter templateArgument_fun templateArgument_opt 
		 [^ annotated templateArgument_type option ^]
		 "templateArgument_type";
	       list_iter sTemplateArgument_fun s_template_arg_list
		 [^ annotated sTemplateArgument list ^] "sTemplateArgument";

	   | PQ_variable(annot, sourceLoc, variable) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       variable_fun variable
	);
	unindent()


and typeSpecifier_fun x = 
  let annot = typeSpecifier_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated typeSpecifier_type ^] "typeSpecifier_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
	   | TS_name(annot, sourceLoc, cVFlags, pQName, bool, 
		     var_opt_1, var_opt_2) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       cVFlags_fun cVFlags;
	       pQName_fun pQName;
	       bool_fun bool;
	       opt_iter variable_fun var_opt_1 [^ annotated variable option ^] 
		 "variable";
	       opt_iter variable_fun var_opt_2 [^ annotated variable option ^] 
		 "variable";

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
		 [^ annotated atomicType option ^] "atomicType";

	   | TS_classSpec(annot, sourceLoc, cVFlags, typeIntr, pQName_opt, 
			  baseClassSpec_list, memberList, compoundType) -> 
	       assert(match compoundType with
			| CompoundType _ -> true
			| _ -> false);
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       cVFlags_fun cVFlags;
	       typeIntr_fun typeIntr;
	       opt_iter pQName_fun pQName_opt 
		 [^ annotated pQName_type option ^] "pQName_type";
	       list_iter baseClassSpec_fun baseClassSpec_list
		 [^ annotated baseClassSpec_type list ^] "baseClassSpec_type";
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
	       opt_iter string_fun stringRef_opt [^ string option ^] "string";
	       list_iter enumerator_fun enumerator_list
		 [^ annotated enumerator_type list ^] "enumerator_type";
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
	);
	unindent()

and baseClassSpec_fun
    ((annot, bool, accessKeyword, pQName, compoundType_opt) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated baseClassSpec_type ^] "baseClassSpec_type"
  then begin
      assert(match compoundType_opt with
	| Some(CompoundType _ ) -> true
	| _ -> false);
    visit annot; indent();
    annotation_fun annot;
    bool_fun bool;
    accessKeyword_fun accessKeyword;
    pQName_fun pQName;
    opt_iter atomicType_fun compoundType_opt 
      [^ annotated atomicType option ^] "atomicType";

    unindent()
  end


and enumerator_fun((annot, sourceLoc, stringRef, 
		   expression_opt, variable, int32) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated enumerator_type ^] "enumerator_type"
  then begin
    visit annot; indent();
    annotation_fun annot;
    sourceLoc_fun sourceLoc;
    string_fun stringRef;
    opt_iter expression_fun expression_opt 
      [^ annotated expression_type option ^] "expression_type";
    variable_fun variable;
    int32_fun int32;

    unindent()
  end


and memberList_fun((annot, member_list) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated memberList_type ^] "memberList_type"
  then begin
    visit annot; indent();
    annotation_fun annot;
    list_iter member_fun member_list
      [^ annotated member_type list ^] "member_type";

    unindent()
  end


and member_fun x = 
  let annot = member_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated member_type ^] "member_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
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
	);
	unindent()


and declarator_fun((annot, iDeclarator, init_opt, 
		   variable_opt, ctype_opt, declaratorContext,
		   statement_opt_ctor, statement_opt_dtor) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated declarator_type ^] "declarator_type"
  then begin
    visit annot; indent();
    annotation_fun annot;
    iDeclarator_fun iDeclarator;
    opt_iter init_fun init_opt [^ annotated initializer_type option ^] 
      "initializer_type";
    opt_iter variable_fun variable_opt [^ annotated variable option ^] 
      "variable";
    opt_iter cType_fun ctype_opt [^ annotated cType option ^] "cType";
    declaratorContext_fun declaratorContext;
    opt_iter statement_fun statement_opt_ctor 
      [^ annotated statement_type option ^] "statement_type";
    opt_iter statement_fun statement_opt_dtor 
      [^ annotated statement_type option ^] "statement_type";

    unindent()
  end


and iDeclarator_fun x = 
  let annot = iDeclarator_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated iDeclarator_type ^] "iDeclarator_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
	   | D_name(annot, sourceLoc, pQName_opt) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       opt_iter pQName_fun pQName_opt 
		 [^ annotated pQName_type option ^] "pQName_type";

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
	       list_iter aSTTypeId_fun aSTTypeId_list
		 [^ annotated aSTTypeId_type list ^] "aSTTypeId_type";
	       cVFlags_fun cVFlags;
	       opt_iter exceptionSpec_fun exceptionSpec_opt 
		 [^ annotated exceptionSpec_type option ^] "exceptionSpec_type";
	       list_iter pQName_fun pq_name_list
		 [^ annotated pQName_type list ^] "pQName_type";
	       bool_fun bool

	   | D_array(annot, sourceLoc, iDeclarator, expression_opt, bool) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       iDeclarator_fun iDeclarator;
	       opt_iter expression_fun expression_opt 
		 [^ annotated expression_type option ^] "expression_type";
	       bool_fun bool

	   | D_bitfield(annot, sourceLoc, pQName_opt, expression, int) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       opt_iter pQName_fun pQName_opt 
		 [^ annotated pQName_type option ^] "pQName_type";
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
	       list_iter 
		 (fun x -> list_iter attribute_fun x
		    [^ annotated attribute_type list ^] "attribute_type")
		 attribute_list_list
		 [^ annotated attribute_type list list ^] "attribute_type list";
	);
	unindent()


and exceptionSpec_fun((annot, aSTTypeId_list) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated exceptionSpec_type ^] "exceptionSpec_type"
  then begin
    visit annot; indent();
    annotation_fun annot;
    list_iter aSTTypeId_fun aSTTypeId_list
      [^ annotated aSTTypeId_type list ^] "aSTTypeId_type";

    unindent()
  end


and operatorName_fun x = 
  let annot = operatorName_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated operatorName_type ^] "operatorName_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
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
	);
	unindent()

and statement_fun x = 
  let annot = statement_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated statement_type ^] "statement_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
	   | S_skip(annot, sourceLoc) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc

	   | S_label(annot, sourceLoc, stringRef, statement) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       string_fun stringRef;
	       statement_fun statement

	   | S_case(annot, sourceLoc, expression, statement, int) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       expression_fun expression;
	       statement_fun statement;
	       int_fun int

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
	       list_iter statement_fun statement_list
		 [^ annotated statement_type list ^] "statement_type";

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
	       opt_iter fullExpression_fun fullExpression_opt 
		 [^ annotated fullExpression_type option ^] "fullExpression_type";
	       opt_iter statement_fun statement_opt 
		 [^ annotated statement_type option ^] "statement_type";

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
	       list_iter handler_fun handler_list
		 [^ annotated handler_type list ^] "handler_type";

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
	);
	unindent()


and condition_fun x = 
  let annot = condition_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated condition_type ^] "condition_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
	   | CN_expr(annot, fullExpression) -> 
	       annotation_fun annot;
	       fullExpression_fun fullExpression

	   | CN_decl(annot, aSTTypeId) -> 
	       annotation_fun annot;
	       aSTTypeId_fun aSTTypeId
	);
	unindent()

and handler_fun((annot, aSTTypeId, statement_body, variable_opt, 
		fullExpressionAnnot, expression_opt, statement_gdtor) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated handler_type ^] "handler_type"
  then begin
    visit annot; indent();
    annotation_fun annot;
    aSTTypeId_fun aSTTypeId;
    statement_fun statement_body;
    opt_iter variable_fun variable_opt [^ annotated variable option ^] 
      "variable";
    fullExpressionAnnot_fun fullExpressionAnnot;
    opt_iter expression_fun expression_opt 
      [^ annotated expression_type option ^] "expression_type";
    opt_iter statement_fun statement_gdtor 
      [^ annotated statement_type option ^] "statement_type";

    unindent()
  end


and expression_fun x = 
  let annot = expression_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated expression_type ^] "expression_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
	   | E_boolLit(annot, type_opt, bool) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       bool_fun bool

	   | E_intLit(annot, type_opt, stringRef, ulong) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       string_fun stringRef;
	       int32_fun ulong

	   | E_floatLit(annot, type_opt, stringRef, double) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       string_fun stringRef;
	       float_fun double

	   | E_stringLit(annot, type_opt, stringRef, e_stringLit_opt) -> 
	       assert(match e_stringLit_opt with 
			| Some(E_stringLit _) -> true 
			| None -> true
			| _ -> false);
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       string_fun stringRef;
	       opt_iter expression_fun e_stringLit_opt 
		 [^ annotated expression_type option ^] "expression_type";

	   | E_charLit(annot, type_opt, stringRef, int32) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       string_fun stringRef;
	       int32_fun int32

	   | E_this(annot, type_opt, variable) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       variable_fun variable

	   | E_variable(annot, type_opt, pQName, var_opt, nondep_var_opt) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       pQName_fun pQName;
	       opt_iter variable_fun var_opt [^ annotated variable option ^] 
		 "variable";
	       opt_iter variable_fun nondep_var_opt 
		 [^ annotated variable option ^] "variable";

	   | E_funCall(annot, type_opt, expression_func, 
		       argExpression_list, expression_retobj_opt) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       expression_fun expression_func;
	       list_iter argExpression_fun argExpression_list
		 [^ annotated argExpression_type list ^] "argExpression_type";
	       opt_iter expression_fun expression_retobj_opt 
		 [^ annotated expression_type option ^] "expression_type";

	   | E_constructor(annot, type_opt, typeSpecifier, argExpression_list, 
			   var_opt, bool, expression_opt) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       typeSpecifier_fun typeSpecifier;
	       list_iter argExpression_fun argExpression_list
		 [^ annotated argExpression_type list ^] "argExpression_type";
	       opt_iter variable_fun var_opt [^ annotated variable option ^] 
		 "variable";
	       bool_fun bool;
	       opt_iter expression_fun expression_opt 
		 [^ annotated expression_type option ^] "expression_type";

	   | E_fieldAcc(annot, type_opt, expression, pQName, var_opt) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       expression_fun expression;
	       pQName_fun pQName;
	       opt_iter variable_fun var_opt [^ annotated variable option ^] 
		 "variable";

	   | E_sizeof(annot, type_opt, expression, int) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       expression_fun expression;
	       int_fun int

	   | E_unary(annot, type_opt, unaryOp, expression) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       unaryOp_fun unaryOp;
	       expression_fun expression

	   | E_effect(annot, type_opt, effectOp, expression) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       effectOp_fun effectOp;
	       expression_fun expression

	   | E_binary(annot, type_opt, expression_left, binaryOp, expression_right) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       expression_fun expression_left;
	       binaryOp_fun binaryOp;
	       expression_fun expression_right

	   | E_addrOf(annot, type_opt, expression) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       expression_fun expression

	   | E_deref(annot, type_opt, expression) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       expression_fun expression

	   | E_cast(annot, type_opt, aSTTypeId, expression, bool) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       aSTTypeId_fun aSTTypeId;
	       expression_fun expression;
	       bool_fun bool

	   | E_cond(annot, type_opt, expression_cond, expression_true, expression_false) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       expression_fun expression_cond;
	       expression_fun expression_true;
	       expression_fun expression_false

	   | E_sizeofType(annot, type_opt, aSTTypeId, int, bool) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       aSTTypeId_fun aSTTypeId;
	       int_fun int;
	       bool_fun bool

	   | E_assign(annot, type_opt, expression_target, binaryOp, expression_src) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       expression_fun expression_target;
	       binaryOp_fun binaryOp;
	       expression_fun expression_src

	   | E_new(annot, type_opt, bool, argExpression_list, aSTTypeId, 
		   argExpressionListOpt_opt, array_size_opt, ctor_opt,
	           statement_opt, heep_var_opt) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       bool_fun bool;
	       list_iter argExpression_fun argExpression_list
		 [^ annotated argExpression_type list ^] "argExpression_type";
	       aSTTypeId_fun aSTTypeId;
	       opt_iter argExpressionListOpt_fun argExpressionListOpt_opt
		 [^ annotated argExpressionListOpt_type option ^]
		 "argExpressionListOpt_type";
	       opt_iter expression_fun array_size_opt 
		 [^ annotated expression_type option ^] "expression_type";
	       opt_iter variable_fun ctor_opt 
		 [^ annotated variable option ^] "variable";
	       opt_iter statement_fun statement_opt 
		 [^ annotated statement_type option ^] "statement_type";
	       opt_iter variable_fun heep_var_opt 
		 [^ annotated variable option ^] "variable";

	   | E_delete(annot, type_opt, bool_colon, bool_array, 
		      expression_opt, statement_opt) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       bool_fun bool_colon;
	       bool_fun bool_array;
	       opt_iter expression_fun expression_opt 
		 [^ annotated expression_type option ^] "expression_type";
	       opt_iter statement_fun statement_opt 
		 [^ annotated statement_type option ^] "statement_type";

	   | E_throw(annot, type_opt, expression_opt, var_opt, statement_opt) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       opt_iter expression_fun expression_opt 
		 [^ annotated expression_type option ^] "expression_type";
	       opt_iter variable_fun var_opt [^ annotated variable option ^] 
		 "variable";
	       opt_iter statement_fun statement_opt 
		 [^ annotated statement_type option ^] "statement_type";

	   | E_keywordCast(annot, type_opt, castKeyword, aSTTypeId, expression) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       castKeyword_fun castKeyword;
	       aSTTypeId_fun aSTTypeId;
	       expression_fun expression

	   | E_typeidExpr(annot, type_opt, expression) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       expression_fun expression

	   | E_typeidType(annot, type_opt, aSTTypeId) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       aSTTypeId_fun aSTTypeId

	   | E_grouping(annot, type_opt, expression) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       expression_fun expression

	   | E_arrow(annot, type_opt, expression, pQName) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       expression_fun expression;
	       pQName_fun pQName

	   | E_statement(annot, type_opt, s_compound) -> 
	       assert(match s_compound with | S_compound _ -> true | _ -> false);
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       statement_fun s_compound

	   | E_compoundLit(annot, type_opt, aSTTypeId, in_compound) -> 
	       assert(match in_compound with | IN_compound _ -> true | _ -> false);
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       aSTTypeId_fun aSTTypeId;
	       init_fun in_compound

	   | E___builtin_constant_p(annot, type_opt, sourceLoc, expression) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       sourceLoc_fun sourceLoc;
	       expression_fun expression

	   | E___builtin_va_arg(annot, type_opt, sourceLoc, expression, aSTTypeId) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       sourceLoc_fun sourceLoc;
	       expression_fun expression;
	       aSTTypeId_fun aSTTypeId

	   | E_alignofType(annot, type_opt, aSTTypeId, int) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       aSTTypeId_fun aSTTypeId;
	       int_fun int

	   | E_alignofExpr(annot, type_opt, expression, int) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       expression_fun expression;
	       int_fun int

	   | E_gnuCond(annot, type_opt, expression_cond, expression_false) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       expression_fun expression_cond;
	       expression_fun expression_false

	   | E_addrOfLabel(annot, type_opt, stringRef) -> 
	       annotation_fun annot;
	       opt_iter cType_fun type_opt [^ annotated cType option ^] "cType";
	       string_fun stringRef
	);
	unindent()

and fullExpression_fun((annot, expression_opt, fullExpressionAnnot) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated fullExpression_type ^] 
    "fullExpression_type"
  then begin
    visit annot; indent();
    annotation_fun annot;
    opt_iter expression_fun expression_opt 
      [^ annotated expression_type option ^] "expression_type";
    fullExpressionAnnot_fun fullExpressionAnnot;

    unindent()
  end


and argExpression_fun((annot, expression) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated argExpression_type ^] "argExpression_type"
  then begin
    visit annot; indent();
    annotation_fun annot;
    expression_fun expression;

    unindent()
  end


and argExpressionListOpt_fun((annot, argExpression_list) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated argExpressionListOpt_type ^] 
    "argExpressionListOpt_type"
  then begin
    visit annot; indent();
    annotation_fun annot;
    list_iter argExpression_fun argExpression_list
      [^ annotated argExpression_type list ^] "argExpression_type";

    unindent()
  end


and init_fun x = 
  let annot = init_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated initializer_type ^] "initializer_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
	   | IN_expr(annot, sourceLoc, fullExpressionAnnot, expression_opt) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       fullExpressionAnnot_fun fullExpressionAnnot;
	       opt_iter expression_fun expression_opt 
		 [^ annotated expression_type option ^] "expression_type"

	   | IN_compound(annot, sourceLoc, fullExpressionAnnot, init_list) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       fullExpressionAnnot_fun fullExpressionAnnot;
	       list_iter init_fun init_list
		 [^ annotated initializer_type list ^] "initializer_type";

	   | IN_ctor(annot, sourceLoc, fullExpressionAnnot, 
		     argExpression_list, var_opt, bool) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       fullExpressionAnnot_fun fullExpressionAnnot;
	       list_iter argExpression_fun argExpression_list
		 [^ annotated argExpression_type list ^] "argExpression_type";
	       opt_iter variable_fun var_opt [^ annotated variable option ^] 
		 "variable";
	       bool_fun bool

	   | IN_designated(annot, sourceLoc, fullExpressionAnnot, 
			   designator_list, init) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       fullExpressionAnnot_fun fullExpressionAnnot;
	       list_iter designator_fun designator_list
		 [^ annotated designator_type list ^] "designator_type";
	       init_fun init
	);
	unindent()


and templateDeclaration_fun x = 
  let annot = templateDeclaration_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated templateDeclaration_type ^] 
      "templateDeclaration_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
	   | TD_func(annot, templateParameter_opt, func) -> 
	       annotation_fun annot;
	       opt_iter templateParameter_fun templateParameter_opt 
		 [^ annotated templateParameter_type option ^] 
		 "templateParameter_type";
	       func_fun func
		 
	   | TD_decl(annot, templateParameter_opt, declaration) -> 
	       annotation_fun annot;
	       opt_iter templateParameter_fun templateParameter_opt 
		 [^ annotated templateParameter_type option ^] 
		 "templateParameter_type";
	       declaration_fun declaration

	   | TD_tmember(annot, templateParameter_opt, templateDeclaration) -> 
	       annotation_fun annot;
	       opt_iter templateParameter_fun templateParameter_opt 
		 [^ annotated templateParameter_type option ^] 
		 "templateParameter_type";
	       templateDeclaration_fun templateDeclaration
	);
	unindent()


and templateParameter_fun x = 
  let annot = templateParameter_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated templateParameter_type ^] 
      "templateParameter_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
	   | TP_type(annot, sourceLoc, variable, stringRef, 
		     aSTTypeId_opt, templateParameter_opt) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       variable_fun variable;
	       string_fun stringRef;
	       opt_iter aSTTypeId_fun aSTTypeId_opt 
		 [^ annotated aSTTypeId_type option ^] "aSTTypeId_type";
	       opt_iter templateParameter_fun templateParameter_opt 
		 [^ annotated templateParameter_type option ^]
		 "templateParameter_type";

	   | TP_nontype(annot, sourceLoc, variable,
			aSTTypeId, templateParameter_opt) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       variable_fun variable;
	       aSTTypeId_fun aSTTypeId;
	       opt_iter templateParameter_fun templateParameter_opt 
		 [^ annotated templateParameter_type option ^]
		 "templateParameter_type";
	);
	unindent()


and templateArgument_fun x = 
  let annot = templateArgument_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated templateArgument_type ^] 
      "templateArgument_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
	   | TA_type(annot, aSTTypeId, templateArgument_opt) -> 
	       annotation_fun annot;
	       aSTTypeId_fun aSTTypeId;
	       opt_iter templateArgument_fun templateArgument_opt 
		 [^ annotated templateArgument_type option ^]
		 "templateArgument_type";

	   | TA_nontype(annot, expression, templateArgument_opt) -> 
	       annotation_fun annot;
	       expression_fun expression;
	       opt_iter templateArgument_fun templateArgument_opt
		 [^ annotated templateArgument_type option ^]
		 "templateArgument_type";

	   | TA_templateUsed(annot, templateArgument_opt) -> 
	       annotation_fun annot;
	       opt_iter templateArgument_fun templateArgument_opt
		 [^ annotated templateArgument_type option ^]
		 "templateArgument_type";
	);
	unindent()


and namespaceDecl_fun x = 
  let annot = namespaceDecl_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated namespaceDecl_type ^] 
      "namespaceDecl_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
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
	);
	unindent()


and fullExpressionAnnot_fun((annot, declaration_list) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated fullExpressionAnnot_type ^] 
    "fullExpressionAnnot_type"
  then begin
    visit annot; indent();
    list_iter declaration_fun declaration_list
      [^ annotated declaration_type list ^] "declaration_type";

    unindent()
  end


and aSTTypeof_fun x = 
  let annot = aSTTypeof_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated aSTTypeof_type ^] "aSTTypeof_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
	   | TS_typeof_expr(annot, ctype, fullExpression) -> 
	       annotation_fun annot;
	       cType_fun ctype;
	       fullExpression_fun fullExpression

	   | TS_typeof_type(annot, ctype, aSTTypeId) -> 
	       annotation_fun annot;
	       cType_fun ctype;
	       aSTTypeId_fun aSTTypeId
	);
	unindent()


and designator_fun x = 
  let annot = designator_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated designator_type ^] "designator_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
	   | FieldDesignator(annot, sourceLoc, stringRef) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       string_fun stringRef

	   | SubscriptDesignator(annot, sourceLoc, expression, expression_opt, 
				 idx_start, idx_end) -> 
	       annotation_fun annot;
	       sourceLoc_fun sourceLoc;
	       expression_fun expression;
	       opt_iter expression_fun expression_opt 
		 [^ annotated expression_type option ^] "expression_type";
	       int_fun idx_start;
	       int_fun idx_end
	);
	unindent()


and attribute_fun x = 
  let annot = attribute_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated attribute_type ^] "attribute_type"
    then
      let _ = visit annot; indent()
      in 
	(match x with
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
	       list_iter argExpression_fun argExpression_list
		 [^ annotated argExpression_type list ^] "argExpression_type";
	);
	unindent()

(**************************************************************************
 *
 * end of astiter.ml 
 *
 **************************************************************************)







let check_inside = ref false

let file = ref None

let set_file f = 
  match !file with
    | None -> file := Some f
    | Some _ -> 
	raise (Arg.Bad (Printf.sprintf "don't know what to do with %s" f))

let options = Arg.align
  [
    ("-inside", Arg.Set check_inside,
     " descend into the ast if the check fails");
  ]

let usage_msg = "check-oast file\npermitted options:"

let usage() = 
  Arg.usage options usage_msg;
  exit 1

let main () =
  let _ = Arg.parse options set_file usage_msg in
  let file = match !file with
    | None -> usage()
    | Some f -> f
  in
  let ic = open_in file in
  let ast = Marshal.from_channel ic 
  in
    close_in ic;
    Printf.printf "%s: typecheck ... %!" file;
    if SafeUnmarshal.check ast [^ annotated translationUnit_type ^]
    then begin
    	print_string "passed\n";
    	exit 0;
      end
    else begin
    	print_string "failed\n";
    	if !check_inside then begin
            print_endline "Descend into ast:";
	    translationUnit_fun ast;
      	  end;
      	exit 1;
      end

    (* 
     * try
     *   ignore(SafeUnmarshal.copy 
     * 	       [^ annotated translationUnit_type ^] ast);
     *   print_string "passed\n";
     *   exit 0
     * with
     *   | SafeUnmarshal.Fail ->
     * 	  print_string "failed\n";
     * 	  exit 1
     *)

;;

Printexc.catch main ()

  
(*** Local Variables: ***)
(*** compile-command: "./make-check-oast" ***)
(*** End: ***)
