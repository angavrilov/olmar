(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* hand written constructor callbacks for cType/Variable/D_attribute *)

open Ml_ctype
open Cc_ast_gen_type

(* type array_size *)

let create_array_size_NO_SIZE_constructor () = NO_SIZE
let create_array_size_DYN_SIZE_constructor () = DYN_SIZE
let create_array_size_FIXED_SIZE_constructor i = FIXED_SIZE i


(* type function_flags *)

let ff_flag_array = [|
  FF_METHOD;        (* = 0x0001 *)
  FF_VARARGS;       (* = 0x0002 *)
  FF_CONVERSION;    (* = 0x0004 *)
  FF_CTOR;          (* = 0x0008 *)
  FF_DTOR;          (* = 0x0010 *)
  FF_BUILTINOP;     (* = 0x0020 *)
  FF_NO_PARAM_INFO; (* = 0x0040 *)
  FF_DEFAULT_ALLOC; (* = 0x0080 *)
  FF_KANDR_DEFN;    (* = 0x0100 *)
|]

let ff_flags_number = 
  assert(Array.length ff_flag_array = 9);
  9

let ff_mask =
  Int32.lognot(
    Int32.of_int(int_of_float(
		   2.0 ** (float_of_int (ff_flags_number))) -1))

let function_flags_from_int32 flags =
  let rec doit i accu =
    if i = ff_flags_number then accu
    else 
      if Int32.logand (Int32.shift_left Int32.one i) flags 
	<> Int32.zero
      then
	doit (i+1) (ff_flag_array.(i) :: accu)
      else
	doit (i+1) accu
  in
    assert(Int32.logand ff_mask flags = Int32.zero);
    doit 0 []


(* type compoundType_Keyword (CompoundType::Keyword) *)

let create_K_STRUCT_constructor () = K_STRUCT
let create_K_CLASS_constructor () = K_CLASS
let create_K_UNION_constructor () = K_UNION


(* type templateThingKind *)

let create_TTK_PRIMARY_constructor () = TTK_PRIMARY
let create_TTK_SPECIALIZATION_constructor () = TTK_SPECIALIZATION
let create_TTK_INSTANTIATION_constructor () = TTK_INSTANTIATION



(* type variable *)

let create_variable_constructor 
    poly loc name var_type flags value
    defparam funcdef overload_ref virt_ride scope_opt templ_info decl_loc =
  { poly_var = poly;
    loc = loc;
    var_decl_loc = decl_loc;
    var_name = name;
    var_type = var_type;
    flags = flags;
    value = value;
    defaultParam = defparam;
    funcDefn = funcdef;
    overload = overload_ref;
    virtuallyOverride = virt_ride;
    scope = scope_opt;
    templ_info = templ_info
  }


(* type templateInfo *)
let create_templateInfo_constructor poly_templ templ_kind template_params
    template_var inherited_params instantiation_of instantiations 
    specialization_of specializations arguments inst_loc
    partial_instantiation_of partial_instantiations arguments_to_primary
    defn_scope definition_template_info instantiate_body
    instantiation_disallowed uninstantiated_default_args dependent_bases =
  { 
    poly_templ = poly_templ;
    templ_kind = templ_kind;
    template_params = template_params;
    template_var = template_var;
    inherited_params = inherited_params;
    instantiation_of = instantiation_of;
    instantiations = instantiations;
    specialization_of = specialization_of;
    specializations = specializations;
    arguments = arguments;
    inst_loc = inst_loc;
    partial_instantiation_of = partial_instantiation_of;
    partial_instantiations = partial_instantiations;
    arguments_to_primary = arguments_to_primary;
    defn_scope = defn_scope;
    definition_template_info = definition_template_info;
    instantiate_body = instantiate_body;
    instantiation_disallowed = instantiation_disallowed;
    uninstantiated_default_args = uninstantiated_default_args;
    dependent_bases = dependent_bases;
  }

(* type inheritedTemplateParams *)
let create_inheritedTemplateParams_constructor poly params encl =
  { 
    poly_inherited_templ = poly;
    inherited_template_params = params;
    enclosing = encl;
  }

(* type baseClass *)
let create_baseClass_constructor poly compound access is_virtual =
  { poly_base = poly;
    compound = compound;
    bc_access = access;
    is_virtual = is_virtual;
  }


(* type compound_info *)

let create_compound_info_constructor 
    poly name typedef_var access scope is_forward_decl is_transparent_union
    keyword data_members bases conversion_operators friends inst_name 
    syntax_opt self_type =
  { compound_info_poly = poly;
    compound_name = name;
    typedef_var = typedef_var;
    ci_access = access;
    compound_scope = scope;
    is_forward_decl = is_forward_decl;
    is_transparent_union = is_transparent_union;
    keyword = keyword;
    data_members = data_members;
    bases = bases;
    conversion_operators = conversion_operators;
    friends = friends;
    inst_name = inst_name;
    syntax = syntax_opt;
    self_type = self_type
  }

(* type enumType_Value *)
let create_EnumType_Value_constructor poly name value = 
  ((poly, name, value) : 'a enumType_Value_type)


(* type atomicType *)

let create_atomic_SimpleType_constructor poly sid = SimpleType(poly, sid)
  (* CompoundType has no poly arg, it's in the compound_info *)
let create_atomic_CompoundType_constructor ci = CompoundType ci
let create_atomic_PseudoInstantiation_constructor poly name var key 
    template_info args =
  PseudoInstantiation(poly, name, var, key, template_info, args)
let create_atomic_EnumType_constructor 
    poly nameopt variable key enum_list has_negatives =
  EnumType(poly, nameopt, variable, key, enum_list, has_negatives)
let create_atomic_TypeVariable_constructor poly name variable key =
  TypeVariable(poly, name, variable, key)
let create_atomic_DependentQType_constructor 
    poly name variable key atomic pqname = 
  DependentQType(poly, name, variable, key, atomic, pqname)
let create_atomic_TemplateTypeVariable_constructor poly name variable key params =
  TemplateTypeVariable(poly, name, variable, key, params)




(* type cType *)
let create_ctype_CVAtomicType_constructor poly cvs at = 
  CVAtomicType(poly, cvs, at)
let create_ctype_PointerType_constructor poly cvs ct = 
  PointerType(poly, cvs, ct)
let create_ctype_ReferenceType_constructor poly ct = ReferenceType(poly, ct)
let create_ctype_FunctionType_constructor poly ff ret vars exs = 
  FunctionType(poly, ff, ret, vars, exs)
let create_ctype_ArrayType_constructor poly ct size = ArrayType(poly, ct, size)
let create_ctype_PointerToMemberType_constructor poly at cvs ct =
  PointerToMemberType(poly, at, cvs, ct)
let create_ctype_DependentSizedArrayType_constructor poly ct size =
  DependentSizedArrayType(poly, ct, size)

(* type sTemplateArgument *)
let create_STA_NONE_constructor poly = STA_NONE poly
let create_STA_TYPE_constructor poly ctype = STA_TYPE(poly, ctype)
let create_STA_INT_constructor poly i = STA_INT(poly, i)
let create_STA_ENUMERATOR_constructor poly variable = 
  STA_ENUMERATOR(poly, variable)
let create_STA_REFERENCE_constructor poly variable = 
  STA_REFERENCE(poly, variable)
let create_STA_POINTER_constructor poly variable = STA_POINTER(poly, variable)
let create_STA_MEMBER_constructor poly variable = STA_MEMBER(poly, variable)
let create_STA_DEPEXPR_constructor poly expr = STA_DEPEXPR(poly, expr)
let create_STA_TEMPLATE_constructor poly atomic_type = 
  STA_TEMPLATE(poly, atomic_type)
let create_STA_ATOMIC_constructor poly atomic_type = 
  STA_ATOMIC(poly, atomic_type)


(* type scope *)
let create_scope_constructor 
    poly variable_hash type_tags_hash parent_scope_opt scope_kind
    namespace_var_opt template_params parameterized_entity_opt =
  {
    poly_scope = poly;
    variables = variable_hash;
    type_tags = type_tags_hash;
    parent_scope = parent_scope_opt;
    scope_kind = scope_kind;
    namespace_var = namespace_var_opt;
    scope_template_params = template_params;
    parameterized_entity = parameterized_entity_opt;
  }


(******************************************************************************
 *
 * D_attribute hack
 *
*******************************************************************************)


let create_D_attribute_constructor poly loc idecl a_list_list =
  D_attribute(poly, loc, idecl, a_list_list)



(* all this is hand written, so put all callback registration in here *)
let register_ml_ctype_constructor_callbacks () =
  (* from array_size *)
  Callback.register "create_array_size_NO_SIZE_constructor" 
    create_array_size_NO_SIZE_constructor;
  Callback.register "create_array_size_DYN_SIZE_constructor" 
    create_array_size_DYN_SIZE_constructor;
  Callback.register "create_array_size_FIXED_SIZE_constructor" 
    create_array_size_FIXED_SIZE_constructor;

  (* from templateThingKind *)
  Callback.register "create_TTK_PRIMARY_constructor" 
    create_TTK_PRIMARY_constructor;
  Callback.register "create_TTK_SPECIALIZATION_constructor" 
    create_TTK_SPECIALIZATION_constructor;
  Callback.register "create_TTK_INSTANTIATION_constructor" 
    create_TTK_INSTANTIATION_constructor ;

  (* from variable *)
  Callback.register "create_variable_constructor" create_variable_constructor;

  (* from templateInfo *)
  Callback.register "create_templateInfo_constructor" 
    create_templateInfo_constructor;

  (* from inheritedTemplateParams *)
  Callback.register "create_inheritedTemplateParams_constructor" 
    create_inheritedTemplateParams_constructor;

  (* from baseClass *)
  Callback.register "create_baseClass_constructor" create_baseClass_constructor;

  (* from compound_info *)
  Callback.register "create_compound_info_constructor" 
    create_compound_info_constructor;

  (* from enumType_Value *)
  Callback.register "create_EnumType_Value_constructor" 
    create_EnumType_Value_constructor;

  (* from atomicType *)
  Callback.register "create_atomic_SimpleType_constructor" 
    create_atomic_SimpleType_constructor;
  Callback.register "create_atomic_CompoundType_constructor" 
    create_atomic_CompoundType_constructor;
  Callback.register "create_atomic_PseudoInstantiation_constructor"
    create_atomic_PseudoInstantiation_constructor;
  Callback.register "create_atomic_EnumType_constructor" 
    create_atomic_EnumType_constructor;
  Callback.register "create_atomic_TypeVariable_constructor"
    create_atomic_TypeVariable_constructor;
  Callback.register "create_atomic_DependentQType_constructor" 
    create_atomic_DependentQType_constructor;
  Callback.register "create_atomic_TemplateTypeVariable_constructor"
    create_atomic_TemplateTypeVariable_constructor;

  (* from cType *)
  Callback.register "create_ctype_CVAtomicType_constructor" 
    create_ctype_CVAtomicType_constructor;
  Callback.register "create_ctype_PointerType_constructor" 
    create_ctype_PointerType_constructor;
  Callback.register "create_ctype_ReferenceType_constructor" 
    create_ctype_ReferenceType_constructor;
  Callback.register "create_ctype_FunctionType_constructor" 
    create_ctype_FunctionType_constructor;
  Callback.register "create_ctype_ArrayType_constructor" 
    create_ctype_ArrayType_constructor;
  Callback.register "create_ctype_PointerToMemberType_constructor" 
    create_ctype_PointerToMemberType_constructor;
  Callback.register "create_ctype_DependentSizedArrayType_constructor"
    create_ctype_DependentSizedArrayType_constructor;

  (* from function_flags *)
  Callback.register "function_flags_from_int32" function_flags_from_int32;

  (* from compoundType_Keyword *)
  Callback.register "create_K_STRUCT_constructor" create_K_STRUCT_constructor;
  Callback.register "create_K_CLASS_constructor" create_K_CLASS_constructor;
  Callback.register "create_K_UNION_constructor" create_K_UNION_constructor;

  (* from sTemplateArgument *)
  Callback.register "create_STA_NONE_constructor" create_STA_NONE_constructor;
  Callback.register "create_STA_TYPE_constructor" create_STA_TYPE_constructor;
  Callback.register "create_STA_INT_constructor" create_STA_INT_constructor;
  Callback.register "create_STA_ENUMERATOR_constructor"
    create_STA_ENUMERATOR_constructor;
  Callback.register "create_STA_REFERENCE_constructor"
    create_STA_REFERENCE_constructor;
  Callback.register "create_STA_POINTER_constructor"
    create_STA_POINTER_constructor;
  Callback.register "create_STA_MEMBER_constructor"
    create_STA_MEMBER_constructor;
  Callback.register "create_STA_DEPEXPR_constructor"
    create_STA_DEPEXPR_constructor;
  Callback.register "create_STA_TEMPLATE_constructor"
    create_STA_TEMPLATE_constructor;
  Callback.register "create_STA_ATOMIC_constructor"
    create_STA_ATOMIC_constructor;

  (* scope *)
  Callback.register "create_scope_constructor" create_scope_constructor;
  (* scope serialization builds hashtables, therefore register some 
   * hashtable functions here
   *)
  Callback.register "scope_hashtbl_create" 
    (Hashtbl.create : int -> (string, 'a variable) Hashtbl.t);
  Callback.register "scope_hashtbl_add"
    (Hashtbl.add : (string, 'a variable) Hashtbl.t -> 
      string -> 'a variable -> unit);

  (* D_attribute hack *)
  Callback.register "create_D_attribute_constructor" 
    create_D_attribute_constructor;
