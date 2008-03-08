(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* 
 * make List.rev accessible from the C side 
 * 
 * not much code, but it has to go somewhere
 *)

let string_table : (int, string) Hashtbl.t = Hashtbl.create 1000

let register_string id value =
  try
    Hashtbl.find string_table id
  with
    Not_found ->
      Hashtbl.add string_table id value;
      value   

let register_ast_util_callbacks () =
  Callback.register "List.rev" List.rev;
  Callback.register "Reg_StringRef" register_string
