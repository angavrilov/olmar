(* cc_ast_gen_type.ml *)
(* *** DO NOT EDIT *** *)
(* generated automatically by astgen, from cc.ast *)
(* active extension modules: gnu_attribute_hack.ast cc_tcheck.ast cc_print.ast cfg.ast cc_elaborate.ast gnu.ast kandr.ast ml_ctype.ast *)


(* *********************************************************************
 * *********** Ast type definition ******************
 * ********************************************************************* *)

(* *** DO NOT EDIT *** *)
(* ocaml type verbatim start *)

  open Cc_ml_types
  open Ml_ctype;;
(* ocaml type verbatim end *)

type 'a translationUnit_type = 'a * 'a topForm_type list * 'a scope option 

and 'a topForm_type = 
  | TF_decl of 'a * sourceLoc * 'a declaration_type 
  | TF_func of 'a * sourceLoc * 'a function_type 
  | TF_template of 'a * sourceLoc * 'a templateDeclaration_type 
  | TF_explicitInst of 'a * sourceLoc * declFlags * 'a declaration_type 
  | TF_linkage of 'a * sourceLoc * stringRef * 'a translationUnit_type 
  | TF_one_linkage of 'a * sourceLoc * stringRef * 'a topForm_type 
  | TF_asm of 'a * sourceLoc * 'a expression_type (* = E_stringLit  *) 
  | TF_namespaceDefn of 'a * sourceLoc * stringRef option * 'a topForm_type list 
  | TF_namespaceDecl of 'a * sourceLoc * 'a namespaceDecl_type 

and 'a function_type = 'a * declFlags * 'a typeSpecifier_type * 'a declarator_type * 'a memberInit_type list * 'a statement_type (* = S_compound  *) option * 'a handler_type list * 'a functionType * 'a variable option * 'a variable option * 'a statement_type option * bool 

and 'a memberInit_type = 'a * 'a pQName_type * 'a argExpression_type list * 'a variable option * 'a compoundType option * 'a variable option * 'a fullExpressionAnnot_type * 'a statement_type option 

and 'a declaration_type = 'a * declFlags * 'a typeSpecifier_type * 'a declarator_type list 

and 'a aSTTypeId_type = 'a * 'a typeSpecifier_type * 'a declarator_type 

and 'a pQName_type = 
  | PQ_qualifier of 'a * sourceLoc * stringRef option * 'a templateArgument_type option * 'a pQName_type * 'a variable option * 'a sTemplateArgument list 
  | PQ_name of 'a * sourceLoc * stringRef 
  | PQ_operator of 'a * sourceLoc * 'a operatorName_type * stringRef 
  | PQ_template of 'a * sourceLoc * stringRef * 'a templateArgument_type option * 'a sTemplateArgument list 
  | PQ_variable of 'a * sourceLoc * 'a variable 

and 'a typeSpecifier_type = 
  | TS_name of 'a * sourceLoc * cVFlags * 'a pQName_type * bool * 'a variable option * 'a variable option 
  | TS_simple of 'a * sourceLoc * cVFlags * simpleTypeId 
  | TS_elaborated of 'a * sourceLoc * cVFlags * typeIntr * 'a pQName_type * 'a namedAtomicType option 
  | TS_classSpec of 'a * sourceLoc * cVFlags * typeIntr * 'a pQName_type option * 'a baseClassSpec_type list * 'a memberList_type * 'a compoundType 
  | TS_enumSpec of 'a * sourceLoc * cVFlags * stringRef option * 'a enumerator_type list * 'a enumType 
  | TS_type of 'a * sourceLoc * cVFlags * 'a cType 
  | TS_typeof of 'a * sourceLoc * cVFlags * 'a aSTTypeof_type 

and 'a baseClassSpec_type = 'a * bool * accessKeyword * 'a pQName_type * 'a compoundType option 

and 'a enumerator_type = 'a * sourceLoc * stringRef * 'a expression_type option * 'a variable * int32 

and 'a memberList_type = 'a * 'a member_type list 

and 'a member_type = 
  | MR_decl of 'a * sourceLoc * 'a declaration_type 
  | MR_func of 'a * sourceLoc * 'a function_type 
  | MR_access of 'a * sourceLoc * accessKeyword 
  | MR_usingDecl of 'a * sourceLoc * 'a namespaceDecl_type (* = ND_usingDecl  *) 
  | MR_template of 'a * sourceLoc * 'a templateDeclaration_type 

and 'a exceptionSpec_type = 'a * 'a aSTTypeId_type list 

and 'a operatorName_type = 
  | ON_newDel of 'a * bool * bool 
  | ON_operator of 'a * overloadableOp 
  | ON_conversion of 'a * 'a aSTTypeId_type 

and 'a statement_type = 
  | S_skip of 'a * sourceLoc 
  | S_label of 'a * sourceLoc * stringRef * 'a statement_type 
  | S_case of 'a * sourceLoc * 'a expression_type * 'a statement_type * int32 
  | S_default of 'a * sourceLoc * 'a statement_type 
  | S_expr of 'a * sourceLoc * 'a fullExpression_type 
  | S_compound of 'a * sourceLoc * 'a statement_type list 
  | S_if of 'a * sourceLoc * 'a condition_type * 'a statement_type * 'a statement_type 
  | S_switch of 'a * sourceLoc * 'a condition_type * 'a statement_type 
  | S_while of 'a * sourceLoc * 'a condition_type * 'a statement_type 
  | S_doWhile of 'a * sourceLoc * 'a statement_type * 'a fullExpression_type 
  | S_for of 'a * sourceLoc * 'a statement_type * 'a condition_type * 'a fullExpression_type * 'a statement_type 
  | S_break of 'a * sourceLoc 
  | S_continue of 'a * sourceLoc 
  | S_return of 'a * sourceLoc * 'a fullExpression_type option * 'a statement_type option 
  | S_goto of 'a * sourceLoc * stringRef 
  | S_decl of 'a * sourceLoc * 'a declaration_type 
  | S_try of 'a * sourceLoc * 'a statement_type * 'a handler_type list 
  | S_asm of 'a * sourceLoc * 'a expression_type (* = E_stringLit  *) 
  | S_namespaceDecl of 'a * sourceLoc * 'a namespaceDecl_type 
  | S_function of 'a * sourceLoc * 'a function_type 
  | S_rangeCase of 'a * sourceLoc * 'a expression_type * 'a expression_type * 'a statement_type * int * int 
  | S_computedGoto of 'a * sourceLoc * 'a expression_type 

and 'a condition_type = 
  | CN_expr of 'a * 'a fullExpression_type 
  | CN_decl of 'a * 'a aSTTypeId_type 

and 'a handler_type = 'a * 'a aSTTypeId_type * 'a statement_type * 'a variable option * 'a fullExpressionAnnot_type * 'a expression_type option * 'a statement_type option 

and 'a expression_type = 
  | E_boolLit of 'a * 'a cType option * bool 
  | E_intLit of 'a * 'a cType option * stringRef * unsigned_long 
  | E_floatLit of 'a * 'a cType option * stringRef * double 
  | E_stringLit of 'a * 'a cType option * stringRef * 'a expression_type (* = E_stringLit  *) option * stringRef option 
  | E_charLit of 'a * 'a cType option * stringRef * unsigned_int 
  | E_this of 'a * 'a cType option * 'a variable 
  | E_variable of 'a * 'a cType option * 'a pQName_type * 'a variable option * 'a variable option 
  | E_funCall of 'a * 'a cType option * 'a expression_type * 'a argExpression_type list * 'a expression_type option 
  | E_constructor of 'a * 'a cType option * 'a typeSpecifier_type * 'a argExpression_type list * 'a variable option * bool * 'a expression_type option 
  | E_fieldAcc of 'a * 'a cType option * 'a expression_type * 'a pQName_type * 'a variable option 
  | E_sizeof of 'a * 'a cType option * 'a expression_type * int 
  | E_unary of 'a * 'a cType option * unaryOp * 'a expression_type 
  | E_effect of 'a * 'a cType option * effectOp * 'a expression_type 
  | E_binary of 'a * 'a cType option * 'a expression_type * binaryOp * 'a expression_type 
  | E_addrOf of 'a * 'a cType option * 'a expression_type 
  | E_deref of 'a * 'a cType option * 'a expression_type 
  | E_cast of 'a * 'a cType option * 'a aSTTypeId_type * 'a expression_type * bool 
  | E_cond of 'a * 'a cType option * 'a expression_type * 'a expression_type * 'a expression_type 
  | E_sizeofType of 'a * 'a cType option * 'a aSTTypeId_type * int * bool 
  | E_assign of 'a * 'a cType option * 'a expression_type * binaryOp * 'a expression_type 
  | E_new of 'a * 'a cType option * bool * 'a argExpression_type list * 'a aSTTypeId_type * 'a argExpressionListOpt_type option * 'a expression_type option * 'a variable option * 'a statement_type option * 'a variable option 
  | E_delete of 'a * 'a cType option * bool * bool * 'a expression_type option * 'a statement_type option 
  | E_throw of 'a * 'a cType option * 'a expression_type option * 'a variable option * 'a statement_type option 
  | E_keywordCast of 'a * 'a cType option * castKeyword * 'a aSTTypeId_type * 'a expression_type 
  | E_typeidExpr of 'a * 'a cType option * 'a expression_type 
  | E_typeidType of 'a * 'a cType option * 'a aSTTypeId_type 
  | E_grouping of 'a * 'a cType option * 'a expression_type 
  | E_arrow of 'a * 'a cType option * 'a expression_type * 'a pQName_type 
  | E_statement of 'a * 'a cType option * 'a statement_type (* = S_compound  *) 
  | E_compoundLit of 'a * 'a cType option * 'a aSTTypeId_type * 'a initializer_type (* = IN_compound  *) 
  | E___builtin_constant_p of 'a * 'a cType option * sourceLoc * 'a expression_type 
  | E___builtin_va_arg of 'a * 'a cType option * sourceLoc * 'a expression_type * 'a aSTTypeId_type 
  | E_alignofType of 'a * 'a cType option * 'a aSTTypeId_type * int 
  | E_alignofExpr of 'a * 'a cType option * 'a expression_type * int 
  | E_gnuCond of 'a * 'a cType option * 'a expression_type * 'a expression_type 
  | E_addrOfLabel of 'a * 'a cType option * stringRef 

and 'a fullExpression_type = 'a * 'a expression_type option * 'a fullExpressionAnnot_type 

and 'a argExpression_type = 'a * 'a expression_type 

and 'a argExpressionListOpt_type = 'a * 'a argExpression_type list 

and 'a initializer_type = 
  | IN_expr of 'a * sourceLoc * 'a fullExpressionAnnot_type * 'a expression_type option 
  | IN_compound of 'a * sourceLoc * 'a fullExpressionAnnot_type * 'a initializer_type list 
  | IN_ctor of 'a * sourceLoc * 'a fullExpressionAnnot_type * 'a argExpression_type list * 'a variable option * bool 
  | IN_designated of 'a * sourceLoc * 'a fullExpressionAnnot_type * 'a designator_type list * 'a initializer_type 

and 'a templateDeclaration_type = 
  | TD_func of 'a * 'a templateParameter_type option * 'a function_type 
  | TD_decl of 'a * 'a templateParameter_type option * 'a declaration_type 
  | TD_tmember of 'a * 'a templateParameter_type option * 'a templateDeclaration_type 

and 'a templateParameter_type = 
  | TP_type of 'a * sourceLoc * 'a variable * stringRef * 'a aSTTypeId_type option * 'a templateParameter_type option 
  | TP_nontype of 'a * sourceLoc * 'a variable * 'a aSTTypeId_type * 'a templateParameter_type option 

and 'a templateArgument_type = 
  | TA_type of 'a * 'a aSTTypeId_type * 'a templateArgument_type option 
  | TA_nontype of 'a * 'a expression_type * 'a templateArgument_type option 
  | TA_templateUsed of 'a * 'a templateArgument_type option 

and 'a namespaceDecl_type = 
  | ND_alias of 'a * stringRef * 'a pQName_type 
  | ND_usingDecl of 'a * 'a pQName_type 
  | ND_usingDir of 'a * 'a pQName_type 

and 'a declarator_type = 'a * 'a iDeclarator_type * 'a initializer_type option * 'a variable option * 'a cType option * declaratorContext * 'a statement_type option * 'a statement_type option 

and 'a iDeclarator_type = 
  | D_name of 'a * sourceLoc * 'a pQName_type option 
  | D_pointer of 'a * sourceLoc * cVFlags * 'a iDeclarator_type 
  | D_reference of 'a * sourceLoc * 'a iDeclarator_type 
  | D_func of 'a * sourceLoc * 'a iDeclarator_type * 'a aSTTypeId_type list * cVFlags * 'a exceptionSpec_type option * 'a pQName_type (* = PQ_name  *) list * bool 
  | D_array of 'a * sourceLoc * 'a iDeclarator_type * 'a expression_type option * bool 
  | D_bitfield of 'a * sourceLoc * 'a pQName_type option * 'a expression_type * int 
  | D_ptrToMember of 'a * sourceLoc * 'a pQName_type * cVFlags * 'a iDeclarator_type 
  | D_grouping of 'a * sourceLoc * 'a iDeclarator_type 

(* *** DO NOT EDIT *** *)
(* ocaml type verbatim start *)


  (* extend iDeclarator_type *)
  | D_attribute of 'a * sourceLoc * 'a iDeclarator_type 
     (* the following is more convenient than AttributeSpecifierList *)
      * 'a attribute_type list list 

(* ocaml type verbatim end *)

and 'a fullExpressionAnnot_type = 'a * 'a declaration_type list 

and 'a aSTTypeof_type = 
  | TS_typeof_expr of 'a * 'a cType * 'a fullExpression_type 
  | TS_typeof_type of 'a * 'a cType * 'a aSTTypeId_type 

and 'a designator_type = 
  | FieldDesignator of 'a * sourceLoc * stringRef 
  | SubscriptDesignator of 'a * sourceLoc * 'a expression_type * 'a expression_type option * int * int 

and 'a attributeSpecifierList_type = 
  | AttributeSpecifierList_cons of 'a * 'a attributeSpecifier_type * 'a attributeSpecifierList_type 

and 'a attributeSpecifier_type = 
  | AttributeSpecifier_cons of 'a * 'a attribute_type * 'a attributeSpecifier_type 

and 'a attribute_type = 
  | AT_empty of 'a * sourceLoc 
  | AT_word of 'a * sourceLoc * stringRef 
  | AT_func of 'a * sourceLoc * stringRef * 'a argExpression_type list 

(* *** DO NOT EDIT *** *)
(* ocaml type verbatim start *)


(***************************** Variable *******************************)

(* this will be inserted in the middle in the ast type defintion *)
and 'a variable = {
  poly_var : 'a;
  loc : sourceLoc;
  (* might be None for abstract declarators (?) *)
  var_name : string option;

  (* var_type is circular for compound types that have an implicit 
   * typedef variable
   * the original pointer is NULL if flags contains DF_NAMESPACE, therefore, 
   * var_type might stay None after resolving circularities
   *)
  var_type : 'a cType option ref;
  flags : declFlags;
  (* value (varValue) might be circular and might be NULL *)
  value : 'a expression_type option ref; (* nullable comment *)
  defaultParam : 'a cType option;	(* nullable comment *)
  (* funcDefn is circular at least for destructor calls (in/t0009.cc)
   * then it points back to a member in the class
   * the original pointer might be NULL, so this might be None even
   * after resolving circularities
   *)
  funcDefn : 'a function_type option ref;
  (* overload is circurlar, it might contain this variable *)
  overload : 'a variable list ref;
  virtuallyOverride : 'a variable list;
  (* scope contains different things, depending on flag DF_NAMESPACE, 
   * see comments in variable.h
   *)
  scope : 'a scope option;

  templ_info : 'a templateInfo option;
}

(************************** TemplateInfo ******************************)

and 'a templateInfo = {
  poly_templ : 'a;
  template_params : 'a variable list;
  (* template_var is circular (in/t0026.cc) 
   * also nullable, might stay None
   *)
  template_var : 'a variable option ref;
  inherited_params : 'a inheritedTemplateParams list;
  (* instantiation_of is circular in in/t0026.cc, 
   * also nullable, might stay None
   *)
  instantiation_of : 'a variable option ref;
  instantiations : 'a variable list;
  (* specialization_of is circular in in/t0054.cc,
   * alse nullable, might stay None
   *)
  specialization_of : 'a variable option ref;
  specializations : 'a variable list;
  arguments : 'a sTemplateArgument list;
  inst_loc : sourceLoc;
  (* partial_instantiation_of is circular in in/t0219.cc,
   * alse nullable, might stay None
   *)
  partial_instantiation_of : 'a variable option ref;
  partial_instantiations : 'a variable list;
  arguments_to_primary : 'a sTemplateArgument list;
  defn_scope : 'a scope option;
  definition_template_info : 'a templateInfo option;
  instantiate_body : bool;
  instantiation_disallowed : bool;
  uninstantiated_default_args : int;
  dependent_bases : 'a cType list;
}


(*********************** InheritedTemplateParams **********************)

and 'a inheritedTemplateParams = {
  poly_inherited_templ : 'a;
  inherited_template_params : 'a variable list;
  (* circular in in/t0224.cc
   * not nullable, will always contain something 
   *)
  enclosing : 'a compound_info option ref;
}


(***************************** CType **********************************)

and 'a baseClass = {
  poly_base : 'a;
  compound : 'a compound_info;		(* the base class itself *)
  bc_access : accessKeyword;		(* public, protected ... *)
  is_virtual : bool;
}

and 'a compound_info = {
  compound_info_poly : 'a;
  (* fields stored in the super class NamedAtomicType *)
  compound_name : string option;	(* user assigned name ?? *)
  typedef_var : 'a variable;		(* implicit typdef variable ???? *)
  ci_access : accessKeyword;		(* accessibility in wider context *)

  (* superclass Scope *)
  compound_scope : 'a scope;

  (* fields of CompoundType itself:
   *     the stuff in comments is currently ommitted
   *)
  is_forward_decl : bool;
  is_transparent_union : bool;
  keyword : compoundType_Keyword; 	(* keyword used for this compound *)
  data_members : 'a variable list;		(* nonstatic data members *)
  bases : 'a baseClass list;		(* base classes *)

  (* subobj : ?? root of the subobject hierarchy *)

  conversion_operators : 'a variable list;
  friends : 'a variable list;
  inst_name : string option;	        (* name for debugging purposes *)

  (* syntax is circular eg in in/t0009.cc
   * nullable as well, so it might stay None
   *)
  syntax : 'a tS_classSpec_type (* = typeSpecifier_type *)  option ref; 

  (* ignore parameterizingScope : Scope; only used when on the scope stack *)

  (* self_type is circular for compounds like in t0009.cc 
   * might stay None after resolving circularities because the 
   * C++ pointer might be NULL
   *)
  self_type : 'a cType option ref;	(* type of the compound *)
}

(* tS_classSpec_type is a typeSpecifier_type built with TS_classSpec *)
and 'a tS_classSpec_type = 'a typeSpecifier_type

and 'a enumType_Value_type = 'a * string * nativeint

and 'a atomicType = 
    (* the subtype NamedAtomicType contains the following constructors:
     * CompoundType, PseudoInstantiation, EnumType, TypeVariable, 
     * DependentQType
     * (i.e, everything apart from SimpleType)
     *)

  | SimpleType of 'a * simpleTypeId

      (* IMPORTANT: if one adds more fields to CompoundType one has also to 
       * change PseudoInstantiation and its serialization
       *)
      (* CompoundType( compound info) *)
      (* 'a annotation is in compound_info *)
  | CompoundType of 'a compound_info

      (* PseudoInstantiation( user given name, ?, public/protected, 
       *           original class template info record, template arguments)
       * variable might be void (regtest 568 , in/t0566.cc)
       *)
  | PseudoInstantiation of 'a * string * 'a variable option * accessKeyword * 
      'a compound_info * 'a sTemplateArgument list

      (* EnumType( user given name, ?, public/protected, 
	             constants, has_negatives)
       *    ignore the next value field 
       *)
  | EnumType of 'a * string option * 'a variable option * accessKeyword * 
      'a enumType_Value_type list * bool

      (* TypeVariable( user given name, ?, public/protected)  *)
  | TypeVariable of 'a * string * 'a variable * accessKeyword

      (* DependentQType( user given name, ?, public/protected, 
       *                 template param/pseudo inst, following name components 
       *)
  | DependentQType of 'a * string * 'a variable * accessKeyword * 
      'a atomicType * 'a pQName_type


(* compoundType is an atomicType built with Compoundtype *)
and 'a compoundType = 'a atomicType

(* a enumType is an atomicType built with EnumType *)
and 'a enumType = 'a atomicType

(* a namedAtomictype is an atomicType built *NOT* with SimpleType *)
and 'a namedAtomicType = 'a atomicType

and 'a cType = 
  | CVAtomicType of 'a * cVFlags * 'a atomicType
      (* PointerType( volatile, pointed type) *)
  | PointerType of 'a * cVFlags * 'a cType
      (* ReferenceType( referenced type ) *)
  | ReferenceType of 'a * 'a cType
      (* FunctionType(flags, return type, parameter list, exception spec)
       * where exceptions spec is either
       *   | None       no exception spec    (* nullable comment *)
       *   | Some list  list of specified exceptions (which can be empty)
       *)
  | FunctionType of 'a * function_flags * 'a cType * 'a variable list * 
      'a cType list option
      (* ArrayType( element type, size )*)
  | ArrayType of 'a * 'a cType * array_size
      (* PointerToMemberType( ?, volatile, type of pointed member ) *)
  | PointerToMemberType of 'a * 'a atomicType (* = NamedAtomicType *) * 
      cVFlags * 'a cType

  (* functionType is a cType build with the FunctionType constructor *)
and 'a functionType = 'a cType

(***************************** TemplateArgument ******************************)

and 'a sTemplateArgument =
                          (* not yet resolved into a valid template argument *)
  | STA_NONE of 'a
                          (* type argument *)
  | STA_TYPE of 'a * 'a cType 
                          (* int argument *)
  | STA_INT of 'a * int   
                          (* enum argument *)
  | STA_ENUMERATOR of 'a * 'a variable 
                          (* reference to global object *)
  | STA_REFERENCE of 'a * 'a variable
                          (* pointer to global object *)
  | STA_POINTER of 'a * 'a variable
                          (* pointer to class member *)
  | STA_MEMBER of 'a * 'a variable
                          (* value-dependent expression *)
  | STA_DEPEXPR of 'a * 'a expression_type
                          (* template argument (not implemented) *)
  | STA_TEMPLATE of 'a
                          (* private to mtype: bind var to AtomicType *)
  | STA_ATOMIC of 'a * 'a atomicType


(***************************** Scope ******************************************)

and 'a scope = {
  poly_scope : 'a;
  variables : (string, 'a variable) Hashtbl.t;
  type_tags : (string, 'a variable) Hashtbl.t;
  parent_scope : 'a scope option;
  scope_kind : scopeKind;
  (* namespace_var is circular:
   * if namespace_var points to a Variable with DF_NAMESPACE 
   * then the scope field of this variable might point back here
   * (might also be always the case).
   * namespace_var is also nullable, it might therefore stay None.
   *)
  namespace_var : 'a variable option ref;
  scope_template_params : 'a variable list;
  parameterized_entity : 'a variable option;
}

(* ocaml type verbatim end *)


