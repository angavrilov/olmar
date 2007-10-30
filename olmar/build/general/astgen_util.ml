(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

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


let get_shared_node (addr : int) (typ : int) =
  try
    Some(Hashtbl.find node_hash (addr, typ))
  with
    | Not_found -> None


let register_shared_node (addr : int) (typ : int) (obj : Obj.t) =
  assert(not (Hashtbl.mem node_hash (addr, typ)));
  Hashtbl.add node_hash (addr, typ) obj


let register_astgen_util_callbacks () =
  Callback.register "get_shared_node" get_shared_node;
  Callback.register "register_shared_node" register_shared_node;
  Callback.register "hashtbl_create" Hashtbl.create;
  Callback.register "hashtbl_add" Hashtbl.add;
  ()
