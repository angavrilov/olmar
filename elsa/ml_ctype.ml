
(* hand translated CType structure *)

open Cc_ml_types

type array_size = 
  | NO_SIZE				(* size unspecified *)
  | DYN_SIZE				(* some gnu extension *)
  | FIXED_SIZE of int			(* suppostly >= 0 *)


(* flags for FunctionType *)
type function_flag =
 (* FF_NONE             = 0x0000,  // nothing special *)
  | FF_METHOD        (* = 0x0001,  // function is a nonstatic method *)
  | FF_VARARGS       (* = 0x0002,  // accepts variable # of arguments *)
  | FF_CONVERSION    (* = 0x0004,  // conversion operator function *)
  | FF_CTOR          (* = 0x0008,  // constructor *)
  | FF_DTOR          (* = 0x0010,  // destructor *)
  | FF_BUILTINOP     (* = 0x0020,  // built-in operator function (cppstd 13.6) *)
  | FF_NO_PARAM_INFO (* = 0x0040,  // C parameter list "()" (C99 6.7.5.3 para 14) *)
  | FF_DEFAULT_ALLOC (* = 0x0080,  // is a default [de]alloc function from 3.7.3p2 *)
  | FF_KANDR_DEFN    (* = 0x0100,  // derived from a K&R-style function definition *)
 (* FF_ALL              = 0x01FF,  // all flags set to 1 *)

type function_flags = function_flag list


type compoundType_Keyword =
    (* this is aparently a subset of typeIntr *)
  | K_STRUCT
  | K_CLASS
  | K_UNION


type baseClass = {
  compound : compound_info;		(* the base class itself *)
  bc_access : accessKeyword;		(* public, protected ... *)
  is_virtual : bool;
}

and compound_info = {
  (* fields stored in the super class NamedAtomicType *)
  name : string;			(* user assigned name ?? *)
  typedef_var : variable;		(* implicit typdef variable ???? *)
  ci_access : accessKeyword;		(* accessibility in wider context *)

  (* fields of CompoundType itself:
   *     the stuff in comments is currently ommitted
   *)
  is_forward_decl : bool;
  keyword : compoundType_Keyword; 	(* keyword used for this compound *)
  data_members : variable list;		(* nonstatic data members *)
  bases : baseClass list;		(* base classes *)

  (* subobj : ?? root of the subobject hierarchy *)

  conversion_operators : variable list;
  friends : variable list;
  inst_name : string;			(* name for debugging purposes *)

  (* mutable syntax : typeSpecifier_type = TS_classSpec list;  * ast node *)
  (* scope : Scope *)

  self_type : cType;			(* type of the compound *)
}

and atomicType = 
  | SimpleType of simpleTypeId

      (* CompoundType( compound info) *)
  | CompoundType of compound_info

      (* EnumType( user given name, ?, public/protected, constants)
       *    ignore the next valye field 
       *)
  | EnumType of string option * variable * accessKeyword * 
      (string * int) list


and cType = 
  | CVAtomicType of cVFlags * atomicType
      (* PointerType( volatile, pointed type) *)
  | PointerType of cVFlags * cType
      (* ReferenceType( referenced type ) *)
  | ReferenceType of cType
      (* FunctionType(flags, return type, parameter list, exception spec)
       * where exceptions spec is either
       *   | None       no exception spec
       *   | Some list  list of specified exceptions (which can be empty)
       *)
  | FunctionType of function_flags * cType * 
      variable list * cType list option
      (* ArrayType( element type, size )*)
  | ArrayType of cType * array_size
      (* PointerToMemberType( ?, volatile, type of pointed member *)
  | PointerToMemberType of atomicType (* = NamedAtomicType *) * cVFlags * cType

