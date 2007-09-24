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
 * accessors implementation ml
 *
 ******************************************************************************
 ******************************************************************************)

let record_annot oc cl =
  fprintf oc "let %s(x : 'a %s) = x.%s\n\n"
    (annotation_access_fun cl)
    (node_ml_type_name cl)
    (annotation_field_name cl)

let variant_annot oc cl =
  let out = output_string oc in
  let fpf format = fprintf oc format 
  in
    fpf "let %s = function\n" (annotation_access_fun cl);
    List.iter
      (fun sub ->
	 fpf "  | %s(annot" (variant_name sub);
	 List.iter
	   (List.iter (fun _ -> out ", _"))
	   (get_all_fields sub);
	 out ")\n")
      cl.ac_subclasses;
    out "    -> annot\n\n"
	      


let implementation input ast oc = 
  let out = output_string oc 
  (* let fpf format = fprintf oc format in *)
  in
    pr_comment oc
      [do_not_edit_line;
       "";
       "automatically generated from gen_superast from " ^ input;
      ];
    out "\n\n";

    pr_comment oc
      [star_line;
       star_line;
       "";
       "               annotation accessors";
       "";
       star_line;
       star_line	 
      ];

    List.iter
      (fun cl ->
	 if cl.ac_subclasses = [] 
	 then
	   record_annot oc cl
	 else
	   variant_annot oc cl)
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
    with_file ".ml" (implementation oast_file ast)
;;

main ()
