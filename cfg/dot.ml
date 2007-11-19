(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Cfg_type
open Cfg_util

module Node_id = 
struct
  type t = function_id_type

  let string_of_t id = String.concat "::" id.name
end


module G = Dot_graph.Make(Node_id)


let make_node cfg id =
  let loc_label =
      (try
	 match (Hashtbl.find cfg id).loc with
	   | (file, line, _) ->
	       [Printf.sprintf "file: %s" file;
		Printf.sprintf "line %d" line]
       with
	 | Not_found -> []
      )
  in
  let color_attribute =
    if Hashtbl.mem cfg id 
    then []
    else [("color", "OrangeRed")]
  in
  let arg_count = ref 0 in
  let args =
    List.map 
      (fun arg -> 
	 incr arg_count;
	 Printf.sprintf "Arg %d: %s" !arg_count arg)
      id.param_types
  in
  let childs =
    try
      (Hashtbl.find cfg id).callees
    with
      | Not_found -> []
  in
  (* 
   * let _ = 
   *   Printf.eprintf "add %s with %d callees\n"
   * 	(String.concat "::" id.name) (List.length childs)
   * in
   *)
    try
      ignore(
	G.make_node_unlabelled id 
	  ((fun_id_name id) ::
	     (args @ loc_label))
	  color_attribute
	  childs)
    with
      | G.Node_id_not_unique -> ()


let make_nodes cfg _id def =
  if def.callees <> [] 
  then
    begin
      make_node cfg def.fun_id;
      List.iter
	(make_node cfg)
	def.callees
    end


let cfg_commands =
  ["node [ color = \"lightblue\", style = filled ]";
   "edge [ arrowtail=odot ]"
  ]


let cfg_to_dot cfg outfile = 
  Hashtbl.iter (make_nodes cfg) cfg;
  G.write_graph "CFG" cfg_commands outfile


let function_call_graphs_to_dot cfg outfile fun_ids =
  iter_callees
    (fun _context fun_id _fun_def_opt -> make_node cfg fun_id)
    cfg
    fun_ids;
  G.write_tree "CFG" cfg_commands outfile
	   
