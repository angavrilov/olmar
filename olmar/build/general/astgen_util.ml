(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* source loc *)

(* SourceLoc is defined as an enum in srcloc.h, here we take the
 * xml representation, which is file * line * char
 *)
type sourceLoc = string * int * int


(* general utility functions *)

let string_of_flag_list string_of_elem = function
  | [] -> "[]"
  | hd::tl ->
      let buf = Buffer.create 20
      in
	Printf.bprintf buf "[%s" (string_of_elem hd);
	List.iter
	  (fun flag -> Printf.bprintf buf ", %s" (string_of_elem flag))
	  tl;
	Buffer.add_char buf ']';
	Buffer.contents buf


let node_hash = Hashtbl.create 19813
let list_hash = Hashtbl.create 19813

let get_shared_node (addr : int) =
  try
    Some(Hashtbl.find node_hash addr)
  with
    | Not_found -> None


let register_shared_node (addr : int) (obj : Obj.t) =
  assert(not (Hashtbl.mem node_hash addr));
  Hashtbl.add node_hash addr obj


let get_shared_list (addr : int) =
  try
    Some(Hashtbl.find list_hash addr)
  with
    | Not_found -> None


let register_shared_list (addr : int) (obj : Obj.t) =
  assert(not (Hashtbl.mem list_hash addr));
  Hashtbl.add list_hash addr obj


let register_astgen_util_callbacks () =
  Callback.register "get_shared_node" get_shared_node;
  Callback.register "get_shared_list" get_shared_list;
  Callback.register "register_shared_node" register_shared_node;
  Callback.register "register_shared_list" register_shared_list;
  ()
