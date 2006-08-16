
open Cc_ast_gen_type
open Ast_annotation

let file = ref None

let set_file f = 
  match !file with
    | None -> file := Some f
    | Some _ -> 
	raise (Arg.Bad (Printf.sprintf "don't know what to do with %s" f))

let options = []

let usage_msg = "check-oast file\npermitted options:"

let usage() = 
  Arg.usage options usage_msg;
  exit 1

let main () =
  let _ = Arg.parse options set_file usage_msg in
  let file = match !file with
    | None -> usage()
    | Some f -> f
  in
  let ic = open_in file 
  in
    Printf.printf "%s: typecheck ... %!" file;
    try
      ignore(SafeUnmarshal.from_channel 
	       [^ annotated translationUnit_type ^] ic);
      print_string "passed\n";
      exit 0
    with
      | SafeUnmarshal.Fail ->
	  print_string "failed\n";
	  exit 1

;;

Printexc.catch main ()

  
(*** Local Variables: ***)
(*** compile-command: "/usr/local/stow/ocaml-ty-2006-06-22/bin/ocamlc.opt -o check-oast -I ../elsa safeUnmarshal.cma ast_annotation.cmo elsa_util.cmo cc_ml_types.cmo ml_ctype.cmo cc_ast_gen_type.cmo check-oast.ml" ***)
(*** End: ***)
