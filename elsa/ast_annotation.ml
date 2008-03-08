(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* annotation type for ast nodes, utility functions *)

(*
 * an element of annotated gets inserted 
 * into every node of the ocaml ast
 *
 * the first int is a unique number for identification, 
 * the second is the address of the C++ object in memory
 * (shifted left to fit into an ocaml int)
 *)
type annotated = int * int

(* accessor Functions *)

let id_annotation ((id,_) : annotated) = id

let caddr_annotation ((_, caddr) : annotated) = caddr

(* Annotation spawner *)

let next_id = ref 1

let create_ast_annotation (c_addr : int) = 
	let id = !next_id
	in
	  incr next_id;
	  (id, c_addr)

let max_annotation() = !next_id -1

(* Pseudo-annotation spawner *)

let last_pseudo_id = ref 0

let pseudo_annotation () =
  incr last_pseudo_id;
  (- !last_pseudo_id, 0)

(* Node reference table *)

let dummy_elt = Obj.repr "dummy"
let block_shift = 19
let key_mask = (1 lsl block_shift) - 1
let node_tbl : Obj.t array array = Array.make 100 [||]
let pseudo_tbl : Obj.t array array = Array.make 100 [||]

let register_tbl_node table id node =
  let block = id lsr block_shift
  and key = id land key_mask
  in  
  assert (block < (Array.length table));
  let arr = 
    let cv = Array.get table block in
    if (Array.length cv) = 0 then
      begin
        let cv2 = Array.make (key_mask+1) dummy_elt in
        Array.set table block cv2;
        cv2
      end
    else
      cv
  in
  assert ((Array.get arr key) == dummy_elt);
  (*Printf.printf "%d (%d %d) <-\n" id block key; flush stdout;*)
  Array.set arr key node

let fetch_tbl_node table id =
  let block = id lsr block_shift
  and key = id land key_mask
  in
  (*Printf.printf "%d (%d %d) ->\n" id block key; flush stdout;*)
  assert (block < (Array.length table));
  let arr = Array.get table block in
  assert (key < (Array.length arr));
  let elt = Array.get arr key in
  assert (elt != dummy_elt);
  elt

let register_id_node ((id, _) : annotated) (node : Obj.t) =
  if id >= 0 then 
    register_tbl_node node_tbl id node
  else
    register_tbl_node pseudo_tbl (-id) node;
  id
  
let fetch_id_node id =
  if id >= 0 then 
    fetch_tbl_node node_tbl id
  else
    fetch_tbl_node pseudo_tbl (-id)

(* Callbacks *)
  
let register_ast_annotation_callbacks () =
  Callback.register "create_ast_annotation" create_ast_annotation;
  Callback.register "ocaml_max_annotation" max_annotation;
  Callback.register "ocaml_register_id_node" register_id_node;
  Callback.register "ocaml_fetch_id_node" fetch_id_node;
  Callback.register "ocaml_pseudo_annot" pseudo_annotation



