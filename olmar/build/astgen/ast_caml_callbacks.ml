(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* register all callback functions *)

let register_caml_callbacks () =
  Astgen_util.register_astgen_util_callbacks ();
  Ast_annotation.register_ast_annotation_callbacks ();
  Ast_ml_constructors.register_ast_ml_constructor_callbacks ();
  Ast_oast_header.register_oast_header_callbacks ();
  ()

let _ = Callback.register
    "register_caml_callbacks"
    register_caml_callbacks

