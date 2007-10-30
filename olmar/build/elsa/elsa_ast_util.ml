(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* general utilities for the elsa ast, not much, but it has to go somewhere *)

open Elsa_reflect_type

let is_named_atomic_type = function
  | SimpleType _ -> false
  | CompoundType _
  | EnumType _
  | PseudoInstantiation _
  | TypeVariable _
  | DependentQType _ -> true


let is_named_atomic_type_option = function
  | None -> true
  | Some x -> is_named_atomic_type x
