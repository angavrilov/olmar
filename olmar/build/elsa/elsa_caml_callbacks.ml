(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* register all callback functions *)

let register_caml_callbacks () =
  Astgen_util.register_astgen_util_callbacks ();
  Ast_annotation.register_ast_annotation_callbacks ();
  Elsa_oast_header.register_oast_header_callbacks ();
  Elsa_ml_base_reflection.register_src_loc_callbacks ();
  Elsa_ml_flag_constructors.register_cc_ml_constructor_callbacks ();
  ()

let _ = Callback.register
    "register_caml_callbacks"
    register_caml_callbacks

