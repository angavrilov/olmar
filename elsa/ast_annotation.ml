(* define the annotation type that gets inserted 
 * into every node of the ocaml ast
 *)

(* the first int is a unique number for identification, 
 * the second is the C++ address (shifted left to fit)
 *)
type annotated = int * int

let addr_hash = Hashtbl.create 10093

let next_id = ref 1

let create_ast_annotation (c_addr : int) = 
  try
    (Hashtbl.find addr_hash c_addr, c_addr)
  with
    | Not_found ->
	let id = !next_id
	in
	  Hashtbl.add addr_hash c_addr id;
	  incr next_id;
	  (id, c_addr)

let register_ast_annotation_callbacks () =
  Callback.register "create_ast_annotation" create_ast_annotation
  


