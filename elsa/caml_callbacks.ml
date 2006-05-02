
open Cc_ml_types
open Cc_ast_gen_ml

let register_caml_callbacks () =
  (* Printf.eprintf "register all callbacks\n%!"; *)
  register_cc_ml_types_callbacks ();
  register_cc_ast_callbacks();
  (* Printf.eprintf "callbacks registered\n%!"; *)
  ()

let _ = Callback.register
    "register_caml_callbacks"
    register_caml_callbacks

