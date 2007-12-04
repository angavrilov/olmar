(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Cfg_type


type cfg_dot_options = {
  node_ids : bool			(* print node id and file *)
}


(* cfg_to_dot cfg outfile:
 * convert a cfg into dot and write to designated file
 * or stdout if outfile = None
 *)
val cfg_to_dot : cfg_dot_options -> cfg_type -> string option -> unit



(* generate only the call graph from the functions given *)
val function_call_graphs_to_dot : 
  cfg_dot_options -> cfg_type -> string option -> function_id_type list -> unit
