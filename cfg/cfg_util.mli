(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Ast_annotation
open Elsa_ml_base_types
open Elsa_reflect_type
open Superast
open Cfg_type


(* return the complete name of the function (with all scopes prepended) *)
val fun_id_name : function_id_type -> string

(* return the complete name of the function (with all scopes prepended) *)
val fun_def_name : function_def -> string

(* construct a hash mapping function names (without scope) to
 * all function id's with that name 
 *)
val make_overload_hash : cfg_type -> overload_hash_type

(* iter_callees f cfg funs
 * iterates over the functions funs and all their callees. f is called 
 * precisely onces (even if the function is client of several other functions). 
 * f is called in the form  f context fun_id definition_opt
 * If context is of the form Some g then g is the first function found that 
 * calls function fun_id. If the definition of fun_id is in cfg, then it is 
 * provided in definition_opt.
 *)
val iter_callees : 
  (function_def option -> function_id_type -> function_def option -> unit) 
    -> cfg_type 
      -> function_id_type list
        -> unit

(* Possible error conditions for iterating over functions in different
 * compilation units
 *)
type function_application =
    (* Everything fine: proper function definition found at the 
     * expected place 
     *)
  | Func_def of function_def * annotated function_type
    (* The ast node array of the required compilation unit does not
     * contain the required index
     *)
  | Short_oast of function_def
    (* The required index does is not a function definition *)
  | Bad_oast of function_def * annotated super_ast
    (* The function is undefined in the current CFG *)
  | Undefined

(* apply_func_def is thought as a plugin for iter_callees. 
 * apply_func_def f provided_context fun_id fun_def_opt
 * If fun_def_opt is different from None (i.e. there is a 
 * function definition in the current CFG) then load the oast file,
 * and call f with the provided context on the function_type node in 
 * the abstract syntax tree.
 * The third argument for f varies according to the various possible errors.
 *)
val apply_func_def : 
  (function_def option -> function_id_type -> function_application -> 'a) 
    -> function_def option 
      -> function_id_type
	-> function_def option -> 'a


(* Same as apply_func_def, but call the first argument before loading 
 * the oast file and the third argument after f has returned and the oast
 * can be garbage collected.
 * At the moment only used to check for the absence of memory leaks.
 *)
val apply_func_def_gc :
  (unit -> unit)
    -> (function_def option -> function_id_type -> function_application -> 'a) 
      -> (unit -> unit)
	-> function_def option 
	  -> function_id_type
	    -> function_def option -> 'a
