
open Cc_ml_types
open Cc_ast_gen_type
open Ml_ctype
open Ast_annotation
open Ast_util


let check_fails node_string (node : 'a) (ty : 'a tyrepr) type_name =
  if SafeUnmarshal.check node ty then begin
      Printf.printf "%s (%s) is ok\n%!"
	node_string
	type_name;
      false
    end
  else begin
      Printf.printf "Failure in %s (%s):\n%!"
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
      Printf.printf "Node %d already visited\n%!" (id_annotation annot);
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

let opt_iter f x = 
  if check_fails "Option" x [^ 'a option ^] "'a option"
  then
    match x with
      | None -> ()
      | Some x -> f x

let bool_fun (b : bool) =
  ignore(check_fails "bool" b [^ bool ^] "bool")

let int_fun (i : int) = 
  ignore(check_fails "int" i [^ int ^] "int")

let nativeint_fun (i : nativeint) = 
  ignore(check_fails "nativeint" i [^ nativeint ^] "nativeint")

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
      visit annot;

      annotation_fun v.poly_var;
      sourceLoc_fun v.loc;
      opt_iter string_fun v.var_name;

      (* POSSIBLY CIRCULAR *)
      opt_iter cType_fun !(v.var_type);
      declFlags_fun v.flags;
      opt_iter expression_fun v.value;
      opt_iter cType_fun v.defaultParam;

      (* POSSIBLY CIRCULAR *)
      opt_iter func_fun !(v.funcDefn)
    end

(***************** cType ******************************)

and baseClass_fun baseClass =
  let annot = baseClass_annotation baseClass
  in
    if not (visited annot) &&
      node_check_fails annot baseClass [^ annotated baseClass ^] "baseClass"
    then begin
	visit annot;
      annotation_fun baseClass.poly_base;
      compound_info_fun baseClass.compound;
      accessKeyword_fun baseClass.bc_access;
      bool_fun baseClass.is_virtual
    end


and compound_info_fun info = 
  let annot = compound_info_annotation info
  in
    if not (visited annot) &&
      node_check_fails annot info [^ annotated compound_info ^] "compound_info"
    then begin
	visit annot;
      annotation_fun info.compound_info_poly;
      string_fun info.compound_name;
      variable_fun info.typedef_var;
      accessKeyword_fun info.ci_access;
      bool_fun info.is_forward_decl;
      compoundType_Keyword_fun info.keyword;
      List.iter variable_fun info.data_members;
      List.iter baseClass_fun info.bases;
      List.iter variable_fun info.conversion_operators;
      List.iter variable_fun info.friends;
      string_fun info.inst_name;

      (* POSSIBLY CIRCULAR *)
      opt_iter cType_fun !(info.self_type)
    end


and atomicType_fun x = 
  let annot = atomicType_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated atomicType ^] "atomicType_type"
    then match x with
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
	  compound_info_fun compound_info;
	  List.iter sTemplateArgument_fun sTemplateArgument_list

      | EnumType(annot, string, variable, accessKeyword, 
		 string_nativeint_list) ->
	  visit annot;
	  annotation_fun annot;
	  string_fun string;
	  variable_fun variable;
	  accessKeyword_fun accessKeyword;
	  List.iter (fun (string, nativeint) -> 
		       (string_fun string; nativeint_fun nativeint))
	    string_nativeint_list

      | TypeVariable(annot, string, variable, accessKeyword) ->
	  visit annot;
	  annotation_fun annot;
	  string_fun string;
	  variable_fun variable;
	  accessKeyword_fun accessKeyword


and cType_fun x = 
  let annot = cType_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated cType ^] "cType"
    then
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
		   | TypeVariable _ -> true);
	  annotation_fun annot;
	  atomicType_fun atomicType;
	  cVFlags_fun cVFlags;
	  cType_fun cType


and sTemplateArgument_fun ta = 
  let annot = sTemplateArgument_annotation ta
  in
    if not (visited annot) &&
      node_check_fails annot ta [^ annotated sTemplateArgument ^] "sTemplateArgument"
    then 
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

	| STA_TEMPLATE annot -> 
	    annotation_fun annot

	| STA_ATOMIC(annot, atomicType) -> 
	    annotation_fun annot;
	    atomicType_fun atomicType



(***************** generated ast nodes ****************)

and translationUnit_fun 
    ((annot, topForm_list) as x : annotated translationUnit_type) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated translationUnit_type ^] 
    "translationUnit_type"
  then begin
    visit annot;
    annotation_fun annot;
    List.iter topForm_fun topForm_list
  end


and topForm_fun x = 
  let annot = topForm_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated topForm_type ^] "topForm_type"
    then
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
	     s_compound_opt, handler_list, statement_opt, bool) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated function_type ^] "function_type"
  then begin
    assert(match s_compound_opt with
	     | None -> true
	     | Some s_compound ->
		 match s_compound with 
		   | S_compound _ -> true 
		   | _ -> false);
    visit annot;
    annotation_fun annot;
    declFlags_fun declFlags;
    typeSpecifier_fun typeSpecifier;
    declarator_fun declarator;
    List.iter memberInit_fun memberInit_list;
    opt_iter statement_fun s_compound_opt;
    List.iter handler_fun handler_list;
    opt_iter statement_fun statement_opt;
    bool_fun bool
  end


and memberInit_fun((annot, pQName, argExpression_list, statement_opt) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated memberInit_type ^] "memberInit_type"
  then begin
    visit annot;
    annotation_fun annot;
    pQName_fun pQName;
    List.iter argExpression_fun argExpression_list;
    opt_iter statement_fun statement_opt
  end


and declaration_fun((annot, declFlags, typeSpecifier, declarator_list) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated declaration_type ^] "declaration_type"
  then begin
    visit annot;
    annotation_fun annot;
    declFlags_fun declFlags;
    typeSpecifier_fun typeSpecifier;
    List.iter declarator_fun declarator_list
  end


and aSTTypeId_fun((annot, typeSpecifier, declarator) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated aSTTypeId_type ^] "aSTTypeId_type"
  then begin
    visit annot;
    annotation_fun annot;
    typeSpecifier_fun typeSpecifier;
    declarator_fun declarator
  end


and pQName_fun x = 
  let annot = pQName_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated pQName_type ^] "pQName_type"
    then
      let _ = visit annot 
      in match x with
	| PQ_qualifier(annot, sourceLoc, stringRef_opt, 
		       templateArgument_opt, pQName) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    opt_iter string_fun stringRef_opt;
	    opt_iter templateArgument_fun templateArgument_opt;
	    pQName_fun pQName

	| PQ_name(annot, sourceLoc, stringRef) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef

	| PQ_operator(annot, sourceLoc, operatorName, stringRef) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    operatorName_fun operatorName;
	    string_fun stringRef

	| PQ_template(annot, sourceLoc, stringRef, templateArgument_opt) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef;
	    opt_iter templateArgument_fun templateArgument_opt

	| PQ_variable(annot, sourceLoc, variable) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    variable_fun variable



and typeSpecifier_fun x = 
  let annot = typeSpecifier_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated typeSpecifier_type ^] "typeSpecifier_type"
    then
      let _ = visit annot 
      in match x with
	| TS_name(annot, sourceLoc, cVFlags, pQName, bool) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    cVFlags_fun cVFlags;
	    pQName_fun pQName;
	    bool_fun bool

	| TS_simple(annot, sourceLoc, cVFlags, simpleTypeId) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    cVFlags_fun cVFlags;
	    simpleTypeId_fun simpleTypeId

	| TS_elaborated(annot, sourceLoc, cVFlags, typeIntr, pQName) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    cVFlags_fun cVFlags;
	    typeIntr_fun typeIntr;
	    pQName_fun pQName

	| TS_classSpec(annot, sourceLoc, cVFlags, typeIntr, pQName_opt, 
		       baseClassSpec_list, memberList) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    cVFlags_fun cVFlags;
	    typeIntr_fun typeIntr;
	    Printf.printf "begin ts classspec opt\n%!";
	    opt_iter pQName_fun pQName_opt;
	    Printf.printf "end ts classspec opt\n%!";
	    List.iter baseClassSpec_fun baseClassSpec_list;
	    memberList_fun memberList      

	| TS_enumSpec(annot, sourceLoc, cVFlags, 
		      stringRef_opt, enumerator_list) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    cVFlags_fun cVFlags;
	    opt_iter string_fun stringRef_opt;
	    List.iter enumerator_fun enumerator_list

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


and baseClassSpec_fun((annot, bool, accessKeyword, pQName) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated baseClassSpec_type ^] "baseClassSpec_type"
  then begin
    visit annot;
    annotation_fun annot;
    bool_fun bool;
    accessKeyword_fun accessKeyword;
    pQName_fun pQName
  end


and enumerator_fun((annot, sourceLoc, stringRef, expression_opt) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated enumerator_type ^] "enumerator_type"
  then begin
    visit annot;
    annotation_fun annot;
    sourceLoc_fun sourceLoc;
    string_fun stringRef;
    opt_iter expression_fun expression_opt
  end


and memberList_fun((annot, member_list) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated memberList_type ^] "memberList_type"
  then begin
    visit annot;
    annotation_fun annot;
    List.iter member_fun member_list
  end


and member_fun x = 
  let annot = member_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated member_type ^] "member_type"
    then
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
		   statement_opt_ctor, statement_opt_dtor) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated declarator_type ^] "declarator_type"
  then begin
    visit annot;
    annotation_fun annot;
    iDeclarator_fun iDeclarator;
    opt_iter init_fun init_opt;
    opt_iter statement_fun statement_opt_ctor;
    opt_iter statement_fun statement_opt_dtor
  end


and iDeclarator_fun x = 
  let annot = iDeclarator_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated iDeclarator_type ^] "iDeclarator_type"
    then
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
		 exceptionSpec_opt, pq_name_list) -> 
	    assert(List.for_all (function | PQ_name _ -> true | _ -> false) 
		     pq_name_list);
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    iDeclarator_fun iDeclarator;
	    List.iter aSTTypeId_fun aSTTypeId_list;
	    cVFlags_fun cVFlags;
	    opt_iter exceptionSpec_fun exceptionSpec_opt;
	    List.iter pQName_fun pq_name_list

	| D_array(annot, sourceLoc, iDeclarator, expression_opt) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    iDeclarator_fun iDeclarator;
	    opt_iter expression_fun expression_opt

	| D_bitfield(annot, sourceLoc, pQName_opt, expression) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    opt_iter pQName_fun pQName_opt;
	    expression_fun expression

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
  if not (visited annot) &&
    node_check_fails annot x [^ annotated exceptionSpec_type ^] "exceptionSpec_type"
  then begin
    visit annot;
    annotation_fun annot;
    List.iter aSTTypeId_fun aSTTypeId_list
  end


and operatorName_fun x = 
  let annot = operatorName_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated operatorName_type ^] "operatorName_type"
    then
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
    if not (visited annot) &&
      node_check_fails annot x [^ annotated statement_type ^] "statement_type"
    then
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

	| S_case(annot, sourceLoc, expression, statement) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    expression_fun expression;
	    statement_fun statement

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
		      expression_lo, expression_hi, statement) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    expression_fun expression_lo;
	    expression_fun expression_hi;
	    statement_fun statement

	| S_computedGoto(annot, sourceLoc, expression) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    expression_fun expression


and condition_fun x = 
  let annot = condition_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated condition_type ^] "condition_type"
    then
      let _ = visit annot 
      in match x with
	| CN_expr(annot, fullExpression) -> 
	    annotation_fun annot;
	    fullExpression_fun fullExpression

	| CN_decl(annot, aSTTypeId) -> 
	    annotation_fun annot;
	    aSTTypeId_fun aSTTypeId


and handler_fun((annot, aSTTypeId, statement_body, 
		expression_opt, statement_gdtor) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated handler_type ^] "handler_type"
  then begin
    visit annot;
    annotation_fun annot;
    aSTTypeId_fun aSTTypeId;
    statement_fun statement_body;
    opt_iter expression_fun expression_opt;
    opt_iter statement_fun statement_gdtor
  end


and expression_fun x = 
  let annot = expression_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated expression_type ^] "expression_type"
    then
      let _ = visit annot 
      in match x with
	| E_boolLit(annot, bool) -> 
	    annotation_fun annot;
	    bool_fun bool

	| E_intLit(annot, stringRef) -> 
	    annotation_fun annot;
	    string_fun stringRef

	| E_floatLit(annot, stringRef) -> 
	    annotation_fun annot;
	    string_fun stringRef

	| E_stringLit(annot, stringRef, e_stringLit_opt) -> 
	    assert(match e_stringLit_opt with 
		     | Some(E_stringLit _) -> true 
		     | None -> true
		     | _ -> false);
	    annotation_fun annot;
	    string_fun stringRef;
	    opt_iter expression_fun e_stringLit_opt

	| E_charLit(annot, stringRef) -> 
	    annotation_fun annot;
	    string_fun stringRef

	| E_this annot -> 
	    annotation_fun annot

	| E_variable(annot, pQName) -> 
	    annotation_fun annot;
	    pQName_fun pQName

	| E_funCall(annot, expression_func, 
		    argExpression_list, expression_retobj_opt) -> 
	    annotation_fun annot;
	    expression_fun expression_func;
	    List.iter argExpression_fun argExpression_list;
	    opt_iter expression_fun expression_retobj_opt

	| E_constructor(annot, typeSpecifier, argExpression_list, 
			bool, expression_opt) -> 
	    annotation_fun annot;
	    typeSpecifier_fun typeSpecifier;
	    List.iter argExpression_fun argExpression_list;
	    bool_fun bool;
	    opt_iter expression_fun expression_opt

	| E_fieldAcc(annot, expression, pQName) -> 
	    annotation_fun annot;
	    expression_fun expression;
	    pQName_fun pQName

	| E_sizeof(annot, expression) -> 
	    annotation_fun annot;
	    expression_fun expression

	| E_unary(annot, unaryOp, expression) -> 
	    annotation_fun annot;
	    unaryOp_fun unaryOp;
	    expression_fun expression

	| E_effect(annot, effectOp, expression) -> 
	    annotation_fun annot;
	    effectOp_fun effectOp;
	    expression_fun expression

	| E_binary(annot, expression_left, binaryOp, expression_right) -> 
	    annotation_fun annot;
	    expression_fun expression_left;
	    binaryOp_fun binaryOp;
	    expression_fun expression_right

	| E_addrOf(annot, expression) -> 
	    annotation_fun annot;
	    expression_fun expression

	| E_deref(annot, expression) -> 
	    annotation_fun annot;
	    expression_fun expression

	| E_cast(annot, aSTTypeId, expression) -> 
	    annotation_fun annot;
	    aSTTypeId_fun aSTTypeId;
	    expression_fun expression

	| E_cond(annot, expression_cond, expression_true, expression_false) -> 
	    annotation_fun annot;
	    expression_fun expression_cond;
	    expression_fun expression_true;
	    expression_fun expression_false

	| E_sizeofType(annot, aSTTypeId) -> 
	    annotation_fun annot;
	    aSTTypeId_fun aSTTypeId

	| E_assign(annot, expression_target, binaryOp, expression_src) -> 
	    annotation_fun annot;
	    expression_fun expression_target;
	    binaryOp_fun binaryOp;
	    expression_fun expression_src

	| E_new(annot, bool, argExpression_list, aSTTypeId, 
		argExpressionListOpt_opt, statement_opt) -> 
	    annotation_fun annot;
	    bool_fun bool;
	    List.iter argExpression_fun argExpression_list;
	    aSTTypeId_fun aSTTypeId;
	    opt_iter argExpressionListOpt_fun argExpressionListOpt_opt;
	    opt_iter statement_fun statement_opt

	| E_delete(annot, bool_colon, bool_array, 
		   expression_opt, statement_opt) -> 
	    annotation_fun annot;
	    bool_fun bool_colon;
	    bool_fun bool_array;
	    opt_iter expression_fun expression_opt;
	    opt_iter statement_fun statement_opt

	| E_throw(annot, expression_opt, statement_opt) -> 
	    annotation_fun annot;
	    opt_iter expression_fun expression_opt;
	    opt_iter statement_fun statement_opt

	| E_keywordCast(annot, castKeyword, aSTTypeId, expression) -> 
	    annotation_fun annot;
	    castKeyword_fun castKeyword;
	    aSTTypeId_fun aSTTypeId;
	    expression_fun expression

	| E_typeidExpr(annot, expression) -> 
	    annotation_fun annot;
	    expression_fun expression

	| E_typeidType(annot, aSTTypeId) -> 
	    annotation_fun annot;
	    aSTTypeId_fun aSTTypeId

	| E_grouping(annot, expression) -> 
	    annotation_fun annot;
	    expression_fun expression

	| E_arrow(annot, expression, pQName) -> 
	    annotation_fun annot;
	    expression_fun expression;
	    pQName_fun pQName

	| E_statement(annot, s_compound) -> 
	    assert(match s_compound with | S_compound _ -> true | _ -> false);
	    annotation_fun annot;
	    statement_fun s_compound

	| E_compoundLit(annot, aSTTypeId, in_compound) -> 
	    assert(match in_compound with | IN_compound _ -> true | _ -> false);
	    annotation_fun annot;
	    aSTTypeId_fun aSTTypeId;
	    init_fun in_compound

	| E___builtin_constant_p(annot, sourceLoc, expression) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    expression_fun expression

	| E___builtin_va_arg(annot, sourceLoc, expression, aSTTypeId) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    expression_fun expression;
	    aSTTypeId_fun aSTTypeId

	| E_alignofType(annot, aSTTypeId) -> 
	    annotation_fun annot;
	    aSTTypeId_fun aSTTypeId

	| E_alignofExpr(annot, expression) -> 
	    annotation_fun annot;
	    expression_fun expression

	| E_gnuCond(annot, expression_cond, expression_false) -> 
	    annotation_fun annot;
	    expression_fun expression_cond;
	    expression_fun expression_false

	| E_addrOfLabel(annot, stringRef) -> 
	    annotation_fun annot;
	    string_fun stringRef


and fullExpression_fun((annot, expression_opt) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated fullExpression_type ^] 
    "fullExpression_type"
  then begin
    visit annot;
    annotation_fun annot;
    opt_iter expression_fun expression_opt
  end


and argExpression_fun((annot, expression) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated argExpression_type ^] "argExpression_type"
  then begin
    visit annot;
    annotation_fun annot;
    expression_fun expression
  end


and argExpressionListOpt_fun((annot, argExpression_list) as x) =
  if not (visited annot) &&
    node_check_fails annot x [^ annotated argExpressionListOpt_type ^] 
    "argExpressionListOpt_type"
  then begin
    visit annot;
    annotation_fun annot;
    List.iter argExpression_fun argExpression_list
  end


and init_fun x = 
  let annot = init_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated initializer_type ^] "initializer_type"
    then
      let _ = visit annot 
      in match x with
	| IN_expr(annot, sourceLoc, expression) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    expression_fun expression

	| IN_compound(annot, sourceLoc, init_list) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    List.iter init_fun init_list

	| IN_ctor(annot, sourceLoc, argExpression_list, bool) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    List.iter argExpression_fun argExpression_list;
	    bool_fun bool

	| IN_designated(annot, sourceLoc, designator_list, init) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    List.iter designator_fun designator_list;
	    init_fun init


and templateDeclaration_fun x = 
  let annot = templateDeclaration_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated templateDeclaration_type ^] 
      "templateDeclaration_type"
    then
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
    if not (visited annot) &&
      node_check_fails annot x [^ annotated templateParameter_type ^] 
      "templateParameter_type"
    then
      let _ = visit annot 
      in match x with
	| TP_type(annot, sourceLoc, stringRef, 
		  aSTTypeId_opt, templateParameter_opt) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef;
	    opt_iter aSTTypeId_fun aSTTypeId_opt;
	    opt_iter templateParameter_fun templateParameter_opt

	| TP_nontype(annot, sourceLoc, aSTTypeId, templateParameter_opt) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    aSTTypeId_fun aSTTypeId;
	    opt_iter templateParameter_fun templateParameter_opt


and templateArgument_fun x = 
  let annot = templateArgument_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated templateArgument_type ^] 
      "templateArgument_type"
    then
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
    if not (visited annot) &&
      node_check_fails annot x [^ annotated namespaceDecl_type ^] 
      "namespaceDecl_type"
    then
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
  if not (visited annot) &&
    node_check_fails annot x [^ annotated fullExpressionAnnot_type ^] 
    "fullExpressionAnnot_type"
  then begin
    visit annot;
    List.iter declaration_fun declaration_list
  end


and aSTTypeof_fun x = 
  let annot = aSTTypeof_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated aSTTypeof_type ^] "aSTTypeof_type"
    then
      let _ = visit annot 
      in match x with
	| TS_typeof_expr(annot, fullExpression) -> 
	    annotation_fun annot;
	    fullExpression_fun fullExpression

	| TS_typeof_type(annot, aSTTypeId) -> 
	    annotation_fun annot;
	    aSTTypeId_fun aSTTypeId


and designator_fun x = 
  let annot = designator_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated designator_type ^] "designator_type"
    then
      let _ = visit annot 
      in match x with
	| FieldDesignator(annot, sourceLoc, stringRef) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef

	| SubscriptDesignator(annot, sourceLoc, expression, expression_opt) -> 
	    annotation_fun annot;
	    sourceLoc_fun sourceLoc;
	    expression_fun expression;
	    opt_iter expression_fun expression_opt


and attribute_fun x = 
  let annot = attribute_annotation x
  in
    if not (visited annot) &&
      node_check_fails annot x [^ annotated attribute_type ^] "attribute_type"
    then
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
    (* 
     * if SafeUnmarshal.check ast [^ annotated translationUnit_type ^]
     * then begin
     * 	print_string "passed\n";
     * 	exit 0;
     *   end
     * else begin
     * 	print_string "failed\n";
     * 	if !check_inside then begin
     *)
            print_endline "Descend into ast:";
	    translationUnit_fun ast;
      (* 
       * 	  end;
       * 	exit 1;
       * end
       *)

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
