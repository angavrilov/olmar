(* This file provides functions that decompose any
   tree node into an annotated list of fields. It also
   reimplements the functionality of the Superast module,
   using the decomposition for initial traversal.

   This module is intended to be used as a base for
   tree dumping/tree exploration facilities of any kind.
   Based on code from superast.ml and ast_iter.ml *)

open Cc_ml_types
open Ml_ctype
open Cc_ast_gen_type
open Ast_annotation
open Ast_util

type 'a super_ast =
  | TranslationUnit_type of 'a translationUnit_type
  | TopForm_type of 'a topForm_type
  | Function_type of 'a function_type
  | MemberInit_type of 'a memberInit_type
  | Declaration_type of 'a declaration_type
  | ASTTypeId_type of 'a aSTTypeId_type
  | PQName_type of 'a pQName_type
  | TypeSpecifier_type of 'a typeSpecifier_type
  | BaseClassSpec_type of 'a baseClassSpec_type
  | Enumerator_type of 'a enumerator_type
  | MemberList_type of 'a memberList_type
  | Member_type of 'a member_type
  | ExceptionSpec_type of 'a exceptionSpec_type
  | OperatorName_type of 'a operatorName_type
  | Statement_type of 'a statement_type
  | Condition_type of 'a condition_type
  | Handler_type of 'a handler_type
  | Expression_type of 'a expression_type
  | FullExpression_type of 'a fullExpression_type
  | ArgExpression_type of 'a argExpression_type
  | ArgExpressionListOpt_type of 'a argExpressionListOpt_type
  | Initializer_type of 'a initializer_type
  | TemplateDeclaration_type of 'a templateDeclaration_type
  | TemplateParameter_type of 'a templateParameter_type
  | TemplateArgument_type of 'a templateArgument_type
  | NamespaceDecl_type of 'a namespaceDecl_type
  | Declarator_type of 'a declarator_type
  | IDeclarator_type of 'a iDeclarator_type
  | FullExpressionAnnot_type of 'a fullExpressionAnnot_type
  | ASTTypeof_type of 'a aSTTypeof_type
  | Designator_type of 'a designator_type
  | Attribute_type of 'a attribute_type
  | Variable of 'a variable
  | TemplateInfo of 'a templateInfo
  | InheritedTemplateParams of 'a inheritedTemplateParams
  | BaseClass of 'a baseClass
  | Compound_info of 'a compound_info
  | EnumType_Value_type of 'a enumType_Value_type
  | AtomicType of 'a atomicType
  | CType of 'a cType
  | STemplateArgument of 'a sTemplateArgument
  | Scope of 'a scope 
  (* Unexistant index placeholder *)
  | NoAstNode

and 'a node_field =
  (* Node fields *)
  | NodeField of 'a super_ast
  | OptNodeField of 'a super_ast option
  | NodeListField of 'a super_ast list
  | NodeListListField of 'a super_ast list list
  | OptNodeListField of 'a super_ast list option
  | StringNodeTableField of (string * 'a super_ast) list
  (* POD fields *)
  | OptField of 'a node_field option
  | ListField of 'a node_field list
  | IntField of int
  | Int32Field of int32
  | FloatField of float
  | NativeintField of nativeint
  | StringField of string
  | AnnotField of 'a
  | SourceLocField of (string * int * int)
  | BoolField of bool
  | DeclFlagField of declFlag list
  | STypeIDField of simpleTypeId
  | TypeIntrField of typeIntr  
  | AccessKwdField of accessKeyword
  | CVFlagsField of cVFlag list
  | OverldOpField of overloadableOp 
  | UnaryOpField of unaryOp
  | EffectOpField of effectOp
  | BinaryOpField of binaryOp
  | CastKwdField of castKeyword
  | FunFlagsField of function_flags
  | DeclCtxField of declaratorContext
  | SkopeKindField of scopeKind
  | ArraySizeField of array_size
  | CompoundTypeField of compoundType_Keyword
  | TemplKindField of templateThingKind
  
module MetaBuilder :
sig
  val get_node_meta : 'a super_ast -> string * (string * 'a node_field) list
  val node_loc : 'a super_ast -> sourceLoc option
  val node_annotation : 'a super_ast -> 'a
end
=
struct
let annotation_fun a = AnnotField a
let bool_fun (b : bool) = BoolField b
let int_fun (i : int) = IntField i
let nativeint_fun (i : nativeint) = NativeintField i
let int32_fun (i : int32) = Int32Field i
let float_fun (x : float) = FloatField x
let string_fun (s : string) = StringField s
let sourceLoc_fun l = SourceLocField l
let declFlags_fun l = DeclFlagField l
let simpleTypeId_fun(id : simpleTypeId) = STypeIDField id
let typeIntr_fun(keyword : typeIntr) = TypeIntrField keyword
let accessKeyword_fun(keyword : accessKeyword) = AccessKwdField keyword
let cVFlags_fun(fl : cVFlag list) = CVFlagsField fl
let overloadableOp_fun(op :overloadableOp) = OverldOpField op
let unaryOp_fun(op : unaryOp) = UnaryOpField op
let effectOp_fun(op : effectOp) = EffectOpField op
let binaryOp_fun(op : binaryOp) = BinaryOpField op
let castKeyword_fun(keyword : castKeyword) = CastKwdField keyword
let function_flags_fun(flags : function_flags) = FunFlagsField flags
let declaratorContext_fun(context : declaratorContext) = DeclCtxField context
let scopeKind_fun(sk : scopeKind) = SkopeKindField sk
let array_size_fun sz = ArraySizeField sz
let compoundType_Keyword_fun kw = CompoundTypeField kw
let templ_kind_fun(kind : templateThingKind) = TemplKindField kind

let node x v = NodeField (x v)
let opt_field f = function
  | None -> OptField None
  | Some x -> OptField (Some (f x)) 
let opt_node f = function
  | None -> OptNodeField None
  | Some x -> OptNodeField (Some (f x)) 
let list_field f l =
    ListField (List.map f l)
let list_node f l =
    NodeListField (List.map f l)
let list_list_node f l =
    NodeListListField (List.map (List.map f) l)
let opt_list_node f = function
  | None -> OptNodeListField None
  | Some x -> OptNodeListField (Some (List.map f x)) 
    
(***************** variable ***************************)

let variable_fun v = Variable v
let templ_info_fun ti = TemplateInfo ti
let inherited_templ_params_fun itp = InheritedTemplateParams itp
let baseClass_fun baseClass = BaseClass baseClass
let compound_info_fun i = Compound_info i    
let enum_value_fun x = EnumType_Value_type x
let atomicType_fun x = AtomicType x  
let cType_fun x = CType x
let sTemplateArgument_fun ta = STemplateArgument ta
let scope_fun s = Scope s
let translationUnit_fun x = TranslationUnit_type x
let topForm_fun x = TopForm_type x
let func_fun f = Function_type f
let memberInit_fun m = MemberInit_type m
let declaration_fun x = Declaration_type x
let aSTTypeId_fun x = ASTTypeId_type x
let pQName_fun x = PQName_type x
let typeSpecifier_fun x = TypeSpecifier_type x
let baseClassSpec_fun x = BaseClassSpec_type x
let enumerator_fun x = Enumerator_type x
let memberList_fun x = MemberList_type x
let member_fun x = Member_type x
let declarator_fun x = Declarator_type x
let iDeclarator_fun x = IDeclarator_type x
let exceptionSpec_fun x = ExceptionSpec_type x
let operatorName_fun x = OperatorName_type x
let statement_fun x = Statement_type x
let condition_fun x = Condition_type x
let handler_fun x = Handler_type x        
let expression_fun x = Expression_type x
let fullExpression_fun x = FullExpression_type x        
let argExpression_fun x = ArgExpression_type x
let argExpressionListOpt_fun x = ArgExpressionListOpt_type x
let init_fun x = Initializer_type x
let templateDeclaration_fun f = TemplateDeclaration_type f
let templateParameter_fun x = TemplateParameter_type x        
let templateArgument_fun x = TemplateArgument_type x
let namespaceDecl_fun x = NamespaceDecl_type x
let fullExpressionAnnot_fun x = FullExpressionAnnot_type x
let aSTTypeof_fun x = ASTTypeof_type x
let designator_fun x = Designator_type x
let attribute_fun x = Attribute_type  x

let variable_split (v : 'a variable) =
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {            
    poly_var = v.poly_var; loc = v.loc; var_name = v.var_name;
    var_type = v.var_type; flags = v.flags; value = v.value;
    defaultParam = v.defaultParam; funcDefn = v.funcDefn;
    overload = v.overload; virtuallyOverride = v.virtuallyOverride;
    scope = v.scope; templ_info = v.templ_info;
  }
  in [ 
    "annot", annotation_fun v.poly_var;
    "loc", sourceLoc_fun v.loc;
    ".var_name", opt_field string_fun v.var_name;
    ".var_type'", opt_node cType_fun !(v.var_type);
    ".flags", declFlags_fun v.flags;
    ".value'", opt_node expression_fun !(v.value);
    ".defaultParam", opt_node cType_fun v.defaultParam;
    ".funcDefn", opt_node func_fun !(v.funcDefn);
    ".overload'", list_node variable_fun !(v.overload);
    ".virtuallyOverride", list_node variable_fun v.virtuallyOverride;
    ".scope", opt_node scope_fun v.scope;
    ".templ_info", opt_node templ_info_fun v.templ_info;
  ]

(**************** templateInfo ************************)

let templ_info_split ti =
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {
    poly_templ = ti.poly_templ; templ_kind = ti.templ_kind;
    template_params = ti.template_params;
    template_var = ti.template_var; inherited_params = ti.inherited_params; 
    instantiation_of = ti.instantiation_of; 
    instantiations = ti.instantiations; 
    specialization_of = ti.specialization_of; 
    specializations = ti.specializations; arguments = ti.arguments; 
    inst_loc = ti.inst_loc; 
    partial_instantiation_of = ti.partial_instantiation_of; 
    partial_instantiations = ti.partial_instantiations; 
    arguments_to_primary = ti.arguments_to_primary; 
    defn_scope = ti.defn_scope; 
    definition_template_info = ti.definition_template_info; 
    instantiate_body = ti.instantiate_body; 
    instantiation_disallowed = ti.instantiation_disallowed; 
    uninstantiated_default_args = ti.uninstantiated_default_args; 
    dependent_bases = ti.dependent_bases;
  }
  in
    [
      "annot", annotation_fun ti.poly_templ;
      "loc", sourceLoc_fun ti.inst_loc;

      ".templ_kind", templ_kind_fun ti.templ_kind;
      ".template_params", list_node variable_fun ti.template_params;

      (* POSSIBLY CIRCULAR *)
      ".template_var'", opt_node variable_fun !(ti.template_var);
      ".inherited_params", list_node inherited_templ_params_fun ti.inherited_params;

      (* POSSIBLY CIRCULAR *)
      ".instantiation_of'", opt_node variable_fun !(ti.instantiation_of);
      ".instantiations", list_node variable_fun ti.instantiations;

      (* POSSIBLY CIRCULAR *)
      ".specialization_of'", opt_node variable_fun !(ti.specialization_of);
      ".specializations", list_node variable_fun ti.specializations;
      ".arguments", list_node sTemplateArgument_fun ti.arguments;

      (* POSSIBLY CIRCULAR *)
      ".partial_instantiation_of'", opt_node variable_fun !(ti.partial_instantiation_of);
      ".partial_instantiations", list_node variable_fun ti.partial_instantiations;
      ".arguments_to_primary", list_node sTemplateArgument_fun ti.arguments_to_primary;
      ".defn_scope", opt_node scope_fun ti.defn_scope;
      ".definition_template_info", opt_node templ_info_fun ti.definition_template_info;
      ".instantiate_body", bool_fun ti.instantiate_body;
      ".instantiation_disallowed", bool_fun ti.instantiation_disallowed;
      ".uninstantiated_default_args", int_fun ti.uninstantiated_default_args;
      ".dependent_bases", list_node cType_fun ti.dependent_bases;
    ]

(************* inheritedTemplateParams ****************)

let inherited_templ_params_split itp =
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {
    poly_inherited_templ = itp.poly_inherited_templ;
    inherited_template_params = itp.inherited_template_params;
    enclosing = itp.enclosing;
  }
  in
    assert(!(itp.enclosing) <> None);
    [
      "annot", annotation_fun itp.poly_inherited_templ;
      ".inherited_template_params", list_node variable_fun itp.inherited_template_params;

      (* POSSIBLY CIRCULAR *)
      ".enclosing'", opt_node compound_info_fun !(itp.enclosing);
    ]

(***************** cType ******************************)

let baseClass_split baseClass =
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {
    poly_base = baseClass.poly_base; compound = baseClass.compound;
    bc_access = baseClass.bc_access; is_virtual = baseClass.is_virtual
  }
  in
    [
      "annot", annotation_fun baseClass.poly_base;
      ".compound", node compound_info_fun baseClass.compound;
      ".bc_access", accessKeyword_fun baseClass.bc_access;
      ".is_virtual", bool_fun baseClass.is_virtual;
    ]

let compound_info_split i = 
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {
    compound_info_poly = i.compound_info_poly;
    compound_name = i.compound_name; typedef_var = i.typedef_var;
    ci_access = i.ci_access; compound_scope = i.compound_scope;
    is_forward_decl = i.is_forward_decl;
    is_transparent_union = i.is_transparent_union; keyword = i.keyword;
    data_members = i.data_members; bases = i.bases;
    conversion_operators = i.conversion_operators;
    friends = i.friends; inst_name = i.inst_name; syntax = i.syntax;
    self_type = i.self_type;
  }
  in
    assert(match !(i.syntax) with
        | None
        | Some(TS_classSpec _) -> true
        | _ -> false);
    [
      "annot", annotation_fun i.compound_info_poly;
      ".compound_name", opt_field string_fun i.compound_name;
      ".typedef_var", node variable_fun i.typedef_var;
      ".ci_access", accessKeyword_fun i.ci_access;
      ".compound_scope", node scope_fun i.compound_scope;
      ".is_forward_decl", bool_fun i.is_forward_decl;
      ".is_transparent_union", bool_fun i.is_transparent_union;
      ".keyword", compoundType_Keyword_fun i.keyword;
      ".data_members", list_node variable_fun i.data_members;
      ".bases", list_node baseClass_fun i.bases;
      ".conversion_operators", list_node variable_fun i.conversion_operators;
      ".friends", list_node variable_fun i.friends;

      (* POSSIBLY CIRCULAR *)
      ".syntax'", opt_node typeSpecifier_fun !(i.syntax);
      ".inst_name", opt_field string_fun i.inst_name;

      (* POSSIBLY CIRCULAR *)
      ".self_type'", opt_node cType_fun !(i.self_type)
    ]

let enum_value_split (annot, string, nativeint) =
  [
    "annot", annotation_fun annot;
    "string", string_fun string;
    "int", nativeint_fun nativeint;
  ]
  
let atomicType_split = function
      | SimpleType(annot, simpleTypeId) -> 
        "SimpleType", [
            "annot", annotation_fun annot;
            "simpleTypeId", simpleTypeId_fun simpleTypeId;
        ]

      | CompoundType(compound_info) ->
        "CompoundType", [
            "compound_info", node compound_info_fun compound_info;
        ]

      | PseudoInstantiation(annot, str, variable_opt, accessKeyword, 
                compound_info, sTemplateArgument_list) ->
        "PseudoInstantiation", [
            "annot", annotation_fun annot;
          "str", string_fun str;
          "variable", opt_node variable_fun variable_opt;
          "accessKeyword", accessKeyword_fun accessKeyword;
          "compound_info'", opt_node atomicType_fun !compound_info;
          "templateArguments", list_node sTemplateArgument_fun sTemplateArgument_list;
        ]

      | EnumType(annot, string, variable, accessKeyword, 
         enum_value_list, has_negatives) ->
        "EnumType", [
          "annot", annotation_fun annot;
          "string", opt_field string_fun string;
          "variable", opt_node variable_fun variable;
          "accessKeyword", accessKeyword_fun accessKeyword;
          "values", list_node enum_value_fun enum_value_list;
          "has_negatives", bool_fun has_negatives;
        ]

      | TypeVariable(annot, string, variable, accessKeyword) ->
        "TypeVariable", [
          "annot", annotation_fun annot;
          "string", string_fun string;
          "variable", node variable_fun variable;
          "accessKeyword", accessKeyword_fun accessKeyword;
        ]

      | DependentQType(annot, string, variable, 
              accessKeyword, atomic, pq_name) ->
        "DependentQType", [
          "annot", annotation_fun annot;
          "string", string_fun string;
          "variable", node variable_fun variable;
          "accessKeyword", accessKeyword_fun accessKeyword;
          "atomic", node atomicType_fun atomic;
          "pq_name", node pQName_fun pq_name;
        ]    

      | TemplateTypeVariable(annot, string, variable, accessKeyword, params) ->
        "TemplateTypeVariable", [
          "annot", annotation_fun annot;
          "string", string_fun string;
          "variable", node variable_fun variable;
          "accessKeyword", accessKeyword_fun accessKeyword;
          "params", list_node variable_fun params;
        ]

let cType_split = function
      | CVAtomicType(annot, cVFlags, atomicType) ->
        "CVAtomicType", [
          "annot", annotation_fun annot;
          "cVFlags", cVFlags_fun cVFlags;
          "atomicType", node atomicType_fun atomicType;
        ]

      | PointerType(annot, cVFlags, cType) ->
        "PointerType", [
          "annot", annotation_fun annot;
          "cVFlags", cVFlags_fun cVFlags;
          "cType", node cType_fun cType;
        ]

      | ReferenceType(annot, cType) ->
        "ReferenceType", [
          "annot", annotation_fun annot;
          "cType", node cType_fun cType
        ]

      | FunctionType(annot, function_flags, cType, 
             variable_list, cType_list_opt) ->
        "FunctionType", [
          "annot", annotation_fun annot;
          "function_flags", function_flags_fun function_flags;
          "cType", node cType_fun cType;
          "variables", list_node variable_fun variable_list;
          "cTypes", opt_list_node cType_fun cType_list_opt;
        ]

      | ArrayType(annot, cType, array_size) ->
        "ArrayType", [
          "annot", annotation_fun annot;
          "cType", node cType_fun cType;
          "array_size", array_size_fun array_size;
        ]

      | PointerToMemberType(annot, atomicType (* = NamedAtomicType *), 
                cVFlags, cType) ->
          assert(match atomicType with 
               | SimpleType _ -> false
               | CompoundType _
               | PseudoInstantiation _
               | EnumType _
               | TypeVariable _
               | TemplateTypeVariable _
               | DependentQType _ -> true);
        "PointerToMemberType", [
          "annot", annotation_fun annot;
          "atomicType", node atomicType_fun atomicType;
          "cVFlags", cVFlags_fun cVFlags;
          "cType", node cType_fun cType
        ]

      | DependentSizedArrayType(annot, cType, array_size) ->
        "DependentSizedArrayType", [
          "annot", annotation_fun annot;
          "cType", node cType_fun cType;
          "array_size", node expression_fun array_size;
        ]

let sTemplateArgument_split = function
    | STA_NONE annot -> 
        "STA_NONE", [ "annot", annotation_fun annot ]

    | STA_TYPE(annot, cType) -> 
        "STA_TYPE", [
            "annot", annotation_fun annot;
            "cType'", opt_node cType_fun !cType;
        ]

    | STA_INT(annot, int) -> 
        "STA_INT", [
        "annot", annotation_fun annot;
        "int", int32_fun int;
        ]

    | STA_ENUMERATOR(annot, variable) -> 
        "STA_ENUMERATOR", [
        "annot", annotation_fun annot;
        "variable", node variable_fun variable;
        ]

    | STA_REFERENCE(annot, variable) -> 
        "STA_REFERENCE", [
        "annot", annotation_fun annot;
        "variable", node variable_fun variable;
        ]

    | STA_POINTER(annot, variable) -> 
        "STA_POINTER", [
        "annot", annotation_fun annot;
        "variable", node variable_fun variable;
        ]

    | STA_MEMBER(annot, variable) -> 
        "STA_MEMBER", [
        "annot", annotation_fun annot;
        "variable", node variable_fun variable;
        ]

    | STA_DEPEXPR(annot, expression) -> 
        "STA_DEPEXPR", [
        "annot", annotation_fun annot;
        "expression", node expression_fun expression
        ]

    | STA_TEMPLATE(annot, atomicType) -> 
        "STA_TEMPLATE", [
        "annot", annotation_fun annot;
        "atomicType", node atomicType_fun atomicType;
        ]

    | STA_ATOMIC(annot, atomicType) -> 
        "STA_ATOMIC", [
        "annot", annotation_fun annot;
        "atomicType", node atomicType_fun atomicType;
        ]

let scope_split s = 
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {
    poly_scope = s.poly_scope; variables = s.variables; 
    type_tags = s.type_tags; parent_scope = s.parent_scope;
    scope_kind = s.scope_kind; namespace_var = s.namespace_var;
    scope_template_params = s.scope_template_params; 
    parameterized_entity = s.parameterized_entity
  }
  in
    [
      "annot", annotation_fun s.poly_scope;
      ".variables", StringNodeTableField 
        (Hashtbl.fold (fun str var l -> (str, variable_fun var) :: l) s.variables []);
      ".type_tags", StringNodeTableField 
        (Hashtbl.fold (fun str var l -> (str, variable_fun var) :: l) s.type_tags []);
      ".parent_scope", opt_node scope_fun s.parent_scope;
      ".scope_kind", scopeKind_fun s.scope_kind;
      ".namespace_var'", opt_node variable_fun !(s.namespace_var);
      ".scope_template_params", list_node variable_fun s.scope_template_params;
      ".parameterized_entity", opt_node variable_fun s.parameterized_entity;
    ]


(***************** generated ast nodes ****************)

let translationUnit_split
    ((annot, topForm_list, scope_opt) : 'a translationUnit_type) =
  [
    "annot", annotation_fun annot;
    "topForms", list_node topForm_fun topForm_list;
    "scope", opt_node scope_fun scope_opt;
  ]

let topForm_split = function
      | TF_decl(annot, sourceLoc, declaration) -> 
        "TF_decl", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "declaration", node declaration_fun declaration;
        ]

    | TF_func(annot, sourceLoc, func) -> 
        "TF_func", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "func", node func_fun func;
        ]

    | TF_template(annot, sourceLoc, templateDeclaration) -> 
        "TF_template", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "templateDeclaration", node templateDeclaration_fun templateDeclaration;
        ]

    | TF_explicitInst(annot, sourceLoc, declFlags, declaration) -> 
        "TF_explicitInst", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "declFlags", declFlags_fun declFlags;
        "declaration", node declaration_fun declaration
        ]

    | TF_linkage(annot, sourceLoc, stringRef, translationUnit) -> 
        "TF_linkage", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "stringRef", string_fun stringRef;
        "translationUnit", node translationUnit_fun translationUnit;
        ]

    | TF_one_linkage(annot, sourceLoc, stringRef, topForm) -> 
        "TF_one_linkage", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "stringRef", string_fun stringRef;
        "topForm", node topForm_fun topForm;
        ]

    | TF_asm(annot, sourceLoc, e_stringLit) -> 
        assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
        "TF_asm", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "e_stringLit", node expression_fun e_stringLit;
        ]

    | TF_namespaceDefn(annot, sourceLoc, stringRef_opt, topForm_list) -> 
        "TF_namespaceDefn", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "stringRef", opt_field string_fun stringRef_opt;
        "topForms", list_node topForm_fun topForm_list;
        ]

    | TF_namespaceDecl(annot, sourceLoc, namespaceDecl) -> 
        "TF_namespaceDecl", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "namespaceDecl", node namespaceDecl_fun namespaceDecl;
        ]

        
let func_split (annot, declFlags, typeSpecifier, declarator, memberInit_list, 
         s_compound_opt, handler_list, func, variable_opt_1, 
         variable_opt_2, statement_opt, bool) =
    assert(match s_compound_opt with
         | None -> true
         | Some s_compound ->
         match s_compound with 
           | S_compound _ -> true 
           | _ -> false);
    assert(match func with 
      | FunctionType _ -> true
      | _ -> false);
    [
    "annot", annotation_fun annot;
    "declFlags", declFlags_fun declFlags;
    "typeSpecifier", node typeSpecifier_fun typeSpecifier;
    "declarator", node declarator_fun declarator;
    "memberInit", list_node memberInit_fun memberInit_list;
    "s_compound", opt_node statement_fun s_compound_opt;
    "handlers", list_node handler_fun handler_list;
    "func", node cType_fun func;
    "variable_1", opt_node variable_fun variable_opt_1;
    "variable_2", opt_node variable_fun variable_opt_2;
    "statement", opt_node statement_fun statement_opt;
    "bool", bool_fun bool;
    ]


let memberInit_split (annot, pQName, argExpression_list, 
           variable_opt_1, compound_opt, variable_opt_2, 
           full_expr_annot, statement_opt) =
    assert(match compound_opt with
    | None
    | Some(CompoundType _) -> true
    | _ -> false);
    [
    "annot", annotation_fun annot;
    "pQName", node pQName_fun pQName;
    "argExpressions", list_node argExpression_fun argExpression_list;
    "variable_1", opt_node variable_fun variable_opt_1;
    "compound", opt_node atomicType_fun compound_opt;
    "variable_2", opt_node variable_fun variable_opt_2;
    "full_expr_annot", node fullExpressionAnnot_fun full_expr_annot;
    "statement", opt_node statement_fun statement_opt
    ]


let declaration_split (annot, declFlags, typeSpecifier, declarator_list) =
    [
    "annot", annotation_fun annot;
    "declFlags", declFlags_fun declFlags;
    "typeSpecifier", node typeSpecifier_fun typeSpecifier;
    "declarators", list_node declarator_fun declarator_list;
    ]

    
let aSTTypeId_split (annot, typeSpecifier, declarator) =
    [
    "annot", annotation_fun annot;
    "typeSpecifier", node typeSpecifier_fun typeSpecifier;
    "declarator", node declarator_fun declarator
    ]

    
let pQName_split = function  
    | PQ_qualifier(annot, sourceLoc, stringRef_opt, 
               templateArgument_opt, pQName, 
              variable_opt, s_template_arg_list) -> 
        "PQ_qualifier", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "stringRef", opt_field string_fun stringRef_opt;
        "templateArgument", opt_node templateArgument_fun templateArgument_opt;
        "pQName", node pQName_fun pQName;
        "variable", opt_node variable_fun variable_opt;
        "s_template_args", list_node sTemplateArgument_fun s_template_arg_list;
        ]

    | PQ_name(annot, sourceLoc, stringRef) -> 
        "PQ_name", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "stringRef", string_fun stringRef;
        ]

    | PQ_operator(annot, sourceLoc, operatorName, stringRef) -> 
        "PQ_operator", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "operatorName", node operatorName_fun operatorName;
        "stringRef", string_fun stringRef;
        ]

    | PQ_template(annot, sourceLoc, stringRef, templateArgument_opt, 
             s_template_arg_list) -> 
        "PQ_template", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "stringRef", string_fun stringRef;
        "templateArgument", opt_node templateArgument_fun templateArgument_opt;
        "s_template_args", list_node sTemplateArgument_fun s_template_arg_list;
        ]

    | PQ_variable(annot, sourceLoc, variable) -> 
        "PQ_variable", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "variable", node variable_fun variable;
        ]

        
let typeSpecifier_split = function
    | TS_name(annot, sourceLoc, cVFlags, pQName, bool, 
         var_opt_1, var_opt_2) -> 
         "TS_name", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "cVFlags", cVFlags_fun cVFlags;
        "pQName", node pQName_fun pQName;
        "bool", bool_fun bool;
        "var_opt_1", opt_node variable_fun var_opt_1;
        "var_opt_2", opt_node variable_fun var_opt_2;
        ]

    | TS_simple(annot, sourceLoc, cVFlags, simpleTypeId) -> 
        "TS_simple", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "cVFlags", cVFlags_fun cVFlags;
        "simpleTypeId", simpleTypeId_fun simpleTypeId;
        ]

    | TS_elaborated(annot, sourceLoc, cVFlags, typeIntr, 
               pQName, namedAtomicType_opt) -> 
        assert(match namedAtomicType_opt with
          | Some(SimpleType _) -> false
          | _ -> true);
        "TS_elaborated", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "cVFlags", cVFlags_fun cVFlags;
        "typeIntr", typeIntr_fun typeIntr;
        "pQName", node pQName_fun pQName;
        "namedAtomicType", opt_node atomicType_fun namedAtomicType_opt;
        ]

    | TS_classSpec(annot, sourceLoc, cVFlags, typeIntr, pQName_opt, 
               baseClassSpec_list, memberList, compoundType) -> 
        assert(match compoundType with
          | CompoundType _ -> true
          | _ -> false);
        "TS_classSpec", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "cVFlags", cVFlags_fun cVFlags;
        "typeIntr", typeIntr_fun typeIntr;
        "pQName", opt_node pQName_fun pQName_opt;
        "baseClassSpec", list_node baseClassSpec_fun baseClassSpec_list;
        "memberList", node memberList_fun memberList;
        "compoundType", node atomicType_fun compoundType;
        ]

    | TS_enumSpec(annot, sourceLoc, cVFlags, 
              stringRef_opt, enumerator_list, enumType) -> 
        assert(match enumType with 
          | EnumType _ -> true
          | _ -> false);
        "TS_enumSpec", [          
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "cVFlags", cVFlags_fun cVFlags;
        "stringRef", opt_field string_fun stringRef_opt;
        "enumerator_list", list_node enumerator_fun enumerator_list;
        "enumType", node atomicType_fun enumType;
        ]

    | TS_type(annot, sourceLoc, cVFlags, cType) -> 
        "TS_type", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "cVFlags", cVFlags_fun cVFlags;
        "cType", node cType_fun cType;
        ]

    | TS_typeof(annot, sourceLoc, cVFlags, aSTTypeof) -> 
        "TS_typeof", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "cVFlags", cVFlags_fun cVFlags;
        "aSTTypeof", node aSTTypeof_fun aSTTypeof;
        ]



let baseClassSpec_split
    (annot, bool, accessKeyword, pQName, compoundType_opt) =
    assert(match compoundType_opt with
         | None
         | Some(CompoundType _ ) -> true
         | _ -> false);
    [
    "annot", annotation_fun annot;
    "bool", bool_fun bool;
    "accessKeyword", accessKeyword_fun accessKeyword;
    "pQName", node pQName_fun pQName;
    "compoundType", opt_node atomicType_fun compoundType_opt
    ]

    
let enumerator_split (annot, sourceLoc, stringRef, 
           expression_opt, variable, int32) =
    [
    "annot", annotation_fun annot;
    "loc", sourceLoc_fun sourceLoc;
    "stringRef", string_fun stringRef;
    "expression", opt_node expression_fun expression_opt;
    "variable", node variable_fun variable;
    "int32", int32_fun int32
    ]

    
let memberList_split (annot, member_list) =
   [
    "annot", annotation_fun annot;
    "members", list_node member_fun member_list;
   ]


let member_split = function
    | MR_decl(annot, sourceLoc, declaration) -> 
        "MR_decl", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "declaration", node declaration_fun declaration;
        ]

    | MR_func(annot, sourceLoc, func) -> 
        "MR_func", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "func", node func_fun func;
        ]

    | MR_access(annot, sourceLoc, accessKeyword) -> 
        "MR_access", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "accessKeyword", accessKeyword_fun accessKeyword;
        ]

    | MR_usingDecl(annot, sourceLoc, nd_usingDecl) -> 
        assert(match nd_usingDecl with ND_usingDecl _ -> true | _ -> false);
        "MR_usingDecl", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "nd_usingDecl", node namespaceDecl_fun nd_usingDecl;
        ]

    | MR_template(annot, sourceLoc, templateDeclaration) -> 
        "MR_template", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "templateDeclaration", node templateDeclaration_fun templateDeclaration;
        ]


let declarator_split (annot, iDeclarator, init_opt, 
           variable_opt, ctype_opt, declaratorContext,
           statement_opt_ctor, statement_opt_dtor) =
  [
    "annot", annotation_fun annot;
    "iDeclarator", node iDeclarator_fun iDeclarator;
    "init_opt", opt_node init_fun init_opt;
    "variable_opt", opt_node variable_fun variable_opt;
    "ctype_opt", opt_node cType_fun ctype_opt;
    "declaratorContext", declaratorContext_fun declaratorContext;
    "statement_opt_ctor", opt_node statement_fun statement_opt_ctor;
    "statement_opt_dtor", opt_node statement_fun statement_opt_dtor
  ]


let iDeclarator_split = function
    | D_name(annot, sourceLoc, pQName_opt) -> 
        "D_name", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "pQName_opt", opt_node pQName_fun pQName_opt;
        ]

    | D_pointer(annot, sourceLoc, cVFlags, iDeclarator) -> 
        "D_pointer", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "cVFlags", cVFlags_fun cVFlags;
        "iDeclarator", node iDeclarator_fun iDeclarator;
        ]

    | D_reference(annot, sourceLoc, iDeclarator) -> 
        "D_reference", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "iDeclarator", node iDeclarator_fun iDeclarator;
        ]

    | D_func(annot, sourceLoc, iDeclarator, aSTTypeId_list, cVFlags, 
         exceptionSpec_opt, pq_name_list, bool) -> 
        assert(List.for_all (function | PQ_name _ -> true | _ -> false) 
             pq_name_list);
        "D_func", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "iDeclarator", node iDeclarator_fun iDeclarator;
        "aSTTypeIds", list_node aSTTypeId_fun aSTTypeId_list;
        "cVFlags", cVFlags_fun cVFlags;
        "exceptionSpec", opt_node exceptionSpec_fun exceptionSpec_opt;
        "pq_names", list_node pQName_fun pq_name_list;
        "bool", bool_fun bool;
        ]

    | D_array(annot, sourceLoc, iDeclarator, expression_opt, bool) -> 
        "D_array", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "iDeclarator", node iDeclarator_fun iDeclarator;
        "expression", opt_node expression_fun expression_opt;
        "bool", bool_fun bool;
        ]

    | D_bitfield(annot, sourceLoc, pQName_opt, expression, int) -> 
        "D_bitfield", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "pQName", opt_node pQName_fun pQName_opt;
        "expression", node expression_fun expression;
        "int", int_fun int;
        ]

    | D_ptrToMember(annot, sourceLoc, pQName, cVFlags, iDeclarator) -> 
        "D_ptrToMember", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "pQName", node pQName_fun pQName;
        "cVFlags", cVFlags_fun cVFlags;
        "iDeclarator", node iDeclarator_fun iDeclarator;
        ]

    | D_grouping(annot, sourceLoc, iDeclarator) -> 
        "D_grouping", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "iDeclarator", node iDeclarator_fun iDeclarator;
        ]

    | D_attribute(annot, sourceLoc, iDeclarator, attribute_list_list) ->
        "D_attribute", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "iDeclarator", node iDeclarator_fun iDeclarator;
        "attributes", list_list_node attribute_fun attribute_list_list;
        ]


let exceptionSpec_split (annot, aSTTypeId_list) =
  [
    "annot", annotation_fun annot;
    "aSTTypeIds", list_node aSTTypeId_fun aSTTypeId_list;
  ]

  
let operatorName_split = function
    | ON_newDel(annot, bool_is_new, bool_is_array) -> 
        "ON_newDel", [
        "annot", annotation_fun annot;
        "is_new", bool_fun bool_is_new;
        "is_array", bool_fun bool_is_array;
        ]

    | ON_operator(annot, overloadableOp) -> 
        "ON_operator", [
        "annot", annotation_fun annot;
        "op", overloadableOp_fun overloadableOp;
        ]

    | ON_conversion(annot, aSTTypeId) -> 
        "ON_conversion", [
        "annot", annotation_fun annot;
        "aSTTypeId", node aSTTypeId_fun aSTTypeId;
        ]
;;

let statement_split = function
    | S_skip(annot, sourceLoc) -> 
        "S_skip", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        ]

    | S_label(annot, sourceLoc, stringRef, statement) -> 
        "S_label", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "stringRef", string_fun stringRef;
        "stmt", node statement_fun statement;
        ]

    | S_case(annot, sourceLoc, expression, statement, int32) -> 
        "S_case", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "expr", node expression_fun expression;
        "stmt", node statement_fun statement;
        "int", int32_fun int32;
        ]

    | S_default(annot, sourceLoc, statement) -> 
        "S_default", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "stmt", node statement_fun statement;
        ]

    | S_expr(annot, sourceLoc, fullExpression) -> 
        "S_expr", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "full_expr", node fullExpression_fun fullExpression;
        ]

    | S_compound(annot, sourceLoc, statement_list) -> 
        "S_compound", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "stmts", list_node statement_fun statement_list;
        ]

    | S_if(annot, sourceLoc, condition, statement_then, statement_else) -> 
        "S_if", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "condition", node condition_fun condition;        
        "s_then", node statement_fun statement_then;
        "s_else", node statement_fun statement_else;
        ]

    | S_switch(annot, sourceLoc, condition, statement) -> 
        "S_switch", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "condition", node condition_fun condition;
        "stmt", node statement_fun statement;
        ]

    | S_while(annot, sourceLoc, condition, statement) -> 
        "S_while", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "condition", node condition_fun condition;
        "stmt", node statement_fun statement;
        ]

    | S_doWhile(annot, sourceLoc, statement, fullExpression) -> 
        "S_doWhile", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "stmt", node statement_fun statement;
        "full_expr", node fullExpression_fun fullExpression;
        ]

    | S_for(annot, sourceLoc, statement_init, condition, fullExpression, 
        statement_body) -> 
        "S_for", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "init", node statement_fun statement_init;
        "condition", node condition_fun condition;
        "step", node fullExpression_fun fullExpression;
        "body", node statement_fun statement_body;
        ]

    | S_break(annot, sourceLoc) -> 
        "S_break", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        ]

    | S_continue(annot, sourceLoc) -> 
        "S_continue", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        ]

    | S_return(annot, sourceLoc, fullExpression_opt, statement_opt) -> 
        "S_return", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "full_expr", opt_node fullExpression_fun fullExpression_opt;
        "stmt", opt_node statement_fun statement_opt;
        ]

    | S_goto(annot, sourceLoc, stringRef) -> 
        "S_goto", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "stringRef", string_fun stringRef;
        ]

    | S_decl(annot, sourceLoc, declaration) -> 
        "S_decl", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "decl", node declaration_fun declaration;
        ]

    | S_try(annot, sourceLoc, statement, handler_list) -> 
        "S_try", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "stmt", node statement_fun statement;
        "handlers", list_node handler_fun handler_list
        ]

    | S_asm(annot, sourceLoc, e_stringLit) -> 
        assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
        "S_asm", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "code", node expression_fun e_stringLit;
        ]

    | S_namespaceDecl(annot, sourceLoc, namespaceDecl) -> 
        "S_namespaceDecl", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "nsdecl", node namespaceDecl_fun namespaceDecl;
        ]

    | S_function(annot, sourceLoc, func) -> 
        "S_function", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "func",node func_fun func;
        ]

    | S_rangeCase(annot, sourceLoc, 
              expression_lo, expression_hi, statement, 
             label_lo, label_hi) -> 
        "S_rangeCase", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "lo", node expression_fun expression_lo;
        "hi", node expression_fun expression_hi;
        "stmt", node statement_fun statement;
        "int_lo", int_fun label_lo;
        "int_hi", int_fun label_hi
        ]

    | S_computedGoto(annot, sourceLoc, expression) -> 
        "S_computedGoto", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "expr", node expression_fun expression;
        ]


let condition_split = function
    | CN_expr(annot, fullExpression) -> 
        "CN_expr", [
        "annot", annotation_fun annot;
        "full_expr", node fullExpression_fun fullExpression;
        ]

    | CN_decl(annot, aSTTypeId) -> 
        "CN_decl", [
        "annot", annotation_fun annot;
        "aSTTypeId", node aSTTypeId_fun aSTTypeId;
        ]

        
let handler_split (annot, aSTTypeId, statement_body, variable_opt, 
        fullExpressionAnnot, expression_opt, statement_gdtor) =
  [
    "annot", annotation_fun annot;
    "aSTTypeId", node aSTTypeId_fun aSTTypeId;
    "body", node statement_fun statement_body;
    "var", opt_node variable_fun variable_opt;
    "full_expr_annot", node fullExpressionAnnot_fun fullExpressionAnnot;
    "expr", opt_node expression_fun expression_opt;
    "stmt", opt_node statement_fun statement_gdtor
  ]


let expression_split = function
    | E_boolLit(annot, type_opt, bool) -> 
        "E_boolLit", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "bool", bool_fun bool;
        ]

    | E_intLit(annot, type_opt, stringRef, ulong) -> 
        "E_intLit", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "stringRef", string_fun stringRef;
        "int32", int32_fun ulong;
        ]

    | E_floatLit(annot, type_opt, stringRef, double) -> 
        "E_floatLit", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "stringRef", string_fun stringRef;
        "float", float_fun double;
        ]

    | E_stringLit(annot, type_opt, stringRef, 
              e_stringLit_opt, stringRef_opt) -> 
        assert(match e_stringLit_opt with 
             | Some(E_stringLit _) -> true 
             | None -> true
             | _ -> false);
        "E_stringLit", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "stringRef", string_fun stringRef;
        "expr", opt_node expression_fun e_stringLit_opt;
        "stringRef", opt_field string_fun stringRef_opt;
        ]

    | E_charLit(annot, type_opt, stringRef, int32) -> 
        "E_charLit", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "stringRef", string_fun stringRef;
        "int32", int32_fun int32;
        ]

    | E_this(annot, type_opt, variable) -> 
        "E_this", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "variable", node variable_fun variable;
        ]

    | E_variable(annot, type_opt, pQName, var_opt, nondep_var_opt) -> 
        "E_variable", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "pQName", node pQName_fun pQName;
        "var", opt_node variable_fun var_opt;
        "nondep_var", opt_node variable_fun nondep_var_opt;
        ]

    | E_funCall(annot, type_opt, expression_func, 
            argExpression_list, expression_retobj_opt) -> 
        "E_funCall", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "expr", node expression_fun expression_func;
        "args", list_node argExpression_fun argExpression_list;
        "retobj", opt_node expression_fun expression_retobj_opt;
        ]

    | E_constructor(annot, type_opt, typeSpecifier, argExpression_list, 
            var_opt, bool, expression_opt) -> 
        "E_constructor", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "typeSpecifier", node typeSpecifier_fun typeSpecifier;
        "args", list_node argExpression_fun argExpression_list;
        "var", opt_node variable_fun var_opt;
        "bool", bool_fun bool;
        "expr", opt_node expression_fun expression_opt;
        ]

    | E_fieldAcc(annot, type_opt, expression, pQName, var_opt) -> 
        "E_fieldAcc", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "expr", node expression_fun expression;
        "pQName", node pQName_fun pQName;
        "field", opt_node variable_fun var_opt;
        ]

    | E_sizeof(annot, type_opt, expression, int) -> 
        "E_sizeof", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "expr", node expression_fun expression;
        "int", int_fun int;
        ]

    | E_unary(annot, type_opt, unaryOp, expression) -> 
        "E_unary", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "op", unaryOp_fun unaryOp;
        "expr", node expression_fun expression;
        ]

    | E_effect(annot, type_opt, effectOp, expression) -> 
        "E_effect", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "op", effectOp_fun effectOp;
        "expr", node expression_fun expression;
        ]

    | E_binary(annot, type_opt, expression_left, binaryOp, expression_right) -> 
        "E_binary", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "expr", node expression_fun expression_left;
        "op", binaryOp_fun binaryOp;
        "expr", node expression_fun expression_right;
        ]

    | E_addrOf(annot, type_opt, expression) -> 
        "E_addrOf", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "expr", node expression_fun expression;
        ]

    | E_deref(annot, type_opt, expression) -> 
        "E_deref", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "expr", node expression_fun expression;
        ]

    | E_cast(annot, type_opt, aSTTypeId, expression, bool) -> 
        "E_cast", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "aSTTypeId", node aSTTypeId_fun aSTTypeId;
        "expr", node expression_fun expression;
        "bool", bool_fun bool;
        ]

    | E_cond(annot, type_opt, expression_cond, expression_true, expression_false) -> 
        "E_cond", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "e_cond", node expression_fun expression_cond;
        "e_true", node expression_fun expression_true;
        "e_false", node expression_fun expression_false;
        ]

    | E_sizeofType(annot, type_opt, aSTTypeId, int, bool) -> 
        "E_sizeofType", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "aSTTypeId", node aSTTypeId_fun aSTTypeId;
        "int", int_fun int;
        "bool", bool_fun bool;
        ]

    | E_assign(annot, type_opt, expression_target, binaryOp, expression_src) -> 
        "E_assign", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "expr", node expression_fun expression_target;
        "op", binaryOp_fun binaryOp;
        "expr", node expression_fun expression_src;
        ]

    | E_new(annot, type_opt, bool, argExpression_list, aSTTypeId, 
        argExpressionListOpt_opt, array_size_opt, ctor_opt,
            statement_opt, heep_var_opt) -> 
        "E_new", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "bool", bool_fun bool;
        "args", list_node argExpression_fun argExpression_list;
        "aSTTypeId", node aSTTypeId_fun aSTTypeId;
        "arg_expr_list", opt_node argExpressionListOpt_fun argExpressionListOpt_opt;
        "arr_size", opt_node expression_fun array_size_opt;
        "ctor_var", opt_node variable_fun ctor_opt;
        "stmt", opt_node statement_fun statement_opt;
        "heap_var", opt_node variable_fun heep_var_opt;
        ]

    | E_delete(annot, type_opt, bool_colon, bool_array, 
           expression_opt, statement_opt) -> 
        "E_delete", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "is_colon", bool_fun bool_colon;
        "is_array", bool_fun bool_array;
        "expr", opt_node expression_fun expression_opt;
        "stmt", opt_node statement_fun statement_opt;
        ]

    | E_throw(annot, type_opt, expression_opt, var_opt, statement_opt) -> 
        "E_throw", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "expr", opt_node expression_fun expression_opt;
        "var", opt_node variable_fun var_opt;
        "stmt", opt_node  statement_fun statement_opt;
        ]

    | E_keywordCast(annot, type_opt, castKeyword, aSTTypeId, expression) -> 
        "E_keywordCast", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "kwd", castKeyword_fun castKeyword;
        "aSTTypeId", node aSTTypeId_fun aSTTypeId;
        "expr", node expression_fun expression;
        ]

    | E_typeidExpr(annot, type_opt, expression) -> 
        "E_typeidExpr", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "expr", node expression_fun expression;
        ]

    | E_typeidType(annot, type_opt, aSTTypeId) -> 
        "E_typeidType", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "aSTTypeId", node aSTTypeId_fun aSTTypeId;
        ]

    | E_grouping(annot, type_opt, expression) -> 
        "E_grouping", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "expr", node expression_fun expression;
        ]

    | E_arrow(annot, type_opt, expression, pQName) -> 
        "E_arrow", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "expr", node expression_fun expression;
        "pQName", node pQName_fun pQName;
        ]

    | E_statement(annot, type_opt, s_compound) -> 
        assert(match s_compound with | S_compound _ -> true | _ -> false);
        "E_statement", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "stmt", node statement_fun s_compound;
        ]

    | E_compoundLit(annot, type_opt, aSTTypeId, in_compound) -> 
        assert(match in_compound with | IN_compound _ -> true | _ -> false);
        "E_compoundLit", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "aSTTypeId", node aSTTypeId_fun aSTTypeId;
        "in_compound", node init_fun in_compound;
        ]

    | E___builtin_constant_p(annot, type_opt, sourceLoc, expression) -> 
        "E___builtin_constant_p", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "loc", sourceLoc_fun sourceLoc;
        "expr", node expression_fun expression;
        ]

    | E___builtin_va_arg(annot, type_opt, sourceLoc, expression, aSTTypeId) -> 
        "E___builtin_va_arg", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "loc", sourceLoc_fun sourceLoc;
        "expr", node expression_fun expression;
        "aSTTypeId", node aSTTypeId_fun aSTTypeId;
        ]

    | E_alignofType(annot, type_opt, aSTTypeId, int) -> 
        "E_alignofType", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "aSTTypeId", node aSTTypeId_fun aSTTypeId;
        "int", int_fun int;
        ]

    | E_alignofExpr(annot, type_opt, expression, int) -> 
        "E_alignofExpr", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "expr", node expression_fun expression;
        "int", int_fun int;
        ]

    | E_gnuCond(annot, type_opt, expression_cond, expression_false) -> 
        "E_gnuCond", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "e_cond", node expression_fun expression_cond;
        "e_false", node expression_fun expression_false
        ]

    | E_addrOfLabel(annot, type_opt, stringRef) -> 
        "E_addrOfLabel", [
        "annot", annotation_fun annot;
        "ctype", opt_node cType_fun type_opt;
        "stringRef", string_fun stringRef;
        ]

let fullExpression_split (annot, expression_opt, fullExpressionAnnot) =
  [
    "annot", annotation_fun annot;
    "expr", opt_node expression_fun expression_opt;
    "full_expr_annot", node fullExpressionAnnot_fun fullExpressionAnnot;
  ]
  
let argExpression_split (annot, expression) =
  [
    "annot", annotation_fun annot;
    "expr", node expression_fun expression
  ]


let argExpressionListOpt_split (annot, argExpression_list) =
  [
    "annot", annotation_fun annot;
    "args", list_node argExpression_fun argExpression_list
  ]

  
let init_split = function
    | IN_expr(annot, sourceLoc, fullExpressionAnnot, expression_opt) -> 
        "IN_expr", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "full_expr_annot", node fullExpressionAnnot_fun fullExpressionAnnot;
        "expr", opt_node expression_fun expression_opt;
        ]

    | IN_compound(annot, sourceLoc, fullExpressionAnnot, init_list) -> 
        "IN_compound", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "full_expr_annot", node fullExpressionAnnot_fun fullExpressionAnnot;
        "inits", list_node init_fun init_list;
        ]

    | IN_ctor(annot, sourceLoc, fullExpressionAnnot, 
         argExpression_list, var_opt, bool) -> 
        "IN_ctor", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "full_expr_annot", node fullExpressionAnnot_fun fullExpressionAnnot;
        "arg_exprs", list_node argExpression_fun argExpression_list;
        "var", opt_node variable_fun var_opt;
        "bool", bool_fun bool;
        ]

    | IN_designated(annot, sourceLoc, fullExpressionAnnot, 
               designator_list, init) -> 
        "IN_designated", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "full_expr_annot", node fullExpressionAnnot_fun fullExpressionAnnot;
        "designators", list_node designator_fun designator_list;
        "init", node init_fun init;
        ]


let templateDeclaration_split = function
    | TD_func(annot, templateParameter_opt, func) -> 
        "TD_func", [
        "annot", annotation_fun annot;
        "param", opt_node templateParameter_fun templateParameter_opt;
        "func", node func_fun func;
        ]

    | TD_decl(annot, templateParameter_opt, declaration) -> 
        "TD_decl", [
        "annot", annotation_fun annot;
        "param", opt_node templateParameter_fun templateParameter_opt;
        "decl", node declaration_fun declaration;
        ]

    | TD_tmember(annot, templateParameter_opt, templateDeclaration) -> 
        "TD_tmember", [
        "annot", annotation_fun annot;
        "param", opt_node templateParameter_fun templateParameter_opt;
        "decl", node templateDeclaration_fun templateDeclaration;
        ]

        
let templateParameter_split = function
    | TP_type(annot, sourceLoc, variable, stringRef, 
          aSTTypeId_opt, templateParameter_opt) -> 
        "TP_type", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "var", node variable_fun variable;
        "stringRef", string_fun stringRef;
        "aSTTypeId", opt_node aSTTypeId_fun aSTTypeId_opt;
        "templateParameter", opt_node templateParameter_fun templateParameter_opt;
        ]

    | TP_nontype(annot, sourceLoc, variable,
            aSTTypeId, templateParameter_opt) -> 
        "TP_nontype", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "var", node variable_fun variable;
        "aSTTypeId", node aSTTypeId_fun aSTTypeId;
        "templateParameter", opt_node templateParameter_fun templateParameter_opt;
        ]

    | TP_template(annot, sourceLoc, variable, params, stringRef,
          pqname, templateParameter_opt) -> 
        "TP_template", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "var", node variable_fun variable;
        "params", list_node templateParameter_fun params;
        "stringRef", string_fun stringRef;
        "pqname", opt_node pQName_fun pqname;
        "templateParameter", opt_node templateParameter_fun templateParameter_opt;
        ]

let templateArgument_split = function
    | TA_type(annot, aSTTypeId, templateArgument_opt) -> 
        "TA_type", [
        "annot", annotation_fun annot;
        "aSTTypeId", node aSTTypeId_fun aSTTypeId;
        "templateArgument", opt_node templateArgument_fun templateArgument_opt;
        ]

    | TA_nontype(annot, expression, templateArgument_opt) -> 
        "TA_nontype", [
        "annot", annotation_fun annot;
        "expr", node expression_fun expression;
        "templateArgument", opt_node templateArgument_fun templateArgument_opt;
        ]

    | TA_templateUsed(annot, templateArgument_opt) -> 
        "TA_templateUsed", [
        "annot", annotation_fun annot;
        "templateArgument", opt_node templateArgument_fun templateArgument_opt;
        ]


let namespaceDecl_split = function
    | ND_alias(annot, stringRef, pQName) -> 
        "ND_alias", [
        "annot", annotation_fun annot;
        "stringRef", string_fun stringRef;
        "pQName", node pQName_fun pQName;
        ]

    | ND_usingDecl(annot, pQName) -> 
        "ND_usingDecl", [
        "annot", annotation_fun annot;
        "pQName", node pQName_fun pQName;
        ]

    | ND_usingDir(annot, pQName) -> 
        "ND_usingDir", [
        "annot", annotation_fun annot;
        "pQName", node pQName_fun pQName;
        ]


let fullExpressionAnnot_split (annot, declaration_list) =
  [
    "annot", annotation_fun annot;
    "decls", list_node declaration_fun declaration_list;
  ]
  
  
let aSTTypeof_split = function
    | TS_typeof_expr(annot, ctype, fullExpression) -> 
        "TS_typeof_expr", [
        "annot", annotation_fun annot;
        "ctype", node cType_fun ctype;
        "full_expr", node fullExpression_fun fullExpression;
        ]

    | TS_typeof_type(annot, ctype, aSTTypeId) -> 
        "TS_typeof_type", [
        "annot", annotation_fun annot;
        "ctype", node cType_fun ctype;
        "aSTTypeId", node aSTTypeId_fun aSTTypeId;
        ]


let designator_split = function
    | FieldDesignator(annot, sourceLoc, stringRef) -> 
        "FieldDesignator", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "stringRef", string_fun stringRef;
        ]

    | SubscriptDesignator(annot, sourceLoc, expression, expression_opt, 
                 idx_start, idx_end) -> 
        "SubscriptDesignator", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "expr", node expression_fun expression;
        "expr2", opt_node expression_fun expression_opt;
        "idx_start", int_fun idx_start;
        "idx_end", int_fun idx_end;
        ]

let attribute_split = function
    | AT_empty(annot, sourceLoc) -> 
        "AT_empty", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        ]

    | AT_word(annot, sourceLoc, stringRef) -> 
        "AT_word", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "stringRef", string_fun stringRef;
        ]

    | AT_func(annot, sourceLoc, stringRef, argExpression_list) -> 
        "AT_func", [
        "annot", annotation_fun annot;
        "loc", sourceLoc_fun sourceLoc;
        "stringRef", string_fun stringRef;
        "exprs", list_node argExpression_fun argExpression_list;
        ]
    
let wrap s f x =
    s, f x
    
let wrap2 s f x =
    let n, v = f x in
    s^":"^n, v

let get_node_meta = function
  | TranslationUnit_type x ->
    wrap "TranslationUnit" translationUnit_split x
  | TopForm_type x ->
    wrap2 "TopForm" topForm_split x
  | Function_type x ->
    wrap "Function" func_split x
  | MemberInit_type x ->
    wrap "MemberInit" memberInit_split x
  | Declaration_type x ->
    wrap "Declaration" declaration_split x
  | ASTTypeId_type x ->
    wrap "ASTTypeId" aSTTypeId_split x
  | PQName_type x ->
    wrap2 "PQName" pQName_split x
  | TypeSpecifier_type x ->
    wrap2 "TypeSpecifier" typeSpecifier_split x
  | BaseClassSpec_type x ->
    wrap "BaseClassSpec" baseClassSpec_split x
  | Enumerator_type x ->
    wrap "Enumerator" enumerator_split x
  | MemberList_type x ->
    wrap "MemberList" memberList_split x
  | Member_type x ->
    wrap2 "Member" member_split x
  | ExceptionSpec_type x ->
    wrap "ExceptionSpec" exceptionSpec_split x
  | OperatorName_type x ->
    wrap2 "OperatorName" operatorName_split x
  | Statement_type x ->
    wrap2 "Statement" statement_split x
  | Condition_type x ->
    wrap2 "Condition" condition_split x
  | Handler_type x ->
    wrap "Handler" handler_split x
  | Expression_type x ->
    wrap2 "Expression" expression_split x
  | FullExpression_type x ->
    wrap "FullExpression" fullExpression_split x
  | ArgExpression_type x ->
    wrap "ArgExpression" argExpression_split x
  | ArgExpressionListOpt_type x ->
    wrap "ArgExpressionListOpt" argExpressionListOpt_split x
  | Initializer_type x ->
    wrap2 "Initializer" init_split x
  | TemplateDeclaration_type x ->
    wrap2 "TemplateDeclaration" templateDeclaration_split x
  | TemplateParameter_type x ->
    wrap2 "TemplateParameter" templateParameter_split x
  | TemplateArgument_type x ->
    wrap2 "TemplateArgument" templateArgument_split x
  | NamespaceDecl_type x ->
    wrap2 "NamespaceDecl" namespaceDecl_split x
  | Declarator_type x ->
    wrap "Declarator" declarator_split x
  | IDeclarator_type x ->
    wrap2 "IDeclarator" iDeclarator_split x
  | FullExpressionAnnot_type x ->
    wrap "fullExpressionAnnot" fullExpressionAnnot_split x
  | ASTTypeof_type x ->
    wrap2 "ASTTypeof" aSTTypeof_split x
  | Designator_type x ->
    wrap2 "Designator" designator_split x
  | Attribute_type x ->
    wrap2 "Attribute" attribute_split x
  | Variable x ->
    wrap "Variable" variable_split x
  | TemplateInfo x ->
    wrap "TemplateInfo" templ_info_split x
  | InheritedTemplateParams x ->
    wrap "InheritedTemplateParams" inherited_templ_params_split x
  | BaseClass x ->
    wrap "BaseClass" baseClass_split x
  | Compound_info x ->
    wrap "CompoundInfo" compound_info_split x
  | EnumType_Value_type x ->
    wrap "EnumType_Value" enum_value_split x
  | AtomicType x ->
    wrap2 "AtomicType" atomicType_split x
  | CType x ->
    wrap2 "CType" cType_split x
  | STemplateArgument x ->
    wrap2 "STemplateArgument" sTemplateArgument_split x
  | Scope x ->
    wrap "Scope" scope_split x
  | NoAstNode -> "<empty>", []

let node_loc = function
  | TopForm_type tf -> Some(topForm_loc tf)
  | PQName_type pq -> Some(pQName_loc pq)
  | TypeSpecifier_type x -> Some(typeSpecifier_loc x)
  | Enumerator_type x -> Some(enumerator_loc x)
  | Member_type x -> Some(member_loc x)
  | Statement_type x -> Some(statement_loc x)
  | Expression_type(E___builtin_constant_p(_,_,loc,_)) -> Some loc
  | Expression_type(E___builtin_va_arg(_,_,loc,_,_)) -> Some loc
  | Initializer_type x -> Some(init_loc x)
  | TemplateParameter_type x -> Some(templateParameter_loc x)
  | IDeclarator_type x -> Some(iDeclarator_loc x)
  | Designator_type x -> Some(designator_loc x)
  | Attribute_type x -> Some(attribute_loc x)
  | Variable v -> Some(v.loc)

  | TranslationUnit_type _
  | Function_type _
  | MemberInit_type _
  | Declaration_type _
  | ASTTypeId_type _
  | BaseClassSpec_type _
  | MemberList_type _
  | ExceptionSpec_type _
  | OperatorName_type _
  | Condition_type _
  | Handler_type _
  | Expression_type _
  | FullExpression_type _
  | ArgExpression_type _
  | ArgExpressionListOpt_type _
  | TemplateDeclaration_type _
  | TemplateArgument_type _
  | NamespaceDecl_type _
  | Declarator_type _
  | FullExpressionAnnot_type _
  | ASTTypeof_type _
  | TemplateInfo _
  | InheritedTemplateParams _
  | BaseClass _
  | Compound_info _
  | EnumType_Value_type _
  | AtomicType _
  | CType _
  | STemplateArgument _
  | Scope _
    -> None

  | NoAstNode -> assert(false)

let node_annotation node = 
  match node with
    | TranslationUnit_type x -> translationUnit_annotation x
    | TopForm_type x -> topForm_annotation x
    | Function_type x -> func_annotation x
    | MemberInit_type x -> memberInit_annotation x
    | Declaration_type x -> declaration_annotation x
    | ASTTypeId_type x -> aSTTypeId_annotation x
    | PQName_type x -> pQName_annotation x
    | TypeSpecifier_type x -> typeSpecifier_annotation x
    | BaseClassSpec_type x -> baseClassSpec_annotation x
    | Enumerator_type x -> enumerator_annotation x
    | MemberList_type x -> memberList_annotation x
    | Member_type x -> member_annotation x
    | ExceptionSpec_type x -> exceptionSpec_annotation x
    | OperatorName_type x -> operatorName_annotation x
    | Statement_type x -> statement_annotation x
    | Condition_type x -> condition_annotation x
    | Handler_type x -> handler_annotation x
    | Expression_type x -> expression_annotation x
    | FullExpression_type x -> fullExpression_annotation x
    | ArgExpression_type x -> argExpression_annotation x
    | ArgExpressionListOpt_type x -> argExpressionListOpt_annotation x
    | Initializer_type x -> init_annotation x
    | TemplateDeclaration_type x -> templateDeclaration_annotation x
    | TemplateParameter_type x -> templateParameter_annotation x
    | TemplateArgument_type x -> templateArgument_annotation x
    | NamespaceDecl_type x -> namespaceDecl_annotation x
    | Declarator_type x -> declarator_annotation x
    | IDeclarator_type x -> iDeclarator_annotation x
    | FullExpressionAnnot_type x -> fullExpressionAnnot_annotation x
    | ASTTypeof_type x -> aSTTypeof_annotation x
    | Designator_type x -> designator_annotation x
    | Attribute_type x -> attribute_annotation x
    | Variable x -> variable_annotation x
    | TemplateInfo x -> templ_info_annotation x
    | InheritedTemplateParams x -> inherited_templ_params_annotation x
    | BaseClass x -> baseClass_annotation x
    | Compound_info x -> compound_info_annotation x
    | EnumType_Value_type x -> enum_value_annotation x
    | AtomicType x -> atomicType_annotation x
    | CType x -> cType_annotation x
    | STemplateArgument x -> sTemplateArgument_annotation x
    | Scope x -> scope_annotation x
    | NoAstNode -> assert false  
end

let node_meta = MetaBuilder.get_node_meta
let node_loc = MetaBuilder.node_loc
let node_annotation = MetaBuilder.node_annotation

module type INDEXED_AST = 
sig
    type annot_t
    val id_annot : annot_t -> int
end

module type NODE_ARRAY =
sig 
    type annot_t
    type t
    
    val node_id : annot_t super_ast -> int
    
    val make : int -> annot_t translationUnit_type -> t
    val get : t -> int -> annot_t super_ast
    val get_root : t -> annot_t super_ast
    val get_unit : t -> annot_t translationUnit_type
    
    val iter : (annot_t super_ast -> unit) -> t -> unit
    val iteri : (int -> annot_t super_ast -> unit) -> t -> unit
    val fold : (annot_t super_ast -> 'a -> 'a) -> t -> 'a -> 'a
end
  
module NodeArrayMake (Index : INDEXED_AST) : (NODE_ARRAY with type annot_t = Index.annot_t) =
struct
    type annot_t = Index.annot_t
    type t = int * annot_t translationUnit_type * annot_t super_ast array array

    let node_id n = Index.id_annot (node_annotation n)

    let block_shift = 19
    let key_mask = (1 lsl block_shift) - 1

    let get (_, _, table) id =
      let block = id lsr block_shift
      and key = id land key_mask
      in
      let arr = Array.get table block in
      let elt = Array.get arr key in
      elt

    let get_root ((root_id, _, _) as x) =
      get x root_id

    let get_unit (_, root, _) = 
      root

    let mem (_, _, table) id =
      let block = id lsr block_shift
      and key = id land key_mask
      in
      assert (block < (Array.length table));
      let arr = Array.get table block in
      assert (key < (Array.length arr));
      let elt = Array.get arr key in
      elt != NoAstNode

    let iter f (_, _, table) =
      for i = 0 to (Array.length table - 1) do
        let sub = Array.get table i in
        for j = 0 to (Array.length sub - 1) do
          f (Array.get sub j)
        done
      done

    let iteri f (_, _, table) =
      for i = 0 to (Array.length table - 1) do
        let sub = Array.get table i 
        and base = i lsl block_shift
        in
        for j = 0 to (Array.length sub - 1) do
          f (base + j) (Array.get sub j)
        done
      done

    let fold f (_, _, table) v =
      let tmp = ref v in
      for i = 0 to (Array.length table - 1) do
        let sub = Array.get table i in
        for j = 0 to (Array.length sub - 1) do
          tmp := f (Array.get sub j) !tmp
        done
      done;
      !tmp

    let make_tbl sz =
      let block = (sz-1) lsr block_shift
      and key = (sz-1) land key_mask
      in
      let tbl = Array.make (block+1) [||] in
      for i = 0 to block-1 do
        tbl.(i) <- Array.make (key_mask+1) NoAstNode
      done;
      tbl.(block) <- Array.make (key+1) NoAstNode;
      tbl

    let set (_, _, table) id elt =
      let block = id lsr block_shift
      and key = id land key_mask
      in
      assert (block < (Array.length table));
      let arr = Array.get table block in
      assert (key < (Array.length arr));
      arr.(key) <- elt

    let rec fill_table' data = function
        | AtomicType (CompoundType ci) ->
            fill_table' data (Compound_info ci)
        | ast ->
            let _name, meta = node_meta ast in
            List.iter (fun (_nm, f) -> match f with 
                | NodeField n ->
                    fill_table data n
                | OptNodeField (Some n) ->
                    fill_table data n
                | NodeListField nl ->
                    List.iter (fill_table data) nl
                | NodeListListField nl ->
                    List.iter (List.iter (fill_table data)) nl
                | OptNodeListField (Some nl) ->
                    List.iter (fill_table data) nl
                | StringNodeTableField nl ->
                    List.iter (fun (_s,n) -> fill_table data n) nl
                | _ -> ()
            ) meta

    and fill_table data ast =
        let id = node_id ast in
        if mem data id then ()
        else (
          set data id ast;
          fill_table' data ast
        )

    let make size root =
      let root_ast = TranslationUnit_type root in
      let root_id = node_id root_ast in
      if size <= 1 then
        (0, root, [|[|root_ast|]|])
      else
      let data = (root_id, root, make_tbl (size+1)) in
      set data root_id root_ast;
      fill_table' data root_ast;
      data
end

module NodeArray = NodeArrayMake(struct type annot_t = annotated let id_annot = id_annotation end)

let load_node_array file =
  let (max_node, ast) = Oast_header.unmarshal_oast file
  in
    NodeArray.make max_node ast

module Superast =
struct
  let into_array = NodeArray.make
  let load_marshaled_ast_array = load_node_array
  let iter = NodeArray.iter
  let iteri = NodeArray.iteri
  let fold = NodeArray.fold
  let node_id = NodeArray.node_id
end
