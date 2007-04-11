(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* marshal the ocaml ast to a file *)

open Cc_ast_gen_type
open Ml_ctype
open Ast_annotation

let marshal_translation_unit (u : annotated translationUnit_type) fname =
  let oc = open_out fname 
  in
    Oast_header.output_header oc (max_annotation());
    Marshal.to_channel oc u [];
    close_out oc
    (* 
     * ;
     * Gc.print_stat stdout;
     * let c = Gc.get() 
     * in
     *   Printf.printf "minor heap size : %d\nmajor heap increment %d\n%!"
     * 	c.Gc.minor_heap_size c.Gc.major_heap_increment
     *)


let marshal_translation_unit_callback u fname =
  try
    marshal_translation_unit u fname
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

  
  
