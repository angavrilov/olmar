(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* 
 * make List.rev accessible from the C side 
 * 
 * not much code, but it has to go somewhere
 *)

let register_ast_util_callbacks () =
  Callback.register "List.rev" List.rev
