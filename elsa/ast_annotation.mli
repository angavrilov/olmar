(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* annotation type for ast nodes, utility functions *)

(*
 * an element of annotated gets inserted 
 * into every node of the ocaml ast
 *
 * Currently the annotation consists of an integer uniqely identifying 
 * the ast node and the address of the corresponding C++ object in ccparse 
 * (shifted 2 to the right to fit into ocaml ints). 
 *)


(* not exported in order to force upward compatibility of clients *)
type annotated

(* return the uniqe integer of this node;
 * result is always in the range 1 .. max_int; 
 * further, these integers are dense, that is all integers up 
 * to the maximum id are indeed used. 
 *)
val id_annotation : annotated -> int

(* return the address (shifted 2 to the right) of the C++ object in ccparse *)
val caddr_annotation : annotated -> int


(* create a unique pseudo annotation. Id's of pseudo annotations will 
 * be negative and grow downward from -1
 *)
val pseudo_annotation : unit -> annotated


(*************************************************************************
 *
 * the following functions are to be used only during ast generation 
 * in ccparse
 *
*************************************************************************)

(* create a new unique ast annotation *)
val create_ast_annotation : int -> annotated

(* return the maximal id used *)
val max_annotation : unit -> int

(* register all callback functions of this module *)
val register_ast_annotation_callbacks : unit -> unit
