(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

val parse_config_file : string -> unit

val translate_olmar_name : string option -> string -> string

val get_ocaml_type_header : unit -> string list

val is_basic_type : string -> bool

val is_implicit_pointer_type : string -> bool

val get_ast_top_nodes : unit -> string list

val get_ocaml_reflect_header : unit -> string list

val get_node_color : string -> string

val superclass_get_kind : string -> string

val get_subclass_tag : string -> string

val get_downcast : string -> string

val get_field_assertion : string -> string -> string

val get_private_accessor : string -> string -> string

val variant_is_record : string -> bool

val get_graph_label_fun : string -> string
