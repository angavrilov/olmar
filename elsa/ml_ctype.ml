
(***************************************************************************
 *
 * non-recursive part of the CType and Variable type structure
 *
 * 
 * The cType and variable types are mutually recursive with the
 * other astgen generated types. Therefore, the relevant type definitions
 * are in a ocaml_type_verbatim in the file ml_ctype.ast, which is 
 * processed by astgen.
 * 
 * This file contains those few types that do not take part in the 
 * recursion.
 *
 **************************************************************************)


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



