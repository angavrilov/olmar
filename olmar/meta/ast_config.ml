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

let translate_backward_hash = Hashtbl.create 479

let basic_type_hash = Hashtbl.create 53

let implicit_pointer_hash = Hashtbl.create 23

let color_hash = Hashtbl.create 127

let ocaml_type_header = ref []

let ocaml_reflect_header = ref []

let ast_top_nodes = ref []

let superclass_kind_hash = Hashtbl.create 7

let subclass_tag_hash = Hashtbl.create 53

let field_assertion_hash = Hashtbl.create 7

let private_accessor_hash = Hashtbl.create 7

let downcast_hash = Hashtbl.create 23

let record_variant_hash = Hashtbl.create 53

let graph_label_fun_hash = Hashtbl.create 53



(******************************************************************************
 ******************************************************************************
 *
 * backward translation
 *
 ******************************************************************************
 ******************************************************************************)

let fill_backward_translation () =
  Hashtbl.iter 
    (fun (context_opt, name) rename ->
       if Hashtbl.mem translate_backward_hash (context_opt, rename)
       then
	 let context = match context_opt with
	   | None -> ""
	   | Some con -> con ^ "."
	 in
	   Printf.eprintf "Warning: %s%s and %s%s are both renamed to %s\n"
	     context name
	     context 
	     (Hashtbl.find translate_backward_hash (context_opt, rename))
	     rename
       else
	 Hashtbl.add translate_backward_hash (context_opt, rename) name)
    translate_name_hash


(******************************************************************************
 ******************************************************************************
 *
 * config file parsing
 *
 ******************************************************************************
 ******************************************************************************)

let error_msg file_name line_number msg = 
  Printf.eprintf "File \"%s\", line %d: %s\n%!"
    file_name line_number msg


(* scan one line from the remaining section of the config file *)
let do_renaming file_name line_number line =
  try
    Scanf.sscanf line "%s@.%s %s%!" 
      (fun context name rename ->
	 if (String.length context) = (String.length line) then
	   raise (Scanf.Scan_failure "dot missing");
	 (* Printf.eprintf "RENAME %s.%s->%s\n" context name rename; *)
	 Hashtbl.add translate_name_hash 
	   ((Some context), name)
	   rename)
  with
    | Scanf.Scan_failure _ ->
	try
	  Scanf.sscanf line "%s %s%!" 
	    (fun name rename ->
	       (* Printf.eprintf "RENAME %s->%s\n" name rename; *)
	       Hashtbl.add translate_name_hash 
		 (None, name)
		 rename)
	with
	  | Scanf.Scan_failure _ ->
	      error_msg file_name line_number 
		"syntax error in renaming section";
	      exit 3

let scanf_line file_name line_number line format error_format continuation =
  try
    Scanf.sscanf line format continuation
  with
    | Scanf.Scan_failure _ ->
	error_msg file_name line_number 
	  (Printf.sprintf "syntax error, expected %s" error_format);
	exit 4

let scanf_id file_name line_number line f =
  scanf_line file_name line_number line "%s@\n" "<id>" f

let do_colon_ids hash file_name line_number line =
  scanf_line file_name line_number line "%s@: %s" "<id>: <id>"
    (fun subject value ->
       Hashtbl.add hash 
	 (trim_white_space subject) (trim_white_space value))


let do_class_field_id hash file_name line_number line =
  scanf_line file_name line_number line "%s@.%s@: %s" "<id>.<id>: <id>"
    (fun class_name field_name assertion ->
       Hashtbl.add hash
	 ((trim_white_space class_name), (trim_white_space field_name))
	 (trim_white_space assertion))


(* type for the config parser state *)
type config_file_modus =
  | Section
  | Renaming
  | Basic_types
  | Implicit_pointers
  | Ocaml_type_header
  | Ocaml_reflect_header
  | Top_node
  | Node_colors
  | Superclass_kind
  | Subclass_tags
  | Subclass_downcast
  | Field_assertions
  | Private_accessors
  | Record_variants
  | Graph_label_funs


let config_sections =
  let hash = Hashtbl.create 23
  in
    List.iter (fun (k,v) -> Hashtbl.add hash k v)
      [ ("[renamings]", Renaming);
	("[basic types]", Basic_types);
	("[implicit pointer types]", Implicit_pointers);
	("[type header]", Ocaml_type_header);
	("[ocaml_reflect header]", Ocaml_reflect_header);
	("[top node]", Top_node);
	("[node colors]", Node_colors);
	("[superclass get kind]", Superclass_kind);
	("[subclass tags]", Subclass_tags);
	("[subclass downcasts]", Subclass_downcast);
	("[field assertions]", Field_assertions);
	("[private accessors]", Private_accessors);
	("[record variants]", Record_variants);
	("[additional graph labels]", Graph_label_funs);
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
			 error_msg tr !line_number "unrecognized section intro";
			 exit 2;
		  )
		   
	    | Renaming -> do_renaming tr !line_number line
	    | Basic_types -> Hashtbl.add basic_type_hash line ()
	    | Implicit_pointers -> Hashtbl.add implicit_pointer_hash line ()
	    | Ocaml_type_header ->
		ocaml_type_header := line :: !ocaml_type_header
	    | Ocaml_reflect_header ->
		ocaml_reflect_header := line :: !ocaml_reflect_header
	    | Top_node ->
		ast_top_nodes := line :: !ast_top_nodes
	    | Node_colors -> do_colon_ids color_hash tr !line_number line
	    | Superclass_kind ->
		do_colon_ids superclass_kind_hash tr !line_number line
	    | Subclass_tags -> 
		do_colon_ids subclass_tag_hash tr !line_number line
	    | Subclass_downcast ->
		do_colon_ids downcast_hash tr !line_number line
	    | Field_assertions ->
		do_class_field_id field_assertion_hash tr !line_number line
	    | Private_accessors ->
		do_class_field_id private_accessor_hash tr !line_number line
	    | Record_variants ->
		scanf_id tr !line_number line 
		  (fun id ->
		     Hashtbl.add record_variant_hash (trim_white_space id) ())
	    | Graph_label_funs ->
		do_colon_ids graph_label_fun_hash tr !line_number line
	end
    done
  in
    try
      read_file ()
    with 
      | End_of_file -> 
	  close_in ic;
	  ocaml_reflect_header := List.rev !ocaml_reflect_header;
	  ocaml_type_header := List.rev !ocaml_type_header;
	  fill_backward_translation()


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


let is_implicit_pointer_type type_name = 
  Hashtbl.mem implicit_pointer_hash type_name


let get_ast_top_nodes () = !ast_top_nodes


let get_ocaml_reflect_header () = !ocaml_reflect_header


let get_node_color class_name =
  Hashtbl.find color_hash (translate_olmar_name None class_name)


let superclass_get_kind class_name =
  Hashtbl.find superclass_kind_hash (translate_olmar_name None class_name)


let get_subclass_tag class_name =
  Hashtbl.find subclass_tag_hash (translate_olmar_name None class_name)


let get_downcast class_name =
  Hashtbl.find downcast_hash (translate_olmar_name None class_name)


let get_field_assertion class_name field_name =
  let tr_class_name = translate_olmar_name None class_name
  in
    Hashtbl.find field_assertion_hash 
      (tr_class_name, (translate_olmar_name (Some tr_class_name) field_name))


let get_private_accessor class_name field_name =
  let tr_class_name = translate_olmar_name None class_name
  in
    Hashtbl.find private_accessor_hash
      (tr_class_name, (translate_olmar_name (Some tr_class_name) field_name))


let variant_is_record class_name =
  Hashtbl.mem record_variant_hash (translate_olmar_name None class_name)


let get_graph_label_fun class_name =
  Hashtbl.find graph_label_fun_hash (translate_olmar_name None class_name)


let translate_back context rename =
  let name = 
    try
      Hashtbl.find translate_backward_hash (context, rename)
    with
      | Not_found -> rename
  in
  let orig_context = match context with
    | None -> None
    | Some con -> 
	try
	  Some(Hashtbl.find translate_backward_hash (None, con))
	with
	  | Not_found -> context
  in
    (orig_context, name)
    
