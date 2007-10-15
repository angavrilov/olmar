(*  Copyright 2007 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Ast_annotation
open Ast_ml_types
open Ast_reflect_type

(******************************************************************************
 ******************************************************************************
 *
 * ml_ast type declaration
 *
 ******************************************************************************
 ******************************************************************************)

(* kinds of ast lists *)
type list_kind =
  | LK_ast_list
  | LK_fake_list

(* type of fields and constructor arguments *)
type ast_type =
  | AT_base of string
  | AT_node of ast_class
  | AT_ref of ast_type
  | AT_option of ast_type
      (* AT_list( ast or fake list, inner kind, inner c-type string ) *)
  | AT_list of list_kind * ast_type * string

(* a field or constructor argument *)
and ast_field = {
  af_name : string;
  af_modifiers : fieldFlag list;
  af_type : ast_type;
  af_mod_type : ast_type;
  af_is_pointer : bool;
  af_is_base_field : bool;
}

(* a superclass or a subclass *)
and ast_class = {
  ac_id : int;
  ac_name : string;
  mutable ac_args : ast_field list;
  mutable ac_last_args : ast_field list;
  mutable ac_fields : ast_field list;
  ac_super : ast_class option;
  mutable ac_subclasses : ast_class list;
}


val ml_ast_of_oast : annotated aSTSpecFile_type -> ast_class list

(*****************************************************************************
 * ml ast utilities
 *)

val is_node_type : string -> bool

val get_node : string -> ast_class

val get_all_fields : ast_class -> ast_field list list

val get_all_fields_flat : ast_class -> ast_field list

val count_fields : 'a list list -> int

val get_source_loc_field : ast_class -> ast_field option

val annotation_field_name : ast_class -> string

val annotation_access_fun : ast_class -> string

val source_loc_access_fun : ast_class -> string

val source_loc_meta_fun : string

val variant_name : ast_class -> string

val node_ml_type_name : ast_class -> string

val translated_class_name : ast_class -> string

val translated_field_name : ast_class -> ast_field -> string

val superast_constructor : ast_class -> string

val name_of_superast_no_ast : string

(* string_of_ast_type with_tick_a type 
 * makes node types polymorphic with 'a if with_tick_a
 *)
val string_of_ast_type : bool -> ast_type -> string

(* setup_oast_translation argument_list application_name
 * adds -tr <config_file> to the argument_list,
 * parses the command line, reads the oast file,
 * and parses the config file, if given.
 * it returns
 * - oast file name
 * - oast size
 * - oast 
 *)

val setup_oast_translation : (Arg.key * Arg.spec * Arg.doc) list 
  -> string -> (string * int * annotated aSTSpecFile_type)

(* setup_ml_ast does the same as setup_oast_translation,
 * but converts the oast into an ml ast_class list
 *)
val setup_ml_ast : (Arg.key * Arg.spec * Arg.doc) list 
  -> string -> (string * ast_class list)
