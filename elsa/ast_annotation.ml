(* define the annotation type that gets inserted 
 * into every node of the ocaml ast
 *)

type annotated = int32

let create_ast_annotation c_addr = (c_addr : annotated)

let register_ast_annotation_callbacks () =
  Callback.register "create_ast_annotation" create_ast_annotation
  


