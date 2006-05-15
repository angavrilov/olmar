
open Cc_ml_constructors
open Cc_ast_gen
open Ast_marshal

let register_caml_callbacks () =
  register_cc_ml_constructor_callbacks ();
  register_cc_ast_callbacks();
  register_marshal_callback();
  ()

let _ = Callback.register
    "register_caml_callbacks"
    register_caml_callbacks


let _ = Gc.set {(Gc.get ()) with 
		  (* Gc.verbose = 0x037;  *)
		  Gc.space_overhead = 200;
	       }