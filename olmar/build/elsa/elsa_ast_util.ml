(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* general utilities for the elsa ast, not much, but it has to go somewhere *)

open Elsa_reflect_type

let is_named_atomic_type = function
  | ATY_Simple _ -> false
  | ATY_Compound _
  | ATY_Enum _
  | ATY_PseudoInstantiation _
  | ATY_TypeVariable _
  | ATY_DependentQ _ -> true


let is_named_atomic_type_option = function
  | None -> true
  | Some x -> is_named_atomic_type x


exception Function_name_absent

let name_of_function fu =
  match fu.nameAndParams.declarator_var with
    | None -> raise Function_name_absent
    | Some variable -> match variable.variable_name with
	| None -> raise Function_name_absent
	| Some name -> name

let safe_name_of_function fu =
  try
    name_of_function fu
  with
    | Function_name_absent -> "? function name not present ?"


let body_of_function fu =
  match fu.function_body with
    | Some compound -> compound
    | None -> assert false
