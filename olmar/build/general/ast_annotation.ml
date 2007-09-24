(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* annotation type for ast nodes, utility functions *)

(*
 * an element of annotated gets inserted 
 * into every node of the ocaml ast
 *
 * the int is a unique number for identification, 
 *)
type annotated = int

(* accessor Functions *)

let id_annotation (id : annotated) = id

let addr_hash = Hashtbl.create 10093

let next_id = ref 1

let next_annotation () =
  let id = !next_id
  in
    incr next_id;
    id

let create_ast_annotation (c_addr : int) = 
  try
    let res = Hashtbl.find addr_hash c_addr
    in
      (* currently we never ask a second time for an annotation *)
      assert false;
      res
  with
    | Not_found ->
	let id = next_annotation ()
	in
	  Hashtbl.add addr_hash c_addr id;
	  id

let max_annotation() = !next_id -1

let set_max_annotation m = next_id := m + 1

let pseudo_annotation = next_annotation


let register_ast_annotation_callbacks () =
  Callback.register "create_ast_annotation" create_ast_annotation;
  Callback.register "ocaml_max_annotation" max_annotation

