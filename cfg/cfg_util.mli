(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Cc_ml_types
open Cc_ast_gen_type
open Ast_annotation
open Superast
open Cfg_type

val fun_id_name : function_id_type -> string

val fun_def_name : function_def -> string

val error_location : sourceLoc -> string

val make_overload_hash : cfg_type -> overload_hash_type

val iter_callees : 
  (function_def option -> function_id_type -> function_def option -> unit) 
    -> cfg_type 
      -> function_id_type list
        -> unit

type function_application =
  | Func_def of function_def * annotated function_type
  | Short_oast of function_def
  | Bad_oast of function_def * annotated super_ast
  | Undefined

val apply_func_def : 
  (function_def option -> function_id_type -> function_application -> 'a) 
    -> function_def option 
      -> function_id_type
	-> function_def option -> 'a

val apply_func_def_gc :
  (unit -> unit)
    -> (function_def option -> function_id_type -> function_application -> 'a) 
      -> (unit -> unit)
	-> function_def option 
	  -> function_id_type
	    -> function_def option -> 'a
