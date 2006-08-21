
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

let opt_iter f = function
  | None -> ()
  | Some x -> f x

(***************** variable ***************************)

let rec variable_fun(v : annotated variable) =
  let annot = variable_annotation v
  in
    if visited annot then ()
    else begin

      (* POSSIBLY CIRCULAR *)
      opt_iter cType_fun !(v.var_type);
      opt_iter expression_fun v.value;
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
      compound_info_fun baseClass.compound;
    end


and compound_info_fun info = 
  let annot = compound_info_annotation info
  in
    if visited annot then ()
    else begin
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
      | SimpleType(annot, simpleTypeId) ->
	  ()

      | CompoundType(compound_info) ->
	  compound_info_fun compound_info

      | PseudoInstantiation(annot, str, variable_opt, accessKeyword, 
			    compound_info, sTemplateArgument_list) ->
	  opt_iter variable_fun variable_opt;
	  compound_info_fun compound_info;
	  List.iter sTemplateArgument_fun sTemplateArgument_list

      | EnumType(annot, string, variable, accessKeyword, string_int_list) ->
	  variable_fun variable;

      | TypeVariable(annot, string, variable, accessKeyword) ->
	  variable_fun variable;


and cType_fun x = 
  let annot = cType_annotation x
  in
    if visited annot then ()
    else match x with
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
		   | TypeVariable _ -> true);
	  atomicType_fun atomicType;
	  cType_fun cType


and sTemplateArgument_fun = function
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
    ((annot, topForm_list) : annotated translationUnit_type) =
  if visited annot then ()
  else begin
    List.iter topForm_fun topForm_list
  end


and topForm_fun x = 
  let annot = topForm_annotation x
  in
    if visited annot then ()
    else match x with
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



and func_fun(annot, declFlags, typeSpecifier, declarator, memberInit_list, 
	     s_compound_opt, handler_list, statement_opt, bool) =
  if visited annot then ()
  else begin
    assert(match s_compound_opt with
	     | None -> true
	     | Some s_compound ->
		 match s_compound with 
		   | S_compound _ -> true 
		   | _ -> false);
    typeSpecifier_fun typeSpecifier;
    declarator_fun declarator;
    List.iter memberInit_fun memberInit_list;
    opt_iter statement_fun s_compound_opt;
    List.iter handler_fun handler_list;
    opt_iter statement_fun statement_opt;
  end


and memberInit_fun(annot, pQName, argExpression_list, statement_opt) =
  if visited annot then ()
  else begin
    pQName_fun pQName;
    List.iter argExpression_fun argExpression_list;
    opt_iter statement_fun statement_opt
  end


and declaration_fun(annot, declFlags, typeSpecifier, declarator_list) =
  if visited annot then ()
  else begin
    typeSpecifier_fun typeSpecifier;
    List.iter declarator_fun declarator_list
  end


and aSTTypeId_fun(annot, typeSpecifier, declarator) =
  if visited annot then ()
  else begin
    typeSpecifier_fun typeSpecifier;
    declarator_fun declarator
  end


and pQName_fun x = 
  let annot = pQName_annotation x
  in
    if visited annot then ()
    else match x with
      | PQ_qualifier(annot, sourceLoc, stringRef_opt, 
		     templateArgument_opt, pQName) -> 
	  opt_iter templateArgument_fun templateArgument_opt;
	  pQName_fun pQName

      | PQ_name(annot, sourceLoc, stringRef) -> 
	  ()

      | PQ_operator(annot, sourceLoc, operatorName, stringRef) -> 
	  operatorName_fun operatorName;

      | PQ_template(annot, sourceLoc, stringRef, templateArgument_opt) -> 
	  opt_iter templateArgument_fun templateArgument_opt

      | PQ_variable(annot, sourceLoc, variable) -> 
	  variable_fun variable



and typeSpecifier_fun x = 
  let annot = typeSpecifier_annotation x
  in
    if visited annot then ()
    else match x with
      | TS_name(annot, sourceLoc, cVFlags, pQName, bool) -> 
	  pQName_fun pQName;

      | TS_simple(annot, sourceLoc, cVFlags, simpleTypeId) -> 
	  ()

      | TS_elaborated(annot, sourceLoc, cVFlags, typeIntr, pQName) -> 
	  pQName_fun pQName

      | TS_classSpec(annot, sourceLoc, cVFlags, typeIntr, pQName_opt, 
		     baseClassSpec_list, memberList) -> 
	  opt_iter pQName_fun pQName_opt;
	  List.iter baseClassSpec_fun baseClassSpec_list;
	  memberList_fun memberList      

      | TS_enumSpec(annot, sourceLoc, cVFlags, stringRef_opt, enumerator_list) -> 
	  List.iter enumerator_fun enumerator_list

      | TS_type(annot, sourceLoc, cVFlags, cType) -> 
	  cType_fun cType

      | TS_typeof(annot, sourceLoc, cVFlags, aSTTypeof) -> 
	  aSTTypeof_fun aSTTypeof


and baseClassSpec_fun(annot, bool, accessKeyword, pQName) =
  if visited annot then ()
  else begin
    pQName_fun pQName
  end


and enumerator_fun(annot, sourceLoc, stringRef, expression_opt) =
  if visited annot then ()
  else begin
    opt_iter expression_fun expression_opt
  end


and memberList_fun(annot, member_list) =
  if visited annot then ()
  else begin
    List.iter member_fun member_list
  end


and member_fun x = 
  let annot = member_annotation x
  in
    if visited annot then ()
    else match x with
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


and declarator_fun(annot, iDeclarator, init_opt, 
		   statement_opt_ctor, statement_opt_dtor) =
  if visited annot then ()
  else begin
    iDeclarator_fun iDeclarator;
    opt_iter init_fun init_opt;
    opt_iter statement_fun statement_opt_ctor;
    opt_iter statement_fun statement_opt_dtor
  end


and iDeclarator_fun x = 
  let annot = iDeclarator_annotation x
  in
    if visited annot then ()
    else match x with
      | D_name(annot, sourceLoc, pQName_opt) -> 
	  opt_iter pQName_fun pQName_opt

      | D_pointer(annot, sourceLoc, cVFlags, iDeclarator) -> 
	  iDeclarator_fun iDeclarator

      | D_reference(annot, sourceLoc, iDeclarator) -> 
	  iDeclarator_fun iDeclarator

      | D_func(annot, sourceLoc, iDeclarator, aSTTypeId_list, cVFlags, 
	       exceptionSpec_opt, pq_name_list) -> 
	  assert(List.for_all (function | PQ_name _ -> true | _ -> false) 
		   pq_name_list);
	  iDeclarator_fun iDeclarator;
	  List.iter aSTTypeId_fun aSTTypeId_list;
	  opt_iter exceptionSpec_fun exceptionSpec_opt;
	  List.iter pQName_fun pq_name_list

      | D_array(annot, sourceLoc, iDeclarator, expression_opt) -> 
	  iDeclarator_fun iDeclarator;
	  opt_iter expression_fun expression_opt

      | D_bitfield(annot, sourceLoc, pQName_opt, expression) -> 
	  opt_iter pQName_fun pQName_opt;
	  expression_fun expression

      | D_ptrToMember(annot, sourceLoc, pQName, cVFlags, iDeclarator) -> 
	  pQName_fun pQName;
	  iDeclarator_fun iDeclarator

      | D_grouping(annot, sourceLoc, iDeclarator) -> 
	  iDeclarator_fun iDeclarator

      | D_attribute(annot, sourceLoc, iDeclarator, attribute_list_list) ->
	  iDeclarator_fun iDeclarator;
	  List.iter (List.iter attribute_fun) attribute_list_list


and exceptionSpec_fun(annot, aSTTypeId_list) =
  if visited annot then ()
  else begin
    List.iter aSTTypeId_fun aSTTypeId_list
  end


and operatorName_fun x = 
  let annot = operatorName_annotation x
  in
    if visited annot then ()
    else match x with
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
    else match x with
      | S_skip(annot, sourceLoc) -> 
	  ()

      | S_label(annot, sourceLoc, stringRef, statement) -> 
	  statement_fun statement

      | S_case(annot, sourceLoc, expression, statement) -> 
	  expression_fun expression;
	  statement_fun statement

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
		    expression_lo, expression_hi, statement) -> 
	  expression_fun expression_lo;
	  expression_fun expression_hi;
	  statement_fun statement

      | S_computedGoto(annot, sourceLoc, expression) -> 
	  expression_fun expression


and condition_fun x = 
  let annot = condition_annotation x
  in
    if visited annot then ()
    else match x with
      | CN_expr(annot, fullExpression) -> 
	  fullExpression_fun fullExpression

      | CN_decl(annot, aSTTypeId) -> 
	  aSTTypeId_fun aSTTypeId


and handler_fun(annot, aSTTypeId, statement_body, 
		expression_opt, statement_gdtor) =
  if visited annot then ()
  else begin
    aSTTypeId_fun aSTTypeId;
    statement_fun statement_body;
    opt_iter expression_fun expression_opt;
    opt_iter statement_fun statement_gdtor
  end


and expression_fun x = 
  let annot = expression_annotation x
  in
    if visited annot then ()
    else match x with
      | E_boolLit(annot, bool) -> 
	  ()

      | E_intLit(annot, stringRef) -> 
	  ()

      | E_floatLit(annot, stringRef) -> 
	  ()

      | E_stringLit(annot, stringRef, e_stringLit_opt) -> 
	  assert(match e_stringLit_opt with 
		   | Some(E_stringLit _) -> true 
		   | None -> true
		   | _ -> false);
	  opt_iter expression_fun e_stringLit_opt

      | E_charLit(annot, stringRef) -> 
	  ()

      | E_this annot -> 
	  ()

      | E_variable(annot, pQName) -> 
	  pQName_fun pQName

      | E_funCall(annot, expression_func, 
		  argExpression_list, expression_retobj_opt) -> 
	  expression_fun expression_func;
	  List.iter argExpression_fun argExpression_list;
	  opt_iter expression_fun expression_retobj_opt

      | E_constructor(annot, typeSpecifier, argExpression_list, 
		      bool, expression_opt) -> 
	  typeSpecifier_fun typeSpecifier;
	  List.iter argExpression_fun argExpression_list;
	  opt_iter expression_fun expression_opt

      | E_fieldAcc(annot, expression, pQName) -> 
	  expression_fun expression;
	  pQName_fun pQName

      | E_sizeof(annot, expression) -> 
	  expression_fun expression

      | E_unary(annot, unaryOp, expression) -> 
	  expression_fun expression

      | E_effect(annot, effectOp, expression) -> 
	  expression_fun expression

      | E_binary(annot, expression_left, binaryOp, expression_right) -> 
	  expression_fun expression_left;
	  expression_fun expression_right

      | E_addrOf(annot, expression) -> 
	  expression_fun expression

      | E_deref(annot, expression) -> 
	  expression_fun expression

      | E_cast(annot, aSTTypeId, expression) -> 
	  aSTTypeId_fun aSTTypeId;
	  expression_fun expression

      | E_cond(annot, expression_cond, expression_true, expression_false) -> 
	  expression_fun expression_cond;
	  expression_fun expression_true;
	  expression_fun expression_false

      | E_sizeofType(annot, aSTTypeId) -> 
	  aSTTypeId_fun aSTTypeId

      | E_assign(annot, expression_target, binaryOp, expression_src) -> 
	  expression_fun expression_target;
	  expression_fun expression_src

      | E_new(annot, bool, argExpression_list, aSTTypeId, 
	      argExpressionListOpt_opt, statement_opt) -> 
	  List.iter argExpression_fun argExpression_list;
	  aSTTypeId_fun aSTTypeId;
	  opt_iter argExpressionListOpt_fun argExpressionListOpt_opt;
	  opt_iter statement_fun statement_opt

      | E_delete(annot, bool_colon, bool_array, 
		 expression_opt, statement_opt) -> 
	  opt_iter expression_fun expression_opt;
	  opt_iter statement_fun statement_opt

      | E_throw(annot, expression_opt, statement_opt) -> 
	  opt_iter expression_fun expression_opt;
	  opt_iter statement_fun statement_opt

      | E_keywordCast(annot, castKeyword, aSTTypeId, expression) -> 
	  aSTTypeId_fun aSTTypeId;
	  expression_fun expression

      | E_typeidExpr(annot, expression) -> 
	  expression_fun expression

      | E_typeidType(annot, aSTTypeId) -> 
	  aSTTypeId_fun aSTTypeId

      | E_grouping(annot, expression) -> 
	  expression_fun expression

      | E_arrow(annot, expression, pQName) -> 
	  expression_fun expression;
	  pQName_fun pQName

      | E_statement(annot, s_compound) -> 
	  assert(match s_compound with | S_compound _ -> true | _ -> false);
	  statement_fun s_compound

      | E_compoundLit(annot, aSTTypeId, in_compound) -> 
	  assert(match in_compound with | IN_compound _ -> true | _ -> false);
	  aSTTypeId_fun aSTTypeId;
	  init_fun in_compound

      | E___builtin_constant_p(annot, sourceLoc, expression) -> 
	  expression_fun expression

      | E___builtin_va_arg(annot, sourceLoc, expression, aSTTypeId) -> 
	  expression_fun expression;
	  aSTTypeId_fun aSTTypeId

      | E_alignofType(annot, aSTTypeId) -> 
	  aSTTypeId_fun aSTTypeId

      | E_alignofExpr(annot, expression) -> 
	  expression_fun expression

      | E_gnuCond(annot, expression_cond, expression_false) -> 
	  expression_fun expression_cond;
	  expression_fun expression_false

      | E_addrOfLabel(annot, stringRef) -> 
	  ()


and fullExpression_fun(annot, expression_opt) =
  if visited annot then ()
  else begin
    opt_iter expression_fun expression_opt
  end


and argExpression_fun(annot, expression) =
  if visited annot then ()
  else begin
    expression_fun expression
  end


and argExpressionListOpt_fun(annot, argExpression_list) =
  if visited annot then ()
  else begin
    List.iter argExpression_fun argExpression_list
  end


and init_fun x = 
  let annot = init_annotation x
  in
    if visited annot then ()
    else match x with
      | IN_expr(annot, sourceLoc, expression) -> 
	  expression_fun expression

      | IN_compound(annot, sourceLoc, init_list) -> 
	  List.iter init_fun init_list

      | IN_ctor(annot, sourceLoc, argExpression_list, bool) -> 
	  List.iter argExpression_fun argExpression_list;

      | IN_designated(annot, sourceLoc, designator_list, init) -> 
	  List.iter designator_fun designator_list;
	  init_fun init


and templateDeclaration_fun x = 
  let annot = templateDeclaration_annotation x
  in
    if visited annot then ()
    else match x with
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
    else match x with
      | TP_type(annot, sourceLoc, stringRef, 
		aSTTypeId_opt, templateParameter_opt) -> 
	  opt_iter aSTTypeId_fun aSTTypeId_opt;
	  opt_iter templateParameter_fun templateParameter_opt

      | TP_nontype(annot, sourceLoc, aSTTypeId, templateParameter_opt) -> 
	  aSTTypeId_fun aSTTypeId;
	  opt_iter templateParameter_fun templateParameter_opt


and templateArgument_fun x = 
  let annot = templateArgument_annotation x
  in
    if visited annot then ()
    else match x with
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
    else match x with
      | ND_alias(annot, stringRef, pQName) -> 
	  pQName_fun pQName

      | ND_usingDecl(annot, pQName) -> 
	  pQName_fun pQName

      | ND_usingDir(annot, pQName) -> 
	  pQName_fun pQName


and fullExpressionAnnot_fun(annot, declaration_list) =
  if visited annot then ()
  else begin
    List.iter declaration_fun declaration_list
  end


and aSTTypeof_fun x = 
  let annot = aSTTypeof_annotation x
  in
    if visited annot then ()
    else match x with
      | TS_typeof_expr(annot, fullExpression) -> 
	  fullExpression_fun fullExpression

      | TS_typeof_type(annot, aSTTypeId) -> 
	  aSTTypeId_fun aSTTypeId


and designator_fun x = 
  let annot = designator_annotation x
  in
    if visited annot then ()
    else match x with
      | FieldDesignator(annot, sourceLoc, stringRef) -> 
	  ()

      | SubscriptDesignator(annot, sourceLoc, expression, expression_opt) -> 
	  expression_fun expression;
	  opt_iter expression_fun expression_opt


and attributeSpecifierList_fun x = 
  let annot = attributeSpecifierList_annotation x
  in
    if visited annot then ()
    else match x with
      | AttributeSpecifierList_cons(annot, attributeSpecifier, 
				    attributeSpecifierList) -> 
	  attributeSpecifier_fun attributeSpecifier;
	  attributeSpecifierList_fun 
	    attributeSpecifierList


and attributeSpecifier_fun x = 
  let annot = attributeSpecifier_annotation x
  in
    if visited annot then ()
    else match x with
      | AttributeSpecifier_cons(annot, attribute, attributeSpecifier) -> 
	  attribute_fun attribute;
	  attributeSpecifier_fun attributeSpecifier


and attribute_fun x = 
  let annot = attribute_annotation x
  in
    if visited annot then ()
    else match x with
      | AT_empty(annot, sourceLoc) -> 
	  ()

      | AT_word(annot, sourceLoc, stringRef) -> 
	  ()

      | AT_func(annot, sourceLoc, stringRef, argExpression_list) -> 
	  List.iter argExpression_fun argExpression_list


(**************************************************************************
 *
 * end of astiter.ml 
 *
 **************************************************************************)


