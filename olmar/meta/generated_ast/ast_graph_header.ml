(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Ast_annotation
open Ast_ml_types
open Ast_reflect_type
open Ast_accessors
open Superast
open Meta_ast

(************************************************************************
 *
 * global state
 *
 ************************************************************************)

let normal_top_level_scope = ref false

let nodes_counter = ref 0

let edge_counter = ref 0

let max_string_length = ref 15

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


type 'a ast_array =
  | Ast_oast of 'a


let rec count_label_rev_rec res count = function
  | [] -> res
  | (x, l) :: xls ->
      count_label_rev_rec 
	((x, Printf.sprintf "%s[%d]" l count) :: res)
	(count + 1)
	xls

let count_label_rev l = count_label_rev_rec [] 0 l

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


