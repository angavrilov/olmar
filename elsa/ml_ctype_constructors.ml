
(* hand written constructor callbacks for ml_ctype *)

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


(* type variable *)

let create_variable_constructor loc name var_type flags value defparam funcdef =
  { loc = loc;
    var_name = name;
    var_type = var_type;
    flags = flags;
    value = value;
    defaultParam = defparam;
    funcDefn = funcdef;
  }


(* type baseClass *)
let create_baseClass_constructor compound access is_virtual =
  { compound = compound;
    bc_access = access;
    is_virtual = is_virtual;
  }


(* type compound_info *)

let create_compound_info_constructor 
    name typedef_var access is_forward_decl keyword data_members bases 
    conversion_operators friends inst_name self_type =
  { compound_name = name;
    typedef_var = typedef_var;
    ci_access = access;
    is_forward_decl = is_forward_decl;
    keyword = keyword;
    data_members = data_members;
    bases = bases;
    conversion_operators = conversion_operators;
    friends = friends;
    inst_name = inst_name;
    self_type = self_type
  }

(* type atomicType *)

let create_atomic_SimpleType_constructor sid = SimpleType sid
let create_atomic_CompoundType_constructor ci = CompoundType ci
let create_atomic_EnumType_constructor nameopt variable key enum_list =
  EnumType(nameopt, variable, key, enum_list)




(* type cType *)
let create_ctype_CVAtomicType_constructor cvs at = CVAtomicType(cvs, at)
let create_ctype_PointerType_constructor cvs ct = PointerType(cvs, ct)
let create_ctype_ReferenceType_constructor ct = ReferenceType ct
let create_ctype_FunctionType_constructor ff ret vars exs = 
  FunctionType(ff, ret, vars, exs)
let create_ctype_ArrayType_constructor ct size = ArrayType(ct, size)
let create_ctype_PointerToMemberType_constructor at cvs ct =
  PointerToMemberType(at, cvs, ct)


(* all this is hand written, so put all callback registration in here *)
let register_ml_ctype_constructor_callbacks () =
  (* from array_size *)
  Callback.register "create_array_size_NO_SIZE_constructor" 
    create_array_size_NO_SIZE_constructor;
  Callback.register "create_array_size_DYN_SIZE_constructor" 
    create_array_size_DYN_SIZE_constructor;
  Callback.register "create_array_size_FIXED_SIZE_constructor" 
    create_array_size_FIXED_SIZE_constructor;

  (* from variable *)
  Callback.register "create_variable_constructor" create_variable_constructor;

  (* from baseClass *)
  Callback.register "create_baseClass_constructor" create_baseClass_constructor;

  (* from compound_info *)
  Callback.register "create_compound_info_constructor" 
    create_compound_info_constructor;

  (* from atomicType *)
  Callback.register "create_atomic_SimpleType_constructor" 
    create_atomic_SimpleType_constructor;
  Callback.register "create_atomic_CompoundType_constructor" 
    create_atomic_CompoundType_constructor;
  Callback.register "create_atomic_EnumType_constructor" 
    create_atomic_EnumType_constructor;

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

  (* from function_flags *)
  Callback.register "function_flags_from_int23" function_flags_from_int32;

  (* from compoundType_Keyword *)
  Callback.register "create_K_STRUCT_constructor" create_K_STRUCT_constructor;
  Callback.register "create_K_CLASS_constructor" create_K_CLASS_constructor;
  Callback.register "create_K_UNION_constructor" create_K_UNION_constructor;




