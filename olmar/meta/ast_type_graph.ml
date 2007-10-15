(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)


open Meta_ast


(******************************************************************************
 ******************************************************************************
 *
 * dot_graph and utility functions
 *
 ******************************************************************************
 ******************************************************************************)


module Node_id = struct
  type t = int

  let string_of_t = string_of_int
end

module G = Dot_graph.Make(Node_id)


(******************************************************************************
 ******************************************************************************
 *
 * colors
 *
 ******************************************************************************
 ******************************************************************************)


let color_hash = Hashtbl.create 153

let get_color id =
  try
    Hashtbl.find color_hash id
  with
    | Not_found ->
	Printf.eprintf "color missing for %s\n" id;
	"white"

let _ = 
  List.iter 
    (fun (id_list, color) -> 
       List.iter (fun id -> Hashtbl.add color_hash id color) id_list)
    [
      (["ASTSpecFile"], "orange");
      (["ToplevelForm";
	"TF_verbatim";
	"TF_impl_verbatim";
	"TF_ocaml_type_verbatim";
	"TF_xml_verbatim";
	"TF_class";
	"TF_option";
	"TF_custom";
	"TF_enum";], "red");
      (["ASTClass"; "FieldOrCtorArg"], "cyan");
      (["BaseClass"], "purple");
      (["AccessMod";
	"Annotation";
	"UserDecl";
	"CustomCode"], "grey")
    ]


(******************************************************************************
 ******************************************************************************
 *
 * make the type graph
 *
 ******************************************************************************
 ******************************************************************************)

let name_and_type_of_field cl f =
  Printf.sprintf "%s : %s" 
    (translated_field_name cl f)
    (string_of_ast_type false f.af_mod_type)


let rec get_pointed_node = function
  | AT_base _ -> assert false
  | AT_node n -> n
  | AT_ref inner 
  | AT_option inner
  | AT_list(_, inner, _) 
    -> get_pointed_node inner


let gen_graph_child_or_record cl =
  let (attributes, fields) = 
    List.partition (fun f -> f.af_is_base_field) (get_all_fields_flat cl)
  in
    (* assure child or record *)
    (assert(cl.ac_subclasses = []));
    ignore(
      G.make_node
	cl.ac_id
	((Printf.sprintf "%s %d" (translated_class_name cl) cl.ac_id) ::
	   (List.map (name_and_type_of_field cl) attributes))
	[("color", get_color (translated_class_name cl))]
	(List.map
	   (fun f ->
	      ((get_pointed_node f.af_mod_type).ac_id, 
	       Some(name_and_type_of_field cl f)))
	   fields))

let gen_graph_super cl =
  let (attributes, _fields) = 
    List.partition (fun f -> f.af_is_base_field) (get_all_fields_flat cl)
  in
    (assert(cl.ac_subclasses <> []));
    ignore(
      G.make_node
	cl.ac_id
	((Printf.sprintf "%s %d" (translated_class_name cl) cl.ac_id) ::
	   (List.map (name_and_type_of_field cl) attributes))
	[("color", get_color (translated_class_name cl))]
	(List.map (fun c -> (c.ac_id, None)) cl.ac_subclasses));
    List.iter gen_graph_child_or_record cl.ac_subclasses


let gen_graph cl =
  if cl.ac_subclasses = []
  then gen_graph_child_or_record cl
  else gen_graph_super cl

(******************************************************************************
 ******************************************************************************
 *
 * arguments and main
 *
 ******************************************************************************
 ******************************************************************************)


let output_option = ref None

let arguments = 
  [
    ("-o", Arg.String (fun s -> output_option := Some s),
     "out set output prefix to out");
  ]

let dot_commands =
  ["color=white";
   "splines=true";
   "node [ style = filled ]";
   "edge [ arrowtail=odot ]"
  ]


let main () =
  let (oast_file, ast) = setup_ml_ast arguments "gen_superast" 
  in
    (List.iter gen_graph ast);
    G.write_dot_file oast_file dot_commands !output_option
    
;;

Printexc.print main ()
