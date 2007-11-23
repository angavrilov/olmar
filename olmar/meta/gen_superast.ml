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
    out "(* super_ast pattern matching template below *)\n\n";
    out "type 'a super_ast =\n";
    List.iter
      (fun cl ->
	 fpf "  | %s of 'a %s\n"
	   (superast_constructor cl)
	   (node_ml_type_name cl);
	 List.iter 
	   (fun recsub -> 
	      fpf "  | %s of 'a %s\n"
		(superast_constructor recsub)
		(node_ml_type_name recsub))
	   (List.filter (fun sub -> sub.ac_record) cl.ac_subclasses)
      )
      ast;
    fpf "  | %s\n" name_of_superast_no_ast;
    out "\n\n(* superast pattern matching template:\n\n";
    fpf "  | %s -> assert false\n" name_of_superast_no_ast;
    List.iter
      (fun cl ->
	 fpf "  | %s x\n"
	   (superast_constructor cl);
	 List.iter 
	   (fun recsub -> 
	      fpf "  | %s x\n"
		(superast_constructor recsub))
	   (List.filter (fun sub -> sub.ac_record) cl.ac_subclasses)
      )
      ast;
    out "\n*)\n\n"



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
 * into array
 *
 ******************************************************************************
 ******************************************************************************)

let into_array_fun_name cl =
  let cl = match cl.ac_super with
    | None -> cl
    | Some super -> if cl.ac_record then cl else super
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
  | AT_list(LK_string_ref_map, inner, _) ->
      output_string oc "Hashtbl.iter (fun (_ : string) -> (";
      into_array_fun oc inner;
      output_string oc "))"
  | AT_list(_, inner, _) ->
      output_string oc "List.iter (";
      into_array_fun oc inner;
      output_string oc ")";
  | AT_node cl -> 
      output_string oc (into_array_fun_name cl);
      output_string oc " ast_array order_fun visited_nodes"
  | AT_option(inner, _) ->
      output_string oc "option_iter (";
      into_array_fun oc inner;
      output_string oc ")"
  | AT_ref _ -> assert false 		(* ref only outermost *)


let into_array_rec_field oc field indent name =
  let out = output_string oc in
  let fpf format = fprintf oc format 
  in
    out indent;
    into_array_fun oc (unref_type field.af_mod_type);
    out " ";
    if is_ref_type field.af_mod_type 
    then fpf "!(%s);\n" name
    else fpf "%s;\n" name
      

let record_into_array first oc cl =
  let out = output_string oc in
  let fpf format = fprintf oc format in
  let fields = get_all_fields_flat cl
  in
    into_array_header first oc cl;
    fpf "  let annot = x.%s\n" (annotation_field_name cl);
    out "  in\n";
    out "    if visited visited_nodes annot then ()\n";
    out "    else begin\n";

    List.iter
      (fun field ->
	 if field_has_assertion cl field
	 then
	   List.iter
	     (fun assertion -> 
		out assertion;
		out ";\n")
	     (generate_field_assertion "      " 
		("x." ^ (translated_field_name cl field)) cl field))
      fields;

    fpf "      ast_array.(id_annotation annot) <- %s x;\n"
      (superast_constructor cl);
    out "      visit visited_nodes annot;\n";
    out "      order_fun (id_annotation annot);\n\n";

    List.iter
      (fun field ->
	 if not field.af_is_base_field
	 then
	   into_array_rec_field oc field "      "
	     ("x." ^ (translated_field_name cl field)))
      fields;

    out "    end\n\n\n"

let variant_case_variant_into_array oc super cl =
  let out = output_string oc in
  let fpf format = fprintf oc format in
  let fields = get_all_fields_flat cl in
  let field_names =
    List.map2
      (fun field name ->
	 if field.af_is_base_field && (not (field_has_assertion cl field))
	 then "_"
	 else name)
      fields
      (generate_names "a" (List.length fields))
  in
    fpf "         | %s(%s) ->\n" 
      (variant_name cl)
      (String.concat ", " ("_" :: field_names));

    List.iter2
      (fun field name ->
	 if field_has_assertion cl field
	 then
	   List.iter
	     (fun assertion -> 
		out assertion;
		out ";\n")
	     (generate_field_assertion "             " name cl field))
      fields field_names;


    fpf "             ast_array.(id_annotation annot) <- %s x;\n"
      (superast_constructor super);
    out "             visit visited_nodes annot;\n";
    out "             order_fun (id_annotation annot);\n";
    List.iter2
      (fun f name ->
	 if not f.af_is_base_field
	 then
	   into_array_rec_field oc f "             " name)
      fields field_names;
    out "\n"


let variant_case_record_into_array oc recsub =
  let out = output_string oc in
  let fpf format = fprintf oc format 
  in
    fpf "         | %s x ->\n" (variant_name recsub);
    out "             ";
    into_array_fun oc (AT_node recsub);
    out " x\n\n"
  

let variant_case_into_array oc super sub =
  if sub.ac_record 
  then variant_case_record_into_array oc sub
  else variant_case_variant_into_array oc super sub


let variant_into_array first oc cl =
  let out = output_string oc in
  let fpf format = fprintf oc format 
  in
    into_array_header first oc cl;
    fpf "  let annot = %s x\n" (annotation_access_fun cl);
    out "  in\n";
    out "    if visited visited_nodes annot then ()\n";
    out "    else begin\n";
    out "      (match x with\n";
    
    List.iter (variant_case_into_array oc cl) cl.ac_subclasses;
    out "      );\n";
    out "    end\n";
    out "\n\n";

    List.iter
      (fun recsub -> record_into_array first oc recsub)
      (List.filter (fun sub -> sub.ac_record) cl.ac_subclasses)


(******************************************************************************
 ******************************************************************************
 *
 * source loc accessor
 *
 ******************************************************************************
 ******************************************************************************)

let do_source_loc_super_case oc cl =
  let out = output_string oc in
  let fpf format = fprintf oc format 
  in
    fpf "  | %s " (superast_constructor cl);
    if
      if cl.ac_subclasses = []
      then
	get_source_loc_field cl <> None
      else
	List.for_all
	  (fun sub -> get_source_loc_field sub <> None)
	  cl.ac_subclasses
    then
      fpf "x -> Some(%s x)\n" (source_loc_access_fun cl)
    else
      out "_ -> None\n"


let do_source_loc_accessor oc ast = 
  let out = output_string oc in
  let fpf format = fprintf oc format 
  in
    fpf "let %s = function\n" source_loc_meta_fun;
    fpf "  | %s -> assert false\n" name_of_superast_no_ast;
    List.iter
      (fun cl ->
	 do_source_loc_super_case oc cl;
	 List.iter
	   (fun recsub -> do_source_loc_super_case oc recsub)
	   (List.filter (fun sub -> sub.ac_record) cl.ac_subclasses))
      ast;
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

    pr_comment_heading oc ["             superast type declaration";];

    superast_type_decl ast oc;
    out "\n\n\n";

    pr_comment_heading oc ["             superast into array";];

    List.iter
      (fun cl ->
	 if cl.ac_subclasses = [] 
	 then
	   record_into_array first oc cl
	 else
	   variant_into_array first oc cl)
      ast;
    out "\n\n";

    pr_comment_heading oc ["             superast source loc accessor";];
    do_source_loc_accessor oc ast;
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
