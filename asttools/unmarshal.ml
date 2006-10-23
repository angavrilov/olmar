open Ast_annotation
open Cc_ast_gen_type

let file = ref None

let set_file f = 
  match !file with
    | None -> file := Some f
    | Some _ -> 
	raise (Arg.Bad (Printf.sprintf "don't know what to do with %s" f))

let options = Arg.align
  [
  ]

let usage_msg = "unmarshal file\npermitted options:"

let usage() = 
  Arg.usage options usage_msg;
  exit 1

let main () =
  let _ = Arg.parse options set_file usage_msg in
  let file = match !file with
    | None -> usage()
    | Some f -> f
  in
  let ic = open_in file in
  let ast = Marshal.from_channel ic 
  in
    close_in ic;
    Printf.printf "%s: typecheck ... %!" file;
    try
      ignore(SafeUnmarshal.copy 
    	       [^ annotated translationUnit_type ^] ast);
      print_string "passed\n";
      exit 0
    with
      | SafeUnmarshal.Fail ->
    	  print_string "failed\n";
    	  exit 1

;;

Printexc.catch main ()

  
(*** Local Variables: ***)
(*** compile-command: "./make-check-oast" ***)
(*** End: ***)
