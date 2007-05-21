(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* register all callback functions *)

open Astgen_util
open Cc_ml_constructors
open Ml_ctype_constructors
open Cc_ast_gen
open Ast_marshal
open Ast_annotation

let register_caml_callbacks () =
  register_ast_util_callbacks();
  register_cc_ml_constructor_callbacks ();
  register_ml_ctype_constructor_callbacks ();
  register_cc_ast_callbacks();
  register_marshal_callback();
  register_ast_annotation_callbacks();
  ()

let _ = Callback.register
    "register_caml_callbacks"
    register_caml_callbacks


let _ = Gc.set {(Gc.get ()) with 
		  (* Gc.verbose = 0x037;  *)
		  Gc.space_overhead = 200;
		  Gc.minor_heap_size = 1024 * 1024 * 8;
	       }
