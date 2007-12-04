(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Cfg_type
open Cfg_util


type cfg_dot_options = {
  node_ids : bool
}


module Node_id = 
struct
  type t = function_id_type

  let string_of_t id = String.concat "::" id.name
end


module G = Dot_graph.Make(Node_id)


let make_node opts cfg id =
  let loc_label =
      (try
	 let def = Hashtbl.find cfg id in
	 let (file, line, _) = def.loc 
	 in
	   [Printf.sprintf "file: %s" file;
	    Printf.sprintf "line %d" line
	   ] 
	   @ 
	     if opts.node_ids
	     then
	       [ Printf.sprintf "node %d" def.node_id;
		 def.oast
	       ]
	     else []
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


let make_nodes opts cfg _id def =
  if def.callees <> [] 
  then
    begin
      make_node opts cfg def.fun_id;
      List.iter
	(make_node opts cfg)
	def.callees
    end


let cfg_commands =
  ["node [ color = \"lightblue\", style = filled ]";
   "edge [ arrowtail=odot ]"
  ]


let cfg_to_dot opts cfg outfile = 
  Hashtbl.iter (make_nodes opts cfg) cfg;
  G.write_graph "CFG" cfg_commands outfile


let function_call_graphs_to_dot opts cfg outfile fun_ids =
  iter_callees
    (fun _context fun_id _fun_def_opt -> make_node opts cfg fun_id)
    cfg
    fun_ids;
  G.write_tree "CFG" cfg_commands outfile
	   
