open Cc_ast_gen_type

let marshal_translation_unit (u : translationUnit_type) fname =
  let oc = open_out fname 
  in
    Marshal.to_channel oc u [];
    close_out oc

let marshal_translation_unit_callback u fname =
  try
    marshal_translation_unit u fname
  with
    | x -> Printf.eprintf
	"exception in marshal_translation_unit:\n%s\n"
	  (Printexc.to_string x)

let register_marshal_callback () =
  Callback.register
    "marshal_translation_unit_callback"
    marshal_translation_unit_callback

  
  
 
