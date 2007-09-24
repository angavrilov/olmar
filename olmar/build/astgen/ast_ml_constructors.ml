(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* constructor callbacks for the types in cc_ml_types *)

open Ast_ml_types


let ff_flag_array = [|
  FF_IS_OWNER; (* = 0x01 *)
  FF_NULLABLE; (* = 0x02 *)
  FF_FIELD;    (* = 0x04 *)
  FF_XML;      (* = 0x08 *)
  FF_PRIVAT;   (* = 0x10 *)
  FF_CIRCULAR; (* = 0x20 *)
|]

let num_ff_flags = Array.length ff_flag_array

let _ = assert(num_ff_flags = 6)

let fieldFlags_from_int(flags : int) =
  let rec doit i accu =
    if i = num_ff_flags then accu
    else 
      if (1 lsl i ) land flags <> 0
      then
	doit (i+1) (ff_flag_array.(i) :: accu)
      else
	doit (i+1) accu
  in
    assert(flags lsr num_ff_flags = 0);
    doit 0 []



let create_AC_PUBLIC_constructor () = AC_PUBLIC
let create_AC_PRIVATE_constructor () = AC_PRIVATE
let create_AC_PROTECTED_constructor () = AC_PROTECTED
let create_AC_CTOR_constructor () = AC_CTOR
let create_AC_DTOR_constructor () = AC_DTOR
let create_AC_PUREVIRT_constructor () = AC_PUREVIRT

let register_AccessFlags_callbacks () =
  Callback.register 
    "create_AC_PUBLIC_constructor"
    create_AC_PUBLIC_constructor;
  Callback.register 
    "create_AC_PRIVATE_constructor"
    create_AC_PRIVATE_constructor;
  Callback.register 
    "create_AC_PROTECTED_constructor"
    create_AC_PROTECTED_constructor;
  Callback.register 
    "create_AC_CTOR_constructor"
    create_AC_CTOR_constructor;
  Callback.register 
    "create_AC_DTOR_constructor"
    create_AC_DTOR_constructor;
  Callback.register 
    "create_AC_PUREVIRT_constructor"
    create_AC_PUREVIRT_constructor;
  ()




(* register all callbacks in this file *)
let register_ast_ml_constructor_callbacks () =
  Callback.register "fieldFlags_from_int" fieldFlags_from_int;
  register_AccessFlags_callbacks ();
  ()
