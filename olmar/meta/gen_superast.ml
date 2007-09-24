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
 * interface mli
 *
 ******************************************************************************
 ******************************************************************************)

let superast_type_decl ast oc =
  let out = output_string oc in
  let fpf format = fprintf oc format 
  in
    out "type 'a super_ast =\n";
    List.iter
      (fun cl ->
	 fpf "  | %s of 'a %s\n"
	   (superast_constructor cl)
	   (node_ml_type_name cl))
      ast;
    out "  | No_ast_node\n"


let interface input ast oc = 
  let out = output_string oc 
  in
    pr_comment oc
      [do_not_edit_line;
       "";
       "automatically generated from gen_superast from " ^ input;
      ];
    out "\n\n";

    superast_type_decl ast oc;

    out "\n\n"


(******************************************************************************
 ******************************************************************************
 *
 * implementation ml
 *
 ******************************************************************************
 ******************************************************************************)

let into_array_fun_name cl =
  let cl = match cl.ac_super with
    | None -> cl
    | Some super -> super
  in 
    (node_ml_type_name cl) ^ "_into_array"

let into_array_header first oc cl =
  let fpf format = fprintf oc format 
  in
    fpf "%s %s ast_array order_fun visited_nodes (x : annotated %s) =\n"
      (if !first
       then 
	 begin first := false; "let rec" end
       else "and")
      (into_array_fun_name cl)
      (node_ml_type_name cl)
       

let rec into_array_fun oc = function
  | AT_base _ -> assert false;
  | AT_list(_, inner, _) ->
      output_string oc "List.iter (";
      into_array_fun oc inner;
      output_string oc ")";
  | AT_node cl -> 
      output_string oc (into_array_fun_name cl);
      output_string oc " ast_array order_fun visited_nodes"


let into_array_rec_field oc field indent name =
  let out = output_string oc in
  let fpf format = fprintf oc format 
  in
    out indent;
    into_array_fun oc field.af_mod_type;
    out " ";
    fpf "%s;\n" name
      

let record_into_array first oc cl =
  let out = output_string oc in
  let fpf format = fprintf oc format 
  in
    into_array_header first oc cl;
    fpf "  let annot = x.%s\n" (annotation_field_name cl);
    out "  in\n";
    out "    if visited visited_nodes annot then ()\n";
    out "    else begin\n";
    fpf "      ast_array.(id_annotation annot) <- %s x;\n"
      (superast_constructor cl);
    out "      visit visited_nodes annot;\n\n";

    List.iter
      (List.iter
	 (fun field ->
	    if not field.af_is_base_field
	    then
	      into_array_rec_field oc field "      " ("x." ^ field.af_name)))
      (get_all_fields cl);

    out "      order_fun (id_annotation annot);\n";
    out "    end\n\n\n"

let variant_case_into_array oc cl =
  let out = output_string oc in
  let fpf format = fprintf oc format in
  let fields = List.flatten (get_all_fields cl) in
  let field_counter = ref 1 
  in
    if List.for_all (fun f -> f.af_is_base_field) fields
    then
      fpf "         | %s _ -> ()\n\n" (variant_name cl)
    else 
      begin
	fpf "         | %s(_" (variant_name cl);
	List.iter 
	  (fun f -> 
	     if f.af_is_base_field 
	     then 
	       out ", _"
	     else
	       fpf ", a%d" !field_counter;
	     incr field_counter)
	  fields;
	out ") ->\n";
	field_counter := 1;
	List.iter
	  (fun f ->
	     if not f.af_is_base_field
	     then
	       into_array_rec_field oc f "             " 
		 (sprintf "a%d" !field_counter);
	     incr field_counter)
	  fields;
	out "\n"
      end
      

let variant_into_array first oc cl =
  let out = output_string oc in
  let fpf format = fprintf oc format 
  in
    into_array_header first oc cl;
    fpf "  let annot = %s x\n" (annotation_access_fun cl);
    out "  in\n";
    out "    if visited visited_nodes annot then ()\n";
    out "    else begin\n";
    fpf "      ast_array.(id_annotation annot) <- %s x;\n"
      (superast_constructor cl);
    out "      visit visited_nodes annot;\n";
    out "      (match x with\n";
    
    List.iter (variant_case_into_array oc) cl.ac_subclasses;
    out "      );\n";
    out "      order_fun (id_annotation annot);\n";
    out "    end\n";
    out "\n\n"



let implementation input ast oc = 
  let out = output_string oc in
  (* let fpf format = fprintf oc format in *)
  let first = ref true
  in
    pr_comment oc
      [do_not_edit_line;
       "";
       "automatically generated from gen_superast from " ^ input;
      ];
    out "\n\n";

    pr_comment oc
      [star_line;
       "superast type declaration";
       ""];

    superast_type_decl ast oc;
    out "\n\n\n";

    pr_comment oc
      [star_line;
       "superast type declaration";
       ""];

    List.iter
      (fun cl ->
	 if cl.ac_subclasses = [] 
	 then
	   record_into_array first oc cl
	 else
	   variant_into_array first oc cl)
      ast;

    out "\n\n"


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
  let (oast_file, ast) = setup_ml_ast arguments "gen_superast" in
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
    with_file ".mli" (interface oast_file ast);
    with_file ".ml" (implementation oast_file ast);

;;

main ()
