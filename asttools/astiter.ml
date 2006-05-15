
open Cc_ml_types
open Cc_ast_gen_type

let opt_iter f = function
  | None -> ()
  | Some x -> f x

let bool_fun (b : bool) = ()

let string_fun (s : string) = ()

let sourceLoc_fun((file : string), (line : int), (char : int)) = ()

let declFlags_fun(l : declFlag list) = ()

let variable_fun(v : variable) = ()

let cType_fun(c : cType) = ()

let simpleTypeId_fun(id : simpleTypeId) = ()

let typeIntr_fun(keywort : typeIntr) = ()

let accessKeyword_fun(keyword : accessKeyword) = ()

let cVFlags_fun(fl : cVFlag list) = ()

let overloadableOp_fun(op :overloadableOp) = ()

let unaryOp_fun(op : unaryOp) = ()

let effectOp_fun(op : effectOp) = ()

let binaryOp_fun(op : binaryOp) = ()

let castKeyword_fun(keyword : castKeyword) = ()





let rec translationUnit_fun topForm_list =
  List.iter topForm_fun topForm_list


and topForm_fun = function
  | TF_decl(sourceLoc, declaration) -> 
      sourceLoc_fun sourceLoc;
      declaration_fun declaration

  | TF_func(sourceLoc, func) -> 
      sourceLoc_fun sourceLoc;
      func_fun func

  | TF_template(sourceLoc, templateDeclaration) -> 
      sourceLoc_fun sourceLoc;
      templateDeclaration_fun templateDeclaration

  | TF_explicitInst(sourceLoc, declFlags, declaration) -> 
      sourceLoc_fun sourceLoc;
      declFlags_fun declFlags;
      declaration_fun declaration

  | TF_linkage(sourceLoc, stringRef, translationUnit) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef;
      translationUnit_fun translationUnit

  | TF_one_linkage(sourceLoc, stringRef, topForm) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef;
      topForm_fun topForm

  | TF_asm(sourceLoc, e_stringLit) -> 
      sourceLoc_fun sourceLoc;
      assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
      expression_fun e_stringLit

  | TF_namespaceDefn(sourceLoc, stringRef_opt, topForm_list) -> 
      sourceLoc_fun sourceLoc;
      opt_iter string_fun stringRef_opt;
      List.iter topForm_fun topForm_list

  | TF_namespaceDecl(sourceLoc, namespaceDecl) -> 
      sourceLoc_fun sourceLoc;
      namespaceDecl_fun namespaceDecl



and func_fun(declFlags, typeSpecifier, declarator, memberInit_list, 
	 s_compound, handler_list, statement_opt, bool) =
  declFlags_fun declFlags;
  typeSpecifier_fun typeSpecifier;
  declarator_fun declarator;
  List.iter memberInit_fun memberInit_list;
  assert(match s_compound with | S_compound _ -> true | _ -> false);
  statement_fun s_compound;
  List.iter handler_fun handler_list;
  opt_iter statement_fun statement_opt;
  bool_fun bool



and memberInit_fun(pQName, argExpression_list, statement_opt) =
  pQName_fun pQName;
  List.iter argExpression_fun argExpression_list;
  opt_iter statement_fun statement_opt



and declaration_fun(declFlags, typeSpecifier, declarator_list) =
  declFlags_fun declFlags;
  typeSpecifier_fun typeSpecifier;
  List.iter declarator_fun declarator_list
  


and aSTTypeId_fun(typeSpecifier, declarator) =
  typeSpecifier_fun typeSpecifier;
  declarator_fun declarator



and pQName_fun = function
  | PQ_qualifier(sourceLoc, stringRef_opt, 
		 templateArgument_opt, pQName) -> 
      sourceLoc_fun sourceLoc;
      opt_iter string_fun stringRef_opt;
      opt_iter templateArgument_fun templateArgument_opt;
      pQName_fun pQName
      
  | PQ_name(sourceLoc, stringRef) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef;

  | PQ_operator(sourceLoc, operatorName, stringRef) -> 
      sourceLoc_fun sourceLoc;
      operatorName_fun operatorName;
      string_fun stringRef;

  | PQ_template(sourceLoc, stringRef, templateArgument_opt) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef;
      opt_iter templateArgument_fun templateArgument_opt

  | PQ_variable(sourceLoc, variable) -> 
      sourceLoc_fun sourceLoc;
      variable_fun variable



and typeSpecifier_fun = function
  | TS_name(sourceLoc, cVFlags, pQName, bool) -> 
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;
      pQName_fun pQName;
      bool_fun bool

  | TS_simple(sourceLoc, cVFlags, simpleTypeId) -> 
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;
      simpleTypeId_fun simpleTypeId

  | TS_elaborated(sourceLoc, cVFlags, typeIntr, pQName) -> 
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;
      typeIntr_fun typeIntr;
      pQName_fun pQName;

  | TS_classSpec(sourceLoc, cVFlags, typeIntr, pQName_opt, 
		 baseClassSpec_list, memberList) -> 
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;
      typeIntr_fun typeIntr;
      opt_iter pQName_fun pQName_opt;
      List.iter baseClassSpec_fun baseClassSpec_list;
      memberList_fun memberList      

  | TS_enumSpec(sourceLoc, cVFlags, stringRef_opt, enumerator_list) -> 
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;
      opt_iter string_fun stringRef_opt;
      List.iter enumerator_fun enumerator_list

  | TS_type(sourceLoc, cVFlags, cType) -> 
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;
      cType_fun cType

  | TS_typeof(sourceLoc, cVFlags, aSTTypeof) -> 
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;
      aSTTypeof_fun aSTTypeof


and baseClassSpec_fun(bool, accessKeyword, pQName) =
  bool_fun bool;
  accessKeyword_fun accessKeyword;
  pQName_fun pQName


and enumerator_fun(sourceLoc, stringRef, expression_opt) =
  sourceLoc_fun sourceLoc;
  string_fun stringRef;
  opt_iter expression_fun expression_opt


and memberList_fun(member_list) =
  List.iter member_fun member_list


and member_fun = function
  | MR_decl(sourceLoc, declaration) -> 
      sourceLoc_fun sourceLoc;
      declaration_fun declaration

  | MR_func(sourceLoc, func) -> 
      sourceLoc_fun sourceLoc;
      func_fun func

  | MR_access(sourceLoc, accessKeyword) -> 
      sourceLoc_fun sourceLoc;
      accessKeyword_fun accessKeyword

  | MR_usingDecl(sourceLoc, nd_usingDecl) -> 
      sourceLoc_fun sourceLoc;
      assert(match nd_usingDecl with ND_usingDecl _ -> true | _ -> false);
      namespaceDecl_fun nd_usingDecl

  | MR_template(sourceLoc, templateDeclaration) -> 
      sourceLoc_fun sourceLoc;
      templateDeclaration_fun templateDeclaration


and declarator_fun(iDeclarator, init_opt,
		   statement_opt_ctor, statement_opt_dtor) =
  iDeclarator_fun iDeclarator;
  opt_iter init_fun init_opt;
  opt_iter statement_fun statement_opt_ctor;
  opt_iter statement_fun statement_opt_dtor



and iDeclarator_fun = function
  | D_name(sourceLoc, pQName_opt) -> 
      sourceLoc_fun sourceLoc;
      opt_iter pQName_fun pQName_opt

  | D_pointer(sourceLoc, cVFlags, iDeclarator) -> 
      sourceLoc_fun sourceLoc;
      cVFlags_fun cVFlags;
      iDeclarator_fun iDeclarator

  | D_reference(sourceLoc, iDeclarator) -> 
      sourceLoc_fun sourceLoc;
      iDeclarator_fun iDeclarator;

  | D_func(sourceLoc, iDeclarator, aSTTypeId_list, cVFlags, 
	   exceptionSpec_opt, pq_name_list) -> 
      sourceLoc_fun sourceLoc;
      iDeclarator_fun iDeclarator;
      List.iter aSTTypeId_fun aSTTypeId_list;
      cVFlags_fun cVFlags;
      opt_iter exceptionSpec_fun exceptionSpec_opt;
      assert(List.for_all (function | PQ_name _ -> true | _ -> false) 
	       pq_name_list);
      List.iter pQName_fun pq_name_list

  | D_array(sourceLoc, iDeclarator, expression_opt) -> 
      sourceLoc_fun sourceLoc;
      iDeclarator_fun iDeclarator;
      opt_iter expression_fun expression_opt

  | D_bitfield(sourceLoc, pQName_opt, expression) -> 
      sourceLoc_fun sourceLoc;
      opt_iter pQName_fun pQName_opt;
      expression_fun expression

  | D_ptrToMember(sourceLoc, pQName, cVFlags, iDeclarator) -> 
      sourceLoc_fun sourceLoc;
      pQName_fun pQName;
      cVFlags_fun cVFlags;
      iDeclarator_fun iDeclarator;

  | D_grouping(sourceLoc, iDeclarator) -> 
      sourceLoc_fun sourceLoc;
      iDeclarator_fun iDeclarator;


and exceptionSpec_fun(aSTTypeId_list) =
  List.iter aSTTypeId_fun aSTTypeId_list


and operatorName_fun = function
  | ON_newDel(bool_is_new, bool_is_array) -> 
      bool_fun bool_is_new;
      bool_fun bool_is_array

  | ON_operator(overloadableOp) -> 
      overloadableOp_fun overloadableOp

  | ON_conversion(aSTTypeId) -> 
      aSTTypeId_fun aSTTypeId


and statement_fun = function
  | S_skip(sourceLoc) -> 
      sourceLoc_fun sourceLoc

  | S_label(sourceLoc, stringRef, statement) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef;
      statement_fun statement;      

  | S_case(sourceLoc, expression, statement) -> 
      sourceLoc_fun sourceLoc;
      expression_fun expression;
      statement_fun statement

  | S_default(sourceLoc, statement) -> 
      sourceLoc_fun sourceLoc;
      statement_fun statement

  | S_expr(sourceLoc, fullExpression) -> 
      sourceLoc_fun sourceLoc;
      fullExpression_fun fullExpression

  | S_compound(sourceLoc, statement_list) -> 
      sourceLoc_fun sourceLoc;
      List.iter statement_fun statement_list

  | S_if(sourceLoc, condition, statement_then, statement_else) -> 
      sourceLoc_fun sourceLoc;
      condition_fun condition;
      statement_fun statement_then;
      statement_fun statement_else

  | S_switch(sourceLoc, condition, statement) -> 
      sourceLoc_fun sourceLoc;
      condition_fun condition;
      statement_fun statement

  | S_while(sourceLoc, condition, statement) -> 
      sourceLoc_fun sourceLoc;
      condition_fun condition;
      statement_fun statement

  | S_doWhile(sourceLoc, statement, fullExpression) -> 
      sourceLoc_fun sourceLoc;
      statement_fun statement;
      fullExpression_fun fullExpression

  | S_for(sourceLoc, statement_init, condition, fullExpression, 
	  statement_body) -> 
      sourceLoc_fun sourceLoc;
      statement_fun statement_init;
      condition_fun condition;
      fullExpression_fun fullExpression;
      statement_fun statement_body

  | S_break(sourceLoc) -> 
      sourceLoc_fun sourceLoc

  | S_continue(sourceLoc) -> 
      sourceLoc_fun sourceLoc

  | S_return(sourceLoc, fullExpression_opt, statement_opt) -> 
      sourceLoc_fun sourceLoc;
      opt_iter fullExpression_fun fullExpression_opt;
      opt_iter statement_fun statement_opt

  | S_goto(sourceLoc, stringRef) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef;

  | S_decl(sourceLoc, declaration) -> 
      sourceLoc_fun sourceLoc;
      declaration_fun declaration

  | S_try(sourceLoc, statement, handler_list) -> 
      sourceLoc_fun sourceLoc;
      statement_fun statement;
      List.iter handler_fun handler_list

  | S_asm(sourceLoc, e_stringLit) -> 
      sourceLoc_fun sourceLoc;
      assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
      expression_fun e_stringLit

  | S_namespaceDecl(sourceLoc, namespaceDecl) -> 
      sourceLoc_fun sourceLoc;
      namespaceDecl_fun namespaceDecl

  | S_function(sourceLoc, func) -> 
      sourceLoc_fun sourceLoc;
      func_fun func

  | S_rangeCase(sourceLoc, expression_lo, expression_hi, statement) -> 
      sourceLoc_fun sourceLoc;
      expression_fun expression_lo;
      expression_fun expression_hi;
      statement_fun statement;

  | S_computedGoto(sourceLoc, expression) -> 
      sourceLoc_fun sourceLoc;
      expression_fun expression


and condition_fun = function
  | CN_expr(fullExpression) -> 
      fullExpression_fun fullExpression

  | CN_decl(aSTTypeId) -> 
      aSTTypeId_fun aSTTypeId


and handler_fun(aSTTypeId, statement_body, expression_opt, statement_gdtor) =
      aSTTypeId_fun aSTTypeId;
      statement_fun statement_body;
      opt_iter expression_fun expression_opt;
      opt_iter statement_fun statement_gdtor


and expression_fun = function
  | E_boolLit(bool) -> 
      bool_fun bool

  | E_intLit(stringRef) -> 
      string_fun stringRef

  | E_floatLit(stringRef) -> 
      string_fun stringRef

  | E_stringLit(stringRef, e_stringLit_opt) -> 
      string_fun stringRef;
      assert(match e_stringLit_opt with 
	       | Some(E_stringLit _) -> true 
	       | None -> true
	       | _ -> false);
      opt_iter expression_fun e_stringLit_opt

  | E_charLit(stringRef) -> 
      string_fun stringRef;

  | E_this -> ()

  | E_variable(pQName) -> 
      pQName_fun pQName

  | E_funCall(expression_func, argExpression_list, expression_retobj_opt) -> 
      expression_fun expression_func;
      List.iter argExpression_fun argExpression_list;
      opt_iter expression_fun expression_retobj_opt

  | E_constructor(typeSpecifier, argExpression_list, bool, expression_opt) -> 
      typeSpecifier_fun typeSpecifier;
      List.iter argExpression_fun argExpression_list;
      bool_fun bool;
      opt_iter expression_fun expression_opt

  | E_fieldAcc(expression, pQName) -> 
      expression_fun expression;
      pQName_fun pQName

  | E_sizeof(expression) -> 
      expression_fun expression

  | E_unary(unaryOp, expression) -> 
      unaryOp_fun unaryOp;
      expression_fun expression

  | E_effect(effectOp, expression) -> 
      effectOp_fun effectOp;
      expression_fun expression

  | E_binary(expression_left, binaryOp, expression_right) -> 
      expression_fun expression_left;
      binaryOp_fun binaryOp;
      expression_fun expression_right

  | E_addrOf(expression) -> 
      expression_fun expression

  | E_deref(expression) -> 
      expression_fun expression

  | E_cast(aSTTypeId, expression) -> 
      aSTTypeId_fun aSTTypeId;
      expression_fun expression

  | E_cond(expression_cond, expression_true, expression_false) -> 
      expression_fun expression_cond;
      expression_fun expression_true;
      expression_fun expression_false

  | E_sizeofType(aSTTypeId) -> 
      aSTTypeId_fun aSTTypeId

  | E_assign(expression_target, binaryOp, expression_src) -> 
      expression_fun expression_target;
      binaryOp_fun binaryOp;
      expression_fun expression_src

  | E_new(bool, argExpression_list, aSTTypeId, argExpressionListOpt_opt,
	  statement_opt) -> 
      bool_fun bool;
      List.iter argExpression_fun argExpression_list;
      aSTTypeId_fun aSTTypeId;
      opt_iter argExpressionListOpt_fun argExpressionListOpt_opt;
      opt_iter statement_fun statement_opt

  | E_delete(bool_colon, bool_array, expression_opt, statement_opt) -> 
      bool_fun bool_colon;
      bool_fun bool_array;
      opt_iter expression_fun expression_opt;
      opt_iter statement_fun statement_opt

  | E_throw(expression_opt, statement_opt) -> 
      opt_iter expression_fun expression_opt;
      opt_iter statement_fun statement_opt

  | E_keywordCast(castKeyword, aSTTypeId, expression) -> 
      castKeyword_fun castKeyword;
      aSTTypeId_fun aSTTypeId;
      expression_fun expression

  | E_typeidExpr(expression) -> 
      expression_fun expression

  | E_typeidType(aSTTypeId) -> 
      aSTTypeId_fun aSTTypeId

  | E_grouping(expression) -> 
      expression_fun expression

  | E_arrow(expression, pQName) -> 
      expression_fun expression;
      pQName_fun pQName

  | E_statement(s_compound) -> 
      assert(match s_compound with | S_compound _ -> true | _ -> false);
      statement_fun s_compound;

  | E_compoundLit(aSTTypeId, in_compound) -> 
      aSTTypeId_fun aSTTypeId;
      assert(match in_compound with | IN_compound _ -> true | _ -> false);
      init_fun in_compound

  | E___builtin_constant_p(sourceLoc, expression) -> 
      sourceLoc_fun sourceLoc;
      expression_fun expression

  | E___builtin_va_arg(sourceLoc, expression, aSTTypeId) -> 
      sourceLoc_fun sourceLoc;
      expression_fun expression;
      aSTTypeId_fun aSTTypeId

  | E_alignofType(aSTTypeId) -> 
      aSTTypeId_fun aSTTypeId

  | E_alignofExpr(expression) -> 
      expression_fun expression

  | E_gnuCond(expression_cond, expression_false) -> 
      expression_fun expression_cond;
      expression_fun expression_false

  | E_addrOfLabel(stringRef) -> 
      string_fun stringRef;


and fullExpression_fun(expression_opt) =
  opt_iter expression_fun expression_opt


and argExpression_fun(expression) =
  expression_fun expression


and argExpressionListOpt_fun(argExpression_list) =
  List.iter argExpression_fun argExpression_list


and init_fun = function
  | IN_expr(sourceLoc, expression) -> 
      sourceLoc_fun sourceLoc;
      expression_fun expression

  | IN_compound(sourceLoc, init_list) -> 
      sourceLoc_fun sourceLoc;
      List.iter init_fun init_list

  | IN_ctor(sourceLoc, argExpression_list, bool) -> 
      sourceLoc_fun sourceLoc;
      List.iter argExpression_fun argExpression_list;
      bool_fun bool

  | IN_designated(sourceLoc, designator_list, init) -> 
      sourceLoc_fun sourceLoc;
      List.iter designator_fun designator_list;
      init_fun init


and templateDeclaration_fun = function
  | TD_func(templateParameter_opt, func) -> 
      opt_iter templateParameter_fun templateParameter_opt;
      func_fun func

  | TD_decl(templateParameter_opt, declaration) -> 
      opt_iter templateParameter_fun templateParameter_opt;
      declaration_fun declaration

  | TD_tmember(templateParameter_opt, templateDeclaration) -> 
      opt_iter templateParameter_fun templateParameter_opt;
      templateDeclaration_fun templateDeclaration


and templateParameter_fun = function
  | TP_type(sourceLoc, stringRef, aSTTypeId_opt, templateParameter_opt) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef;
      opt_iter aSTTypeId_fun aSTTypeId_opt;
      opt_iter templateParameter_fun templateParameter_opt

  | TP_nontype(sourceLoc, aSTTypeId, templateParameter_opt) -> 
      sourceLoc_fun sourceLoc;
      aSTTypeId_fun aSTTypeId;
      opt_iter templateParameter_fun templateParameter_opt


and templateArgument_fun = function
  | TA_type(aSTTypeId, templateArgument_opt) -> 
      aSTTypeId_fun aSTTypeId;
      opt_iter templateArgument_fun templateArgument_opt

  | TA_nontype(expression, templateArgument_opt) -> 
      expression_fun expression;
      opt_iter templateArgument_fun templateArgument_opt

  | TA_templateUsed(templateArgument_opt) -> 
      opt_iter templateArgument_fun templateArgument_opt


and namespaceDecl_fun = function
  | ND_alias(stringRef, pQName) -> 
      string_fun stringRef;
      pQName_fun pQName

  | ND_usingDecl(pQName) -> 
      pQName_fun pQName

  | ND_usingDir(pQName) -> 
      pQName_fun pQName


and fullExpressionAnnot_fun(declaration_list) =
    List.iter declaration_fun declaration_list


and aSTTypeof_fun = function
  | TS_typeof_expr(fullExpression) -> 
      fullExpression_fun fullExpression

  | TS_typeof_type(aSTTypeId) -> 
      aSTTypeId_fun aSTTypeId


and designator_fun = function
  | FieldDesignator(sourceLoc, stringRef) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef;

  | SubscriptDesignator(sourceLoc, expression, expression_opt) -> 
      sourceLoc_fun sourceLoc;
      expression_fun expression;
      opt_iter expression_fun expression_opt


and attributeSpecifierList_fun = function
  | AttributeSpecifierList_cons(attributeSpecifier, attributeSpecifierList) -> 
      attributeSpecifier_fun attributeSpecifier;
      attributeSpecifierList_fun attributeSpecifierList


and attributeSpecifier_fun = function
  | AttributeSpecifier_cons(attribute, attributeSpecifier) -> 
      attribute_fun attribute;
      attributeSpecifier_fun attributeSpecifier


and attribute_fun = function
  | AT_empty(sourceLoc) -> 
      sourceLoc_fun sourceLoc

  | AT_word(sourceLoc, stringRef) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef;

  | AT_func(sourceLoc, stringRef, argExpression_list) -> 
      sourceLoc_fun sourceLoc;
      string_fun stringRef;
      List.iter argExpression_fun argExpression_list



(*** Local Variables: ***)
(*** compile-command: "ocamlc.opt -c -I ../elsa astiter.ml" ***)
(*** End: ***)