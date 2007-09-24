(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Ast_annotation
open Ast_reflect_type
open Ast_accessors
open Superast


let add_link up down myindex annot =
  let child = id_annotation annot
  in
    down.(myindex) <- child :: down.(myindex);
    up.(child) <- myindex :: up.(child)

