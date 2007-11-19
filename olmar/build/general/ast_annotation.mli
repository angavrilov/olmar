(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* annotation type for ast nodes, utility functions *)

(*
 * an element of annotated gets inserted 
 * into every node of the ocaml ast
 *
 * Currently the annotation consists only of an integer uniqely identifying 
 * the ast node.
 *)


(* not exported in order to force upward compatibility of clients *)
type annotated

(* return the uniqe integer of this node;
 * result is always in the range 1 .. max_int; 
 * further, these integers are dense, that is all integers up 
 * to the maximum id are indeed used. 
 *)
val id_annotation : annotated -> int

(* Set the internal annotation counter.
 * Needed if pseudo annotations are generated after the reflection,
 * for instance in the meta ast tools.
 *)
val set_max_annotation : int -> unit

val pseudo_annotation : unit -> annotated

(*************************************************************************
 *
 * the following functions are to be used only during ast generation 
 * in ccparse
 *
*************************************************************************)

(* create a new unique ast annotation *)
val create_ast_annotation : nativeint -> annotated

(* return the maximal id used *)
val max_annotation : unit -> int

(* register all callback functions of this module *)
val register_ast_annotation_callbacks : unit -> unit
