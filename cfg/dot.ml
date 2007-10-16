(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Cfg_type

module Node_id = 
struct
  type t = function_id_type

  let string_of_t id = String.concat "::" id.name
end


module G = Dot_graph.Make(Node_id)

let make_nodes cfg _id def =
  let add id =
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
      try
	ignore(
	  G.make_node_unlabelled id 
	    ((String.concat "::" id.name) ::
	       (args @ loc_label))
	    []
	    childs)
      with
	| G.Node_id_not_unique -> ()
  in
  if def.callees <> [] 
  then
    begin
      add def.fun_id;
      List.iter
	add
	def.callees
    end


let cfg_commands =
  ["node [ color = \"lightblue\", style = filled ]";
   "edge [ arrowtail=odot ]"
  ]

let cfg_to_dot cfg outfile = 
  Hashtbl.iter (make_nodes cfg) cfg;
  G.write_dot_file "CFG" cfg_commands outfile
