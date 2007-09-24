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

val count_fields : 'a list list -> int

val annotation_field_name : ast_class -> string

val annotation_access_fun : ast_class -> string

val variant_name : ast_class -> string

val node_ml_type_name : ast_class -> string

val superast_constructor : ast_class -> string

val name_of_superast_no_ast : string


val setup_ml_ast : (Arg.key * Arg.spec * Arg.doc) list 
  -> string -> (string * ast_class list)
