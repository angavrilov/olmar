(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Ast_annotation
open Elsa_ml_base_types
open Elsa_ml_flag_types
open Elsa_reflect_type
open Elsa_oast_header
open Ast_accessors
open Superast


(************************************************************************
 *
 * global state
 *
 ************************************************************************)

let normal_scope = ref false

let normal_top_level_scope = ref false

let nodes_counter = ref 0

let edge_counter = ref 0

let max_string_length = ref 30


(************************************************************************
 *
 * dot_graph and utility functions
 *
 ************************************************************************)

module Node_id = struct
  type t = int

  let string_of_t = string_of_int
end

module G = Dot_graph.Make(Node_id)

let make_node id labels attributes childs =
  let res = G.make_node id labels attributes childs 
  in
    incr nodes_counter;
    edge_counter := !edge_counter + List.length childs;
    res


let rec count_label_rev_rec res count = function
  | [] -> res
  | (x, l) :: xls ->
      count_label_rev_rec 
	((x, Printf.sprintf "%s[%d]" l count) :: res)
	(count + 1)
	xls

let count_label_rev l = count_label_rev_rec [] 0 l

let list_of_option f = function
  | None -> []
  | Some o -> [f o]

let string_of_option f = function
  | None -> "<None>"
  | Some o -> f o

let string_of_int32 = Int32.to_string
let string_of_nativeint = Nativeint.to_string
let string_of_unsigned_long = string_of_int32
let string_of_unsigned_int = string_of_int32
let string_of_double = string_of_float
let string_of_boolValue = string_of_bool

let rec filter_childs node_array res = function
  | [] -> res
  | [] :: annot_list_list -> filter_childs node_array res annot_list_list
  | ((annot, label) :: annot_list) :: annot_list_list -> 
      let id = id_annotation annot
      in
	filter_childs node_array
	  (if node_array.(id)
	   then (id, Some label) :: res
	   else res)
	(annot_list :: annot_list_list)

let string_of_list l =
  Printf.sprintf "[%s]" (String.concat "; " l)


let truncate_string s =
  if String.length s > !max_string_length 
  then 
    let r = String.sub s 0 !max_string_length
    in
      r.[!max_string_length -1] <- '.';
      r.[!max_string_length -2] <- '.';
      r.[!max_string_length -3] <- '.';
      r
  else s


let loc_label (file, line, char) =
  Printf.sprintf "loc: %s:%d:%d" file line char


(************************************************************************
 *
 * node specific labelling functions
 *
 ************************************************************************)

let fun_class_constructor_label (f : 'a function_type) = 
  let same_class_compound compound =
    match f.nameAndParams.declarator_var with
      | None -> false
      | Some var -> match var.scope with
	  | None -> false
	  | Some scope -> match scope.scope_compound with
	      | None -> false
	      | Some class_compound -> class_compound == compound
  in
    match f.funcType with
      | FunctionType(_, flags, _rettype, args, _excn) ->
	  (match (List.mem FF_CTOR flags, List.mem FF_DTOR flags) with
	     | true, true -> ["constructor AND destructor"]
	     | true, _ -> 
		 (match args with
		    | [] -> ["default constructor"]
		    | [arg] -> 
			(match !(arg.variable_type) with
			   | None -> ["missing type in only constructor arg"]
			   | Some typ ->
			       match typ with
				 | ReferenceType
				     (_, CVAtomicType
					(_, CompoundType(compound) ,
					 [CV_CONST])) 
				     when same_class_compound compound ->
				     ["copy constructor"]
				     
				 | ReferenceType _
				 | CVAtomicType _
				 | PointerType _
				 | FunctionType _
				 | ArrayType _
				 | DependentSizedArrayType _
				 | PointerToMemberType _
				   -> ["other constructor"]
			)
		    | _ -> ["other constructor"]
		 )
	     | _, true -> ["destructor"]
	     | _ -> []
	  )
      | _ -> assert false


let var_class_constructor_label (v : 'a variable_type) =
  match !(v.funcDefn) with
    | None -> []
    | Some fun_type -> fun_class_constructor_label fun_type

