(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Ast_annotation
open Elsa_reflect_type
open Elsa_ast_util
open Ast_accessors

let visited visited_nodes (annot : annotated) =
  Dense_set.mem (id_annotation annot) visited_nodes

let visit visited_nodes (annot : annotated) =
  Dense_set.add (id_annotation annot) visited_nodes

let option_iter f = function
  | None -> ()
  | Some o -> f o


let option_for_all f = function
  | None -> true
  | Some x -> f x
