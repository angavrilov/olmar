(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Printf
open More_string
open Ast_config
open Meta_ast


(******************************************************************************
 ******************************************************************************
 *
 * uplinks ml
 *
 ******************************************************************************
 ******************************************************************************)

let rec get_string_of_type oc id = function
  | AT_base base_type -> 
      if base_type = "string" 
      then 
	fprintf oc "(truncate_string %s)" id
      else
	fprintf oc "(string_of_%s %s)" (String.uncapitalize base_type) id
  | AT_node _ -> assert false
  | AT_list(_, inner, _) ->
      output_string oc "(string_of_list (List.map (fun x -> ";
      get_string_of_type oc "x" inner;
      fprintf oc ") %s))" id

let rec gen_id_annot oc label id top_level = function
  | AT_base _ -> assert false
  | AT_node cl -> 
      if top_level then output_string oc "[";
      fprintf oc "(%s %s, \"%s\")" (annotation_access_fun cl) id label;
      if top_level then output_string oc "]";
  | AT_list(_, inner, _) ->
      output_string oc "count_label_rev (List.map (fun x -> ";
      gen_id_annot oc label "x" false inner;
      fprintf oc ") %s)" id


let graph_node oc cl annot fields field_names =
  let out = output_string oc in
  let fpf format = fprintf oc format in
  let super_class = match cl.ac_super with
    | None -> cl
    | Some super -> super
  in
  let source_loc_field = get_source_loc_field cl in
  let field_assoc = List.combine fields field_names in
  let (attributes, childs) = 
    List.partition 
      (fun (f,_) -> f.af_is_base_field)
      (match source_loc_field with 
	   (* delete source loc field from attributes *)
	 | None -> field_assoc
	 | Some f -> List.remove_assq f field_assoc)
  in
    fpf "      G.make_node (id_annotation %s)\n" annot;
    fpf "        ((Printf.sprintf \"%s %%d\" (id_annotation %s)) ::\n"
      (translated_class_name cl) annot;
    (match source_loc_field with
       | None -> ()
       | Some f ->
	   fpf "          (loc_label %s) ::\n"
	     (List.assq f field_assoc)
    );
    (List.iter
       (fun (attr, id) ->
	  fpf "           [(\"%s: \" ^ " (translated_field_name cl attr);
	  get_string_of_type oc id attr.af_mod_type;
	  out ")] @\n"
       )
       attributes);
    out "           [])\n";

    fpf "        [(\"color\", \"%s\")]\n" 
      (try
	 get_node_color super_class.ac_name
       with
	 | Not_found -> 
	     eprintf "No color configured for %s\n"
	       (translated_class_name super_class);
	     "white"
      );

    out "        (filter_childs node_array []\n";
    out "           [\n";
    (List.iter 
       (fun (child_field, child_field_name) -> 
	  out "             ";
	  gen_id_annot oc (translated_field_name cl child_field)
	    child_field_name true child_field.af_mod_type;
	  out ";\n"
       ) 
       (List.rev childs)
    );
    out "           ])\n";
    out "\n\n"
  


let graph_super oc cl =
  let fpf format = fprintf oc format in
  let fields = get_all_fields_flat cl in
  let field_names = 
    List.map (fun f -> "x." ^ (translated_field_name cl f)) fields
  in
    fpf "  | %s x ->\n" (superast_constructor cl);
    graph_node oc cl 
      ("x." ^ (annotation_field_name cl))
      fields field_names


let graph_sub oc super cl =
  let fpf format = fprintf oc format in
  let fields = get_all_fields_flat cl in
  let field_names = generate_names "a" (List.length fields)
  in
    fpf "  | %s(%s(%s)) ->\n" 
      (superast_constructor super) (variant_name cl)
      (String.concat ", " ("annot" :: field_names));
    graph_node oc cl "annot" fields field_names


let implementation input ast oc = 
  let out = output_string oc in
  let fpf format = fprintf oc format in
  let counter = ref 1
  in
    pr_comment oc
      [do_not_edit_line;
       "";
       "automatically generated from gen_graph from " ^ input;
      ];
    out "\n\n";
    
    out "let ast_node_fun node_array = function\n";
    fpf "  | %s -> assert false\n\n" name_of_superast_no_ast;

    List.iter 
      (fun cl ->
	 if cl.ac_subclasses = [] 
	 then
	   begin
	     fpf "  (* %d *)\n" !counter;
	     incr counter;
	     graph_super oc cl
	   end 
	 else 
	   List.iter 
	     (fun sub -> 
		fpf "  (* %d *)\n" !counter;
		incr counter;
		graph_sub oc cl sub) 
	     cl.ac_subclasses
      )
      ast;

    out "\n"


(******************************************************************************
 ******************************************************************************
 *
 * arguments and main
 *
 ******************************************************************************
 ******************************************************************************)


let output_prefix_option = ref None

let arguments = 
  [
    ("-o", Arg.String (fun s -> output_prefix_option := Some s),
     "out set output prefix to out");
  ]

let main () =
  let (oast_file, ast) = setup_ml_ast arguments "gen_graph" in
  let output_prefix = match !output_prefix_option with
    | Some prefix -> prefix
    | None -> 
	if Filename.check_suffix oast_file ".oast" 
	then
	  Filename.chop_suffix oast_file ".oast"
	else
	  oast_file
  in
  let file ext = output_prefix ^ ext in
  let with_file ext action =
    let file_name = file ext in
    let oc = open_out file_name
    in
      action oc;
      close_out oc
  in
    with_file ".ml" (implementation oast_file ast)
;;

main ()
