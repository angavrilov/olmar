
open Cc_ml_types
open Cc_ast_gen_type

let opt_map f = function
  | None -> None
  | Some x -> Some(f x)

let string_fun s = (s : string)

let bool_fun b = (b : bool)

let sourceLoc_fun((file : string), (line : int), (char : int) as loc) = 
  (loc : sourceLoc)

let declFlags_fun(l : declFlag list) = l

let variable_fun(v : variable) = v

let cType_fun(c : cType) = c

let simpleTypeId_fun(id : simpleTypeId) = id

let typeIntr_fun(keywort : typeIntr) = keywort

let accessKeyword_fun(keyword : accessKeyword) = keyword

let cVFlags_fun(fl : cVFlag list) = fl

let overloadableOp_fun(op :overloadableOp) = op

let unaryOp_fun(op : unaryOp) = op

let effectOp_fun(op : effectOp) = op

let binaryOp_fun(op : binaryOp) = op

let castKeyword_fun(keyword : castKeyword) = keyword




let rec translationUnit_fun topForm_list =
  List.map topForm_fun topForm_list


and topForm_fun = function
  | TF_decl(sourceLoc, declaration) -> 
      TF_decl(sourceLoc_fun sourceLoc,
	      declaration_fun declaration)

  | TF_func(sourceLoc, func) -> 
      TF_func(sourceLoc_fun sourceLoc,
	      func_fun func)

  | TF_template(sourceLoc, templateDeclaration) -> 
      TF_template(sourceLoc_fun sourceLoc,
		  templateDeclaration_fun templateDeclaration)

  | TF_explicitInst(sourceLoc, declFlags, declaration) -> 
      TF_explicitInst(sourceLoc_fun sourceLoc,
		      declFlags_fun declFlags,
		      declaration_fun declaration)

  | TF_linkage(sourceLoc, stringRef, translationUnit) -> 
      TF_linkage(sourceLoc_fun sourceLoc,
		 string_fun stringRef,
		 translationUnit_fun translationUnit)

  | TF_one_linkage(sourceLoc, stringRef, topForm) -> 
      TF_one_linkage(sourceLoc_fun sourceLoc,
		     string_fun stringRef,
		     topForm_fun topForm)

  | TF_asm(sourceLoc, e_stringLit) -> 
      assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
      TF_asm(sourceLoc_fun sourceLoc,
	     expression_fun e_stringLit)

  | TF_namespaceDefn(sourceLoc, stringRef_opt, topForm_list) -> 
      TF_namespaceDefn(sourceLoc_fun sourceLoc,
		       opt_map string_fun stringRef_opt,
		       List.map topForm_fun topForm_list)

  | TF_namespaceDecl(sourceLoc, namespaceDecl) -> 
      TF_namespaceDecl(sourceLoc_fun sourceLoc,
		       namespaceDecl_fun namespaceDecl)



and func_fun(declFlags, typeSpecifier, declarator, memberInit_list, 
	 s_compound, handler_list, statement_opt, bool) =
  assert(match s_compound with | S_compound _ -> true | _ -> false);
  (declFlags_fun declFlags,
   typeSpecifier_fun typeSpecifier,
   declarator_fun declarator,
   List.map memberInit_fun memberInit_list,
   statement_fun s_compound,
   List.map handler_fun handler_list,
   opt_map statement_fun statement_opt,
   bool_fun bool)



and memberInit_fun(pQName, argExpression_list, statement_opt) =
  (pQName_fun pQName,
   List.map argExpression_fun argExpression_list,
   opt_map statement_fun statement_opt)



and declaration_fun(declFlags, typeSpecifier, declarator_list) =
  (declFlags_fun declFlags,
   typeSpecifier_fun typeSpecifier,
   List.map declarator_fun declarator_list)


and aSTTypeId_fun(typeSpecifier, declarator) =
  (typeSpecifier_fun typeSpecifier,
   declarator_fun declarator)



and pQName_fun = function
  | PQ_qualifier(sourceLoc, stringRef_opt, 
		 templateArgument_opt, pQName) -> 
      PQ_qualifier(sourceLoc_fun sourceLoc,
		   opt_map string_fun stringRef_opt,
		   opt_map templateArgument_fun templateArgument_opt,
		   pQName_fun pQName)

  | PQ_name(sourceLoc, stringRef) -> 
      PQ_name(sourceLoc_fun sourceLoc,
	      string_fun stringRef)

  | PQ_operator(sourceLoc, operatorName, stringRef) -> 
      PQ_operator(sourceLoc_fun sourceLoc,
		  operatorName_fun operatorName,
		  string_fun stringRef)

  | PQ_template(sourceLoc, stringRef, templateArgument_opt) -> 
      PQ_template(sourceLoc_fun sourceLoc,
		  string_fun stringRef,
		  opt_map templateArgument_fun templateArgument_opt)

  | PQ_variable(sourceLoc, variable) -> 
      PQ_variable(sourceLoc_fun sourceLoc,
		  variable_fun variable)



and typeSpecifier_fun = function
  | TS_name(sourceLoc, cVFlags, pQName, bool) -> 
      TS_name(sourceLoc_fun sourceLoc,
	      cVFlags_fun cVFlags,
	      pQName_fun pQName,
	      bool_fun bool)

  | TS_simple(sourceLoc, cVFlags, simpleTypeId) -> 
      TS_simple(sourceLoc_fun sourceLoc,
		cVFlags_fun cVFlags,
		simpleTypeId_fun simpleTypeId)

  | TS_elaborated(sourceLoc, cVFlags, typeIntr, pQName) -> 
      TS_elaborated(sourceLoc_fun sourceLoc,
		    cVFlags_fun cVFlags,
		    typeIntr_fun typeIntr,
		    pQName_fun pQName)

  | TS_classSpec(sourceLoc, cVFlags, typeIntr, pQName_opt, 
		 baseClassSpec_list, memberList) -> 
      TS_classSpec(sourceLoc_fun sourceLoc,
		   cVFlags_fun cVFlags,
		   typeIntr_fun typeIntr,
		   opt_map pQName_fun pQName_opt,
		   List.map baseClassSpec_fun baseClassSpec_list,
		   memberList_fun memberList      )

  | TS_enumSpec(sourceLoc, cVFlags, stringRef_opt, enumerator_list) -> 
      TS_enumSpec(sourceLoc_fun sourceLoc,
		  cVFlags_fun cVFlags,
		  opt_map string_fun stringRef_opt,
		  List.map enumerator_fun enumerator_list)

  | TS_type(sourceLoc, cVFlags, cType) -> 
      TS_type(sourceLoc_fun sourceLoc,
	      cVFlags_fun cVFlags,
	      cType_fun cType)

  | TS_typeof(sourceLoc, cVFlags, aSTTypeof) -> 
      TS_typeof(sourceLoc_fun sourceLoc,
		cVFlags_fun cVFlags,
		aSTTypeof_fun aSTTypeof)


and baseClassSpec_fun(bool, accessKeyword, pQName) =
  (bool_fun bool,
   accessKeyword_fun accessKeyword,
   pQName_fun pQName)


and enumerator_fun(sourceLoc, stringRef, expression_opt) =
  (sourceLoc_fun sourceLoc,
   string_fun stringRef,
   opt_map expression_fun expression_opt)


and memberList_fun(member_list) =
  List.map member_fun member_list


and member_fun = function
  | MR_decl(sourceLoc, declaration) -> 
      MR_decl(sourceLoc_fun sourceLoc,
	      declaration_fun declaration)

  | MR_func(sourceLoc, func) -> 
      MR_func(sourceLoc_fun sourceLoc,
	      func_fun func)

  | MR_access(sourceLoc, accessKeyword) -> 
      MR_access(sourceLoc_fun sourceLoc,
		accessKeyword_fun accessKeyword)

  | MR_usingDecl(sourceLoc, nd_usingDecl) -> 
      assert(match nd_usingDecl with ND_usingDecl _ -> true | _ -> false);
      MR_usingDecl(sourceLoc_fun sourceLoc,
		   namespaceDecl_fun nd_usingDecl)

  | MR_template(sourceLoc, templateDeclaration) -> 
      MR_template(sourceLoc_fun sourceLoc,
		  templateDeclaration_fun templateDeclaration)


and declarator_fun(iDeclarator, init_opt, 
		   statement_opt_ctor, statement_opt_dtor) =
  (iDeclarator_fun iDeclarator,
   opt_map init_fun init_opt,
   opt_map statement_fun statement_opt_ctor,
   opt_map statement_fun statement_opt_dtor)


and iDeclarator_fun = function
  | D_name(sourceLoc, pQName_opt) -> 
      D_name(sourceLoc_fun sourceLoc,
	     opt_map pQName_fun pQName_opt)

  | D_pointer(sourceLoc, cVFlags, iDeclarator) -> 
      D_pointer(sourceLoc_fun sourceLoc,
		cVFlags_fun cVFlags,
		iDeclarator_fun iDeclarator)

  | D_reference(sourceLoc, iDeclarator) -> 
      D_reference(sourceLoc_fun sourceLoc,
		  iDeclarator_fun iDeclarator)

  | D_func(sourceLoc, iDeclarator, aSTTypeId_list, cVFlags, 
	   exceptionSpec_opt, pq_name_list) -> 
      assert(List.for_all (function | PQ_name _ -> true | _ -> false) 
	       pq_name_list);
      D_func(sourceLoc_fun sourceLoc,
	     iDeclarator_fun iDeclarator,
	     List.map aSTTypeId_fun aSTTypeId_list,
	     cVFlags_fun cVFlags,
	     opt_map exceptionSpec_fun exceptionSpec_opt,
	     List.map pQName_fun pq_name_list)

  | D_array(sourceLoc, iDeclarator, expression_opt) -> 
      D_array(sourceLoc_fun sourceLoc,
	      iDeclarator_fun iDeclarator,
	      opt_map expression_fun expression_opt)

  | D_bitfield(sourceLoc, pQName_opt, expression) -> 
      D_bitfield(sourceLoc_fun sourceLoc,
		 opt_map pQName_fun pQName_opt,
		 expression_fun expression)

  | D_ptrToMember(sourceLoc, pQName, cVFlags, iDeclarator) -> 
      D_ptrToMember(sourceLoc_fun sourceLoc,
		    pQName_fun pQName,
		    cVFlags_fun cVFlags,
		    iDeclarator_fun iDeclarator)

  | D_grouping(sourceLoc, iDeclarator) -> 
      D_grouping(sourceLoc_fun sourceLoc,
		 iDeclarator_fun iDeclarator)


and exceptionSpec_fun(aSTTypeId_list) =
  List.map aSTTypeId_fun aSTTypeId_list


and operatorName_fun = function
  | ON_newDel(bool_is_new, bool_is_array) -> 
      ON_newDel(bool_fun bool_is_new,
		bool_fun bool_is_array)

  | ON_operator(overloadableOp) -> 
      ON_operator(overloadableOp_fun overloadableOp)

  | ON_conversion(aSTTypeId) -> 
      ON_conversion(aSTTypeId_fun aSTTypeId)


and statement_fun = function
  | S_skip(sourceLoc) -> 
      S_skip(sourceLoc_fun sourceLoc)

  | S_label(sourceLoc, stringRef, statement) -> 
      S_label(sourceLoc_fun sourceLoc,
	      string_fun stringRef,
	      statement_fun statement)

  | S_case(sourceLoc, expression, statement) -> 
      S_case(sourceLoc_fun sourceLoc,
	     expression_fun expression,
	     statement_fun statement)

  | S_default(sourceLoc, statement) -> 
      S_default(sourceLoc_fun sourceLoc,
		statement_fun statement)

  | S_expr(sourceLoc, fullExpression) -> 
      S_expr(sourceLoc_fun sourceLoc,
	     fullExpression_fun fullExpression)

  | S_compound(sourceLoc, statement_list) -> 
      S_compound(sourceLoc_fun sourceLoc,
		 List.map statement_fun statement_list)

  | S_if(sourceLoc, condition, statement_then, statement_else) -> 
      S_if(sourceLoc_fun sourceLoc,
	   condition_fun condition,
	   statement_fun statement_then,
	   statement_fun statement_else)

  | S_switch(sourceLoc, condition, statement) -> 
      S_switch(sourceLoc_fun sourceLoc,
	       condition_fun condition,
	       statement_fun statement)

  | S_while(sourceLoc, condition, statement) -> 
      S_while(sourceLoc_fun sourceLoc,
	      condition_fun condition,
	      statement_fun statement)

  | S_doWhile(sourceLoc, statement, fullExpression) -> 
      S_doWhile(sourceLoc_fun sourceLoc,
		statement_fun statement,
		fullExpression_fun fullExpression)

  | S_for(sourceLoc, statement_init, condition, fullExpression, 
	  statement_body) -> 
      S_for(sourceLoc_fun sourceLoc,
	    statement_fun statement_init,
	    condition_fun condition,
	    fullExpression_fun fullExpression,
	    statement_fun statement_body)

  | S_break(sourceLoc) -> 
      S_break(sourceLoc_fun sourceLoc)

  | S_continue(sourceLoc) -> 
      S_continue(sourceLoc_fun sourceLoc)

  | S_return(sourceLoc, fullExpression_opt, statement_opt) -> 
      S_return(sourceLoc_fun sourceLoc,
	       opt_map fullExpression_fun fullExpression_opt,
	       opt_map statement_fun statement_opt)

  | S_goto(sourceLoc, stringRef) -> 
      S_goto(sourceLoc_fun sourceLoc,
	     string_fun stringRef)

  | S_decl(sourceLoc, declaration) -> 
      S_decl(sourceLoc_fun sourceLoc,
	     declaration_fun declaration)

  | S_try(sourceLoc, statement, handler_list) -> 
      S_try(sourceLoc_fun sourceLoc,
	    statement_fun statement,
	    List.map handler_fun handler_list)

  | S_asm(sourceLoc, e_stringLit) -> 
      assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
      S_asm(sourceLoc_fun sourceLoc,
	    expression_fun e_stringLit)

  | S_namespaceDecl(sourceLoc, namespaceDecl) -> 
      S_namespaceDecl(sourceLoc_fun sourceLoc,
		      namespaceDecl_fun namespaceDecl)

  | S_function(sourceLoc, func) -> 
      S_function(sourceLoc_fun sourceLoc,
		 func_fun func)

  | S_rangeCase(sourceLoc, expression_lo, expression_hi, statement) -> 
      S_rangeCase(sourceLoc_fun sourceLoc,
		  expression_fun expression_lo,
		  expression_fun expression_hi,
		  statement_fun statement)

  | S_computedGoto(sourceLoc, expression) -> 
      S_computedGoto(sourceLoc_fun sourceLoc,
		     expression_fun expression)


and condition_fun = function
  | CN_expr(fullExpression) -> 
      CN_expr(fullExpression_fun fullExpression)

  | CN_decl(aSTTypeId) -> 
      CN_decl(aSTTypeId_fun aSTTypeId)


and handler_fun(aSTTypeId, statement_body, expression_opt, statement_gdtor) =
      (aSTTypeId_fun aSTTypeId,
       statement_fun statement_body,
       opt_map expression_fun expression_opt,
       opt_map statement_fun statement_gdtor)


and expression_fun = function
  | E_boolLit(bool) -> 
      E_boolLit(bool_fun bool)

  | E_intLit(stringRef) -> 
      E_intLit(string_fun stringRef)

  | E_floatLit(stringRef) -> 
      E_floatLit(string_fun stringRef)

  | E_stringLit(stringRef, e_stringLit_opt) -> 
      assert(match e_stringLit_opt with 
	       | Some(E_stringLit _) -> true 
	       | None -> true
	       | _ -> false);
      E_stringLit(string_fun stringRef,
		  opt_map expression_fun e_stringLit_opt)

  | E_charLit(stringRef) -> 
      E_charLit(string_fun stringRef)

  | E_this -> E_this

  | E_variable(pQName) -> 
      E_variable(pQName_fun pQName)

  | E_funCall(expression_func, argExpression_list, expression_retobj_opt) -> 
      E_funCall(expression_fun expression_func,
		List.map argExpression_fun argExpression_list,
		opt_map expression_fun expression_retobj_opt)

  | E_constructor(typeSpecifier, argExpression_list, bool, expression_opt) -> 
      E_constructor(typeSpecifier_fun typeSpecifier,
		    List.map argExpression_fun argExpression_list,
		    bool_fun bool,
		    opt_map expression_fun expression_opt)

  | E_fieldAcc(expression, pQName) -> 
      E_fieldAcc(expression_fun expression,
		 pQName_fun pQName)

  | E_sizeof(expression) -> 
      E_sizeof(expression_fun expression)

  | E_unary(unaryOp, expression) -> 
      E_unary(unaryOp_fun unaryOp,
	      expression_fun expression)

  | E_effect(effectOp, expression) -> 
      E_effect(effectOp_fun effectOp,
	       expression_fun expression)

  | E_binary(expression_left, binaryOp, expression_right) -> 
      E_binary(expression_fun expression_left,
	       binaryOp_fun binaryOp,
	       expression_fun expression_right)

  | E_addrOf(expression) -> 
      E_addrOf(expression_fun expression)

  | E_deref(expression) -> 
      E_deref(expression_fun expression)

  | E_cast(aSTTypeId, expression) -> 
      E_cast(aSTTypeId_fun aSTTypeId,
	     expression_fun expression)

  | E_cond(expression_cond, expression_true, expression_false) -> 
      E_cond(expression_fun expression_cond,
	     expression_fun expression_true,
	     expression_fun expression_false)

  | E_sizeofType(aSTTypeId) -> 
      E_sizeofType(aSTTypeId_fun aSTTypeId)

  | E_assign(expression_target, binaryOp, expression_src) -> 
      E_assign(expression_fun expression_target,
	       binaryOp_fun binaryOp,
	       expression_fun expression_src)

  | E_new(bool, argExpression_list, aSTTypeId, argExpressionListOpt_opt,
	  statement_opt) -> 
      E_new(bool_fun bool,
	    List.map argExpression_fun argExpression_list,
	    aSTTypeId_fun aSTTypeId,
	    opt_map argExpressionListOpt_fun argExpressionListOpt_opt,
	    opt_map statement_fun statement_opt)

  | E_delete(bool_colon, bool_array, expression_opt, statement_opt) -> 
      E_delete(bool_fun bool_colon,
	       bool_fun bool_array,
	       opt_map expression_fun expression_opt,
	       opt_map statement_fun statement_opt)

  | E_throw(expression_opt, statement_opt) -> 
      E_throw(opt_map expression_fun expression_opt,
	      opt_map statement_fun statement_opt)

  | E_keywordCast(castKeyword, aSTTypeId, expression) -> 
      E_keywordCast(castKeyword_fun castKeyword,
		    aSTTypeId_fun aSTTypeId,
		    expression_fun expression)

  | E_typeidExpr(expression) -> 
      E_typeidExpr(expression_fun expression)

  | E_typeidType(aSTTypeId) -> 
      E_typeidType(aSTTypeId_fun aSTTypeId)

  | E_grouping(expression) -> 
      E_grouping(expression_fun expression)

  | E_arrow(expression, pQName) -> 
      E_arrow(expression_fun expression,
	      pQName_fun pQName)

  | E_statement(s_compound) -> 
      assert(match s_compound with | S_compound _ -> true | _ -> false);
      E_statement(statement_fun s_compound)

  | E_compoundLit(aSTTypeId, in_compound) -> 
      assert(match in_compound with | IN_compound _ -> true | _ -> false);
      E_compoundLit(aSTTypeId_fun aSTTypeId,
		    init_fun in_compound)

  | E___builtin_constant_p(sourceLoc, expression) -> 
      E___builtin_constant_p(sourceLoc_fun sourceLoc,
			     expression_fun expression)

  | E___builtin_va_arg(sourceLoc, expression, aSTTypeId) -> 
      E___builtin_va_arg(sourceLoc_fun sourceLoc,
			 expression_fun expression,
			 aSTTypeId_fun aSTTypeId)

  | E_alignofType(aSTTypeId) -> 
      E_alignofType(aSTTypeId_fun aSTTypeId)

  | E_alignofExpr(expression) -> 
      E_alignofExpr(expression_fun expression)

  | E_gnuCond(expression_cond, expression_false) -> 
      E_gnuCond(expression_fun expression_cond,
		expression_fun expression_false)

  | E_addrOfLabel(stringRef) -> 
      E_addrOfLabel(string_fun stringRef)


and fullExpression_fun(expression_opt) =
  opt_map expression_fun expression_opt


and argExpression_fun(expression) =
  expression_fun expression


and argExpressionListOpt_fun(argExpression_list) =
  List.map argExpression_fun argExpression_list


and init_fun = function
  | IN_expr(sourceLoc, expression) -> 
      IN_expr(sourceLoc_fun sourceLoc,
	      expression_fun expression)

  | IN_compound(sourceLoc, init_list) -> 
      IN_compound(sourceLoc_fun sourceLoc,
		  List.map init_fun init_list)

  | IN_ctor(sourceLoc, argExpression_list, bool) -> 
      IN_ctor(sourceLoc_fun sourceLoc,
	      List.map argExpression_fun argExpression_list,
	      bool_fun bool)

  | IN_designated(sourceLoc, designator_list, init) -> 
      IN_designated(sourceLoc_fun sourceLoc,
		    List.map designator_fun designator_list,
		    init_fun init)


and templateDeclaration_fun = function
  | TD_func(templateParameter_opt, func) -> 
      TD_func(opt_map templateParameter_fun templateParameter_opt,
	      func_fun func)

  | TD_decl(templateParameter_opt, declaration) -> 
      TD_decl(opt_map templateParameter_fun templateParameter_opt,
	      declaration_fun declaration)

  | TD_tmember(templateParameter_opt, templateDeclaration) -> 
      TD_tmember(opt_map templateParameter_fun templateParameter_opt,
		 templateDeclaration_fun templateDeclaration)


and templateParameter_fun = function
  | TP_type(sourceLoc, stringRef, aSTTypeId_opt, templateParameter_opt) -> 
      TP_type(sourceLoc_fun sourceLoc,
	      string_fun stringRef,
	      opt_map aSTTypeId_fun aSTTypeId_opt,
	      opt_map templateParameter_fun templateParameter_opt)

  | TP_nontype(sourceLoc, aSTTypeId, templateParameter_opt) -> 
      TP_nontype(sourceLoc_fun sourceLoc,
		 aSTTypeId_fun aSTTypeId,
		 opt_map templateParameter_fun templateParameter_opt)


and templateArgument_fun = function
  | TA_type(aSTTypeId, templateArgument_opt) -> 
      TA_type(aSTTypeId_fun aSTTypeId,
	      opt_map templateArgument_fun templateArgument_opt)

  | TA_nontype(expression, templateArgument_opt) -> 
      TA_nontype(expression_fun expression,
		 opt_map templateArgument_fun templateArgument_opt)

  | TA_templateUsed(templateArgument_opt) -> 
      TA_templateUsed(opt_map templateArgument_fun templateArgument_opt)


and namespaceDecl_fun = function
  | ND_alias(stringRef, pQName) -> 
      ND_alias(string_fun stringRef,
	       pQName_fun pQName)

  | ND_usingDecl(pQName) -> 
      ND_usingDecl(pQName_fun pQName)

  | ND_usingDir(pQName) -> 
      ND_usingDir(pQName_fun pQName)


and fullExpressionAnnot_fun(declaration_list) =
    List.map declaration_fun declaration_list


and aSTTypeof_fun = function
  | TS_typeof_expr(fullExpression) -> 
      TS_typeof_expr(fullExpression_fun fullExpression)

  | TS_typeof_type(aSTTypeId) -> 
      TS_typeof_type(aSTTypeId_fun aSTTypeId)


and designator_fun = function
  | FieldDesignator(sourceLoc, stringRef) -> 
      FieldDesignator(sourceLoc_fun sourceLoc,
		      string_fun stringRef)

  | SubscriptDesignator(sourceLoc, expression, expression_opt) -> 
      SubscriptDesignator(sourceLoc_fun sourceLoc,
			  expression_fun expression,
			  opt_map expression_fun expression_opt)


and attributeSpecifierList_fun = function
  | AttributeSpecifierList_cons(attributeSpecifier, attributeSpecifierList) -> 
      AttributeSpecifierList_cons(attributeSpecifier_fun attributeSpecifier,
				  attributeSpecifierList_fun attributeSpecifierList)


and attributeSpecifier_fun = function
  | AttributeSpecifier_cons(attribute, attributeSpecifier) -> 
      AttributeSpecifier_cons(attribute_fun attribute,
			      attributeSpecifier_fun attributeSpecifier)


and attribute_fun = function
  | AT_empty(sourceLoc) -> 
      AT_empty(sourceLoc_fun sourceLoc)

  | AT_word(sourceLoc, stringRef) -> 
      AT_word(sourceLoc_fun sourceLoc,
	      string_fun stringRef)

  | AT_func(sourceLoc, stringRef, argExpression_list) -> 
      AT_func(sourceLoc_fun sourceLoc,
	      string_fun stringRef,
	      List.map argExpression_fun argExpression_list)



(*** Local Variables: ***)
(*** compile-command: "ocamlc.opt -c -I ../elsa astmap.ml" ***)
(*** End: ***)
 
