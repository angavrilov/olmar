(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Ast_annotation
open Elsa_reflect_type
open Ast_accessors
open Superast


let add_link up down myindex annot =
  let child = id_annotation annot
  in
    down.(myindex) <- child :: down.(myindex);
    up.(child) <- myindex :: up.(child)

let option_iter f = function
  | None -> ()
  | Some o -> f o


let list_of_hashed_values (h : (string, 'a) Hashtbl.t) =
  let res = ref []
  in
    Hashtbl.iter (fun _ v -> res := v :: !res) h;
    !res


