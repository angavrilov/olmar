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



let name_of_function fu =
  match fu.nameAndParams.declarator_decl with
    | D_func (_annot, _sourceLoc, iDeclarator, _aSTTypeId_list, _cVFlags,
	      _exceptionSpec_opt, _pq_name_list, _bool) ->
	(match iDeclarator with
	   | D_name (_annot, _sourceLoc, pQName_opt) ->
	       (match pQName_opt with
		  | Some pq_name ->
		      (match pq_name with
			 | PQ_name (_annot, _sourceLoc, name) -> name
			 | _ -> assert false)
		  | None -> assert false)
	   | _ -> assert false)
    | _ -> assert false


let body_of_function fu =
  match fu.function_body with
    | Some compound -> compound
    | None -> assert false
