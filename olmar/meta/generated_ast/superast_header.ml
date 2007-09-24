(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Ast_annotation
open Ast_reflect_type
open Ast_accessors

let visited visited_nodes (annot : annotated) =
  Dense_set.mem (id_annotation annot) visited_nodes

let visit visited_nodes (annot : annotated) =
  Dense_set.add (id_annotation annot) visited_nodes

