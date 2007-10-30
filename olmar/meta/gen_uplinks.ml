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


let rec uplinks_field oc indent field_val field_type =
  let fpf format = fprintf oc format
  in match field_type with
    | AT_base _ -> assert false
    | AT_list(kind, inner, _) ->
	fpf "%sList.iter\n" indent;
	fpf "%s  (fun x ->\n" indent;
	uplinks_field oc (indent ^ "     ") "x" inner;
	fpf "%s  )\n" indent;
	(match kind with
	   | LK_ast_list
	   | LK_fake_list
	   | LK_obj_list
	   | LK_sobj_list
	   | LK_sobj_set
	   | LK_string_obj_dict ->
	       fpf "%s  (List.rev %s);\n" indent field_val
	   | LK_string_ref_map ->
	       fpf "%s  (list_of_hashed_values %s);\n" indent field_val
	)
    | AT_node cl -> 
	fpf "%sadd_link up down myindex (%s %s);\n"
	  indent
	  (annotation_access_fun cl)
	  field_val

    | AT_option(inner, _) ->
	fpf "%soption_iter\n" indent;
	fpf "%s  (fun x ->\n" indent;
	uplinks_field oc (indent ^ "     ") "x" inner;
	fpf "%s  )\n" indent;
	fpf "%s  %s;\n" indent field_val

    | AT_ref inner ->
	uplinks_field oc indent (sprintf "!(%s)" field_val) inner


let uplinks_super oc cl =
  let out = output_string oc in
  let fpf format = fprintf oc format in
    (* do fields from right to left to get down links in left to right order *)
  let fields = List.rev (get_all_fields_flat cl)
  in
    fpf "  | %s %sx ->\n" 
      (superast_constructor cl)
      (if List.for_all (fun f -> f.af_is_base_field) fields
       then "_"
       else "");
    List.iter
      (fun f ->
	 if not f.af_is_base_field then
	   uplinks_field oc "      " 
	     ("x." ^ (translated_field_name cl f))
	     f.af_mod_type
      )
      fields;
    out "      ()\n\n"


let uplinks_sub_variant oc super cl =
  let out = output_string oc in
  let fpf format = fprintf oc format in
  let fields = get_all_fields_flat cl in
    (* do fields from right to left to get down links in left to right order *)
  let fields_rev = List.rev fields in
  let field_counter = ref 1 
  in
    fpf "  | %s(%s(_" (superast_constructor super) (variant_name cl);
    List.iter 
      (fun f -> 
	 if f.af_is_base_field 
	 then 
	   out ", _"
	 else
	   fpf ", a%d" !field_counter;
	 incr field_counter)
      fields;
    out ")) ->\n";
    (* field_counter is now number of fields + 1 *)
    List.iter
      (fun f ->
	 decr field_counter;
	 if not f.af_is_base_field
	 then
	   uplinks_field oc "      " 
	     (sprintf "a%d" !field_counter) 
	     f.af_mod_type;
	 )
      fields_rev;
    out "      ()\n\n"


let uplinks_sub_rec oc super recsub =
  let fpf format = fprintf oc format 
  in
    fpf "  | %s(%s _) -> assert false\n\n"
      (superast_constructor super) 
      (variant_name recsub)


let uplinks_sub oc super sub =
  if sub.ac_record 
  then uplinks_sub_rec oc super sub
  else uplinks_sub_variant oc super sub

	

let implementation input ast oc = 
  let out = output_string oc in
  let fpf format = fprintf oc format in
  let counter = ref 1
  in
    pr_comment oc
      [do_not_edit_line;
       "";
       "automatically generated from gen_superast from " ^ input;
      ];
    out "\n\n";
    
    (* make it recursive such that record variants can call it on themselves *)
    out "let ast_node_fun up down myindex = function\n";
    fpf "  | %s -> assert false\n\n" name_of_superast_no_ast;

    List.iter 
      (fun cl ->
	 if cl.ac_subclasses = [] 
	 then
	   begin
	     fpf "  (* %d *)\n" !counter;
	     incr counter;
	     uplinks_super oc cl
	   end 
	 else 
	   begin
	     List.iter 
	       (fun sub -> 
		  fpf "  (* %d *)\n" !counter;
		  incr counter;
		  uplinks_sub oc cl sub;
	       )
	       cl.ac_subclasses;
	     List.iter
	       (fun recsub -> 
		  fpf "  (* %d *)\n" !counter;
		  incr counter;
		  uplinks_super oc recsub)
	       (List.filter (fun sub -> sub.ac_record) cl.ac_subclasses)
	   end
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
  let (oast_file, ast) = setup_ml_ast arguments "gen_uplinks" in
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
