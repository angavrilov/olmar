(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* marshal the ocaml ast to a file *)

open Cc_ast_gen_type
open Ml_ctype
open Ast_annotation
open Oast_header

let marshal_translation_unit_callback u fname =
  try
    marshal_oast u fname
  with
    | x -> 
	Printf.eprintf
	  "MARSHALL ERROR: exception in marshal_translation_unit:\n%s\n%!"
	  (Printexc.to_string x);
	exit(1)

let register_marshal_callback () =
  Callback.register
    "marshal_translation_unit_callback"
    marshal_translation_unit_callback

  
  
