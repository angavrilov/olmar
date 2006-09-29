
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
 *)

open Cc_ast_gen_type
open Ml_ctype
open Ast_annotation
open Ast_util


module DS = Dense_set

let visited_nodes = DS.make ()

let count_nodes = ref 0

let max_node_id = ref 0

let visited (annot : annotated) =
  DS.mem (id_annotation annot) visited_nodes

let visit (annot : annotated) =
  DS.add (id_annotation annot) visited_nodes;
  incr count_nodes;
  if id_annotation annot > !max_node_id then
    max_node_id := id_annotation annot


(**************************************************************************
 *
 * contents of astiter.ml
 *
 **************************************************************************)


(* let annotation_fun ((id, c_addr) : int * int) = () *)

let opt_iter f = function
  | None -> ()
  | Some x -> f x

let count_bool = ref 0
let bool_fun (b : bool) = incr count_bool

let count_int = ref 0
let int_fun (i : int) = incr count_int

let count_nativeint = ref 0
let nativeint_fun (i : nativeint) = incr count_nativeint

let count_string = ref 0
let string_fun (s : string) = incr count_nativeint

let count_sourceLoc = ref 0
let sourceLoc_fun((file : string), (line : int), (char : int)) = 
  incr count_sourceLoc;
  incr count_string;
  incr count_int;
  incr count_int

(* 
 * let declFlags_fun(l : declFlag list) = ()
 * 
 * let simpleTypeId_fun(id : simpleTypeId) = ()
 * 
 * let typeIntr_fun(keyword : typeIntr) = ()
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
 *)


(***************** variable ***************************)

let rec variable_fun(v : annotated variable) =
  let annot = variable_annotation v
  in
    if visited annot then ()
    else begin
      visit annot;

      sourceLoc_fun v.loc;
      opt_iter string_fun v.var_name;

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
      bool_fun baseClass.is_virtual
    end


and compound_info_fun info = 
  let annot = compound_info_annotation info
  in
    if visited annot then ()
    else begin
	visit annot;
      opt_iter string_fun info.compound_name;
      variable_fun info.typedef_var;
      bool_fun info.is_forward_decl;
      List.iter variable_fun info.data_members;
      List.iter baseClass_fun info.bases;
      List.iter variable_fun info.conversion_operators;
      List.iter variable_fun info.friends;
      opt_iter string_fun info.inst_name;

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
	  string_fun str;
	  opt_iter variable_fun variable_opt;
	  compound_info_fun compound_info;
	  List.iter sTemplateArgument_fun sTemplateArgument_list

      | EnumType(annot, string, variable, accessKeyword, 
		 string_nativeint_list) ->
	  visit annot;
	  opt_iter string_fun string;
	  opt_iter variable_fun variable;
	  List.iter (fun (string, nativeint) -> 
		       (string_fun string; nativeint_fun nativeint))
	    string_nativeint_list

      | TypeVariable(annot, string, variable, accessKeyword) ->
	  visit annot;
	  string_fun string;
	  variable_fun variable;

      | DependentQType(annot, string, variable, 
		      accessKeyword, atomic, pq_name) ->
	  visit annot;
	  string_fun string;
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
	    int_fun int

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
	    sourceLoc_fun sourceLoc;
	    declaration_fun declaration

	| TF_func(annot, sourceLoc, func) -> 
	    sourceLoc_fun sourceLoc;
	    func_fun func

	| TF_template(annot, sourceLoc, templateDeclaration) -> 
	    sourceLoc_fun sourceLoc;
	    templateDeclaration_fun templateDeclaration

	| TF_explicitInst(annot, sourceLoc, declFlags, declaration) -> 
	    sourceLoc_fun sourceLoc;
	    declaration_fun declaration

	| TF_linkage(annot, sourceLoc, stringRef, translationUnit) -> 
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef;
	    translationUnit_fun translationUnit

	| TF_one_linkage(annot, sourceLoc, stringRef, topForm) -> 
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef;
	    topForm_fun topForm

	| TF_asm(annot, sourceLoc, e_stringLit) -> 
	    assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
	    sourceLoc_fun sourceLoc;
	    expression_fun e_stringLit

	| TF_namespaceDefn(annot, sourceLoc, stringRef_opt, topForm_list) -> 
	    sourceLoc_fun sourceLoc;
	    opt_iter string_fun stringRef_opt;
	    List.iter topForm_fun topForm_list

	| TF_namespaceDecl(annot, sourceLoc, namespaceDecl) -> 
	    sourceLoc_fun sourceLoc;
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
    bool_fun bool
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
	    sourceLoc_fun sourceLoc;
	    opt_iter string_fun stringRef_opt;
	    opt_iter templateArgument_fun templateArgument_opt;
	    pQName_fun pQName;
	    opt_iter variable_fun variable_opt;
	    List.iter sTemplateArgument_fun s_template_arg_list

	| PQ_name(annot, sourceLoc, stringRef) -> 
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef

	| PQ_operator(annot, sourceLoc, operatorName, stringRef) -> 
	    sourceLoc_fun sourceLoc;
	    operatorName_fun operatorName;
	    string_fun stringRef

	| PQ_template(annot, sourceLoc, stringRef, templateArgument_opt, 
		     s_template_arg_list) -> 
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef;
	    opt_iter templateArgument_fun templateArgument_opt;
	    List.iter sTemplateArgument_fun s_template_arg_list

	| PQ_variable(annot, sourceLoc, variable) -> 
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
	    sourceLoc_fun sourceLoc;
	    pQName_fun pQName;
	    bool_fun bool;
	    opt_iter variable_fun var_opt_1;
	    opt_iter variable_fun var_opt_2

	| TS_simple(annot, sourceLoc, cVFlags, simpleTypeId) -> 
	    sourceLoc_fun sourceLoc;

	| TS_elaborated(annot, sourceLoc, cVFlags, typeIntr, 
		       pQName, namedAtomicType_opt) -> 
	    assert(match namedAtomicType_opt with
	      | Some(SimpleType _) -> false
	      | _ -> true);
	    sourceLoc_fun sourceLoc;
	    pQName_fun pQName;
	    opt_iter atomicType_fun namedAtomicType_opt

	| TS_classSpec(annot, sourceLoc, cVFlags, typeIntr, pQName_opt, 
		       baseClassSpec_list, memberList, compoundType) -> 
	    assert(match compoundType with
	      | CompoundType _ -> true
	      | _ -> false);
	    sourceLoc_fun sourceLoc;
	    opt_iter pQName_fun pQName_opt;
	    List.iter baseClassSpec_fun baseClassSpec_list;
	    memberList_fun memberList;
	    atomicType_fun compoundType

	| TS_enumSpec(annot, sourceLoc, cVFlags, 
		      stringRef_opt, enumerator_list, enumType) -> 
	    assert(match enumType with 
	      | EnumType _ -> true
	      | _ -> false);
	    sourceLoc_fun sourceLoc;
	    opt_iter string_fun stringRef_opt;
	    List.iter enumerator_fun enumerator_list;
	    atomicType_fun enumType

	| TS_type(annot, sourceLoc, cVFlags, cType) -> 
	    sourceLoc_fun sourceLoc;
	    cType_fun cType

	| TS_typeof(annot, sourceLoc, cVFlags, aSTTypeof) -> 
	    sourceLoc_fun sourceLoc;
	    aSTTypeof_fun aSTTypeof


and baseClassSpec_fun
    ((annot, bool, accessKeyword, pQName, compoundType_opt) ) =
  if visited annot then ()
  else begin
      assert(match compoundType_opt with
	| Some(CompoundType _ ) -> true
	| _ -> false);
    visit annot;
    bool_fun bool;
    pQName_fun pQName;
    opt_iter atomicType_fun compoundType_opt
  end


and enumerator_fun((annot, sourceLoc, stringRef, 
		   expression_opt, variable, int32) ) =
  if visited annot then ()
  else begin
    visit annot;
    sourceLoc_fun sourceLoc;
    string_fun stringRef;
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
	    sourceLoc_fun sourceLoc;
	    declaration_fun declaration

	| MR_func(annot, sourceLoc, func) -> 
	    sourceLoc_fun sourceLoc;
	    func_fun func

	| MR_access(annot, sourceLoc, accessKeyword) -> 
	    sourceLoc_fun sourceLoc;

	| MR_usingDecl(annot, sourceLoc, nd_usingDecl) -> 
	    assert(match nd_usingDecl with ND_usingDecl _ -> true | _ -> false);
	    sourceLoc_fun sourceLoc;
	    namespaceDecl_fun nd_usingDecl

	| MR_template(annot, sourceLoc, templateDeclaration) -> 
	    sourceLoc_fun sourceLoc;
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
	    sourceLoc_fun sourceLoc;
	    opt_iter pQName_fun pQName_opt

	| D_pointer(annot, sourceLoc, cVFlags, iDeclarator) -> 
	    sourceLoc_fun sourceLoc;
	    iDeclarator_fun iDeclarator

	| D_reference(annot, sourceLoc, iDeclarator) -> 
	    sourceLoc_fun sourceLoc;
	    iDeclarator_fun iDeclarator

	| D_func(annot, sourceLoc, iDeclarator, aSTTypeId_list, cVFlags, 
		 exceptionSpec_opt, pq_name_list, bool) -> 
	    assert(List.for_all (function | PQ_name _ -> true | _ -> false) 
		     pq_name_list);
	    sourceLoc_fun sourceLoc;
	    iDeclarator_fun iDeclarator;
	    List.iter aSTTypeId_fun aSTTypeId_list;
	    opt_iter exceptionSpec_fun exceptionSpec_opt;
	    List.iter pQName_fun pq_name_list;
	    bool_fun bool

	| D_array(annot, sourceLoc, iDeclarator, expression_opt, bool) -> 
	    sourceLoc_fun sourceLoc;
	    iDeclarator_fun iDeclarator;
	    opt_iter expression_fun expression_opt;
	    bool_fun bool

	| D_bitfield(annot, sourceLoc, pQName_opt, expression, int) -> 
	    sourceLoc_fun sourceLoc;
	    opt_iter pQName_fun pQName_opt;
	    expression_fun expression;
	    int_fun int

	| D_ptrToMember(annot, sourceLoc, pQName, cVFlags, iDeclarator) -> 
	    sourceLoc_fun sourceLoc;
	    pQName_fun pQName;
	    iDeclarator_fun iDeclarator

	| D_grouping(annot, sourceLoc, iDeclarator) -> 
	    sourceLoc_fun sourceLoc;
	    iDeclarator_fun iDeclarator

	| D_attribute(annot, sourceLoc, iDeclarator, attribute_list_list) ->
	    sourceLoc_fun sourceLoc;
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
	    bool_fun bool_is_new;
	    bool_fun bool_is_array

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
	    sourceLoc_fun sourceLoc

	| S_label(annot, sourceLoc, stringRef, statement) -> 
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef;
	    statement_fun statement

	| S_case(annot, sourceLoc, expression, statement, int) -> 
	    sourceLoc_fun sourceLoc;
	    expression_fun expression;
	    statement_fun statement;
	    int_fun int

	| S_default(annot, sourceLoc, statement) -> 
	    sourceLoc_fun sourceLoc;
	    statement_fun statement

	| S_expr(annot, sourceLoc, fullExpression) -> 
	    sourceLoc_fun sourceLoc;
	    fullExpression_fun fullExpression

	| S_compound(annot, sourceLoc, statement_list) -> 
	    sourceLoc_fun sourceLoc;
	    List.iter statement_fun statement_list

	| S_if(annot, sourceLoc, condition, statement_then, statement_else) -> 
	    sourceLoc_fun sourceLoc;
	    condition_fun condition;
	    statement_fun statement_then;
	    statement_fun statement_else

	| S_switch(annot, sourceLoc, condition, statement) -> 
	    sourceLoc_fun sourceLoc;
	    condition_fun condition;
	    statement_fun statement

	| S_while(annot, sourceLoc, condition, statement) -> 
	    sourceLoc_fun sourceLoc;
	    condition_fun condition;
	    statement_fun statement

	| S_doWhile(annot, sourceLoc, statement, fullExpression) -> 
	    sourceLoc_fun sourceLoc;
	    statement_fun statement;
	    fullExpression_fun fullExpression

	| S_for(annot, sourceLoc, statement_init, condition, fullExpression, 
		statement_body) -> 
	    sourceLoc_fun sourceLoc;
	    statement_fun statement_init;
	    condition_fun condition;
	    fullExpression_fun fullExpression;
	    statement_fun statement_body

	| S_break(annot, sourceLoc) -> 
	    sourceLoc_fun sourceLoc

	| S_continue(annot, sourceLoc) -> 
	    sourceLoc_fun sourceLoc

	| S_return(annot, sourceLoc, fullExpression_opt, statement_opt) -> 
	    sourceLoc_fun sourceLoc;
	    opt_iter fullExpression_fun fullExpression_opt;
	    opt_iter statement_fun statement_opt

	| S_goto(annot, sourceLoc, stringRef) -> 
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef

	| S_decl(annot, sourceLoc, declaration) -> 
	    sourceLoc_fun sourceLoc;
	    declaration_fun declaration

	| S_try(annot, sourceLoc, statement, handler_list) -> 
	    sourceLoc_fun sourceLoc;
	    statement_fun statement;
	    List.iter handler_fun handler_list

	| S_asm(annot, sourceLoc, e_stringLit) -> 
	    assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
	    sourceLoc_fun sourceLoc;
	    expression_fun e_stringLit

	| S_namespaceDecl(annot, sourceLoc, namespaceDecl) -> 
	    sourceLoc_fun sourceLoc;
	    namespaceDecl_fun namespaceDecl

	| S_function(annot, sourceLoc, func) -> 
	    sourceLoc_fun sourceLoc;
	    func_fun func

	| S_rangeCase(annot, sourceLoc, 
		      expression_lo, expression_hi, statement, 
		     label_lo, label_hi) -> 
	    sourceLoc_fun sourceLoc;
	    expression_fun expression_lo;
	    expression_fun expression_hi;
	    statement_fun statement;
	    int_fun label_lo;
	    int_fun label_hi

	| S_computedGoto(annot, sourceLoc, expression) -> 
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
	    bool_fun bool

	| E_intLit(annot, type_opt, stringRef, ulong) -> 
	    opt_iter cType_fun type_opt;
	    string_fun stringRef;

	| E_floatLit(annot, type_opt, stringRef, double) -> 
	    opt_iter cType_fun type_opt;
	    string_fun stringRef;

	| E_stringLit(annot, type_opt, stringRef, e_stringLit_opt) -> 
	    assert(match e_stringLit_opt with 
		     | Some(E_stringLit _) -> true 
		     | None -> true
		     | _ -> false);
	    opt_iter cType_fun type_opt;
	    string_fun stringRef;
	    opt_iter expression_fun e_stringLit_opt

	| E_charLit(annot, type_opt, stringRef, int32) -> 
	    opt_iter cType_fun type_opt;
	    string_fun stringRef;

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
	    bool_fun bool;
	    opt_iter expression_fun expression_opt

	| E_fieldAcc(annot, type_opt, expression, pQName, var_opt) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression;
	    pQName_fun pQName;
	    opt_iter variable_fun var_opt

	| E_sizeof(annot, type_opt, expression, int) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression;
	    int_fun int

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
	    bool_fun bool

	| E_cond(annot, type_opt, expression_cond, expression_true, expression_false) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression_cond;
	    expression_fun expression_true;
	    expression_fun expression_false

	| E_sizeofType(annot, type_opt, aSTTypeId, int, bool) -> 
	    opt_iter cType_fun type_opt;
	    aSTTypeId_fun aSTTypeId;
	    int_fun int;
	    bool_fun bool

	| E_assign(annot, type_opt, expression_target, binaryOp, expression_src) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression_target;
	    expression_fun expression_src

	| E_new(annot, type_opt, bool, argExpression_list, aSTTypeId, 
		argExpressionListOpt_opt, array_size_opt, ctor_opt,
	        statement_opt, heep_var) -> 
	    opt_iter cType_fun type_opt;
	    bool_fun bool;
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
	    bool_fun bool_colon;
	    bool_fun bool_array;
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
	    sourceLoc_fun sourceLoc;
	    expression_fun expression

	| E___builtin_va_arg(annot, type_opt, sourceLoc, expression, aSTTypeId) -> 
	    opt_iter cType_fun type_opt;
	    sourceLoc_fun sourceLoc;
	    expression_fun expression;
	    aSTTypeId_fun aSTTypeId

	| E_alignofType(annot, type_opt, aSTTypeId, int) -> 
	    opt_iter cType_fun type_opt;
	    aSTTypeId_fun aSTTypeId;
	    int_fun int

	| E_alignofExpr(annot, type_opt, expression, int) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression;
	    int_fun int

	| E_gnuCond(annot, type_opt, expression_cond, expression_false) -> 
	    opt_iter cType_fun type_opt;
	    expression_fun expression_cond;
	    expression_fun expression_false

	| E_addrOfLabel(annot, type_opt, stringRef) -> 
	    opt_iter cType_fun type_opt;
	    string_fun stringRef


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
	    sourceLoc_fun sourceLoc;
	    fullExpressionAnnot_fun fullExpressionAnnot;
	    expression_fun expression

	| IN_compound(annot, sourceLoc, fullExpressionAnnot, init_list) -> 
	    sourceLoc_fun sourceLoc;
	    fullExpressionAnnot_fun fullExpressionAnnot;
	    List.iter init_fun init_list

	| IN_ctor(annot, sourceLoc, fullExpressionAnnot, 
		 argExpression_list, var_opt, bool) -> 
	    sourceLoc_fun sourceLoc;
	    fullExpressionAnnot_fun fullExpressionAnnot;
	    List.iter argExpression_fun argExpression_list;
	    opt_iter variable_fun var_opt;
	    bool_fun bool

	| IN_designated(annot, sourceLoc, fullExpressionAnnot, 
		       designator_list, init) -> 
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
	    sourceLoc_fun sourceLoc;
	    variable_fun variable;
	    string_fun stringRef;
	    opt_iter aSTTypeId_fun aSTTypeId_opt;
	    opt_iter templateParameter_fun templateParameter_opt

	| TP_nontype(annot, sourceLoc, variable,
		    aSTTypeId, templateParameter_opt) -> 
	    sourceLoc_fun sourceLoc;
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
	    string_fun stringRef;
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
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef

	| SubscriptDesignator(annot, sourceLoc, expression, expression_opt, 
			     idx_start, idx_end) -> 
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
	    sourceLoc_fun sourceLoc

	| AT_word(annot, sourceLoc, stringRef) -> 
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef

	| AT_func(annot, sourceLoc, stringRef, argExpression_list) -> 
	    sourceLoc_fun sourceLoc;
	    string_fun stringRef;
	    List.iter argExpression_fun argExpression_list


(**************************************************************************
 *
 * end of astiter.ml 
 *
 **************************************************************************)


let file = ref ""

let file_set = ref false


let print_stats () =
  let missing_ids = ref []
  in
    Printf.printf "%s contains %d ast nodes with in total:\n" 
      !file !count_nodes;
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
    Printf.printf "maximal node id: %d\n" !max_node_id;
    for i = 1 to !max_node_id do
      if not (DS.mem i visited_nodes) then
	missing_ids := i :: !missing_ids
    done;
    missing_ids := List.rev !missing_ids;
    if !missing_ids = [] then
      Printf.printf "all node ids from 1 to %d present\n" !max_node_id
    else begin
	Printf.printf "missing node ids: %d" (List.hd !missing_ids);
	List.iter
	  (fun i -> Printf.printf ", %d" i)
	  (List.tl !missing_ids);
	print_endline "";
      end




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
  let ic = open_in !file in
  let ast = (Marshal.from_channel ic : annotated translationUnit_type)
  in
    translationUnit_fun ast;
    print_stats()
;;


Printexc.catch main ()


