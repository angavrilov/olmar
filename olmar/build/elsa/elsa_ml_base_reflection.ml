(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* reflection code for types in elsa_ml_base_types *)

open Elsa_ml_base_types

(* source loc hashing stuff *)
type source_loc_hash =
    (string, string) Hashtbl.t * (nativeint, sourceLoc) Hashtbl.t

let source_loc_hash : source_loc_hash =
  ((Hashtbl.create 50), (Hashtbl.create 1543))

let source_loc_hash_find 
    ((_strings, locs) : source_loc_hash) (loc : nativeint) =
  let ret = Hashtbl.find locs loc
  in
    ret


let source_loc_hash_add ((strings, locs) : source_loc_hash)
    (loc : nativeint) ((file,line,char) as srcloc : sourceLoc) =
  assert(not (Hashtbl.mem locs loc));
  let new_file =
    try
      Hashtbl.find strings file
    with
      | Not_found ->
	  Hashtbl.add strings file file;
	  file
  in
  let ret = if new_file == file then srcloc else (new_file, line, char) in
    Hashtbl.add locs loc ret;
    (* 
     * if debug_print_locs then
     *   Printf.eprintf "srcloc hash %nd -> %s:%d:%d\n%!" loc file line char;
     *)
    ret


(* type array_size *)

let create_array_size_NO_SIZE_constructor () = NO_SIZE
let create_array_size_DYN_SIZE_constructor () = DYN_SIZE
let create_array_size_FIXED_SIZE_constructor i = FIXED_SIZE i

let register_array_size_constructors () =
  Callback.register "create_array_size_NO_SIZE_constructor" 
    create_array_size_NO_SIZE_constructor;
  Callback.register "create_array_size_DYN_SIZE_constructor" 
    create_array_size_DYN_SIZE_constructor;
  Callback.register "create_array_size_FIXED_SIZE_constructor" 
    create_array_size_FIXED_SIZE_constructor


let register_src_loc_callbacks () =
  Callback.register_exception "not_found_exception_id" (Not_found);
  Callback.register "source_loc_hash" source_loc_hash;
  Callback.register "source_loc_hash_find" source_loc_hash_find;
  Callback.register "source_loc_hash_add" source_loc_hash_add;
  register_array_size_constructors();
  ()
