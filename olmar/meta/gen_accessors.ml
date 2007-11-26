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
 * annotation accessors 
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
	 if sub.ac_record then
	   begin
	     fpf "  | %s x -> x.%s\n" 
	       (variant_name sub)
	       (annotation_field_name sub)
	   end
	 else
	   begin
	     fpf "  | %s(annot" (variant_name sub);
	     List.iter
	       (List.iter (fun _ -> out ", _"))
	       (get_all_fields sub);
	     out ") -> annot\n"
	   end)
      cl.ac_subclasses;
    out "\n\n";
    List.iter
      (fun recsub -> record_annot oc recsub)
      (List.filter (fun sub -> sub.ac_record) cl.ac_subclasses)
	      

let do_annot oc ast =
  List.iter
    (fun cl ->
       if cl.ac_subclasses = [] 
       then
	 record_annot oc cl
       else
	 variant_annot oc cl)
    ast


(******************************************************************************
 ******************************************************************************
 *
 * SourceLoc accessors 
 *
 ******************************************************************************
 ******************************************************************************)

let record_source_loc oc cl =
  match get_source_loc_field cl with
    | None -> ()
    | Some f ->
	fprintf oc "let %s x = x.%s\n\n" 
	  (source_loc_access_fun cl)
	  (translated_field_name cl f)



let variant_source_loc oc cl =
  let out = output_string oc in
  let fpf format = fprintf oc format in
  let fields = List.map get_source_loc_field cl.ac_subclasses
  in
  if List.for_all (fun field_opt -> field_opt <> None) fields
  then begin
    fpf "let %s = function\n" (source_loc_access_fun cl);
    List.iter2
      (fun sub field_opt ->
	 match field_opt with
	   | None -> assert false
	   | Some source_loc_field ->
	       if sub.ac_record 
	       then
		 fpf "  | %s x -> %s x\n" 
		   (variant_name sub)
		   (source_loc_access_fun sub)
	       else
		 fpf "  | %s(_, %s) -> loc\n"
		   (variant_name sub)
		   (String.concat ", "
		      (List.map
			 (fun f -> if f == source_loc_field then "loc" else "_")
			 (get_all_fields_flat sub))))
      cl.ac_subclasses fields;
    out "\n\n"
  end


let do_source_loc oc ast =
  List.iter
    (fun cl ->
       if cl.ac_subclasses = [] 
       then
	 record_source_loc oc cl
       else
	 begin
	   List.iter (fun reccl -> record_source_loc oc reccl)
	     (List.filter (fun cl -> cl.ac_record) cl.ac_subclasses);
	   variant_source_loc oc cl
	 end
	 )
    ast


(******************************************************************************
 ******************************************************************************
 *
 * Expression Type Accessor
 *
 ******************************************************************************
 ******************************************************************************)


let do_expr_type oc expr_cl =
  let out = output_string oc in
  let fpf format = fprintf oc format in
  let type_field = List.hd expr_cl.ac_fields
  in
    out "let expression_type = function\n";
    List.iter
      (fun sub_expr ->
	 if sub_expr.ac_record then
	   fpf "  | %s x -> x.%s\n" 
	     (variant_name sub_expr)
	     (translated_field_name sub_expr type_field)
	 else
	   fpf "  | %s(_, %s) -> expr_type\n"
	     (variant_name sub_expr)
	     (String.concat ", "
		(List.map
		   (fun f -> if f == type_field then "expr_type" else "_")
		   (get_all_fields_flat sub_expr)))
      )
      expr_cl.ac_subclasses



(******************************************************************************
 ******************************************************************************
 *
 * Implementation unit
 *
 ******************************************************************************
 ******************************************************************************)

let expr_type_accessor = ref false


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

    pr_comment_heading oc ["               annotation accessors";];
    do_annot oc ast;
    out "\n\n";

    pr_comment_heading oc ["               SourceLoc accessors";];
    do_source_loc oc ast;

    out "\n\n";
    
    if !expr_type_accessor then begin
      pr_comment_heading oc ["               Expression type accessor";];
      do_expr_type oc (get_node "Expression");

      out "\n\n"
    end
    




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
    ("-expr-type-accessor", Arg.Set expr_type_accessor,
     " generate the type accessor for expressions");
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
