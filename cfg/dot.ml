(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Cfg_type

let pf = Printf.fprintf

let line oc line = 
  output_string oc line;
  output_string oc "\n"


let assign_node_ids cfg nodes =
  let counter = ref 0 in
  let add fun_id =
    if not (Hashtbl.mem nodes fun_id) 
    then
      begin
	Hashtbl.add nodes fun_id !counter;
	incr counter;
      end
  in
    Hashtbl.iter
      (fun _ fun_def ->
	 if fun_def.callees <> [] 
	 then
	   begin
	     add fun_def.fun_id;
	     List.iter
	       add
	       fun_def.callees
	   end
      )
      cfg


let output_node_id oc this_node fun_id =
  pf oc "    \"%d\" [label=\"%s" 
    this_node 
    (String.concat "::" fun_id.name);
  ignore(
    List.fold_left
      (fun count p -> 
	 pf oc "\\nArg %d: %s" count p;
	 count +1)
      1
      fun_id.param_types
  )

let node_id_end oc = 
    line oc "\"]"

	 
let output_node oc cfg nodes fun_id this_node =
  output_node_id oc this_node fun_id;
  (try
     match (Hashtbl.find cfg fun_id).loc with
       | (file, line, _) ->
	   pf oc "\\nfile: %s\\nline %d" file line
   with
     | Not_found -> ()
  );
  node_id_end oc;
  (try
     List.iter
       (fun callee ->
	  pf oc "    \"%d\" -> \"%d\";\n"
	    this_node
	    (Hashtbl.find nodes callee))
     (Hashtbl.find cfg fun_id).callees
   with
     | Not_found -> ()
  )
  


let cfg_to_dot cfg outfile = 
  let oc = 
    match outfile with
      | None -> stdout
      | Some f -> open_out f
  in
  let nodes = Hashtbl.create (2 * Hashtbl.length cfg) 
  in
    assign_node_ids cfg nodes;
    line oc "digraph \"CFG\" {";
    line oc "  node [ color = \"lightblue\"; style = filled ];";
    line oc "  edge [ arrowtail=odot ];";
    Hashtbl.iter (output_node oc cfg nodes) nodes;
    line oc "}"
    
