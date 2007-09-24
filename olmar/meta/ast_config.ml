(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open More_string

(******************************************************************************
 ******************************************************************************
 *
 * config file state
 *
 ******************************************************************************
 ******************************************************************************)

let translate_name_hash = Hashtbl.create 479

let basic_type_hash = Hashtbl.create 53

let ocaml_type_header = ref []

let ocaml_reflect_header = ref []

let ast_top_nodes = ref []


(******************************************************************************
 ******************************************************************************
 *
 * config file parsing
 *
 ******************************************************************************
 ******************************************************************************)

(* scan one line from the remaining section of the config file *)
let do_renaming file_name line_number line =
  try
    Scanf.sscanf line "%s@.%s %s" 
      (fun context name rename ->
	 Hashtbl.add translate_name_hash 
	   ((Some context), name)
	   rename)
  with
    | Scanf.Scan_failure _ ->
	try
	  Scanf.sscanf line "%s %s" 
	    (fun name rename ->
	       Hashtbl.add translate_name_hash 
		 (None, name)
		 rename)
	with
	  | Scanf.Scan_failure _ ->
	      Printf.eprintf 
		"File \"%s\", line %d: syntax error in renaming section\n"
		file_name line_number;
	      exit 3


(* type for the config parser state *)
type config_file_modus =
  | Section
  | Renaming
  | Basic_types
  | Ocaml_type_header
  | Ocaml_reflect_header
  | Top_node


let config_sections =
  let hash = Hashtbl.create 23
  in
    List.iter (fun (k,v) -> Hashtbl.add hash k v)
      [ ("[renamings]", Renaming);
	("[basic types]", Basic_types);
	("[type header]", Ocaml_type_header);
	("[ocaml_reflect header]", Ocaml_reflect_header);
	("[top node]", Top_node);
      ];
    hash

(* parse the config file *)
let parse_config_file tr = 
  let ic = open_in tr in
  let line_number = ref 0 in
  let current_modus = ref Section in
  let read_file () =
    while true do
      let verb_line = incr line_number; input_line ic in
      let line = trim_white_space verb_line in
      let len = String.length line
      in
	if len > 0 && verb_line.[0] <> '#'
	then begin
	  if line.[0] = '[' 
	  then current_modus := Section;
	  match !current_modus with
	    | Section ->
		let line = String.lowercase line
		in
		  (try
		     current_modus := Hashtbl.find config_sections line
		   with
		     | Not_found ->
			 Printf.eprintf 
			   "File \"%s\", line %d: unrecognized section intro\n"
			   tr !line_number;
			 exit 2;
		  )
		   
	    | Renaming -> do_renaming tr !line_number line
	    | Basic_types -> Hashtbl.add basic_type_hash line ()
	    | Ocaml_type_header ->
		ocaml_type_header := line :: !ocaml_type_header
	    | Ocaml_reflect_header ->
		ocaml_reflect_header := line :: !ocaml_reflect_header
	    | Top_node ->
		ast_top_nodes := line :: !ast_top_nodes
	end
    done
  in
    try
      read_file ()
    with 
      | End_of_file -> 
	  close_in ic;
	  ocaml_reflect_header := List.rev !ocaml_reflect_header;
	  ocaml_type_header := List.rev !ocaml_type_header


(******************************************************************************
 ******************************************************************************
 *
 * access functions
 *
 ******************************************************************************
 ******************************************************************************)


(* translate an identifier through the config file renaming section *)
let translate_olmar_name context name = 
  try
    Hashtbl.find translate_name_hash (context, name)
  with
    | Not_found -> name


let get_ocaml_type_header () = !ocaml_type_header


let is_basic_type type_name = 
  Hashtbl.mem basic_type_hash type_name


let get_ast_top_nodes () = !ast_top_nodes


let get_ocaml_reflect_header () = !ocaml_reflect_header
